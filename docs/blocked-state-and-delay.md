# 任务阻塞状态与 `vTaskDelay` / `vTaskDelayUntil`

本文总结 `Core/Src/main.c` 中示例所演示的内容:任务如何进入阻塞(Blocked)状态,以及两个延时函数的区别。

## 1. 示例概览

| 任务 | 优先级 | 行为 |
| --- | --- | --- |
| `vContinousProcessingTask`(Task1 / Task2) | 1 | 死循环不停打印,从不主动阻塞 |
| `vPeriodicTask` | 2 | 打印后调用 `vTaskDelayUntil`,周期性运行,并带系统时间 |

串口打印由互斥量 `xPrintMutex` 保护,并由 DMA 发送。发送期间任务等待信号量 `xPrintDoneSem`,同样会进入阻塞态。

## 2. 任务状态

```
            vTaskDelay / vTaskDelayUntil / 等待信号量或互斥量
  Running ───────────────────────────────────────────────▶ Blocked
     ▲                                                        │
     │ 被调度器选中                                            │ 超时到期 / 等待的事件发生
     │                                                        ▼
     └─────────────────────────── Ready ◀─────────────────────┘
```

- **Running**:正在 CPU 上执行,同一时刻只有一个。
- **Ready**:可以运行,但有更高优先级的任务在运行。
- **Blocked**:在等待某个事件(延时到期、信号量、互斥量等),不占用 CPU。

阻塞态的任务不参与调度,所以优先级低的任务才有机会运行。
如果高优先级任务从不阻塞,低优先级任务会饿死。

## 3. 为什么调用 `vTaskDelay` 会触发调度

`vTaskDelay`(`Middlewares/FreeRTOS/tasks.c`)本身不做上下文切换,它做三件事:

1. `vTaskSuspendAll()` 挂起调度器,保护链表操作。
2. `prvAddCurrentTaskToDelayedList()` 把当前任务从就绪链表摘掉,记录唤醒时刻(当前 tick + 延时 tick),再插入延时链表。
3. `xTaskResumeAll()` 恢复调度器。如果没有因此切换,则调用 `taskYIELD_WITHIN_API()`。

第 3 步最终展开为 `portYIELD()`(`portable/GCC/ARM_CM3/portmacro.h`):
往 NVIC 的 `ICSR` 寄存器(`0xE000ED04`)写 `PENDSVSET` 位,挂起 PendSV 异常。

此时 `pxCurrentTCB` 仍指向已经不在就绪链表里的当前任务,必须立刻切换。
PendSV 优先级最低,所以在 `vTaskDelay` 返回前就会进入 `xPortPendSVHandler`:

1. 保存当前任务的 R4–R11 和 PSP。
2. 调用 `vTaskSwitchContext()`,从就绪链表中选出最高优先级任务,更新 `pxCurrentTCB`。
3. 恢复新任务的寄存器并返回。

到期后,SysTick 中断里的 `xTaskIncrementTick()` 把任务从延时链表移回就绪链表。
如果它的优先级更高,会再次触发 PendSV 切换回去。

`vTaskDelay(0)` 不会进入延时流程,只执行 `taskYIELD_WITHIN_API()`,相当于主动让出 CPU。

## 4. `vTaskDelay` 与 `vTaskDelayUntil` 的区别

| | `vTaskDelay(xTicksToDelay)` | `vTaskDelayUntil(&xLastWakeTime, xFrequency)` |
| --- | --- | --- |
| 时间基准 | **相对**:从调用时刻起延时 N 个 tick | **绝对**:相对上一次唤醒时刻 `xLastWakeTime` |
| 周期 | 实际周期 = 任务执行时间 + 延时,会漂移 | 固定周期,不受任务执行时间影响 |
| 适用场景 | 只需要"等一会儿" | 需要严格周期执行的任务 |
| 额外状态 | 无 | 需要保存 `xLastWakeTime`,先用 `xTaskGetTickCount()` 初始化 |

示意图(周期 T,任务执行时间 C):

```
vTaskDelay:        |--C--|----T----|--C--|----T----|--C--|
                   实际周期 = C + T,越来越晚

vTaskDelayUntil:   |--C--|--空闲--|--C--|--空闲--|--C--|
                   |<----T------>|<----T------>|
                   周期固定为 T
```

`vTaskDelayUntil` 会把 `xLastWakeTime` 自动更新为下一次唤醒时刻。
如果任务执行时间超过了周期(已经错过唤醒点),它不会阻塞,而是立即返回。

## 5. 示例中观察到的现象

`vPeriodicTask` 的周期很短(几十毫秒或更短),而每行输出约 27 个字符,在 115200 波特率下约需 2.3 ms。

- Task1 / Task2 没有延时,只要拿到串口就一直发送,串口带宽是瓶颈。
- `vPeriodicTask` 优先级更高,拿到互斥量后优先发送;它阻塞期间,低优先级任务才能发送。
- 输出中 `Periodic` 行远多于 `Continous` 行,符合"优先级高者先得"的设计。
- 如果 `vPeriodicTask` 在等待互斥量时被正在发送的低优先级任务拖住,打印时间戳会比理想周期晚几毫秒。

## 6. 小结

- 任务阻塞是 RTOS 让出 CPU 的基本机制,不要用空循环做延时。
- `vTaskDelay` 用于简单延时;需要精确周期时用 `vTaskDelayUntil`。
- 阻塞调用最终依赖 PendSV 触发上下文切换,SysTick 负责到期唤醒。
