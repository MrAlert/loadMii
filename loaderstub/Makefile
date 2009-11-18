CROSS ?= powerpc-eabi-

ifeq ($(origin AR), default)
	AR := $(CROSS)ar
endif
ifeq ($(origin AS), default)
	AS := $(CROSS)as
endif
ifeq ($(origin CC), default)
	CC := $(CROSS)gcc -m32
endif
ifeq ($(origin LD), default)
	LD := $(CROSS)ld
endif
OBJCOPY ?= $(CROSS)objcopy
RANLIB ?= $(CROSS)ranlib

CFLAGS := -Wall -Os -ffreestanding -std=gnu99 -Wstrict-aliasing=2 \
	-mno-eabi -mno-sdata -mcpu=750
DEBUG_CFLAGS := $(CFLAGS) -g -DDEBUG

LDFLAGS := -n

lib := lib/libloaderstub.a
debug_lib := lib/libloaderstub-debug.a

targets := loaderstub.o
debug_targets := loaderstub-debug.o

objs := string.o cache.o time.o ios.o sd.o fat.o elf.o stm.o main.o crt0.o
debug_objs := $(objs:.o=.debug.o) font.debug.o fb.debug.o debug.debug.o

headers := src/types.h src/string.h src/util.h src/cache.h src/time.h \
	src/ios.h src/sd.h src/fat.h src/elf.h src/stm.h src/debug.h
debug_headers := $(headers) src/font.h src/fb.h

ifeq ($(V),1)
	Q :=
else
	Q := @
	MAKEFLAGS += --no-print-directory
endif

ifeq ($(DEBUG),1)
	target_lib := $(debug_lib)
else
	target_lib := $(lib)
endif

all: $(target_lib)

$(lib): $(targets)
	@echo "  AR        $@"
	$(Q)rm -f $@
	$(Q)$(AR) rc $@ $^
	@echo "  RANLIB    $@"
	$(Q)$(RANLIB) $@

$(debug_lib): $(debug_targets)
	@echo "  AR        $@"
	$(Q)rm -f $@
	$(Q)$(AR) rc $@ $^
	@echo "  RANLIB    $@"
	$(Q)$(RANLIB) $@

$(targets) $(debug_targets): %.o: %.bin
	@echo "  BIN2O     $@"
	$(Q)bin2s $< | $(AS) -o $@

bins := $(targets:.o=.bin)
debug_bins := $(debug_targets:.o=.bin)
$(bins) $(debug_bins): %.bin: %.elf
	@echo "  OBJCOPY   $@"
	$(Q)$(OBJCOPY) -O binary $< $@

elfs := $(bins:.bin=.elf)
$(elfs): %.elf: %.lds $(objs)
	@echo "  LINK      $@"
	$(Q)$(LD) $(LDFLAGS) -T $^ -o $@

debug_elfs := $(debug_bins:.bin=.elf)
$(debug_elfs): %.elf: %.lds $(debug_objs)
	@echo "  LINK      $@"
	$(Q)$(LD) $(LDFLAGS) -T $^ -o $@

%.o: src/%.c $(headers)
	@echo "  COMPILE   $@"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

%.debug.o: src/%.c $(debug_headers)
	@echo "  COMPILE   $@"
	$(Q)$(CC) $(DEBUG_CFLAGS) -c $< -o $@

%.o: src/%.S
	@echo "  ASSEMBLE  $@"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

%.debug.o: src/%.S
	@echo "  ASSEMBLE  $@"
	$(Q)$(CC) $(DEBUG_CFLAGS) -c $< -o $@

clean:
	rm -rf $(lib) $(targets) $(bins) $(elfs) $(objs)
	rm -rf $(debug_lib) $(debug_targets) $(debug_bins) $(debug_elfs) $(debug_objs)
