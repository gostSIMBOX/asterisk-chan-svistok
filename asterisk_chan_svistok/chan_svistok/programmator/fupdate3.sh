#!/bin/sh

USB=$1
DEVICE=$2
MODEL=$3

ls -t -1 /var/simnode/firmware/$MODEL/*firmware*_$MODEL_*.bin > /var/simnode/fw_$MODEL.list
BIN=`tail --lines=1 /var/simnode/fw_$MODEL.list`

echo "$USB ($MODEL)< $BIN"

mv $BIN /tmp/fw_$DEVICE.bin
./programmator $USB $DEVICE /tmp/fw_$DEVICE.bin > /tmp/fw_$DEVICE.log
rm /tmp/fw_$DEVICE.bin