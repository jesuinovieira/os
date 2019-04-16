#!/bin/sh

device="lcdisplay"

major=`cat /proc/devices | awk "{if(\\$2==\"$device\")print \\$1}"`
minor=0

rm -f /dev/${device} c $major $minor
