#!/bin/sh

#
# Build the kernel again (just a few commands I'm bored to type)
#

KERNEL="GENERIC_TINY486SX"

cd /usr/src/sys/arch/i386/conf
config "$KERNEL" &&
    cd ../compile/"$KERNEL" &&
    make clean &&
    make depend &&
    make
