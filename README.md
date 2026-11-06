# FreeRTOS-STM32F103

FreeRTOS 学习工程,目标芯片 **STM32F103RCT6**(Cortex-M3, 256K Flash / 48K RAM, 使用 `stm32f103xe` 设备头文件)。

## 目录结构
- `Core/` 应用代码、`FreeRTOSConfig.h`、`system_stm32f1xx.c`
- `Startup/` 启动文件
- `Drivers/CMSIS/` CMSIS 核心与 STM32F1 设备头文件
- `Middlewares/FreeRTOS/` FreeRTOS 内核(ARM_CM3 移植, heap_4)
- `STM32F103RCTx_FLASH.ld` 链接脚本

## 构建与烧录
需要 `arm-none-eabi-gcc`(`sudo apt install gcc-arm-none-eabi`)和 `openocd`。

    make          # 编译
    make flash    # 通过 ST-Link 烧录
    make clean

## 说明
- 时钟:8 MHz HSE → PLL → 72 MHz(`SystemInit`)。
- `Core/Src/main.c` 目前只有时钟与调度器初始化,在 `vTaskStartScheduler()` 前创建自己的任务即可。
