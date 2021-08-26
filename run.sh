export PATH="$PATH:$HOME/opt/cross/s390-linux/bin"

if ! test -f stage2/stivale.h; then
    wget https://github.com/stivale/stivale/raw/master/stivale.h
    mv stivale.h stage2/stivale.h
fi

if ! test -f stage2/stivale2.h; then
    wget https://github.com/stivale/stivale/raw/master/stivale2.h
    mv stivale2.h stage2/stivale2.h
fi

make -j || exit
rm flat00.cckd

s390-linux-objcopy -O binary stage1/stage1 stage1/stage1.bin || exit
./tools/bin2rec stage1/stage1.bin stage1.txt || exit
s390-linux-objcopy -O binary stage2/stage2 stage2/stage2.bin || exit

dasdload -bz2 ctl.txt flat00.cckd || exit
hercules -f s390.cnf >hercules.log || exit
