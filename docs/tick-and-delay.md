# 滴答时钟与精确延时(ms)的设置流程

## 1. 核心概念

| 名称 | 作用 |
|---|---|
| `configCPU_CLOCK_HZ` | CPU 实际主频,本项目为 `SystemCoreClock` |
| `configTICK_RATE_HZ` | 滴答中断频率,决定 1 tick 的时长:`1 tick = 1000 / configTICK_RATE_HZ ms` |
| `pdMS_TO_TICKS(ms)` | 把毫秒换算成 tick:`ms * configTICK_RATE_HZ / 1000`(整数除法,向下取整) |
| `vTaskDelay(ticks)` | 参数单位是 **tick**,不是毫秒 |

SysTick 重装值由端口层(`port.c`)自动计算:
`configCPU_CLOCK_HZ / configTICK_RATE_HZ - 1`。

## 2. 设置流程

1. **确认主频正确**
   - `SystemInit()` 配好时钟后调用 `SystemCoreClockUpdate()`,保证 `SystemCoreClock` 等于真实主频。
   - `configCPU_CLOCK_HZ` 必须等于真实主频,否则所有延时都会按比例偏差。
2. **选择滴答频率** `configTICK_RATE_HZ`(在 `Core/Inc/FreeRTOSConfig.h`)
   - 常用 1000 Hz(1 ms 分辨率);100 Hz 开销更小但分辨率只有 10 ms。
   - 选择依据:最短需要的延时精度。比 1 tick 更短的延时无法表达。
3. **所有延时一律用 `pdMS_TO_TICKS`**
   ```c
   vTaskDelay(pdMS_TO_TICKS(500));   // 正确:与滴答频率无关
   vTaskDelay(500);                  // 错误:改频率后时长会变
   ```
4. **检查取整**:`ms` 必须是 `1000 / configTICK_RATE_HZ` 的整数倍。
   - 100 Hz 时 `pdMS_TO_TICKS(5)` = 0,不足 1 tick。
   - 需要更细的精度就提高 `configTICK_RATE_HZ`。
5. **需要周期性精确节拍时用 `vTaskDelayUntil`**
   ```c
   TickType_t xLast = xTaskGetTickCount();
   for (;;) {
       vTaskDelayUntil(&xLast, pdMS_TO_TICKS(100));
   }
   ```
   `vTaskDelay` 是"从现在起等",每次循环会累积任务执行耗时;`vTaskDelayUntil` 以固定周期为基准,不累积误差。
6. **实测验证**(见第 4 节)。

## 3. 精度说明

- 延时精度为 **±1 tick**:调用时刻通常落在两次滴答之间,所以实际延时会比请求值少 0~1 tick。
- 滴答本身的时间精度取决于时钟源(HSE 晶振约几十 ppm,HSI 约 1%)。
- 高优先级任务、中断、`taskENTER_CRITICAL` 会推迟滴答和任务唤醒。
- 需要微秒级精度时,不要用 `vTaskDelay`,改用硬件定时器或 DWT 周期计数器。

## 4. 本项目的验证方法

`Core/Src/tick_test.c` 用 DWT 周期计数器(`DWT->CYCCNT`)测量:

- 滴答周期:在 `vApplicationTickHook` 中记录相邻滴答间的周期数,期望值为 `SystemCoreClock / configTICK_RATE_HZ`。
- 延时时长:延时前后读取 `DWT->CYCCNT`,换算成 ms/us。

运行:

```
make run        # 烧录并在终端显示 vPrintString 输出 (Ctrl+C 退出)
```

输出示例(100 Hz):

```
[demo] vTaskDelay(pdMS_TO_TICKS(1000)) -> real wait 999 ms    # 约 1 秒,正确
[demo] vTaskDelay(1000)                -> real wait 9999 ms   # 约 10 秒,错误
```

写死 `vTaskDelay(1000)` 要 10 秒才打印一次,请至少等待 11 秒再观察。

## 5. 对照示例

| `configTICK_RATE_HZ` | 1 tick | `pdMS_TO_TICKS(1000)` | `vTaskDelay(1000)` 实际时长 | `pdMS_TO_TICKS(5)` |
|---|---|---|---|---|
| 1000 | 1 ms | 1000 | 1 s | 5 |
| 100 | 10 ms | 100 | 10 s | 0 |

## 6. 注意事项

- 实测周期为 80000 周期/滴答(100 Hz)时,说明 `SystemCoreClock` 为 8 MHz;若预期为 72 MHz,需检查 PLL 是否配置成功。
- 半主机打印(`vPrintString`)依赖调试器连接,且打印过多会拖慢程序。
- `configUSE_TICK_HOOK` 目前为 1 仅用于测试,正式使用可改回 0。
