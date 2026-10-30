# Aurora OS - build system
NASM    ?= nasm
CC      ?= gcc
LD      ?= ld
OBJCOPY ?= objcopy

BUILD := build

CFLAGS := -m32 -std=gnu11 -O2 -g -ffreestanding -fno-pie -fno-stack-protector -fno-strict-aliasing \
          -fno-builtin -nostdinc -Wall -Wextra -Werror \
          -mno-sse -mno-sse2 -mno-mmx \
          -Ikernel -Ikernel/lib -I$(shell $(CC) -print-file-name=include)
LDFLAGS := -m elf_i386 -nostdlib -T linker.ld

C_SRCS   := $(wildcard kernel/*.c kernel/lib/*.c)
ASM_SRCS := $(wildcard kernel/*.asm)
OBJS     := $(patsubst kernel/%.c,$(BUILD)/%.o,$(C_SRCS)) \
            $(patsubst kernel/%.asm,$(BUILD)/%.asm.o,$(ASM_SRCS))

all: $(BUILD)/aurora.img

$(BUILD)/%.o: kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.asm.o: kernel/%.asm
	@mkdir -p $(dir $@)
	$(NASM) -f elf32 $< -o $@

$(BUILD)/kernel.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/defines.inc: $(BUILD)/kernel.bin
	@KS=$$(( ($$(stat -c%s $<) + 511) / 512 )); \
	{ echo "%define LOAD_SEG 0x1000"; \
	  echo "%define LOADER_SECTORS 8"; \
	  echo "%define KERNEL_SECTORS $$KS"; \
	  echo "%define KERNEL_SIZE ($$KS*512)"; \
	  echo "%define BLOB_SECTORS (8+$$KS)"; } > $@
	@echo "kernel: $$(stat -c%s $<) bytes, $$KS sectors"

$(BUILD)/boot.bin: boot/boot.asm $(BUILD)/defines.inc
	$(NASM) -f bin -I$(BUILD)/ $< -o $@
	@test $$(stat -c%s $@) -eq 512 || (echo "boot sector must be 512 bytes"; false)

$(BUILD)/loader.bin: boot/loader.asm $(BUILD)/defines.inc
	$(NASM) -f bin -I$(BUILD)/ $< -o $@
	@test $$(stat -c%s $@) -le $$((8*512)) || (echo "loader too big for 8 sectors"; false)

$(BUILD)/aurora.img: $(BUILD)/boot.bin $(BUILD)/loader.bin $(BUILD)/kernel.bin
	cat $(BUILD)/boot.bin > $@
	dd if=$(BUILD)/loader.bin of=$@ bs=512 seek=1 conv=notrunc status=none
	dd if=$(BUILD)/kernel.bin of=$@ bs=512 seek=9 conv=notrunc status=none
	@KS=$$(( ($$(stat -c%s $(BUILD)/kernel.bin) + 511) / 512 )); \
	truncate -s $$(( (9 + $$KS) * 512 )) $@
	@echo "image: $@"

run: $(BUILD)/aurora.img
	./scripts/run.sh $(BUILD)/aurora.img

clean:
	rm -rf $(BUILD)

.PHONY: all run clean
