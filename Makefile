BINARY   = main
OPENCM3_DIR = libopencm3
DEVICE   = stm32f401re

OBJS     = src/main.o

include $(OPENCM3_DIR)/mk/genlink-config.mk
include $(OPENCM3_DIR)/mk/gcc-config.mk

CFLAGS  += -Isrc -I$(OPENCM3_DIR)/include
LDFLAGS += -L$(OPENCM3_DIR)/lib -nostartfiles -specs=nosys.specs

all: $(BINARY).elf

include $(OPENCM3_DIR)/mk/genlink-rules.mk
include $(OPENCM3_DIR)/mk/gcc-rules.mk

flash: $(BINARY).elf
	openocd -f openocd.cfg -c "program $(BINARY).elf verify reset exit"

clean:
	rm -f src/*.o $(BINARY).elf $(BINARY).bin $(BINARY).map generated.$(DEVICE).ld