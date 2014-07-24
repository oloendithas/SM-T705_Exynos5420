#!/bin/sh

make -j4
rm /home/arthur/android/samkitchen/unpack_boot/boot.img-zImage
cp arch/arm/boot/zImage /home/arthur/android/samkitchen/unpack_boot/boot.img-zImage

