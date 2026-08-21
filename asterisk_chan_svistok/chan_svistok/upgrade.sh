#!/bin/sh

cd /usr/simbox/svistok-aa
make clean
./configure
make
make install

gcc programmator/ttyprog_programmator.c -o /usr/simbox/programmator/programmator
gcc hub-ctrl.c -lusb -o /usr/simbox/bin/hub-ctrl

#asterisk -rx "core restart now"
#asterisk start