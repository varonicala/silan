#
# Makefile for Linux arch/csky/silan source directory
#


#obj-y := config.o timer.o irq.o platform.o padmux.o reset.o clock.o memory.o \
		spi_board_info.o i2c_board_info.o dev-amba.o dev-audio.o console.o usb_init.o
obj-y               := reset.o config.o irq.o  platform.o memory.o\
					   timer.o clock.o padmux.o setup.o\
					   dev-amba.o dev-audio.o\
					   spi_board_info.o i2c_board_info.o usb_init.o pm.o

obj-$(CONFIG_CPU_USE_FIQ) += fiq.o
