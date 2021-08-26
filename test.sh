#!/bin/bash
./compile.sh || exit

rm flat00.cckd

s390-linux-objcopy -O binary barebones/kernel barebones/kernel.bin || exit

dasdload -bz2 ctl.txt flat00.cckd || exit
hercules -f s390.cnf >hercules.log || exit
