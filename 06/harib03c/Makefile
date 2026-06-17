OBJS_BOOTPACK = bootpack.o graphic.o dsctbl.o naskfunc.o hankaku.o mysprintf.o

CC = i386-elf-gcc
CFLAGS = -march=i486 -m32 -nostdlib -fno-builtin

default :
	make img

convHankakuTxt : convHankakuTxt.c # convHankakuTxt.cは標準ライブラリを使うのでMacOSのgccを用いる
	gcc $< -o $@

hankaku.c : hankaku.txt convHankakuTxt
	./convHankakuTxt

ipl10.bin : ipl10.nas
	nasm ipl10.nas -o ipl10.bin -l ipl10.lst

asmhead.bin : asmhead.nas
	nasm $< -o $@ -l asmhead.lst

naskfunc.o : naskfunc.nas
	nasm -f elf32 $< -o $@ -l naskfunc.lst

bootpack.hrb : $(OBJS_BOOTPACK) hrb.ld 
	$(CC) $(CFLAGS) -T hrb.ld -g $(OBJS_BOOTPACK) -o $@

haribote.sys : asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > haribote.sys

haribote.img : ipl10.bin haribote.sys Makefile
	mformat -f 1440 -C -B ipl10.bin -i haribote.img ::
	mcopy -i haribote.img haribote.sys ::

# 一般規則
%.o : %.c
	$(CC) $(CFLAGS) -c $*.c -o $*.o
#以下のようにも書ける
#%.o : %.c
#	$(CC) $(CFLAGS) -c $< -o $@

# コマンド 
img :
	make -r haribote.img

run :
	make img
	qemu-system-i386 -drive file=haribote.img,format=raw,if=floppy -boot a

clean :
	rm -f *.lst *.bin *.o *.img *.sys *.hrb hankaku.c convHankakuTxt
