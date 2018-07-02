#!/bin/bash

# Author: Panjianguang

RELEASE=`pwd`
KERNELROOT=$RELEASE/..

cd $KERNELROOT

rm -rf $KERNELROOT/.git*

mv $KERNELROOT/arch/mips/configs/silan_releaseconfig $KERNELROOT/arch/mips/configs/silan_defconfig
make silan_defconfig
make -j8

cd $KERNELROOT/drivers/video/suvii/
if [ -f silanfb.ko ];then
cp -rf silanfb.ko $KERNELROOT/release
fi
cd $KERNELROOT
rm -rf $KERNELROOT/drivers/video/suvii

cd $KERNELROOT/drivers/video/suviii/
if [ -f silanfb.ko ];then
cp -rf silanfb.ko $KERNELROOT/release
fi
cd $KERNELROOT
rm -rf $KERNELROOT/drivers/video/suviii
sed -i '/SUV/d' $KERNELROOT/drivers/video/Makefile

cp -rf $KERNELROOT/drivers/usb/host/dwcotg/silandwcotg.ko $KERNELROOT/release
rm -rf $KERNELROOT/drivers/usb/host/dwcotg
sed -i '/DWC_HCD/d' $KERNELROOT/drivers/usb/host/Makefile

cp -rf $KERNELROOT/drivers/mmc/host/silan_mmc.ko $KERNELROOT/release
rm -rf $KERNELROOT/drivers/mmc/host/silan_mmc.c
rm -rf $KERNELROOT/drivers/mmc/host/silan_mmc.h
sed -i '/SILAN/d' $KERNELROOT/drivers/mmc/host/Makefile

cd $KERNELROOT/drivers/gpu/silan/gpu2.x
if [ -f silangpu.ko ];then
cp -rf silangpu.ko $KERNELROOT/release
fi
cd $KERNELROOT
rm -rf $KERNELROOT/drivers/gpu/silan/gpu2.x

cd $KERNELROOT/drivers/gpu/silan/gpu4.x
if [ -f silangpu.ko ];then
cp -rf silangpu.ko $KERNELROOT/release
fi
cd $KERNELROOT
rm -rf $KERNELROOT/drivers/gpu/silan/gpu4.x
sed -i '/silan/d' $KERNELROOT/drivers/gpu/Makefile

sed -i '/CONFIG_VPP_SUV/d' $KERNELROOT/arch/mips/configs/silan_defconfig
sed -i '/CONFIG_MMC_SILAN/d' $KERNELROOT/arch/mips/configs/silan_defconfig
sed -i '/CONFIG_SILAN_GPU/d' $KERNELROOT/arch/mips/configs/silan_defconfig
sed -i '/CONFIG_USB_DWC_HCD/d' $KERNELROOT/arch/mips/configs/silan_defconfig

#mv $KERNELROOT/arch/mips/configs/silan_userconfig $KERNELROOT/arch/mips/configs/silan_defconfig
make silan_defconfig
make -j8

rm -rf $RELEASE/release.sh
