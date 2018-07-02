#
# Copyright (C) SILAN Technologies, Inc.  All rights reserved.
#
# written by panjianguang  <panjianguang@silan.com.cn>
#
obj-y				:= amon.o cmdline.o init.o\
				    irq.o memory.o platform.o\
				    reset.o setup.o time.o clock.o padmux.o\
					dev-amba.o dev-audio.o\
					spi_board_info.o i2c_board_info.o unicom_cxc.o usb_init.o

#obj-y				+= sie_malloc.o
obj-$(CONFIG_EARLY_PRINTK)	+= console.o

# FIXME FIXME FIXME
obj-$(CONFIG_MIPS_MT_SMTC)	+= smtc.o

EXTRA_CFLAGS += -Werror
