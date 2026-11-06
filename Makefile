TARGET := freertos_stm32f103rc
BUILD  := build
CC     := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE   := arm-none-eabi-size

FREERTOS := Middlewares/FreeRTOS

SRCS := $(wildcard Core/Src/*.c) \
        $(addprefix $(FREERTOS)/,tasks.c queue.c list.c timers.c event_groups.c stream_buffer.c) \
        $(FREERTOS)/portable/GCC/ARM_CM3/port.c \
        $(FREERTOS)/portable/MemMang/heap_4.c
ASMS := Startup/startup_stm32f103xe.s

INCS := -ICore/Inc -I$(FREERTOS)/include -I$(FREERTOS)/portable/GCC/ARM_CM3 \
        -IDrivers/CMSIS/Include -IDrivers/CMSIS/Device/ST/STM32F1xx/Include

# STM32F103RCT6 is a high-density device -> stm32f103xe
CPU    := -mcpu=cortex-m3 -mthumb
CFLAGS := $(CPU) -DSTM32F103xE -Os -g3 -Wall -ffunction-sections -fdata-sections $(INCS)
LDFLAGS := $(CPU) -TSTM32F103RCTx_FLASH.ld --specs=nano.specs --specs=nosys.specs \
           -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map

OBJS := $(addprefix $(BUILD)/,$(SRCS:.c=.o) $(ASMS:.s=.o))

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin
	$(SIZE) $<

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(CPU) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Flash via CMSIS-DAP (SWD), connect under reset
flash: all
	openocd -f interface/cmsis-dap.cfg -c "transport select swd" -f target/stm32f1x.cfg \
	  -c "adapter speed 200" -c "reset_config srst_only srst_nogate connect_assert_srst" \
	  -c "program $(BUILD)/$(TARGET).elf verify reset exit"

# Reset the target and show vPrintString output in this terminal (Ctrl+C to quit)
monitor:
	openocd -f interface/cmsis-dap.cfg -c "transport select swd" -f target/stm32f1x.cfg \
	  -c "adapter speed 200" -c "reset_config srst_only srst_nogate connect_assert_srst" \
	  -c "init" -c "arm semihosting enable" -c "reset run"

clean:
	rm -rf $(BUILD)

-include $(OBJS:.o=.d)
.PHONY: all flash monitor clean
