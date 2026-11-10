# 空闲任务与空闲钩子函数

## 1. 空闲任务(Idle Task)

- 调用 `vTaskStartScheduler()` 时,内核**自动创建**空闲任务,无需在 `main` 中声明。
- 优先级为 `tskIDLE_PRIORITY`(0),是最低优先级。
- 保证 CPU 任何时刻都至少有一个任务可运行。
- 当所有用户任务都处于阻塞状态(如 `vTaskDelayUntil`)时,调度器切换到空闲任务。

## 2. `prvIdleTask` 的循环流程

源码位置:`Middlewares/FreeRTOS/tasks.c` 中的 `prvIdleTask`。每轮循环依次执行:

1. `prvCheckTasksWaitingTermination()`:释放已自删除任务的 TCB 与栈内存。
2. 让出 CPU:
   - `configUSE_PREEMPTION == 0`:每轮都 `taskYIELD()`。
   - 抢占模式且 `configIDLE_SHOULD_YIELD == 1`:若有其他任务与空闲任务同为优先级 0 且就绪,则让出。
3. `vApplicationIdleHook()`:仅当 `configUSE_IDLE_HOOK == 1`。
4. Tickless 低功耗:仅当 `configUSE_TICKLESS_IDLE != 0`,估算空闲时间,暂停 tick 并睡眠。
5. `vApplicationPassiveIdleHook()`:仅多核(SMP)使用,单核 STM32F103 用不到。

## 3. 空闲钩子(Idle Hook)

### 启用方式

1. `Core/Inc/FreeRTOSConfig.h` 中设置 `#define configUSE_IDLE_HOOK 1`。
2. 在应用中实现 `void vApplicationIdleHook(void)`。

### 本项目示例(`Core/Src/main.c`)

```c
volatile uint32_t uldleCycleCount = 0;

void vApplicationIdleHook(void)
{
    uldleCycleCount++;
}
```

任务中通过串口周期性打印 `uldleCycleCount`,数值持续增大即说明钩子在运行。

### 使用限制

- 钩子在空闲任务上下文中运行,**不能调用会阻塞的 API**,否则空闲任务阻塞,可能导致没有任务可运行。
- 应尽量短小,否则会延迟已删除任务的内存回收。

### 为什么使用钩子

不使用钩子程序也能正常运行,只是空闲任务里什么额外的事都不做。常见用途:

- 执行 `WFI` 进入低功耗,省电。
- 统计 CPU 占用率(空闲计数越高越空闲)。
- 低优先级后台工作(自检、喂狗),无需单独创建任务。

其他钩子:启用 `configUSE_MALLOC_FAILED_HOOK` 或 `configCHECK_FOR_STACK_OVERFLOW` 后,必须提供对应实现,否则链接失败。

## 4. `taskYIELD()`

- 主动让出 CPU,请求调度器立即切换。
- Cortex-M3 上通过置位 PendSV 挂起位实现,在 PendSV 中完成上下文切换。
- 只会切换到**同优先级**的其他就绪任务;若只有更低优先级任务就绪,当前任务继续运行。
- 不会使任务阻塞,任务保持就绪状态。这是与 `vTaskDelay` 的区别,后者会让任务进入阻塞状态。

## 5. 验证步骤

1. 确认 `configUSE_IDLE_HOOK` 为 `1`。
2. `make clean && make`,烧录 `build/freertos_stm32f103rc.bin`。
3. 串口(USART1,PA9,115200)观察 `idle count`,或在调试器中对 `uldleCycleCount` 设观察点。
