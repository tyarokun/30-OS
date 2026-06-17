#include "bootpack.h"

struct MOUSE_DEC{
	unsigned char buf[3], phase;
	int x, y, btn;
};

void init_keyboard(void);
void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

void HariMain(void){
	//HariMainの変数
	struct BOOTINFO *binfo = (struct BOOTINFO *)0x0ff0;;
	char s[40], mcursor[256], keybuf[32], mousebuf[128];
	int mx, my , i, j;
	struct MOUSE_DEC mdec;

	//GDT/IDT 初期化
	init_gdtidt();
	init_pic();
	io_sti(); /* IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除 */
	io_out8(PIC0_IMR, 0xf9); /* PIC1とキーボードを許可(11111001) */
	io_out8(PIC1_IMR, 0xef); /* マウスを許可(11101111) */

	//パレット初期化
	init_palette();

	//スクリーン
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

	//マウスカーソル
	mx = binfo->scrnx / 2;
	my = binfo->scrny / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

	//キーボード, マウス有効化
	init_keyboard();
	enable_mouse(&mdec);
	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);


	for(;;){
		io_cli();
		if(fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0){
			io_stihlt();
		}else{
			if(fifo8_status(&keyfifo) != 0){
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s, "%x", i);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
				putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
			}else if(fifo8_status(&mousefifo) != 0){
				i = fifo8_get(&mousefifo);
				io_sti();
				if(mouse_decode(&mdec, i) != 0){
					sprintf(s, "[lcr %d %d]", mdec.x, mdec.y);
					if((mdec.btn & 0x01) != 0){
						s[1] = 'L';
					}
					if((mdec.btn & 0x02) != 0){
						s[3] = 'R';
					}
					if((mdec.btn & 0x04) != 0){
						s[2] = 'C';
					}
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
					putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
					// マウスカーソルの移動
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);	//マウス消す
					mx += mdec.x;	//マウスのx座標更新
					my += mdec.y;	//マウスのy座標更新
					if(mx < 0){	//左画面から出ないように調整
						mx = 0;
					}
					if(my < 0){	//上画面から出ないように調整
						my = 0;
					}
					if(mx > binfo->scrnx - 16){	//右画面から出ないように調整
						mx = binfo->scrnx - 16;
					}
					if(my > binfo->scrny - 16){	//下画面から出ないように調整
						my = binfo->scrny - 16;
					}
					sprintf(s, "%d %d", mx, my);	//マウス位置の座標を更新
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);			//座標消す
					putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);			//座標書く
					putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);	//マウス描く
				}
			}
			
		}
	}
}

#define PORT_KEYDAT				0x0060	//KBC(key board controller)(キーボード制御回路)のデータポート
#define PORT_KEYSTA				0x0064	//0x0064は読み取りの時ステータスレジスタ
#define PORT_KEYCMD				0x0064 	//0x0064は書き込みの時コマンドレジスタ
#define KEYSTA_SEND_NOTREADY	0x02	//下から2bit目が0になるとコマンド受付可能
#define KEYCMD_WRITE_MODE		0x60	//コマンドを書くモード
#define KBC_MODE				0x47	//マウス利用モード

void wait_KBC_sendready(void){	//KBCが準備できるまで待つ
	for(;;){
		if((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0){
			break;
		}
	}
	return;
}

void init_keyboard(void){
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);	//キーボード制御回路へこれからモードを設定することを知らせる
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);				//キーボード制御回路へマウス利用モードを送信
	return;
}

#define KEYCMD_SENDTO_MOUSE	0xd4
#define MOUSECMD_ENABLE		0xf4

void enable_mouse(struct MOUSE_DEC *mdec){
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	//うまくいくとACK(0xfa)が送信されてくる
	mdec->phase = 0;
	return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat){
	if(mdec->phase == 0){
		if(dat == 0xfa){
			mdec->phase = 1;
		}
		return 0;
	}
	if(mdec->phase == 1){
		if((dat & 0xc8) == 0x08){
			//正しい1バイト目だった時(1バイト目は、移動に反応する桁は0~3の範囲, クリックに反応する桁は8~Fの範囲)
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}
		return 0;
	}
	if(mdec->phase == 2){
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	if(mdec->phase == 3){
		mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;	//ボタンの状態はバフ0の下位3bit(0x07 = 0b0111)
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		/*
		1バイト目の情報を使ってx,yの第8ビットより上を全部1にするか0のままにするかを決める
		→1バイト目(buf[0])には移動量の付合情報も入っている
			buf[0] の bit4 = X方向の符号
			buf[0] の bit5 = Y方向の符号
		→負の時:1, 正の時:2
		*/
		if((mdec->buf[0] & 0x10) != 0){	//負の時
			mdec->x |= 0xffffff00;
		}
		if((mdec->buf[0] & 0x20) != 0){	//負の時
			mdec->y |= 0xffffff00;
		}
		mdec->y = - mdec->y; //マウスではy方向の符号が画面と反対
		return 1;
	}
	return -1;
}