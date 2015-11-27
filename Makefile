# Yubo Zhi (normanzyb@gmail.com)

TRG	= elec3222a1
SUBDIRS	+= FreeRTOS
INCDIRS	+= .

PRGER		= usbasp
MCU_TARGET	= atmega644p
MCU_FREQ	= 12000000

RTOSPORT	= ATMega644P

LIBS	+= -lm

-include Makefile_mac
-include Makefile_llc
-include Makefile_test
include Makefile_AVR.defs
