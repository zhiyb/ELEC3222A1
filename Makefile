# Yubo Zhi (normanzyb@gmail.com)

TRG	= elec3222a1
SUBDIRS	+= FreeRTOS

PRGER		= usbasp
MCU_TARGET	= atmega644p
MCU_FREQ	= 12000000

LIBS	+= -lm

-include Makefile_phy
-include Makefile_dll
-include Makefile_net
-include Makefile_tran
-include Makefile_app
-include Makefile_test
include Makefile_AVR.defs
