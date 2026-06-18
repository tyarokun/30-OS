#include "bootpack.h"

#define	MEMMAN_FREES	4090	//約32KBの管理領域
#define	MEMMAN_ADDR	0x003c0000

struct FREEINFO{	//空き情報
	unsigned int addr, size;
};

struct MEMMAN{		//メモリ管理
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);

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

	//メモリ管理
	unsigned int memtotal;
	struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
	memtotal = memtest(0x00400000, 0x0bfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	sprintf(s, "memory %dMB  free : %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

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

#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000

unsigned int memtest(unsigned int start, unsigned int end){
	char flg486 = 0;
	unsigned int eflg, cr0, i;
	// 386か、486以降なのかを確認
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT;
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if((eflg & EFLAGS_AC_BIT) != 0){	//386ではAC=1にしても自動で0に戻ってしまう
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT; 	// ACビット = 0
	io_store_eflags(eflg);
	if(flg486 != 0){
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE;	//キャッシュ禁止
		store_cr0(cr0);
	}
	i = memtest_sub(start, end);
	if(flg486 != 0){
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE;	//キャッシュ許可
		store_cr0(cr0);
	}
	return i;
}


void memman_init(struct MEMMAN *man){
	man->frees = 0;		//空き情報の個数
	man->maxfrees = 0;	//状況観察用 : freesの最大値
	man->lostsize = 0;	//解放に失敗した合計サイズ
	man->losts = 0;		//解放に失敗した回数
	return;
}

unsigned int memman_total(struct MEMMAN *man){	//空きサイズの合計を報告
	unsigned int i, t = 0;
	for(i = 0; i < man->frees; i++){
		t += man->free[i].size;
	}
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size){	//確保
	unsigned int i, a;
	for(i = 0; i < man->frees; i++){
		if(man->free[i].size >= size){
			//十分な広さの空きを発見
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if(man->free[i].size == 0){
				//free[i]がなくなったので前へつめる
				man->frees--;
				for(; i < man->frees; i++){
					man->free[i] = man->free[i + 1];	//構造体の代入
				}
			}
			return a;
		}
	}
	return 0;	//空きがない
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size){	//解放
	int i, j;
	// まとめやすさを考えると、free[]がaddr順に並んでいる方がいい
	// だからまず、どこに入れるべきかを決める
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}
	// free[i - 1].addr < addr < free[i].addr
	
	if (i > 0) {
		// 前がある
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			man->free[i - 1].size += size;
			// 前の空き領域にまとめられる
			if (i < man->frees) {
				// 後ろもある
				if (addr + size == man->free[i].addr) {
					// 後ろもまとめられる
					man->free[i - 1].size += man->free[i].size;
					// free[i]の削除
					// free[i]がなくなったので前へつめる
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; //構造体の代入
					}
				}
			}
			return 0; // 成功終了
		}
	}
	// 前とはまとめられなかった
	if (i < man->frees) {
		// 後ろがある
		if (addr + size == man->free[i].addr) {
			//後ろとはまとめられる
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; // 成功終了
		}
	}
	// 前にも後ろにもまとめられない
	if (man->frees < MEMMAN_FREES) {
		// free[i]より後ろを、後ろへずらして、すきまを作る
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees;	//最大値を更新
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0;	// 成功終了
	}
	// 後ろにずらせなかった
	man->losts++;
	man->lostsize += size;
	return -1; // 失敗終了
}