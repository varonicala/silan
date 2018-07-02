
EXTRA_CFLAGS += -Dlinux -DDWC_HS_ELECT_TSC #-DDWC_DEVICE_ONLY
obj-$(CONFIG_USB_DWC_HCD_OLD) += silandwcotg.o

silandwcotg-objs := dwc_otg_driver.o  dwc_otg_cil.o dwc_otg_cil_intr.o dwc_otg_pcd.o dwc_otg_pcd_intr.o \
		    dwc_otg_hcd.o dwc_otg_hcd_intr.o dwc_otg_hcd_queue.o
