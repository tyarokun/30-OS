#include "bootpack.h"

extern struct FIFO8 keyfifo;

void HariMain(void){
	//HariMainの変数
	struct BOOTINFO *binfo = (struct BOOTINFO *)0x0ff0;;
	char s[12], mcursor[256], keybuf[32];
	int mx, my , i, j;

	//GDT/IDT 初期化
	init_gdtidt();
	init_pic();
	io_sti(); /* IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除 */

	//パレット初期化
	init_palette();

	//スクリーン
	init_screen(binfo->vram, binfo->scrnx, binfo->scrny);


	//マウスカーソル
	mx = binfo->scrnx / 2;
	my = binfo->scrny / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

	//キーボード
	fifo8_init(&keyfifo, 32, keybuf);

	io_out8(PIC0_IMR, 0xf9); /* PIC1とキーボードを許可(11111001) */
	io_out8(PIC1_IMR, 0xef); /* マウスを許可(11101111) */

	for(;;){
		io_cli();
		if(fifo8_status(&keyfifo) == 0){
			io_stihlt();
		}else{
			i = fifo8_get(&keyfifo);
			io_sti();
			sprintf(s, "%x", i);
			boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
			putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
		}
	}
}
