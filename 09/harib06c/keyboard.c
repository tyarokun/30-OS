#include "bootpack.h"

struct FIFO8 keyfifo;

void inthandler21(int *esp)
/* PS/2キーボードからの割り込み */
{
	unsigned char data;
	io_out8(PIC0_OCW2, 0x61);	//IRQ01 受付完了をPICに通知
	data = io_in8(PORT_KEYDAT);
	fifo8_put(&keyfifo, data);
	return;
}

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