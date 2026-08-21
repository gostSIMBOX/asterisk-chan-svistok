#!/bin/sh
gcc programmator/ttyprog_programmator.c -o /usr/simbox/programmator/programmator
gcc hub-ctrl.c -lusb -o /usr/simbox/bin/hub-ctrl
