convHankakuTxt : convHankakuTxt.c # convHnkakuTxt.cは標準ライブラリをつくのでMacOSのgccを用いる
	gcc convHankakuTxt.c -o convHankakuTxt

hankaku.c : hankaku.txt convHankakuTxt
	./convHankakuTxt

ipl10.bin : ipl10.nas Makefile
	nasm ipl10.nas -o ipl10.bin -l ipl10.lst

asmhead.bin : asmhead.nas Makefile
	nasm asmhead.nas -o asmhead.bin -l asmhead.lst

naskfunc.o : naskfunc.nas Makefile
	nasm -f elf32 naskfunc.nas -o naskfunc.o -l naskfunc.lst

bootpack.hrb : bootpack.c hankaku.c naskfunc.o hrb.ld Makefile
	i386-elf-gcc -march=i486 -m32 -nostdlib -T hrb.ld bootpack.c hankaku.c naskfunc.o -o bootpack.hrb

haribote.sys : asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > haribote.sys

haribote.img : ipl10.bin haribote.sys Makefile
	mformat -f 1440 -C -B ipl10.bin -i haribote.img ::
	mcopy -i haribote.img haribote.sys ::

img :
	make -r haribote.img

run :
	make img
	qemu-system-i386 -drive file=haribote.img,format=raw,if=floppy -boot a

clean :
	rm -f *.lst *.bin *.o *.img *.sys *.hrb hankaku.c convHankakuTxt
