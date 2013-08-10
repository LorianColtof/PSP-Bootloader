#	Makefile for PSP Bootloader
#	Created by Lorian Coltof
#
# Configuration:

MAJOR_VERSION = 1
MINOR_VERSION = 0

INSTALL_DIR := /media/disk/PSP/GAME/pspboot

ENABLE_DEBUG_LOG=1
ENABLE_ERROR_LOG=1
ENABLE_UART3=0


TARGET = pspbootloader
OBJS = main.o strings.o kmodlibloader.o configloader.o menu.o kernel.o


# Other stuff:

ifndef FW150
OBJS += kmodlib.o kmodlibloader.o
endif

ifeq ($(ENABLE_UART3), 1) 
OBJS += uart3.o
endif

INCDIR = 
CFLAGS = -DMAJOR_VERSION=$(MAJOR_VERSION) -DMINOR_VERSION=$(MINOR_VERSION)
	
ifeq ($(ENABLE_DEBUG_LOG), 1) 
CFLAGS += -DENABLE_DEBUG_LOG
endif

ifeq ($(ENABLE_ERROR_LOG), 1) 
CFLAGS += -DENABLE_ERROR_LOG
endif

ifeq ($(ENABLE_UART3), 1) 
CFLAGS += -DENABLE_UART3
endif

CFLAGS += -O2 -G0 -Wall

#-DENABLE_DEBUG_LOG=$(ENABLE_DEBUG_LOG) -DENABLE_ERROR_LOG=$(ENABLE_ERROR_LOG) -DENABLE_UART3=$(ENABLE_UART3) 
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

ifdef FW150
CFLAGS += -DFW150
endif

LIBS += -lm -lz -lbz2
LIBDIR =
LDFLAGS =

EXTRA_TARGETS = EBOOT.PBP

ifdef FW150
PSP_EBOOT_TITLE = PSP Bootloader $(MAJOR_VERSION).$(MINOR_VERSION) (for FW150)
else
EXTRA_TARGETS += ./kmodlib/kmodlib.prx
PSP_EBOOT_TITLE = PSP Bootloader $(MAJOR_VERSION).$(MINOR_VERSION)
endif

PSP_EBOOT_ICON = icon.png
PSP_EBOOT_PIC1 =
PSP_EBOOT_SND0 =

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

# Extra rules:

./kmodlib/kmodlib.prx:
	$(MAKE) -C ./kmodlib

kmodlib.o: kmodlib.S

kmodlib.S: ./kmodlib/kmodlib.exp
	psp-build-exports -s $<

clean: kmodlib_clean

.PHONY: kmodlib_clean
kmodlib_clean:
	$(MAKE) -C ./kmodlib clean
	rm -f kmodlib.S

.PNONY: install
install: $(EXTRA_TARGETS)
	cp ./EBOOT.PBP $(INSTALL_DIR)/EBOOT.PBP
ifndef FW150
	cp ./kmodlib/kmodlib.prx $(INSTALL_DIR)/kmodlib.prx
endif
	cp ./pspboot.conf $(INSTALL_DIR)/./pspboot.conf
	@echo "*** Done ***"
