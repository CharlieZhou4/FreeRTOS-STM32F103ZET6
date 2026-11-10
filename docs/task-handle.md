# 任务句柄(TaskHandle_t)

## 1. 句柄是什么

`xTaskCreate` 的最后一个参数 `pxCreatedTask` 用来接收新任务的句柄(`TaskHandle_t`)。

- 句柄是任务的"身份标识",本质是内核里任务控制块(TCB)的指针。
- 参数类型是 `TaskHandle_t *`,函数要把句柄写回您的变量,所以必须传变量的**地址**。
- 它是**可选的输出参数**:传 `NULL` 表示不关心句柄,传 `&xxxHandle` 表示请保存句柄。
- 是否保存句柄,不影响任务本身的创建和运行,只决定之后能否从别处指名操作它。

## 2. 本项目里的用法

```c
xTaskCreate(vTask1, "Task1", 1000, NULL, 2, NULL);              // 不保存
xTaskCreate(vTask2, "Task2", 1000, NULL, 1, &xTask2Handle);     // 保存
```

- Task2 的句柄被保存:Task1 里要执行 `vTaskPrioritySet(xTask2Handle, uxPriority+1)`,必须指明对象是 Task2。
- Task1 的句柄不保存:没有代码需要指名操作 Task1,Task2 只改自己的优先级。

## 3. 判断是否需要保存句柄

**谁会被别人指名操作,谁才需要保存句柄**,与创建先后无关。

| 情况 | 是否需要句柄 |
|------|--------------|
| 别的任务或中断要操作它(改优先级、挂起、恢复、删除、通知等) | 需要 |
| 没人从外部操作它 | 不需要,传 `NULL` |
| 任务只操作自己 | 不需要,用 `NULL` |

可以类比:句柄是任务的名字。要给别人下命令得知道对方叫什么,所以被操作的任务要登记名字。

## 4. `NULL` 的含义

在 `vTaskPrioritySet`、`uxTaskPriorityGet`、`vTaskDelete`、`vTaskSuspend` 等函数里,传 `NULL` 表示**调用这个函数的任务自己**,与写在哪个函数里无关,只看谁在调用。

| 想操作的对象 | 参数 |
|-------------|------|
| 自己 | `NULL` 或自己的句柄 |
| 别的任务 | 必须是目标任务的句柄 |

本项目中:
- Task2 里 `vTaskPrioritySet(NULL, ...)`:改的是 Task2 自己。
- Task1 里 `vTaskPrioritySet(xTask2Handle, ...)`:改的是 Task2。这里不能传 `NULL`,否则改的是 Task1 自己。

## 5. 实际踩到的问题

创建 Task2 时最后一个参数写成了 `NULL`,全局变量 `xTask2Handle` 一直是初始值 `NULL`。

- Task1 的 `vTaskPrioritySet(xTask2Handle, 3)` 实际等价于 `vTaskPrioritySet(NULL, 3)`,改的是 **Task1 自己**。
- Task1 变成最高优先级并且不阻塞,Task2 得不到 CPU。
- 串口只能看到 Task1 的输出。

修复:`xTaskCreate(vTask2, "Task2", 1000, NULL, 1, &xTask2Handle);`

## 6. 注意事项

- 句柄必须在使用前赋值。`xTaskCreate` 在调度器启动前执行完毕,所以任务运行时句柄已有效。
- 需要被其他任务访问的句柄,应定义成全局变量(如 `xTask2Handle`),不要用局部变量。
- 可以检查 `xTaskCreate` 的返回值是否为 `pdPASS`,创建失败(例如内存不足)时句柄无效。
- 将来如果 Task2 要改 Task1 的优先级,Task1 也需要保存句柄(`&xTask1Handle`)。
