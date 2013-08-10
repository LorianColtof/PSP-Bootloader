TARGET = kmodlib

OBJS = kmodlib_impl.o
PRX_EXPORTS = kmodlib.exp

#PSP_FW_VERSION = 371
BUILD_PRX = 1
USE_KERNEL_LIBC = 1
USE_KERNEL_LIBS = 1

INCDIR =
LIBDIR =
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LDFLAGS = -mno-crt0 -nostartfiles

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
