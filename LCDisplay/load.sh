#!/bin/sh

device="lcdisplay"

major=`cat /proc/devices | awk "{if(\\$2==\"$device\")print \\$1}"`
minor=0

rm -f /dev/${device} c $major $minor

# Create the file which will be used to access the device driver
mknod /dev/${device} c $major $minor

chmod 666 /dev/${device}
