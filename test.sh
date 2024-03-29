#!/bin/bash

export PATH="$PATH:$HOME/opt/cross/s390-linux/bin"
#./configure --host=s390-linux --target=s390-linux

./compile.sh || exit

rm flat00.cckd

s390x-ibm-linux-objcopy -O binary barebones/kernel barebones/kernel.bin || exit

dasdload -bz2 ctl.txt flat00.cckd || exit
hercules -f s390.cnf >hercules.log || exit
