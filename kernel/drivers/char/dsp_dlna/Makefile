#
#Makefile for the kernel dsp device drivers
#

ifdef CONFIG_MIPS_SILAN_SUVII
obj-$(CONFIG_SILAN_DSP) += dsp_core.o dsp_ctl.o dsp_mem.o mailbox.o queue.o task.o ringbuffer.o aud_pro.o
else
obj-$(CONFIG_SILAN_DSP) += dsp_core.o dsp_ctl.o dsp_mem.o cxc.o queue.o task.o ringbuffer.o aud_pro.o
endif
