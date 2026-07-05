vpath %.s hal/rda/device/TOOLCHAIN_GCC_ARM
vpath %.c main hal/rda hal/rda/device/rda5981
vpath %.h hal/rda hal/rda/device/rda5981

DEFS += -DUNO_81C_U04

INCS += -Imain -Ihal/rda -Ihal/rda/device/rda5981

OBJS += main/main.o main/retarget.o
OBJS += hal/rda/board.o hal/rda/drv_uart.o hal/rda/rda_api.o
OBJS += hal/rda/device/rda5981/system_rda5981.o
OBJS += hal/rda/device/TOOLCHAIN_GCC_ARM/startup_rda5981.o 
OBJS += ../OpenBK7231T_App/libraries/miniz/miniz.o
OBJS += ../OpenBK7231T_App/libraries/miniz/miniz_tdef.o
OBJS += ../OpenBK7231T_App/libraries/miniz/miniz_tinfl.o

CFLAGS += -mcpu=cortex-m4 -mthumb -Wall
CFLAGS += -Os
CFLAGS += -ffunction-sections -fdata-sections

LFLAGS += -mcpu=cortex-m4 -mthumb
LFLAGS += -specs=nosys.specs
LFLAGS += -Wl,--gc-sections -Wl,--wrap=malloc

all:RDA5981_Stub.bin

clean:
	@rm -f $(OBJS) RDA5981_Stub.bin RDA5981_Stub.elf RDA5981_Stub.asm
	
RDA5981_Stub.bin:RDA5981_Stub.elf
	@arm-none-eabi-objcopy -O binary -S $< $@
	@arm-none-eabi-objdump -d $< > bootloader.asm

RDA5981_Stub.elf:$(OBJS)
	@arm-none-eabi-gcc $(LFLAGS) $^ -Thal/rda/device/TOOLCHAIN_GCC_ARM/rda5981.ld -o $@
	@arm-none-eabi-size $@
	
%.o:%.s
	@arm-none-eabi-gcc $(CFLAGS) -c $< -o $@

%.o:%.c
	@arm-none-eabi-gcc $(CFLAGS) $(DEFS) $(INCS) -c $< -o $@
