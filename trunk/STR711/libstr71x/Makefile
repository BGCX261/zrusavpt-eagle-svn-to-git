# library makefile by Martin Thomas (based on the efsl library makefile)

MCU   = arm7tdmi
THUMB = -mthumb -mthumb-interwork
#THUMB =

LIBNAME = libSTR71Xlibstd.a

COPT= -mcpu=$(MCU) $(THUMB) -gdwarf-2 -Wall -Os
INCLUDEDIRS=-Iinc
CFLAGS=$(COPT) $(INCLUDEDIRS)
# gcc4 unused code-removal:
CFLAGS += -ffunction-sections -fdata-sections -Wall

CC=arm-none-eabi-gcc
AR=arm-none-eabi-ar
OBJCOPY=arm-none-eabi-objcopy

OBJ=src/71x_lib.o       src/71x_adc12.o     src/71x_apb.o      src/71x_bspi.o
OBJ+=src/71x_can.o      src/71x_eic.o       src/71x_emi.o      src/71x_gpio.o
OBJ+=src/71x_i2c.o      src/71x_pcu.o       src/71x_rccu.o     src/71x_rtc.o
OBJ+=src/71x_tim.o      src/71x_uart.o      src/71x_wdg.o      src/71x_xti.o


all: lib

libandclean: lib srcclean

lib: $(OBJ)
	$(AR) rcs $(LIBNAME) $(OBJ)
	
srcclean :
	rm -f $(OBJ)

clean :
	rm -f $(OBJ) $(LIBNAME)
