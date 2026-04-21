CC = gcc
STD_CFLAGS = -Wall -Wno-multichar -std=gnu99 -c -O2

# Architecture detection.
#
# The kernel's arch (uname -m) and the compiler's target (gcc -dumpmachine)
# can differ — a 64-bit aarch64 kernel running a 32-bit armhf userland is a
# common setup on recent Raspberry Pi OS. We therefore pick ABI-specific
# flags (-march / -mfloat-abi / -mfpu) from the *compiler* target, and the
# RASPI peripheral-base macro from the *hardware* CPU model (via lscpu).
GCC_MACHINE := $(shell $(CC) -dumpmachine)
CPU_MODEL   := $(shell lscpu 2>/dev/null | grep "Model name" | awk '{print $$3}')

# Pi generation → RASPI=<n> (drives peripheral base addresses in pi_fm_adv.c).
# RASPI=2 covers Pi 2 (Cortex-A7) and Pi 3 (Cortex-A53), which share the
# same peripheral base at 0x3f000000.
ifeq ($(CPU_MODEL), ARM1176JZF-S)
    PI_MODEL := 1
else ifeq ($(CPU_MODEL), Cortex-A72)
    PI_MODEL := 4
else ifneq (,$(filter $(CPU_MODEL), Cortex-A7 Cortex-A53))
    PI_MODEL := 2
else
    PI_MODEL := unknown
endif

# CFLAGS: (compiler ABI) x (Pi model). Only combinations that actually exist
# in the wild are enumerated; anything else falls through to TARGET=other.
ifneq (,$(findstring aarch64,$(GCC_MACHINE)))
    # AArch64 (64-bit) toolchain. armv8-a applies uniformly.
    ifeq ($(PI_MODEL), 4)
        CFLAGS = $(STD_CFLAGS) -march=armv8-a -mtune=cortex-a72 -ffast-math -DRASPI=4
        TARGET = pi4
    else ifeq ($(PI_MODEL), 2)
        CFLAGS = $(STD_CFLAGS) -march=armv8-a -mtune=cortex-a53 -ffast-math -DRASPI=2
        TARGET = pi2
    else
        CFLAGS = $(STD_CFLAGS)
        TARGET = other
    endif
else ifneq (,$(findstring arm-linux-gnueabihf,$(GCC_MACHINE)))
    # 32-bit armhf (ARM hard-float) toolchain.
    ifeq ($(PI_MODEL), 1)
        CFLAGS = $(STD_CFLAGS) -march=armv6 -mtune=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp -ffast-math -DRASPI=1
        TARGET = pi1
    else ifeq ($(PI_MODEL), 4)
        # Pi 4 on a 32-bit userland — A72 runs any armv7-a in AArch32 mode,
        # and has NEONv4 so we use it for FP perf.
        CFLAGS = $(STD_CFLAGS) -march=armv7-a -mtune=cortex-a72 -mfloat-abi=hard -mfpu=neon-vfpv4 -ffast-math -DRASPI=4
        TARGET = pi4
    else ifeq ($(PI_MODEL), 2)
        # Pi 2 (Cortex-A7) and Pi 3 (Cortex-A53) on armhf. Both support NEON.
        CFLAGS = $(STD_CFLAGS) -march=armv7-a -mtune=cortex-a53 -mfloat-abi=hard -mfpu=neon-vfpv4 -ffast-math -DRASPI=2
        TARGET = pi2
    else
        CFLAGS = $(STD_CFLAGS)
        TARGET = other
    endif
else ifneq (,$(findstring arm-linux-gnueabi,$(GCC_MACHINE)))
    # 32-bit soft-float (armel) toolchain — rare on Pi but supported for Pi 1.
    ifeq ($(PI_MODEL), 1)
        CFLAGS = $(STD_CFLAGS) -march=armv6 -mtune=arm1176jzf-s -ffast-math -DRASPI=1
        TARGET = pi1
    else
        CFLAGS = $(STD_CFLAGS)
        TARGET = other
    endif
else
    CFLAGS = $(STD_CFLAGS)
    TARGET = other
endif

ifneq ($(TARGET), other)

app: rds.o waveforms.o pi_fm_adv.o fm_mpx.o mpx_input.o control_pipe.o mailbox.o
	$(CC) -o pi_fm_mpx rds.o waveforms.o mailbox.o pi_fm_adv.o fm_mpx.o mpx_input.o control_pipe.o -lm -lsndfile -lsoxr

endif


rds_wav: rds.o waveforms.o rds_wav.o fm_mpx.o
	$(CC) -o rds_wav rds_wav.o rds.o waveforms.o fm_mpx.o -lm -lsndfile

rds.o: rds.c waveforms.h
	$(CC) $(CFLAGS) rds.c

control_pipe.o: control_pipe.c control_pipe.h rds.h
	$(CC) $(CFLAGS) control_pipe.c

waveforms.o: waveforms.c waveforms.h
	$(CC) $(CFLAGS) waveforms.c

mailbox.o: mailbox.c mailbox.h
	$(CC) $(CFLAGS) mailbox.c

pi_fm_adv.o: pi_fm_adv.c control_pipe.h fm_mpx.h rds.h mailbox.h mpx_input.h
	$(CC) $(CFLAGS) pi_fm_adv.c

rds_wav.o: rds_wav.c
	$(CC) $(CFLAGS) rds_wav.c

fm_mpx.o: fm_mpx.c fm_mpx.h
	$(CC) $(CFLAGS) fm_mpx.c

mpx_input.o: mpx_input.c mpx_input.h
	$(CC) $(CFLAGS) mpx_input.c

clean:
	rm -f *.o
