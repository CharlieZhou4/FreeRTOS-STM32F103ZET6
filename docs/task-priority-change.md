# 调度器运行中修改任务优先级实验

## 1. 实验目的

在调度器启动之后,通过 `vTaskPrioritySet()` 动态修改任务优先级,观察任务切换的现象。

## 2. 代码逻辑(`Core/Src/main.c`)

```c
xTaskCreate(vTask1, "Task1", 1000, NULL, 2, NULL);
xTaskCreate(vTask2, "Task2", 1000, NULL, 1, &xTask2Handle);
```

| 任务 | 初始优先级 | 行为 |
|------|-----------|------|
| Task1 | 2 | 打印两行,然后 `vTaskPrioritySet(xTask2Handle, uxPriority+1)`,把 Task2 升到 3 |
| Task2 | 1 | 打印两行,然后 `vTaskPrioritySet(NULL, uxPriority-2)`,降低自己的优先级 |

`uxPriority` 在任务函数开头通过 `uxTaskPriorityGet(NULL)` 读取一次,之后不会随优先级变化而更新。

## 3. 任务句柄(`xTaskCreate` 最后一个参数)

- 接收新任务的 `TaskHandle_t`,是该任务的身份标识,参数类型是 `TaskHandle_t *`,所以要传变量地址。
- Task1 传 `NULL`:不保存句柄。
- Task2 传 `&xTask2Handle`:Task1 才能用它修改 Task2 的优先级。
- 对调用者自己操作时可以传 `NULL`,例如 `vTaskPrioritySet(NULL, ...)` 改的是当前任务。

## 4. 预期时序

1. Task1(优先级 2)先运行,打印后把 Task2 升到 3。
2. Task2(3)高于 Task1,立刻抢占,此时读到 `uxPriority = 3`。
3. Task2 执行 `3 - 2 = 1`,把自己降回 1,Task1 恢复运行。
4. 两个任务交替运行。

**前提:Task2 在被升到 3 之前不能开始运行**,这样它读到的 `uxPriority` 才是 3。

## 5. 遇到的问题与原因

### 现象
串口只能看到一小段输出(如 `he Task1 Priority`),之后什么都没有;每次复位重复同样现象。

### 原因
1. `vPrintString` 通过 DMA 发送,内部 `xSemaphoreTake(xPrintDoneSem)` 会**让任务进入阻塞态**。
2. Task1 打印时阻塞,优先级 1 的 Task2 提前运行,`uxTaskPriorityGet(NULL)` 读到 **1**。
3. Task2 执行 `vTaskPrioritySet(NULL, 1 - 2)`。`UBaseType_t` 是无符号数,`1 - 2` 下溢成 `0xFFFFFFFF`。
4. `vTaskPrioritySet` 内部 `configASSERT(uxNewPriority < configMAX_PRIORITIES)`(此处为 7)失败。
5. 项目的 `configASSERT` 会 `taskDISABLE_INTERRUPTS(); for(;;);`,关中断后调度器停转,系统停住。

### 结论
任务里使用会阻塞的函数,会改变任务的启动顺序,从而打乱实验设计的时序。

## 6. 解决方式

使用**不阻塞**的轮询打印 `vPrintStringPolled()`(`Core/Src/print.c`、`Core/Inc/print.h`):

- 通过忙等待 `USART1->SR` 的 TXE/TC 位发送,不使用 DMA 和信号量,不会让任务阻塞。
- Task1 因此能一路运行到 `vTaskPrioritySet`,时序符合预期。
- 没有互斥锁保护,但同一时刻只有一个任务在运行打印,不会互相打断。

## 7. 串口输出位置

- 输出在 **USART1 的 TX(PA9)**,115200 8N1。
- `printf` 没有重定向(`_write` 未实现),调试器控制台看不到。需要串口模块或串口助手查看。
- 调度器启动前,`vPrintString` 自动使用轮询发送;启动后才使用 DMA + 互斥锁。

## 8. 补充知识

- 不阻塞的两个任务会占满 CPU,优先级 0 的空闲任务得不到运行,`uldleCycleCount` 不增长。
- 优先级是无符号数,做减法前必须确认结果不会下溢。
- `configASSERT` 失败后系统会死循环。排查时可用调试器暂停,PC 会停在断言处。
