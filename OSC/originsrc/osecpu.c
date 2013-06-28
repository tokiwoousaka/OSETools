#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <setjmp.h>

#if (!defined(JITC_OSNUM))
	#if (defined(_WIN32))
		#define JITC_OSNUM			0x0001
	#endif
	#if (defined(__APPLE__))
		#define JITC_OSNUM			0x0002
	#endif
	#if (defined(__linux__))
		#define JITC_OSNUM			0x0003
	#endif
	/* 0001: win32-x86-32bit */
	/* 0002: MacOS-x86-32bit */
	/* 0003: linux-x86-32bit */
#endif

#define USE_TEK5		1

#if (USE_TEK5 != 0)
	#include "tek.c"
	int tek5Decomp(UCHAR *buf, UCHAR *buf1, UCHAR *tmp);
#else
	typedef unsigned char UCHAR;
#endif

#define	APPSIZ1		1024 * 1024 /* とりあえず1MBで */
#define	JITSIZ1		1024 * 1024 /* とりあえず1MBで */
#define	SJITSIZ1	1024 * 1024 /* とりあえず1MBで */
#define	SYSLIBSIZ1	1024 * 1024 /* とりあえず1MBで */

struct PtrCtrl {
	int liveSign;
	int size;
	UCHAR *p0;
};

struct Ptr {	/* 32バイト(=256bit!) */
	UCHAR *p;
	int typ;
	UCHAR *p0, *p1;
	int liveSign;
	struct PtrCtrl *pls;
	int flags, dummy;	/* read/writeなど */
};

struct Regs {
	int ireg[64]; /* 32bit整数レジスタ */
	struct Ptr preg[64];	/* ポインタレジスタ */

	int debugInfo0, debugInfo1; /* 2304 */
	char winClosed, autoSleep;
	jmp_buf *setjmpEnv;

	/* for-junkApi */
	int argc;
	const UCHAR **argv;
	UCHAR *buf0, *buf1, *junkStack, lastConsoleChar;
};

struct LabelTable {
	UCHAR *p;
	int typ;
};

/* JITCのフラグ群 */
#define	JITC_LV_SLOWEST		0	/* デバッグ支援は何でもやる */
#define	JITC_LV_SLOWER		1	/* エラーモジュールはレポートできるが、行番号は分からない、テストは過剰 */
#define	JITC_LV_SAFE		2	/* とにかく止まる、場所は不明、テストは必要最小限 */
#define	JITC_LV_FASTER		4	/* 情報は生成するがチェックをしない */
#define	JITC_LV_FASTEST		5	/* 情報すら生成しない */
#define JITC_PHASE1			0x0001
#define JITC_SKIPCHECK		0x0002	/* セキュリティチェックを省略する（高速危険モード） */

#define JITC_MAXLABELS		4096

/* JITC_ARCNOについて */
	/* 0001: x86-32bitモード */

#if (JITC_OSNUM == 0x0001)	/* win32-x86-32bit */
	#define	JITC_ARCNUM			0x0001	/* x86-32bit */
	#define DRV_OSNUM			0x0001	/* win32 */
#endif

#if (JITC_OSNUM == 0x0002)	/* MacOS-x86-32bit */
	#define JITC_ARCNUM			0x0001	/* x86-32bit */
	#define DRV_OSNUM			0x0002	/* MacOS */
#endif

#if (JITC_OSNUM == 0x0003)	/* linux-x86-32bit */
	#define JITC_ARCNUM			0x0001	/* x86-32bit */
	#define DRV_OSNUM			0x0000	/* unknown */
#endif

#define KEYBUFSIZ		4096
static int *keybuf, keybuf_r, keybuf_w, keybuf_c;
static int *vram = NULL, v_xsiz, v_ysiz;

#define KEY_ENTER		'\n'
#define KEY_ESC			27
#define KEY_BACKSPACE	8
#define KEY_TAB			9
#define KEY_PAGEUP		0x1020
#define KEY_PAGEDWN		0x1021
#define	KEY_END			0x1022
#define	KEY_HOME		0x1023
#define KEY_LEFT		0x1024
#define KEY_UP			0x1025
#define KEY_RIGHT		0x1026
#define KEY_DOWN		0x1027
#define KEY_INS			0x1028
#define KEY_DEL			0x1029

/* 雑関数 */
const UCHAR *searchArg(int argc, const UCHAR **argv, const UCHAR *tag, int i); // コマンドライン引数処理.
void randStatInit(UINT32 seed);
void devFunc(struct Regs *r); // junkApiを処理する関数

/* JITコンパイラ関係 */
int jitc0(UCHAR **qq, UCHAR *q1, const UCHAR *p0, const UCHAR *p1, int level, struct LabelTable *label);
int jitCompiler(UCHAR *dst, UCHAR *dst1, const UCHAR *src, const UCHAR *src1, const UCHAR *src0, struct LabelTable *label, int maxLabels, int level, int debugInfo1, int flags);
UCHAR *jitCompCallFunc(UCHAR *dst, void *func);

/* OS依存関数 */
void *mallocRWE(int bytes); // 実行権付きメモリのmalloc.
void drv_openWin(int x, int y, UCHAR *buf, char *winClosed);
void drv_flshWin(int sx, int sy, int x0, int y0);
void drv_sleep(int msec);

int OsecpuMain(int argc, const UCHAR **argv)
{
	UCHAR *appbin = malloc(APPSIZ1);
	UCHAR *jitbuf = mallocRWE(1024 * 1024); /* とりあえず1MBで */
	UCHAR *sysjit0 = mallocRWE(SJITSIZ1), *sysjit1 = sysjit0;
	UCHAR *systmp0 = malloc(1024 * 1024);
	UCHAR *systmp1 = malloc(1024 * 1024);
	UCHAR *systmp2 = malloc(1024 * 1024);
	struct LabelTable *label = malloc(JITC_MAXLABELS * sizeof (struct LabelTable));
	int level = JITC_LV_SLOWEST, tmpsiz;
	keybuf = malloc(KEYBUFSIZ * sizeof (int));
	keybuf_r = keybuf_w = keybuf_c = 0;
	jmp_buf setjmpEnv;
	double tm0, tm1, tm2;

	randStatInit((unsigned int) time(NULL));

	/* syslibの読み込み */
	UCHAR *syslib = malloc(SYSLIBSIZ1);
	int appsiz0, appsiz1;
	FILE *fp = fopen("syslib.ose", "rb");
	if (fp == NULL) {
		fputs("syslib-file fopen error.\n", stderr);
		return 1;
	}
	appsiz0 = fread(syslib, 1, SYSLIBSIZ1 - 4, fp);
	fclose(fp);
	if (appsiz0 >= SYSLIBSIZ1 - 4) {
		fputs("syslib-file too large.\n", stderr);
		return 1;
	}
	int i = jitc0(&sysjit1, sysjit0 + SJITSIZ1, syslib + 32, syslib + SYSLIBSIZ1, JITC_LV_SLOWEST, label);
	if (i != 0) {
		fputs("syslib-file JITC error.\n", stderr);
		return 1;
	}

	/* アプリバイナリの読み込み */
	if (argc <= 1)
		return 0;
	const UCHAR *argv1 = argv[1];
	if (argv1[0] == ':' && argv1[2] == ':') {
		level = argv1[1] - '0';
		if (level < 0 || level > 9)
			level = JITC_LV_SLOWEST;
		argv1 += 3;
	}
	fp = fopen(argv1, "rb");
	if (fp == NULL) {
		fputs("app-file load error.\n", stderr);
		return 1;
	}
	appsiz1 = appsiz0 = fread(appbin, 1, APPSIZ1 - 4, fp);
	fclose(fp);
	if (appsiz0 >= APPSIZ1 - 4) {
		fputs("app-file too large.\n", stderr);
		return 1;
	}
	if (appsiz0 < 3) {
header_error:
		fputs("app-file header error.\n", stderr);
		return 1;
	}

	tm0 = clock() / (double) CLOCKS_PER_SEC;

	if (appbin[2] == 0xf0) {
		#if (USE_TEK5 != 0)
			appsiz1 = tek5Decomp(appbin + 2, appbin + appsiz0, systmp0) + 2;
		#else
			appsiz1 = -9;
		#endif
		if (appsiz1 < 0) {
			fputs("unsupported-format(tek5)\n", stderr);
			return 1;
		}
	}

	int argDebug = 0;
	const UCHAR *cp = searchArg(argc, argv, "debug:", 0);
	if (cp != NULL) argDebug = *cp - '0';

	struct Regs regs;
	void (*jitfunc)(char *);
	UCHAR *jp = jitbuf; /* JIT-pointer */

	/* フロントエンドコードをバックエンドコードに変換する */
	if ((appbin[2] & 0xf0) != 0) {
		systmp0[0] = appbin[0];
		systmp0[1] = appbin[1];
		regs.preg[2].p = systmp0 + 2;
		regs.preg[3].p = systmp0 + 1024 * 1024;
		regs.preg[4].p = appbin + 2;
		regs.preg[5].p = appbin + appsiz1;
		regs.preg[6].p = systmp1;
		regs.preg[7].p = systmp1 + 1024 * 1024;
		regs.preg[10].p = systmp2;
		int pxxFlag[64], typLabel[4096];
		regs.preg[0x0b].p = (void *) pxxFlag;
		regs.preg[0x0c].p = (void *) typLabel;
		jitfunc = (void *) sysjit0;
		(*jitfunc)(((char *) &regs) + 128); /* サイズを節約するためにEBPを128バイトずらす */
		if (regs.ireg[0] != 0) {
			jp = regs.preg[2].p - 1;
			fprintf(stderr, "unpack error: %02X (at %06X) (R00=%d)\n", *jp, jp - systmp0, regs.ireg[0]);
			if ((argDebug & 2) != 0) {
				fp = fopen("debug2.bin", "wb");
				fwrite(systmp0, 1, jp - systmp0 + 16, fp);
				fclose(fp);
			}
			exit(1);
		}
		tmpsiz = regs.preg[2].p - systmp0;
	} else {
		memcpy(systmp0, appbin, appsiz1);
		tmpsiz = appsiz1;
	}

	if ((argDebug & 2) != 0) {
		fp = fopen("debug2.bin", "wb");
		fwrite(systmp0, 1, tmpsiz, fp);
		fclose(fp);
	}

	i = jitc0(&jp, jitbuf + 1024 * 1024, systmp0, systmp0 + tmpsiz, level, label);
	if (i == 1) goto header_error;
	if (i != 0) return 1;
	
	int appsiz2 = jp - jitbuf;

	UCHAR *p28 = jp;
	jp = jitCompCallFunc(jp, &devFunc);

	tm1 = clock() / (double) CLOCKS_PER_SEC;

	/* レジスタ初期化 */
	for (i = 0; i < 64; i++)
		regs.ireg[i] = 0;
	for (i = 0; i < 64; i++)
		regs.preg[i].typ = 0;

	regs.argc = argc;
	regs.argv = argv;
	regs.buf0 = regs.buf1 = NULL;
	regs.preg[0x28].p = p28;
//	regs.preg[0x00].p = malloc(1024 * 1024) + (1024 * 1024 - 32);
	regs.junkStack = malloc(1024 * 1024);
	regs.winClosed = 0;
	regs.autoSleep = 0;
	regs.setjmpEnv = &setjmpEnv;
	regs.lastConsoleChar = '\n';

	if ((argDebug & 1) != 0) {
		fp = fopen("debug1.bin", "wb");
		fwrite(jitbuf, 1, jp - jitbuf, fp);
		fclose(fp);
	}

	/* JITコード実行 */
	jitfunc = (void *) jitbuf;
	if (setjmp(setjmpEnv) == 0)
		(*jitfunc)(((char *) &regs) + 128); /* サイズを節約するためにEBPを128バイトずらす */
	if (regs.autoSleep != 0) {
		if (vram != NULL)
			drv_flshWin(v_xsiz, v_ysiz, 0, 0);
		while (regs.winClosed == 0)
			drv_sleep(100);
	}
	if (regs.lastConsoleChar != '\n')
		putchar('\n');

	tm2 = clock() / (double) CLOCKS_PER_SEC;

	/* 実行結果確認のためのレジスタダンプ */
	if (searchArg(argc, argv, "verbose:1", 0) != NULL) {
		printf("time: JITC=%.3f[sec], exec=%.3f[sec]\n", tm1 - tm0, tm2 - tm1);
		printf("size: OSECPU=%d, decomp=%d, tmp=%d, native=%d\n", appsiz0, appsiz1, tmpsiz, appsiz2);
		printf("result:\n");
		printf("R00:0x%08X  R01:0x%08X  R02:0x%08X  R03:0x%08X\n", regs.ireg[0], regs.ireg[1], regs.ireg[2], regs.ireg[3]);
	}
	return 0;
}

const UCHAR *searchArg(int argc, const UCHAR **argv, const UCHAR *tag, int i)
{
	int j, l;
	const UCHAR *r = NULL;
	if (tag != NULL) {
		l = strlen(tag);
		for (j = 1; j < argc; j++) {
			if (strncmp(argv[j], tag, l) == 0) {
				r = argv[j] + l;
				if (i == 0)	break;
				i--;
			}
		}
	} else {
		for (j = 1; j < argc; j++) {
			if (strchr(argv[j], ':') == NULL) {
				r = argv[j];
				if (i == 0)	break;
				i--;
			}
		}
	}
	if (i != 0) r = NULL;
	return r;
}

static int iColor1[] = {
	0x000000, 0xff0000, 0x00ff00, 0xffff00,
	0x0000ff, 0xff00ff, 0x00ffff, 0xffffff
};

void putOsaskChar(int c, struct Regs *r)
{
	if (0x10 <= c && c <= 0x1f)
		c = "0123456789ABCDEF"[c & 0x0f];
	putchar(r->lastConsoleChar = c);
	return;
}

static unsigned char fontdata[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x10, 0x10, 0x00, 0x00,
	0x28, 0x28, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x44, 0x44, 0x44, 0xfe, 0x44, 0x44, 0x44, 0x44, 0x44, 0xfe, 0x44, 0x44, 0x44, 0x00, 0x00,
	0x10, 0x3a, 0x56, 0x92, 0x92, 0x90, 0x50, 0x38, 0x14, 0x12, 0x92, 0x92, 0xd4, 0xb8, 0x10, 0x10,
	0x62, 0x92, 0x94, 0x94, 0x68, 0x08, 0x10, 0x10, 0x20, 0x2c, 0x52, 0x52, 0x92, 0x8c, 0x00, 0x00,
	0x00, 0x70, 0x88, 0x88, 0x88, 0x90, 0x60, 0x47, 0xa2, 0x92, 0x8a, 0x84, 0x46, 0x39, 0x00, 0x00,
	0x04, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x04, 0x08, 0x08, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x08, 0x08, 0x04, 0x02, 0x00,
	0x80, 0x40, 0x20, 0x20, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x20, 0x20, 0x40, 0x80, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x92, 0x54, 0x38, 0x54, 0x92, 0x10, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0xfe, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x08, 0x08, 0x10,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00,
	0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x10, 0x10, 0x20, 0x20, 0x40, 0x40, 0x40, 0x80, 0x80,
	0x00, 0x18, 0x24, 0x24, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x24, 0x24, 0x18, 0x00, 0x00,
	0x00, 0x08, 0x18, 0x28, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x3e, 0x00, 0x00,
	0x00, 0x18, 0x24, 0x42, 0x42, 0x02, 0x04, 0x08, 0x10, 0x20, 0x20, 0x40, 0x40, 0x7e, 0x00, 0x00,
	0x00, 0x18, 0x24, 0x42, 0x02, 0x02, 0x04, 0x18, 0x04, 0x02, 0x02, 0x42, 0x24, 0x18, 0x00, 0x00,
	0x00, 0x0c, 0x0c, 0x0c, 0x14, 0x14, 0x14, 0x24, 0x24, 0x44, 0x7e, 0x04, 0x04, 0x1e, 0x00, 0x00,
	0x00, 0x7c, 0x40, 0x40, 0x40, 0x58, 0x64, 0x02, 0x02, 0x02, 0x02, 0x42, 0x24, 0x18, 0x00, 0x00,
	0x00, 0x18, 0x24, 0x42, 0x40, 0x58, 0x64, 0x42, 0x42, 0x42, 0x42, 0x42, 0x24, 0x18, 0x00, 0x00,
	0x00, 0x7e, 0x42, 0x42, 0x04, 0x04, 0x08, 0x08, 0x08, 0x10, 0x10, 0x10, 0x10, 0x38, 0x00, 0x00,
	0x00, 0x18, 0x24, 0x42, 0x42, 0x42, 0x24, 0x18, 0x24, 0x42, 0x42, 0x42, 0x24, 0x18, 0x00, 0x00,
	0x00, 0x18, 0x24, 0x42, 0x42, 0x42, 0x42, 0x42, 0x26, 0x1a, 0x02, 0x42, 0x24, 0x18, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x08, 0x08, 0x10,
	0x00, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00,
	0x00, 0x38, 0x44, 0x82, 0x82, 0x82, 0x04, 0x08, 0x10, 0x10, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00,
	0x00, 0x38, 0x44, 0x82, 0x9a, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x9c, 0x80, 0x46, 0x38, 0x00, 0x00,
	0x00, 0x18, 0x18, 0x18, 0x18, 0x24, 0x24, 0x24, 0x24, 0x7e, 0x42, 0x42, 0x42, 0xe7, 0x00, 0x00,
	0x00, 0xf0, 0x48, 0x44, 0x44, 0x44, 0x48, 0x78, 0x44, 0x42, 0x42, 0x42, 0x44, 0xf8, 0x00, 0x00,
	0x00, 0x3a, 0x46, 0x42, 0x82, 0x80, 0x80, 0x80, 0x80, 0x80, 0x82, 0x42, 0x44, 0x38, 0x00, 0x00,
	0x00, 0xf8, 0x44, 0x44, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x44, 0x44, 0xf8, 0x00, 0x00,
	0x00, 0xfe, 0x42, 0x42, 0x40, 0x40, 0x44, 0x7c, 0x44, 0x40, 0x40, 0x42, 0x42, 0xfe, 0x00, 0x00,
	0x00, 0xfe, 0x42, 0x42, 0x40, 0x40, 0x44, 0x7c, 0x44, 0x44, 0x40, 0x40, 0x40, 0xf0, 0x00, 0x00,
	0x00, 0x3a, 0x46, 0x42, 0x82, 0x80, 0x80, 0x9e, 0x82, 0x82, 0x82, 0x42, 0x46, 0x38, 0x00, 0x00,
	0x00, 0xe7, 0x42, 0x42, 0x42, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x42, 0x42, 0x42, 0xe7, 0x00, 0x00,
	0x00, 0x7c, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 0x00,
	0x00, 0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x84, 0x48, 0x30, 0x00,
	0x00, 0xe7, 0x42, 0x44, 0x48, 0x50, 0x50, 0x60, 0x50, 0x50, 0x48, 0x44, 0x42, 0xe7, 0x00, 0x00,
	0x00, 0xf0, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x42, 0x42, 0xfe, 0x00, 0x00,
	0x00, 0xc3, 0x42, 0x66, 0x66, 0x66, 0x5a, 0x5a, 0x5a, 0x42, 0x42, 0x42, 0x42, 0xe7, 0x00, 0x00,
	0x00, 0xc7, 0x42, 0x62, 0x62, 0x52, 0x52, 0x52, 0x4a, 0x4a, 0x4a, 0x46, 0x46, 0xe2, 0x00, 0x00,
	0x00, 0x38, 0x44, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x44, 0x38, 0x00, 0x00,
	0x00, 0xf8, 0x44, 0x42, 0x42, 0x42, 0x44, 0x78, 0x40, 0x40, 0x40, 0x40, 0x40, 0xf0, 0x00, 0x00,
	0x00, 0x38, 0x44, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x92, 0x8a, 0x44, 0x3a, 0x00, 0x00,
	0x00, 0xfc, 0x42, 0x42, 0x42, 0x42, 0x7c, 0x44, 0x42, 0x42, 0x42, 0x42, 0x42, 0xe7, 0x00, 0x00,
	0x00, 0x3a, 0x46, 0x82, 0x82, 0x80, 0x40, 0x38, 0x04, 0x02, 0x82, 0x82, 0xc4, 0xb8, 0x00, 0x00,
	0x00, 0xfe, 0x92, 0x92, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 0x00,
	0x00, 0xe7, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x24, 0x3c, 0x00, 0x00,
	0x00, 0xe7, 0x42, 0x42, 0x42, 0x42, 0x24, 0x24, 0x24, 0x24, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00,
	0x00, 0xe7, 0x42, 0x42, 0x42, 0x5a, 0x5a, 0x5a, 0x5a, 0x24, 0x24, 0x24, 0x24, 0x24, 0x00, 0x00,
	0x00, 0xe7, 0x42, 0x42, 0x24, 0x24, 0x24, 0x18, 0x24, 0x24, 0x24, 0x42, 0x42, 0xe7, 0x00, 0x00,
	0x00, 0xee, 0x44, 0x44, 0x44, 0x28, 0x28, 0x28, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 0x00,
	0x00, 0xfe, 0x84, 0x84, 0x08, 0x08, 0x10, 0x10, 0x20, 0x20, 0x40, 0x42, 0x82, 0xfe, 0x00, 0x00,
	0x00, 0x3e, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3e, 0x00,
	0x80, 0x80, 0x40, 0x40, 0x20, 0x20, 0x20, 0x10, 0x10, 0x08, 0x08, 0x04, 0x04, 0x04, 0x02, 0x02,
	0x00, 0x7c, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x7c, 0x00,
	0x00, 0x10, 0x28, 0x44, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00,
	0x10, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x08, 0x04, 0x3c, 0x44, 0x84, 0x84, 0x8c, 0x76, 0x00, 0x00,
	0xc0, 0x40, 0x40, 0x40, 0x40, 0x58, 0x64, 0x42, 0x42, 0x42, 0x42, 0x42, 0x64, 0x58, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x4c, 0x84, 0x84, 0x80, 0x80, 0x82, 0x44, 0x38, 0x00, 0x00,
	0x0c, 0x04, 0x04, 0x04, 0x04, 0x34, 0x4c, 0x84, 0x84, 0x84, 0x84, 0x84, 0x4c, 0x36, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x44, 0x82, 0x82, 0xfc, 0x80, 0x82, 0x42, 0x3c, 0x00, 0x00,
	0x0e, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x4c, 0x84, 0x84, 0x84, 0x84, 0x4c, 0x34, 0x04, 0x04, 0x78,
	0xc0, 0x40, 0x40, 0x40, 0x40, 0x58, 0x64, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0xe3, 0x00, 0x00,
	0x00, 0x10, 0x10, 0x00, 0x00, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x38, 0x00, 0x00,
	0x00, 0x04, 0x04, 0x00, 0x00, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x30,
	0xc0, 0x40, 0x40, 0x40, 0x40, 0x4e, 0x44, 0x48, 0x50, 0x60, 0x50, 0x48, 0x44, 0xe6, 0x00, 0x00,
	0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x38, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xf6, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0xdb, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x64, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0xe3, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x44, 0x82, 0x82, 0x82, 0x82, 0x82, 0x44, 0x38, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x64, 0x42, 0x42, 0x42, 0x42, 0x42, 0x64, 0x58, 0x40, 0xe0,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x4c, 0x84, 0x84, 0x84, 0x84, 0x84, 0x4c, 0x34, 0x04, 0x0e,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0x62, 0x42, 0x40, 0x40, 0x40, 0x40, 0x40, 0xe0, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x7a, 0x86, 0x82, 0xc0, 0x38, 0x06, 0x82, 0xc2, 0xbc, 0x00, 0x00,
	0x00, 0x00, 0x10, 0x10, 0x10, 0x7c, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0e, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc6, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x46, 0x3b, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xe7, 0x42, 0x42, 0x42, 0x24, 0x24, 0x24, 0x18, 0x18, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xe7, 0x42, 0x42, 0x5a, 0x5a, 0x5a, 0x24, 0x24, 0x24, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc6, 0x44, 0x28, 0x28, 0x10, 0x28, 0x28, 0x44, 0xc6, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xe7, 0x42, 0x42, 0x24, 0x24, 0x24, 0x18, 0x18, 0x10, 0x10, 0x60,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x82, 0x84, 0x08, 0x10, 0x20, 0x42, 0x82, 0xfe, 0x00, 0x00,
	0x00, 0x06, 0x08, 0x10, 0x10, 0x10, 0x10, 0x60, 0x10, 0x10, 0x10, 0x10, 0x08, 0x06, 0x00, 0x00,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x00, 0x60, 0x10, 0x08, 0x08, 0x08, 0x08, 0x06, 0x08, 0x08, 0x08, 0x08, 0x10, 0x60, 0x00, 0x00,
	0x00, 0x72, 0x8c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x10, 0x28, 0x44, 0x82, 0xfe, 0x82, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00
};

//typedef unsigned int UINT32;
typedef signed int SINT32;

/* tinyMTの32bit版のアルゴリズムを使っています */
/* http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/TINYMT/index-jp.html */

static struct {
	UINT32 stat[4], mat1, mat2, tmat;
} randStat;

void randStatNext()
{
	UINT32 x, y;
	x = (randStat.stat[0] & 0x7fffffff) ^ randStat.stat[1] ^ randStat.stat[2];
	y = randStat.stat[3];
	x ^= x << 1;
	y ^= (y >> 1) ^ x;
	randStat.stat[1] = randStat.stat[2] ^ (-((SINT32)(y & 1)) & randStat.mat1);
	randStat.stat[2] = x ^ (y << 10)    ^ (-((SINT32)(y & 1)) & randStat.mat2);
	randStat.stat[3] = y;
	return;
}

void randStatInit(UINT32 seed)
{
	int i;
	randStat.stat[0] = seed;
	randStat.stat[1] = randStat.mat1 = 0x8f7011ee;
	randStat.stat[2] = randStat.mat2 = 0xfc78ff1f;
	randStat.stat[3] = randStat.tmat = 0x3793fdff;
	for (i = 1; i < 8; i++)
		randStat.stat[i & 3] ^= i + ((UINT32)1812433253) * (randStat.stat[(i - 1) & 3] ^ (randStat.stat[(i - 1) & 3] >> 30));
	for (i = 0; i < 8; i++)
		randStatNext();
	return;
}

void devFunc(struct Regs *r)
{
	FILE *fp;
	r = (struct Regs *) (((char *) r) - 128); /* サイズを節約するためにEBPを128バイトずらしているのを元に戻す */
	int i, c;
	UCHAR *puc;
	if (r->winClosed != 0)
		longjmp(*(r->setjmpEnv), 1);
	if (0xff44 <= r->ireg[0x30] && r->ireg[0x30] <= 0xff48) {
		if (vram == NULL) {
			v_xsiz = 640;
			v_ysiz = 480;
			vram = malloc(640 * 480 * 4);
			drv_openWin(640, 480, (void *) vram, &r->winClosed);
			r->autoSleep = 1;
			for (i = 640 * 480 - 1; i >= 0; i--)
				vram[i] = 0;
		}
	}

	switch (r->ireg[0x30]) {
	case 0xff00:
		printf("R31=%d(dec)\n", r->ireg[0x31]);
		break;

	case 0xff01:
		/* return: R30, P31 */
		if (r->buf0 == NULL)
			r->buf0 = malloc(1024 * 1024);
		if (r->argc <= r->ireg[0x31]) {
			fprintf(stderr, "devFunc: error: R30=ff01: argc error: R31=%08X\n", r->ireg[0x31]);
			exit(1);
		}
		fp = fopen(r->argv[r->ireg[0x31]], "r");
		if (fp == NULL) {
			fprintf(stderr, "devFunc: error: R30=ff01: fopen error: '%s'\n", r->argv[r->ireg[0x31]]);
			exit(1);
		}
		i = fread(r->buf0, 1, 1024 * 1024 - 4, fp);			
		if (i >= 1024 * 1024 - 4 || i < 0) {
			fprintf(stderr, "devFunc: error: R30=ff01: fread error: '%s'\n", r->argv[r->ireg[0x31]]);
			exit(1);
		}
		fclose(fp);
		r->preg[0x31].p  = r->buf0;
		r->preg[0x31].p0 = r->buf0;
		r->preg[0x31].p1 = r->buf0 + i;
		r->ireg[0x30] = i;
		break;

	case 0xff02:
		/* return: none */
		if (r->argc <= r->ireg[0x31]) {
			fprintf(stderr, "devFunc: error: R30=ff02: argc error: R31=%08X\n", r->ireg[0x31]);
			exit(1);
		}
		fp = fopen(r->argv[r->ireg[0x31]], "w");
		if (fp == NULL) {
			fprintf(stderr, "devFunc: error: R30=ff02: fopen error: '%s'\n", r->argv[r->ireg[0x31]]);
			exit(1);
		}
		if (r->ireg[0x32] >= 1024 * 1024 || r->ireg[0x32] < 0){
			fprintf(stderr, "devFunc: error: R30=ff02: fwrite error: R02=%08X\n", r->ireg[0x32]);
			exit(1);
		}
		fwrite(r->buf1, 1, r->ireg[0x32], fp);
		fclose(fp);
		break;

	case 0xff03:
		/* return: P31 */
		if (r->buf1 == NULL)
			r->buf1 = malloc(1024 * 1024);
		r->preg[0x31].p  = r->buf1;
		r->preg[0x31].p0 = r->buf1;
		r->preg[0x31].p1 = r->buf1 + 1024 * 1024;
		break;

	case 0xff04:
		printf("P31.(p-p0)=%d(dec)\n", r->preg[0x31].p - r->preg[0x31].p0);
		break;

	case 0xff05:
		fwrite(r->preg[0x31].p, 1, r->ireg[0x31], stdout);
		break;

	case 0xff06:
		// R31はリターンコード.
		// これを反映すべきだが、現状は手抜きでいつも正常終了.
		longjmp(*(r->setjmpEnv), 1);
		break;

	case 0xff07:
		// マシになった文字列表示.OSASK文字列に対応.offにすれば通常の文字列処理もできる.現状はonのみサポート.
		i = r->ireg[0x31];
		puc = r->preg[0x31].p;
		c = 0x01;
		while (i > 0) {
			c = c << 1 | ((*puc >> 7) & 1);
			putOsaskChar((*puc++) & 0x7f, r);
			if (c >= 0x80) {
				if (c > 0x80)
					putOsaskChar(c & 0x7f, r);
				c = 0x01;
			}
			i--;
		}
		break;

	case 0xff40:
		/* R31とR32でサイズを指定 */
		v_xsiz = r->ireg[0x31];
		v_ysiz = r->ireg[0x32];
		r->preg[0x31].p = (void *) (vram = malloc(v_xsiz * v_ysiz * 4));
		r->preg[0x31].p0 = r->preg[0x31].p;
		r->preg[0x31].p1 = r->preg[0x31].p + v_xsiz * v_ysiz * 4;
		drv_openWin(r->ireg[0x31], r->ireg[0x32], r->preg[0x31].p, &r->winClosed);
	//	drv_flshWin(r->ireg[1], r->ireg[2], 0, 0);
		r->autoSleep = 1;
		for (i = v_xsiz * v_ysiz - 1; i >= 0; i--)
			vram[i] = 0;
		break;

	case 0xff41:
		/* R31とR32でサイズを指定、R33とR34でx0,y0指定 */
		drv_flshWin(r->ireg[0x31], r->ireg[0x32], r->ireg[0x33], r->ireg[0x34]);
		break;

	case 0xff42:
		if (r->ireg[0x32] == -1) {
			r->autoSleep = 1;
			longjmp(*(r->setjmpEnv), 1);
		}
		r->autoSleep = 0;
		if ((r->ireg[0x31] & 1) == 0 && vram != NULL)
			drv_flshWin(v_xsiz, v_ysiz, 0, 0);
		for (;;) {
			if (r->winClosed != 0)
				longjmp(*(r->setjmpEnv), 1);
			drv_sleep(r->ireg[0x32]);
			if ((r->ireg[0x31] & 2) != 0 && keybuf_c <= 0) continue;
			break;
		}
		break;

	case 0xff43:
		r->ireg[0x30] = -1;
		if (keybuf_c > 0) {
			r->ireg[0x30] = keybuf[keybuf_r];
			if ((r->ireg[0x31] & 1) != 0) {
				keybuf_c--;
				keybuf_r = (keybuf_r + 1) & (KEYBUFSIZ - 1);
			}
		}
		break;

	case 0xff44:
		c = r->ireg[0x34];
		if ((r->ireg[0x31] & 0xc) == 0x4) c = iColor1[c];
		/* 画面内かどうかのチェックがあるべき */
		if ((r->ireg[0x31] & 3) == 0) vram[r->ireg[0x32] + r->ireg[0x33] * v_xsiz] =  c;
		if ((r->ireg[0x31] & 3) == 1) vram[r->ireg[0x32] + r->ireg[0x33] * v_xsiz] |= c;
		if ((r->ireg[0x31] & 3) == 2) vram[r->ireg[0x32] + r->ireg[0x33] * v_xsiz] ^= c;
		if ((r->ireg[0x31] & 3) == 3) vram[r->ireg[0x32] + r->ireg[0x33] * v_xsiz] &= c;
		break;

	case 0xff45:
		c = r->ireg[0x36];
		if ((r->ireg[0x31] & 0xc) == 0x4) c = iColor1[c];
		int x, y, len, dx, dy;
		dx = r->ireg[0x34] - r->ireg[0x32];
		dy = r->ireg[0x35] - r->ireg[0x33];
		x = r->ireg[0x32] << 10;
		y = r->ireg[0x33] << 10;
		if (dx < 0) dx = - dx;
		if (dy < 0) dy = - dy;
		if (dx >= dy) {
			len = dx + 1;
			if (r->ireg[0x32] < r->ireg[0x34]) dx = 1024; else dx = -1024;
			if (r->ireg[0x33] < r->ireg[0x35]) dy++;      else dy = -1 - dy;
			dy = (dy << 10) / len;
		} else {
			len = dy + 1;
			if (r->ireg[0x33] < r->ireg[0x35]) dy = 1024; else dy = -1024;
			if (r->ireg[0x32] < r->ireg[0x34]) dx++;      else dx = -1 - dx;
			dx = (dx << 10) / len;
		}
		if ((r->ireg[0x31] & 3) == 0) {
			for (i = 0; i < len; i++) {
				/* 画面内かどうかのチェックがあるべき */
				vram[(x >> 10) + (y >> 10) * v_xsiz] =  c;
				x += dx;
				y += dy;
			}
			break;
		}
		for (i = 0; i < len; i++) {
			/* 画面内かどうかのチェックがあるべき */
		//	if ((r->ireg[0x31] & 3) == 0) vram[(x >> 10) + (y >> 10) * v_xsiz] =  c;
			if ((r->ireg[0x31] & 3) == 1) vram[(x >> 10) + (y >> 10) * v_xsiz] |= c;
			if ((r->ireg[0x31] & 3) == 2) vram[(x >> 10) + (y >> 10) * v_xsiz] ^= c;
			if ((r->ireg[0x31] & 3) == 3) vram[(x >> 10) + (y >> 10) * v_xsiz] &= c;
			x += dx;
			y += dy;
		}
		break;

	case 0xff46:	// fillRect(opt:R31, xsiz:R32, ysiz:R33, x0:R34, y0:R35, c:R36)
		c = r->ireg[0x36];
		if ((r->ireg[0x31] & 0xc) == 0x4) c = iColor1[c];
		if ((r->ireg[0x31] & 3) == 0) {
			for (y = 0; y < r->ireg[0x33]; y++) {
				for (x = 0; x < r->ireg[0x32]; x++) {
					vram[(x + r->ireg[0x34]) + (y + r->ireg[0x35]) * v_xsiz] =  c;
				}
			}
		}
		for (y = 0; y < r->ireg[0x33]; y++) {
			for (x = 0; x < r->ireg[0x32]; x++) {
			//	if ((r->ireg[0x31] & 3) == 0) vram[(x + r->ireg[0x34]) + (y + r->ireg[0x35]) * v_xsiz] =  c;
				if ((r->ireg[0x31] & 3) == 1) vram[(x + r->ireg[0x34]) + (y + r->ireg[0x35]) * v_xsiz] |= c;
				if ((r->ireg[0x31] & 3) == 2) vram[(x + r->ireg[0x34]) + (y + r->ireg[0x35]) * v_xsiz] ^= c;
				if ((r->ireg[0x31] & 3) == 3) vram[(x + r->ireg[0x34]) + (y + r->ireg[0x35]) * v_xsiz] &= c;
			}
		}
		break;

	case 0xff47:	// fillOval(opt:R31, xsiz:R32, ysiz:R33, x0:R34, y0:R35, c:R36)
		// これの計算精度はアーキテクチャに依存する.
		c = r->ireg[0x36];
		if ((r->ireg[0x31] & 0xc) == 0x4) c = iColor1[c];
		double dcx = 0.5 * (r->ireg[0x32] - 1), dcy = 0.5 * (r->ireg[0x33] - 1), dcxy = (dcx + 0.5) * (dcy + 0.5) - 0.1;
		dcxy *= dcxy;
		if ((r->ireg[0x31] & 3) == 0) {
			for (y = 0; y < r->ireg[0x33]; y++) {
				double dty = (y - dcy) * dcx; 
				for (x = 0; x < r->ireg[0x32]; x++) {
					double dtx = (x - dcx) * dcy;
					if (dtx * dtx + dty * dty > dcxy) continue; 
					vram[(x + r->ireg[0x34]) + (y + r->ireg[0x35]) * v_xsiz] =  c;
				}
			}
			break;
		}
		for (y = 0; y < r->ireg[0x33]; y++) {
			double dty = (y - dcy) * dcx; 
			for (x = 0; x < r->ireg[0x32]; x++) {
				double dtx = (x - dcx) * dcy;
				if (dtx * dtx + dty * dty > dcxy) continue; 
			//	if ((r->ireg[0x31] & 3) == 0) vram[(x + r->ireg[0x34]) + (y + r->ireg[0x35]) * v_xsiz] =  c;
				if ((r->ireg[0x31] & 3) == 1) vram[(x + r->ireg[0x34]) + (y + r->ireg[0x35]) * v_xsiz] |= c;
				if ((r->ireg[0x31] & 3) == 2) vram[(x + r->ireg[0x34]) + (y + r->ireg[0x35]) * v_xsiz] ^= c;
				if ((r->ireg[0x31] & 3) == 3) vram[(x + r->ireg[0x34]) + (y + r->ireg[0x35]) * v_xsiz] &= c;
			}
		}
		break;

	case 0xff48:	// drawString(opt:R31, xsiz:R32, ysiz:R33, x0:R34, y0:R35, c:R36, s.len:R37, s.p:P31)
		c = r->ireg[0x36];
		if ((r->ireg[0x31] & 0xc) == 0x4) c = iColor1[c];
		len = r->ireg[0x37];
		puc = r->preg[0x31].p;
		x = r->ireg[0x34];
		y = r->ireg[0x35];
		int ddx, ddy, j, ch;
		if ((r->ireg[0x31] & 3) == 0 && r->ireg[0x32] == 1 && r->ireg[0x33] == 1) {
			// メジャーケースを高速化.
			for (i = 0; i < len; i++) {
				ch = puc[i];
				if (0x10 <= ch && ch <= 0x1f)
					ch = "0123456789ABCDEF"[ch & 0x0f];
				for (dy = 0; dy < 16; dy++) {
					j = fontdata[(ch - ' ') * 16 + dy];
					for (dx = 0; dx < 8; dx++) {
						if ((j & (0x80 >> dx)) != 0) vram[(x + dx) + (y + dy) * v_xsiz] = c;
					}
				}
				x += 8;
			}
			break;
		}
		for (i = 0; i < len; i++) {
			ch = puc[i];
			if (0x10 <= ch && ch <= 0x1f)
				ch = "0123456789ABCDEF"[ch & 0x0f];
			for (dy = 0; dy < 16; dy++) {
				j = fontdata[(ch - ' ') * 16 + dy];
				for (ddy = 0; ddy < r->ireg[0x33]; ddy++) {
					for (dx = 0; dx < 8; dx++) {
						if ((j & (0x80 >> dx)) != 0) {
							for (ddx = 0; ddx < r->ireg[0x32]; ddx++) {
								if ((r->ireg[0x31] & 3) == 0) vram[x + y * v_xsiz] =  c;
								if ((r->ireg[0x31] & 3) == 1) vram[x + y * v_xsiz] |= c;
								if ((r->ireg[0x31] & 3) == 2) vram[x + y * v_xsiz] ^= c;
								if ((r->ireg[0x31] & 3) == 3) vram[x + y * v_xsiz] &= c;
								x++;
							}
						} else
							x += r->ireg[0x32];
					}
					x -= r->ireg[0x32] * 8;
					y++;
				}
			}
			x += r->ireg[0x32] * 8;
			y -= r->ireg[0x33] * 16;
		}
		break;

	case 0xff49:	/* 適当な乱数を返す */
		randStatNext();
		UINT32 u32t;
		u32t = randStat.stat[0] + (randStat.stat[2] >> 8);
		r->ireg[0x30] = randStat.stat[3] ^ u32t ^ (-((SINT32)(u32t & 1)) & randStat.tmat);
		break;

	case 0xff4a:	/* seedの指定 */
		randStatInit(r->ireg[0x31]);
		break;

	case 0xff4b:	/* 適当なseedを提供 */
		r->ireg[0x30] = time(NULL) ^ 0x55555555;
		break;

	default:
		printf("devFunc: error: R30=%08X\n", r->ireg[0x30]);
		exit(1);

	}
	return;
}

void putKeybuf(int i)
{
	if (keybuf_c < KEYBUFSIZ) {
		keybuf[keybuf_w] = i;
		keybuf_c++;
		keybuf_w = (keybuf_w + 1) & (KEYBUFSIZ - 1);
	}
	return;
}

/* tek5圧縮関係 */

#if (USE_TEK5 != 0)

int appackSub2(const UCHAR **pp, char *pif)
{
	int r = 0;
	const UCHAR *p = *pp;
	if (*pif == 0) {
		r = *p >> 4;
	} else {
		r = *p & 0x0f;
		p++;
		*pp = p;
	}
	*pif ^= 1;
	return r;
}

int appackSub3u(const UCHAR **pp, char *pif)
{
	int r = 0, i, j;

		r = appackSub2(pp, pif);
		if (0x8 <= r && r <= 0xb) {
			r = r << 4 | appackSub2(pp, pif);
			r &= 0x3f;
		} else if (0xc <= r && r <= 0xd) {
			r = r << 4 | appackSub2(pp, pif);
			r = r << 4 | appackSub2(pp, pif);
			r &= 0x1ff;
		} else if (r == 0xe) {
			r = appackSub2(pp, pif);
			r = r << 4 | appackSub2(pp, pif);
			r = r << 4 | appackSub2(pp, pif);
		} else if (r == 0x7) {
			i = appackSub3u(pp, pif);
			r = 0;
			while (i > 0) {
				r = r << 4 | appackSub2(pp, pif);
				i--;
			}
		}

	return r;
}

int tek5Decomp(UCHAR *buf, UCHAR *buf1, UCHAR *tmp)
{
	static char tek5head[16] = {
		0x89, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00,
		0x4f, 0x53, 0x41, 0x53, 0x4b, 0x43, 0x4d, 0x50
	};
	memcpy(tmp, tek5head, 16);
	UCHAR *q = tmp + 16;
	char iif = 0;
	int i, tmpsiz;
	const UCHAR *p = &buf[1];
	i = appackSub3u(&p, &iif);
	tmpsiz = appackSub3u(&p, &iif);
	tmpsiz = tmpsiz << 8 | *p++;
	*q++ = (tmpsiz >> 27) & 0xfe;
	*q++ = (tmpsiz >> 20) & 0xfe;
	*q++ = (tmpsiz >> 13) & 0xfe;
	*q++ = (tmpsiz >>  6) & 0xfe;
	*q++ = (tmpsiz << 1 | 1) & 0xff;
	if (i == 1) *q++ = 0x15;
	if (i == 2) *q++ = 0x19;
	if (i == 3) *q++ = 0x21;
	if (i >= 4) return -9;
	while (p < buf1)
		*q++ = *p++;
	tek_decomp(tmp, buf, tmpsiz);
	return tmpsiz;
}

#endif

/* JITC関係 */

#define JITC_ERR_MASK			255
#define	JITC_ERR_PHASE0ONLY		256
#define JITC_ERR_REGNUM			(1 | JITC_ERR_PHASE0ONLY)
#define JITC_ERR_DST1			(2 | JITC_ERR_PHASE0ONLY)
#define JITC_ERR_OPECODE		(3 | JITC_ERR_PHASE0ONLY)
#define	JITC_ERR_LABELNUM		(4 | JITC_ERR_PHASE0ONLY)
#define	JITC_ERR_LABELREDEF		(5 | JITC_ERR_PHASE0ONLY)
#define JITC_ERR_PREFIX			(6 | JITC_ERR_PHASE0ONLY)
#define	JITC_ERR_LABELNODEF		7
#define	JITC_ERR_LABELTYP		8
#define	JITC_ERR_IDIOM			9
#define JITC_ERR_PREGNUM		(10 | JITC_ERR_PHASE0ONLY)
#define JITC_ERR_SRC1			(11 | JITC_ERR_PHASE0ONLY)
#define JITC_ERR_BADTYPE		(12 | JITC_ERR_PHASE0ONLY)
#define JITC_ERR_PREFIXFAR		(13 | JITC_ERR_PHASE0ONLY)

int jitCompCmdLen(const UCHAR *src)
{
	int i = 1;
	if (0x01 <= *src && *src < 0x04) i = 6;
	if (*src == 0x04) i = 2;
	if (0x08 <= *src && *src < 0x0d) i = 8 + src[7] * 4;
	if (0x0e <= *src && *src < 0x10) i = 8;
	if (0x10 <= *src && *src < 0x2e) i = 4;
	if (0x1c <= *src && *src < 0x1f) i = 3;
	if (*src == 0x1f) i = 11;
	if (*src == 0x2f) i = 4 + src[1];
	if (0x30 <= *src && *src <= 0x33) i = 4;
	if (0x3c <= *src && *src <= 0x3d) i = 7;
	if (*src == 0xfe) i = 2 + src[1];
	return i;
}

#if (JITC_ARCNUM == 0x0001)	/* x86-32bit */

/* 他のCPUへ移植する人へ:
    以下は最適化のためのものなので、すべて0として簡単に移植しても問題ありません */
#define	jitCompA0001_USE_R3F_CMPJMP		1*1
#define	jitCompA0001_USE_R3F_IMM32		1*1
#define jitCompA0001_USE_R3F_IMM8		1*1
#define jitCompA0001_USE_R3F_INCDEC		1*1
#define	jitCompA0001_OPTIMIZE_JMP		1*1
#define	jitCompA0001_OPTIMIZE_MOV		1*1	/* 1にすると速度低下する？ */
#define	jitCompA0001_OPTIMIZE_CMP		1*1
#define	jitCompA0001_OPTIMIZE_ALIGN		4*1	/* 0-8を想定 */
#define	jitCompA0001_EBP128				128*1

struct JitCompWork {
	UCHAR *dst, *dst0;
	int err, maxLabels;
	#if (jitCompA0001_USE_R3F_IMM32 != 0)
		int r3f;
	#endif
	char prefix;
};

#define	jitCompPutByte1(p, c0)				*p++ = c0
#define	jitCompPutByte2(p, c0, c1)			*p++ = c0; *p++ = c1
#define	jitCompPutByte3(p, c0, c1, c2)		*p++ = c0; *p++ = c1; *p++ = c2
#define	jitCompPutByte4(p, c0, c1, c2, c3)	*p++ = c0; *p++ = c1; *p++ = c2; *p++ = c3

static inline void jitCompPutImm32(struct JitCompWork *w, int i)
{
	jitCompPutByte1(w->dst,  i        & 0xff);
	jitCompPutByte1(w->dst, (i >>  8) & 0xff);
	jitCompPutByte1(w->dst, (i >> 16) & 0xff);
	jitCompPutByte1(w->dst, (i >> 24) & 0xff);
	return;
}

int jitCompGetImm32(const UCHAR *src)
{
	return (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
}

int jitCompGetLabelNum(struct JitCompWork *w, const UCHAR *src)
{
	int i = jitCompGetImm32(src);
	if (i < 0 || i >= w->maxLabels) {
		w->err = JITC_ERR_LABELNUM;
		i = 0;
	}
	return i;
}

void jitCompA0001_85DispN(struct JitCompWork *w, int disp, int n)
{
	disp -= jitCompA0001_EBP128;
	if (-128 <= disp && disp <= 127) {
		jitCompPutByte2(w->dst, 0x45 | (n << 3), disp & 0xff);
	} else {
		jitCompPutByte1(w->dst, 0x85 | (n << 3));
		jitCompPutImm32(w, disp);
	}
	return;
}

void jitCompA0001_movEbpDispReg32(struct JitCompWork *w, int disp, int reg32)
{
	jitCompPutByte1(w->dst, 0x89); /* MOV(mem, reg32); */
	jitCompA0001_85DispN(w, disp, reg32);
	return;
}

void jitCompA0001_movReg32EbpDisp(struct JitCompWork *w, int reg32, int disp)
{
	jitCompPutByte1(w->dst, 0x8b); /* MOV(reg32, mem); */
	jitCompA0001_85DispN(w, disp, reg32);
	return;
}

void jitCompA0001_movEaxRxx(struct JitCompWork *w, int rxx)
{
	#if (jitCompA0001_USE_R3F_IMM32 != 0)
		if (rxx == 0x3f) {
			jitCompPutByte1(w->dst, 0xb8); /* MOV(EAX, ?); */
			jitCompPutImm32(w, w->r3f);
			return;
		}
	#endif
	if (rxx >= 0x40 || rxx < 0) w->err = JITC_ERR_REGNUM;
	jitCompA0001_movReg32EbpDisp(w, 0 /* EAX */, rxx * 4); /* MOV(EAX, [EBP+?]); */
	return;
}

void jitCompA0001_movRxxEax(struct JitCompWork *w, int rxx)
{
	if (rxx >= 0x40 || rxx < 0) w->err = JITC_ERR_REGNUM;
	jitCompA0001_movEbpDispReg32(w, rxx * 4, 0 /* EAX */); /* MOV([EBP+?], EAX); */
	return;
}

void jitCompA0001_fixPrefix(struct JitCompWork *w)
{
	if (w->prefix != 0) {
		if (w->dst - w->dst0 > 127) w->err = JITC_ERR_REGNUM;
		w->dst0[-1] = (UCHAR) ((w->dst - w->dst0) & 0xff);
	}
	return;
}

void jitCompA0001_checkCompPtr(struct JitCompWork *w, int p0, int p1)
{
	if (p0 >= 0x3f || p0 < 0) w->err = JITC_ERR_PREGNUM;
	if (p1 >= 0x3f || p1 < 0) w->err = JITC_ERR_PREGNUM;
	/* 比較可能可能なのかのチェックのコードを出力 */	/* 未完成 */
	return;
}

void jitCompA000_loadRegCacheAll(struct JitCompWork *w)
{
	jitCompA0001_movReg32EbpDisp(w, 3 /* EBX */, 0 * 4); /* EBX = R00; */
	jitCompA0001_movReg32EbpDisp(w, 1 /* ECX */, 1 * 4); /* ECX = R01; */
	jitCompA0001_movReg32EbpDisp(w, 2 /* EDX */, 2 * 4); /* EDX = R02; */
	return;
}

void jitCompA000_storeRegCacheAll(struct JitCompWork *w)
{
	jitCompA0001_movEbpDispReg32(w, 0 * 4, 3 /* EBX */); /* R00 = EBX; */
	jitCompA0001_movEbpDispReg32(w, 1 * 4, 1 /* ECX */); /* R01 = ECX; */
	jitCompA0001_movEbpDispReg32(w, 2 * 4, 2 /* EDX */); /* R02 = EDX; */
	return;
}

void jitCompA000_loadRegCacheEcx(struct JitCompWork *w)
{
	jitCompA0001_movReg32EbpDisp(w, 1 /* ECX */, 1 * 4); /* ECX = R01; */
	return;
}

void jitCompA000_storeRegCacheEcx(struct JitCompWork *w)
{
	jitCompA0001_movEbpDispReg32(w, 1 * 4, 1 /* ECX */); /* R01 = ECX; */
	return;
}

void jitCompA000_loadRegCacheEdx(struct JitCompWork *w)
{
	jitCompA0001_movReg32EbpDisp(w, 2 /* EDX */, 2 * 4); /* EDX = R02; */
	return;
}

void jitCompA000_storeRegCacheEdx(struct JitCompWork *w)
{
	jitCompA0001_movEbpDispReg32(w, 2 * 4, 2 /* EDX */); /* R02 = EDX; */
	return;
}

int jitCompA000_selectRegCache(int rxx, int reg)
{
	if (rxx == 0) reg = 3; /* EBX */
	if (rxx == 1) reg = 1; /* ECX */
	if (rxx == 2) reg = 2; /* EDX */
	return reg;
}

void jitCompA000_loadPRegCacheAll(struct JitCompWork *w)
{
//	jitCompA0001_movReg32EbpDisp(w, 5 /* EBP */, 256 + 0 * 32 + 0); /* EBP = P00; */
	jitCompA0001_movReg32EbpDisp(w, 6 /* ESI */, 256 + 1 * 32 + 0); /* ESI = P01; */
	jitCompA0001_movReg32EbpDisp(w, 7 /* EDI */, 256 + 2 * 32 + 0); /* EDI = P02; */
	return;
}

void jitCompA000_storePRegCacheAll(struct JitCompWork *w)
{
//	jitCompA0001_movEbpDispReg32(w, 256 + 0 * 32 + 0, 5 /* EBP */); /* P00 = EBP; */
	jitCompA0001_movEbpDispReg32(w, 256 + 1 * 32 + 0, 6 /* ESI */); /* P01 = ESI; */
	jitCompA0001_movEbpDispReg32(w, 256 + 2 * 32 + 0, 7 /* EDI */); /* P02 = EDI; */
	return;
}

int jitCompA000_selectPRegCache(int pxx, int reg)
{
//	if (pxx == 0) reg = 5; /* EBP */
	if (pxx == 1) reg = 6; /* ESI */
	if (pxx == 2) reg = 7; /* EDI */
	return reg;
}

int jitCompA000_convTyp(int t)
{
	int r = -1;
	if (2 <= t && t <= 7) r = t;
	if (8 <= t && t <= 13) r = 2 | (t & 1);
	if (14 <= t && t <= 15) r = 4 | (t & 1);
	if (16 <= t && t <= 21) r = 6 | (t & 1);
	return r;
}

int jitCompA000_dataWidth(int t)
{
	int r = -1;
	t >>= 1;
	if (t == 0x0002 / 2) r =  8;
	if (t == 0x0004 / 2) r = 16;
	if (t == 0x0006 / 2) r = 32;
	if (t == 0x0008 / 2) r =  4;
	if (t == 0x000a / 2) r =  2;
	if (t == 0x000c / 2) r =  1;
	if (t == 0x000e / 2) r = 12;
	if (t == 0x0010 / 2) r = 20;
	if (t == 0x0012 / 2) r = 24;
	if (t == 0x0014 / 2) r = 28;
	return r;
}

void func3c(char *ebp, int opt, int r1, int p1, int lenR, int lenP, int r0, int p0);
void func3d(char *ebp, int opt, int r1, int p1, int lenR, int lenP, int r0, int p0);
void funcf4(char *ebp, int pxx, int typ, int len);
void funcf5(char *ebp, int pxx, int typ, int len); // pxxはダミーで参照されない.
void funcf6(char *ebp, int pxx, int typ, int len);
void funcf7(char *ebp, int pxx, int typ, int len); // typとlenはダミーで参照されない.
// F5の場合、decoderが対応するalloc-freeを結びつけるのが簡単で、typやlenを指定必須にしてもフロントエンドコードに影響はない.

int jitCompiler(UCHAR *dst, UCHAR *dst1, const UCHAR *src, const UCHAR *src1, const UCHAR *src0, struct LabelTable *label, int maxLabels, int level, int debugInfo1, int flags)
/* IA-32用 */
/* 本来ならこのレイヤでは文法チェックしない */
{
	struct JitCompWork w;
	UCHAR *dst00 = dst, *errmsg = "", *enter0 = NULL;
	const UCHAR *oldsrc;
	int timecount = 0, i, j, lastlabel = -1, debugInfo0 = -1;
	int reg0, reg1, reg2, cmp0reg = -1, cmp0lev = 0;
	w.dst = w.dst0 = dst;
	w.err = 0;
	w.maxLabels = maxLabels;
	jitCompPutByte1(w.dst, 0x60); /* PUSHAD(); */
	jitCompA000_loadRegCacheAll(&w); /* start-up */
	jitCompA000_loadPRegCacheAll(&w);
	while (src < src1) {
		w.prefix = 0;
		if (w.dst + 256 > dst1) { w.err = JITC_ERR_DST1; goto err_w; }
		timecount++;
		if (timecount >= 64) {
			timecount -= 64;
			/* 未完成(timeoutチェックコードを入れる) */
		}
prefix_continue:
		switch (*src) {

		case 0x00:	/* NOP */
			if (w.prefix != 0) { w.err = JITC_ERR_PREFIX; goto err_w; }
			break;

		case 0x01:	/* LB */
			if (enter0 == NULL && (src[6] == 0x3c || (src[6] == 0xfe && src[7] == 0x01 && src[9] == 0x3c))) {
				jitCompPutByte1(w.dst, 0xe9);
				enter0 = w.dst;
				jitCompPutImm32(&w, 0);
			}
			if (src[6] == 0x34) {
				jitCompPutByte1(w.dst, 0xe9);
				i = jitCompA000_convTyp(jitCompGetImm32(&src[7]));
				j = 0;
				if (i == 2 || i == 3) { j = 1; }
				if (i == 4 || i == 5) { j = 2; }
				if (i == 6 || i == 7) { j = 4; }
				j *= jitCompGetImm32(&src[11]);
				if (j <= 0) w.err = JITC_ERR_BADTYPE;
				jitCompPutImm32(&w, j);
				#if (jitCompA0001_OPTIMIZE_JMP != 0)
					if (j <= 127) {
						w.dst -= 5;
						jitCompPutByte2(w.dst, 0xeb, j);
					}
				#endif
			}
			#if (jitCompA0001_OPTIMIZE_ALIGN != 0)
				for (;;) {
					i = ((int) w.dst) & (jitCompA0001_OPTIMIZE_ALIGN - 1); /* ALIGNで割ったあまりを計算 */
					if (i == 0) break;
					i = jitCompA0001_OPTIMIZE_ALIGN - i;
					if (i == 1) { jitCompPutByte1(w.dst, 0x90); } /* NOP(); */
					if (i == 2) { jitCompPutByte2(w.dst, 0x89, 0xc0); } /* MOV(EAX, EAX); */
					if (i == 3) { jitCompPutByte3(w.dst, 0x8d, 0x76, 0x00); } /* LEA(ESI, [ESI+0]); */
					if (i == 4) { jitCompPutByte4(w.dst, 0x8d, 0x74, 0x26, 0x00); } /* LEA(ESI, [ESI*1+0]); */
					if (i == 5) { jitCompPutByte1(w.dst, 0x0d); jitCompPutImm32(&w, 0); } /* OR(EAX, 0); */
					if (i == 6) { jitCompPutByte2(w.dst, 0x8d, 0xb6); jitCompPutImm32(&w, 0); } /* LEA(ESI, [ESI+0]); */
					if (i >= 7) { jitCompPutByte3(w.dst, 0x8d, 0xb4, 0x26); jitCompPutImm32(&w, 0); } /* LEA(ESI, [ESI*1+0]); */
				}
			#endif
			if ((flags & JITC_PHASE1) == 0) {
				i = jitCompGetLabelNum(&w, src + 2);
//printf("i=%06X %06X\n", i, src-src0);
				if (label[i].typ != 0 && w.err == 0) { w.err = JITC_ERR_LABELREDEF; goto err_w; }
				if (w.prefix != 0) { w.err = JITC_ERR_PREFIX; goto err_w; }
				label[i].typ = src[1] + 1;
				label[i].p = w.dst;
				lastlabel = i;
			}
			cmp0reg = -1;
			timecount = 0;
			/* 未完成(timeoutチェックコードを入れる) */
			break;

		case 0x02:	/* LIMM */
			if (src[1] == 0x3f && w.prefix != 0) w.err = JITC_ERR_PREFIX;
			#if (jitCompA0001_USE_R3F_IMM32 != 0)
				if (src[1] == 0x3f) {
					w.r3f = jitCompGetImm32(src + 2);
					break;
				}
			#endif
			i = jitCompGetImm32(src + 2);
			reg0 = jitCompA000_selectRegCache(src[1], 0 /* EAX */);
			#if (jitCompA0001_OPTIMIZE_MOV != 0)
				if (i == 0) {
					jitCompPutByte2(w.dst, 0x31, 0xc0 | reg0 << 3 | reg0);	/* XOR(reg0, reg0); */
					jitCompA0001_movRxxEax(&w, src[1]);
					break;
				}
			#endif
			jitCompPutByte1(w.dst, 0xb8 | reg0);	/* MOV(reg0, ?); */
			jitCompPutImm32(&w, i);
			if (reg0 == 0)
				jitCompA0001_movRxxEax(&w, src[1]);
			break;

		case 0x03:	/* PLIMM */	/* 未完成 */
			i = jitCompGetLabelNum(&w, src + 2);
			if ((flags & JITC_PHASE1) != 0 && w.err != 0) {
				if (label[i].typ == 0) { w.err = JITC_ERR_LABELNODEF; goto err_w; }
				if (src[1] != 0 && label[i].typ != 2) { w.err = JITC_ERR_LABELTYP; goto err_w; }
			}
			if (src[1] == 0x3f) {
				if (w.prefix == 0) {
					jitCompPutByte1(w.dst, 0xe9); /* JMP(?); */
				} else {
					w.dst[-1] = w.dst[-2] ^ 0xf1; /* 74->85, 75->84 */
					w.dst[-2] = 0x0f;
					w.prefix = 0;
				}
				j = 0;
				if ((flags & JITC_PHASE1) != 0 || ((flags & JITC_PHASE1) == 0) && label[i].typ != 0)
					j = label[i].p - (w.dst + 4);
				jitCompPutImm32(&w, j);
				#if (jitCompA0001_OPTIMIZE_JMP != 0)
					if (-128 - 3 <= j && j < 0) {
						if (w.dst[-5] == 0xe9) {
							j += 3;
							w.dst -= 5;
							jitCompPutByte1(w.dst, 0xeb); /* JMP(?); */
						} else {
							j += 4;
							w.dst -= 6;
							jitCompPutByte1(w.dst, w.dst[1] ^ 0xf0);
						}
						jitCompPutByte1(w.dst, j & 0xff);
					}
				#endif
			} else {
				reg0 = jitCompA000_selectPRegCache(src[1], 0 /* EAX */);
				jitCompPutByte1(w.dst, 0xb8 | reg0);	/* MOV(reg0, ?); */
				jitCompPutImm32(&w, (int) label[i].p);
				if (reg0 == 0)
					jitCompA0001_movEbpDispReg32(&w, 256 + src[1] * 32, reg0); /* MOV([EBP+?], reg0); */
			}
			break;

		case 0x04:	/* CND (prefix) */
			if (src[1] >= 0x40) w.err = JITC_ERR_REGNUM;
			reg0 = jitCompA000_selectRegCache(src[1], -1 /* mem */);
			if (reg0 < 0) {
				jitCompPutByte1(w.dst, 0xf7); /* TEST([EBP+?],1); */
				jitCompA0001_85DispN(&w, src[1] * 4, 0);
			} else {
				jitCompPutByte2(w.dst, 0xf7, 0xc0 | reg0); /* TEST(reg0,1); */
			}
			jitCompPutImm32(&w, 1);
			jitCompPutByte2(w.dst, 0x74, 0x00);	/* JZ($+2) */
			cmp0reg = -1;
			if (w.err != 0) goto err_w;
			src += 2;
			w.prefix = 1;
			w.dst0 = w.dst;
			goto prefix_continue;

		case 0x08: /* LMEM */	/* 未完成 */
			reg0 = jitCompA000_selectRegCache(src[1], 0 /* EAX */);
			reg1 = jitCompA000_selectPRegCache(src[6], 2 /* EDX */);
			if (reg0 != 0 /* EAX */ && reg1 == 2 /* EDX */)
				reg1 = 0; /* EAX */
			if (reg1 == 2 /* EDX */)
				jitCompA000_storeRegCacheEdx(&w);
			if (reg1 <= 3 /* EAX, EDX */)
				jitCompA0001_movReg32EbpDisp(&w, reg1, 256 + src[6] * 32 + 0); /* MOV(reg1, [EBP+?]); */
			/* p0, p1と比較 */
			/* read権はあるのか？ */
			i = jitCompA000_convTyp(jitCompGetImm32(src + 2));
			switch (i) {
			case 0x0002:
				jitCompPutByte3(w.dst, 0x0f, 0xbe, reg0 << 3 | reg1);	/* MOVSX(reg0,BYTE [reg1]); */
				break;
			case 0x0003:
				jitCompPutByte3(w.dst, 0x0f, 0xb6, reg0 << 3 | reg1);	/* MOVZX(reg0,BYTE [reg1]); */
				break;
			case 0x0004:
				jitCompPutByte3(w.dst, 0x0f, 0xbf, reg0 << 3 | reg1);	/* MOVSX(reg0,WORD [reg1]); */
				break;
			case 0x0005:
				jitCompPutByte3(w.dst, 0x0f, 0xb7, reg0 << 3 | reg1);	/* MOVZX(reg0,WORD [reg1]); */
				break;
			case 0x0006:
			case 0x0007:
				jitCompPutByte2(w.dst, 0x8b, reg0 << 3 | reg1);	/* MOV(reg0, [reg1]); */
				break;
			default:
				w.err = JITC_ERR_BADTYPE;
			}
			if (reg0 == 0 /* EAX */)
				jitCompA0001_movRxxEax(&w, src[1]);
			if (reg1 == 2 /* EDX */)
				jitCompA000_loadRegCacheEdx(&w);
			break;

		case 0x09: /* SMEM */	/* 未完成 */
			reg0 = jitCompA000_selectRegCache(src[1], 0 /* EAX */);
			reg1 = jitCompA000_selectPRegCache(src[6], 2 /* EDX */);
			if (reg0 != 0 /* EAX */ && reg1 == 2 /* EDX */)
				reg1 = 0; /* EAX */
			if (reg1 == 2 /* EDX */)
				jitCompA000_storeRegCacheEdx(&w);
			if (reg1 <= 3 /* EAX, EDX */)
				jitCompA0001_movReg32EbpDisp(&w, reg1, 256 + src[6] * 32 + 0); /* MOV(reg1, [EBP+?]); */
			/* p0, p1と比較 */
			/* write権はあるのか？ */
			if (reg0 == 0 /* EAX */)
				jitCompA0001_movEaxRxx(&w, src[1]);
			/* 値の範囲チェック */
			i = jitCompA000_convTyp(jitCompGetImm32(src + 2));
			switch (i) {
			case 0x0002:
			case 0x0003:
				jitCompPutByte2(w.dst, 0x88, reg0 << 3 | reg1);	/* MOV([reg1], BYTE(reg0)); */
				break;
			case 0x0004:
			case 0x0005:
				jitCompPutByte3(w.dst, 0x66, 0x89, reg0 << 3 | reg1);	/* MOV([reg1], WORD(reg0)); */
				break;
			case 0x0006:
			case 0x0007:
				jitCompPutByte2(w.dst, 0x89, reg0 << 3 | reg1);	/* MOV([reg1], reg0); */
				break;
			default:
				w.err = JITC_ERR_BADTYPE;
			}
			if (reg1 == 2 /* EDX */)
				jitCompA000_loadRegCacheEdx(&w);
			break;

		case 0x0e: /* PADD */		/* 未完成 */
			reg0 = jitCompA000_selectPRegCache(src[1], 0 /* EAX */);
			reg1 = jitCompA000_selectPRegCache(src[6], -1 /* mem */);
			if (reg1 < 0 /* mem */)
				jitCompA0001_movReg32EbpDisp(&w, reg0, 256 + src[6] * 32 + 0); /* MOV(reg0, [EBP+?]); */
			if (reg1 >= 0 && reg0 != reg1) {
				jitCompPutByte2(w.dst, 0x89, 0xc0 | reg1 << 3 | reg0); /* MOV(reg0, reg1); */
			}
			i = jitCompA000_convTyp(jitCompGetImm32(src + 2));
			j = -1;
			if (0x0002 <= i && i <= 0x0007)
				j = (i - 0x0002) >> 1;
			if (j < 0) { w.err = JITC_ERR_BADTYPE; goto err_w; }
			#if (jitCompA0001_USE_R3F_IMM32 != 0)
				if (src[7] == 0x3f) {
					j = w.r3f << j;
					#if (jitCompA0001_USE_R3F_IMM8 != 0)
						if (-0x80 <= j && j <= 0x7f) {
							#if (jitCompA0001_USE_R3F_INCDEC != 0)
								if (j ==  1) { jitCompPutByte1(w.dst, 0x40 | reg0); goto padd1; } /* INC */
								if (j == -1) { jitCompPutByte1(w.dst, 0x48 | reg0); goto padd1; } /* DEC */
							#endif
							jitCompPutByte3(w.dst, 0x83, 0xc0 | reg0, j & 0xff);	/* ADD(reg0, im8); */
							goto padd1;
						}
					#endif
					if (reg0 == 0) {
						jitCompPutByte1(w.dst, 0x05);	/* ADD(reg0, ?); */
					} else {
						jitCompPutByte2(w.dst, 0x81, 0xc0 | reg0);	/* ADD(reg0, ?); */
					}
					jitCompPutImm32(&w, j);
					goto padd1;
				}
			#endif
			if (src[7] >= 0x40) w.err = JITC_ERR_REGNUM;
			if (j == 0) {
				reg1 = jitCompA000_selectRegCache(src[7], -1 /* mem */);
				if (reg1 >= 0) {
					jitCompPutByte2(w.dst, 0x01, 0xc0 | reg1 << 3 | reg0);	/* ADD(reg0, reg1); */
				} else {
					jitCompPutByte1(w.dst, 0x03);	/* ADD(reg0, [EBP+?]); */
					jitCompA0001_85DispN(&w, src[7] * 4, reg0);
				}
			} else {
				reg1 = jitCompA000_selectRegCache(src[7], -1 /* mem */);
				reg2 = 2; /* EDX */
				jitCompA000_storeRegCacheEdx(&w);
				if (reg1 < 0)
					jitCompA0001_movReg32EbpDisp(&w, reg2, src[7] * 4); /* MOV(reg2, [EBP+?]); */
				if (reg1 >= 0 && reg1 != reg2) {
					jitCompPutByte2(w.dst, 0x89, 0xc0 | reg1 << 3 | reg2); /* MOV(reg2, reg1); */
				}
				jitCompPutByte3(w.dst, 0xc1, 0xe0 | reg2, j);	/* SHL(reg2, ?); */
				jitCompPutByte2(w.dst, 0x01, 0xc0 | reg2 << 3 | reg0);	/* ADD(reg0, reg2); */
				jitCompA000_loadRegCacheEdx(&w);
			}
#if (jitCompA0001_USE_R3F_IMM32 != 0)
	padd1:
#endif
			if (reg0 == 0 /* EAX */)
				jitCompA0001_movEbpDispReg32(&w, 256 + src[1] * 32 + 0, reg0); /* MOV([EBP+?], reg0); */
			if (src[1] != src[6]) {
				for (i = 4; i < 32; i += 4) {
					jitCompA0001_movReg32EbpDisp(&w, 0 /* EAX */, 256 + src[6] * 32 + i); /* MOV(EAX, [EBP+?]); */
					jitCompA0001_movEbpDispReg32(&w, 256 + src[1] * 32 + i, 0 /* EAX */); /* MOV([EBP+?], EAX); */
				}
			}
			cmp0reg = -1;
			break;

		case 0x0f: /* PDIF */
			reg0 = jitCompA000_selectRegCache(src[1], 0 /* EAX */);
			jitCompA000_storePRegCacheAll(&w); // 手抜き.
			jitCompA0001_checkCompPtr(&w, src[6], src[7]);
			jitCompA0001_movReg32EbpDisp(&w, reg0, 256 + src[6] * 32 + 0); /* MOV(reg0, [EBP+?]); */
			jitCompPutByte1(w.dst, 0x2b);	/* SUB(EAX, [EBP+?]); */
			jitCompA0001_85DispN(&w, 256 + src[7] * 32 + 0, reg0);
			i = jitCompA000_convTyp(jitCompGetImm32(src + 2));
			j = -1;
			if (0x0002 <= i && i <= 0x0007)
				j = (i - 0x0002) >> 1;
			if (j < 0) { w.err = JITC_ERR_BADTYPE; goto err_w; }
			if (j > 0) {
				jitCompPutByte3(w.dst, 0xc1, 0xf8 | reg0, j);	/* SAR(reg0,?); */
			}
			if (reg0 == 0 /* EAX */)
				jitCompA0001_movRxxEax(&w, src[1]);
			cmp0reg = src[1]; cmp0lev = 1;
			break;

		case 0x10:	/* OR */
		case 0x11:	/* XOR */
		case 0x12:	/* AND */
		case 0x14:	/* ADD */
		case 0x15:	/* SUB */
		case 0x16:	/* MUL */
			if (src[1] >= 0x3f) w.err = JITC_ERR_REGNUM;
			reg0 = jitCompA000_selectRegCache(src[1], 0 /* EAX */);
			reg1 = jitCompA000_selectRegCache(src[2], -1 /* mem */);
			#if (jitCompA0001_USE_R3F_IMM32 != 0)
				if (src[2] == 0x3f) {	// SUBのみ該当.
					if (*src != 0x15) w.err = JITC_ERR_REGNUM;
					reg2 = jitCompA000_selectRegCache(src[3], -1 /* mem */);
					if (reg2 >= 0)
						jitCompA000_storeRegCacheAll(&w);
					jitCompPutByte1(w.dst, 0xb8 | reg0);	/* MOV(reg0, ?); */
					jitCompPutImm32(&w, w.r3f);
					jitCompPutByte1(w.dst, 0x2b);
					jitCompA0001_85DispN(&w, src[3] * 4, reg0);
					if (reg0 == 0)
						jitCompA0001_movRxxEax(&w, src[1]);
					break;
				}
			#endif
			if (reg1 < 0) {
				jitCompA0001_movReg32EbpDisp(&w, reg0, src[2] * 4); /* MOV(reg0, [EBP+?]); */
			}
			if (reg1 >= 0 && reg0 != reg1) {
				jitCompPutByte2(w.dst, 0x89, 0xc0 | reg1 << 3 | reg0); /* MOV(reg0, reg1); */
			}
			if (!(src[0] == 0x10 && src[3] == 0xff)) {  // bugfix: hinted by Iris, 2013.06.26. thanks!
				cmp0reg = src[1];
				cmp0lev = 1;
				if (src[0] < 0x14)
					cmp0lev = 2;
				if (src[0] == 0x16)
					cmp0reg = -1;
			}
			if (!(src[0] == 0x10 && src[3] == 0xff)) {
				#if (jitCompA0001_USE_R3F_IMM32 != 0)
					if (src[3] == 0x3f) {
						if (*src == 0x16 && w.r3f == -1) {
							jitCompPutByte2(w.dst, 0xf7, 0xd8 | reg0); /* NEG(reg0); */
							if (reg0 == 0)
								jitCompA0001_movRxxEax(&w, src[1]);
							break;
						}
						#if (jitCompA0001_USE_R3F_INCDEC != 0)
							if ((*src == 0x14 && w.r3f == 1) || (*src == 0x15 && w.r3f == -1)) {
								jitCompPutByte1(w.dst, 0x40 | reg0);	/* INC(reg0); */
								if (reg0 == 0)
									jitCompA0001_movRxxEax(&w, src[1]);
								break;
							}
							if ((*src == 0x15 && w.r3f == 1) || (*src == 0x14 && w.r3f == -1)) {
								jitCompPutByte1(w.dst, 0x48 | reg0);	/* DEC(reg0); */
								if (reg0 == 0)
									jitCompA0001_movRxxEax(&w, src[1]);
								break;
							}
						#endif
						#if (jitCompA0001_USE_R3F_IMM8 != 0)
							if (-0x80 <= w.r3f && w.r3f <= 0x7f) {
								if (*src != 0x16) {
									static UCHAR basic_op_table_im8[] = { 0xc8, 0xf0, 0xe0, 0, 0xc0, 0xe8 };
									jitCompPutByte3(w.dst, 0x83, basic_op_table_im8[*src - 0x10] | reg0, w.r3f & 0xff);
								} else {
									jitCompPutByte3(w.dst, 0x6b, 0xc0 | reg0 << 3 | reg0, w.r3f & 0xff);
								}
								if (reg0 == 0)
									jitCompA0001_movRxxEax(&w, src[1]);
								break;
							}
						#endif
						if (reg0 == 0 /* EAX */) {
							static UCHAR basic_op_table_im32_eax[] = { 0x0d, 0x35, 0x25, 0, 0x05, 0x2d, 0xc0 };
							if (*src == 0x16) { jitCompPutByte1(w.dst, 0x69); }
							jitCompPutByte1(w.dst, basic_op_table_im32_eax[*src - 0x10]);
						} else {
							if (*src != 0x16) {
								static UCHAR basic_op_table_im32_reg[] = { 0xc8, 0xf0, 0xe0, 0, 0xc0, 0xe8 };
								jitCompPutByte2(w.dst, 0x81, basic_op_table_im32_reg[*src - 0x10] | reg0);
							} else {
								jitCompPutByte2(w.dst, 0x69, 0xc0 | reg0 << 3 | reg0);
							}
						}
						jitCompPutImm32(&w, w.r3f);
						if (reg0 == 0)
							jitCompA0001_movRxxEax(&w, src[1]);
						break;
					}
				#endif
				reg1 = jitCompA000_selectRegCache(src[3], -1 /* mem */);
				if (src[3] >= 0x40) w.err = JITC_ERR_REGNUM;
				if (*src != 0x16) {
					if (reg1 >= 0) {
						static UCHAR basic_op_table_rr[] = { 0x09, 0x31, 0x21, 0, 0x01, 0x29 }; /* op(reg,reg); */
						jitCompPutByte2(w.dst, basic_op_table_rr[*src - 0x10], 0xc0 | reg1 << 3 | reg0);
					} else {
						static UCHAR basic_op_table_rm[] = { 0x0b, 0x33, 0x23, 0, 0x03, 0x2b, 0xaf }; /* op(reg,mem); */
						jitCompPutByte1(w.dst, basic_op_table_rm[*src - 0x10]);
						jitCompA0001_85DispN(&w, src[3] * 4, reg0);
					}
				} else {
					if (reg1 >= 0) {
						jitCompPutByte3(w.dst, 0x0f, 0xaf, 0xc0 | reg0 << 3 | reg1);
					} else {
						jitCompPutByte2(w.dst, 0x0f, 0xaf);
						jitCompA0001_85DispN(&w, src[3] * 4, reg0);
					}
				}
			}
			if (reg0 == 0)
				jitCompA0001_movRxxEax(&w, src[1]);
			break;

		case 0x18:	/* SHL */
		case 0x19:	/* SAR */
			if (src[1] >= 0x3f) w.err = JITC_ERR_REGNUM;
			if (src[3] >= 0x40) w.err = JITC_ERR_REGNUM;
			#if (jitCompA0001_USE_R3F_IMM32 != 0)
				if (src[3] == 0x3f) {
					reg0 = jitCompA000_selectRegCache(src[1], 0 /* EAX */);
					reg1 = jitCompA000_selectRegCache(src[2], -1 /* mem */);
					if (src[1] >= 0x3f) w.err = JITC_ERR_REGNUM;
					if (reg1 == -1)
						jitCompA0001_movReg32EbpDisp(&w, reg0, src[2] * 4); /* MOV(reg1, [EBP+?]); */
					else {
						if (reg0 != reg1) {
							jitCompPutByte2(w.dst, 0x89, 0xc0 | reg1 << 3 | reg0); /* MOV(reg0, reg1); */
						}
					}
					if (*src == 0x18) { jitCompPutByte3(w.dst, 0xc1, 0xe0 | reg0, w.r3f); } /* SHL(reg0, im8); */
					if (*src == 0x19) { jitCompPutByte3(w.dst, 0xc1, 0xf8 | reg0, w.r3f); } /* SAR(reg0, im8); */
					if (reg0 == 0 /* EAX */)
						jitCompA0001_movRxxEax(&w, src[1]);
					break;
				}
			#endif
			jitCompA000_storeRegCacheAll(&w); // 手抜き.
			jitCompA0001_movReg32EbpDisp(&w, 1 /* ECX */, src[3] * 4); /* MOV(ECX, [EBP+?]); */
			#if (jitCompA0001_USE_R3F_IMM32 != 0)
				if (src[2] == 0x3f) {
					jitCompPutByte1(w.dst, 0xb8);	/* MOV(EAX, ?); */
					jitCompPutImm32(&w, w.r3f);
				} else {
					jitCompA0001_movEaxRxx(&w, src[2]);
				}
			#else
				jitCompA0001_movEaxRxx(&w, src[2]);
			#endif
			if (*src == 0x18) { jitCompPutByte2(w.dst, 0xd3, 0xe0); } /* SHL(EAX, CL); */
			if (*src == 0x19) { jitCompPutByte2(w.dst, 0xd3, 0xf8); } /* SAR(EAX, CL); */
			jitCompA0001_movRxxEax(&w, src[1]);
			jitCompA000_loadRegCacheAll(&w); // 手抜き.
			cmp0reg = src[1];
			cmp0lev = 1;
			break;

		case 0x1a:	/* DIV */
		case 0x1b:	/* MOD */
			if (src[1] >= 0x3f) w.err = JITC_ERR_REGNUM;
			if (src[2] >= 0x40) w.err = JITC_ERR_REGNUM;
			if (src[3] >= 0x40) w.err = JITC_ERR_REGNUM;
			jitCompA000_storeRegCacheAll(&w); // 手抜き.
			#if (jitCompA0001_USE_R3F_IMM32 != 0)
				if (src[3] == 0x3f) {
					jitCompPutByte1(w.dst, 0xb8 | 1);	/* MOV(ECX, ?); */
					jitCompPutImm32(&w, w.r3f);
				} else {
					jitCompA0001_movReg32EbpDisp(&w, 1 /* ECX */, src[3] * 4); /* MOV(ECX, [EBP+?]); */
				}
				if (src[2] == 0x3f) {
					jitCompPutByte1(w.dst, 0xb8 | 0);	/* MOV(EAX, ?); */
					jitCompPutImm32(&w, w.r3f);
				} else {
					jitCompA0001_movReg32EbpDisp(&w, 0 /* EAX */, src[2] * 4); /* MOV(EAX, [EBP+?]); */
				}
			#else
				jitCompA0001_movReg32EbpDisp(&w, 1 /* ECX */, src[3] * 4); /* MOV(ECX, [EBP+?]); */
				jitCompA0001_movReg32EbpDisp(&w, 0 /* EAX */, src[2] * 4); /* MOV(EAX, [EBP+?]); */
			#endif
			jitCompPutByte1(w.dst, 0x99);	/* CDQ(); */
			/* ECXがゼロではないことを確認すべき */
			jitCompPutByte2(w.dst, 0xf7, 0xf9);	/* IDIV(ECX); */
			if (*src == 0x1a) { jitCompA0001_movEbpDispReg32(&w, src[1] * 4, 0 /* EAX */); }
			if (*src == 0x1b) { jitCompA0001_movEbpDispReg32(&w, src[1] * 4, 2 /* EDX */); }
			jitCompA000_loadRegCacheAll(&w); // 手抜き.
			cmp0reg = -1;
			break;

		case 0x1e: /* PCP */		/* 未完成 */
			if (src[1] >= 0x40 || src[2] >= 0x40) w.err = JITC_ERR_PREGNUM;
			if (src[2] == 0x3f) w.err = JITC_ERR_PREGNUM;
			if (src[1] != 0x3f) {
				/* src[2] == 0xff の場合に対応できてない */
				jitCompA000_storePRegCacheAll(&w); // 手抜き.
				for (i = 0; i < 32; i += 4) {
					jitCompA0001_movReg32EbpDisp(&w, 0 /* EAX */, 256 + src[2] * 32 + i); /* MOV(EAX, [EBP+?]); */
					jitCompA0001_movEbpDispReg32(&w, 256 + src[1] * 32 + i, 0 /* EAX */); /* MOV([EBP+?], EAX); */
				}
				jitCompA000_loadPRegCacheAll(&w); // 手抜き.
			} else {
				/* セキュリティチェックが足りてない! */
				reg0 = 0; /* EAX */
				jitCompA000_storePRegCacheAll(&w); // 手抜き.
				jitCompA0001_movReg32EbpDisp(&w, reg0, 256 + src[2] * 32 + 0); /* MOV(EAX, [EBP+?]); */
				jitCompPutByte2(w.dst, 0xff, 0xe0);	/* JMP(EAX); */
			}
			break;

		case 0x20:	/* CMPE */
		case 0x21:	/* CMPNE */
		case 0x22:	/* CMPL */
		case 0x23:	/* CMPGE */
		case 0x24:	/* CMPLE */
		case 0x25:	/* CMPG */
		case 0x26:	/* TSTZ */
		case 0x27:	/* TSTNZ */
			reg0 = jitCompA000_selectRegCache(src[2], 0 /* EAX */);
			reg1 = jitCompA000_selectRegCache(src[3], -1 /* mem */);
			if (src[1] == 0x3f) {
				/* 特殊構文チェック */
				if (w.prefix != 0) { w.err = JITC_ERR_PREFIX; goto err_w; }
				if (src[4] != 0x04 || src[5] != 0x3f || src[6] != 0x03 || src[7] != 0x3f) {
					w.err = JITC_ERR_IDIOM; goto err_w;
				}
			}
			if (reg0 == 0)
				jitCompA0001_movEaxRxx(&w, src[2]);
			#if (jitCompA0001_USE_R3F_IMM32 != 0)
				if (src[3] == 0x3f) {
					#if (jitCompA0001_OPTIMIZE_CMP != 0)
						if ((*src <= 0x25 && w.r3f == 0) || (*src >= 0x26 && w.r3f == -1)) {
							i = 0;
							if (cmp0reg == src[2]) {
								if (cmp0lev >= 1 && (src[0] == 0x20 || src[0] == 0x21 || src[0] == 0x26 || src[0] == 0x27))
									i = 1;
								if (cmp0lev >= 2 && (src[0] == 0x22 || src[0] == 0x23 || src[0] == 0x24 || src[0] == 0x25))
									i = 1;
							}
							if (i == 0) {
								jitCompPutByte2(w.dst, 0x85, 0xc0 | reg0 << 3 | reg0);	/* TEST(reg0, reg0); */
							}
							cmp0reg = src[2];
							cmp0lev = 2;
							goto cmpcc1;
						}
					#endif
					#if (jitCompA0001_USE_R3F_IMM8 != 0)
						if (-0x80 <= w.r3f && w.r3f <= 0x7f && *src <= 0x25) {
							jitCompPutByte3(w.dst, 0x83, 0xf8 | reg0, w.r3f);
							goto cmpcc1;
						}
					#endif
					if (reg0 == 0) {
						if (*src <= 0x25) { jitCompPutByte1(w.dst, 0x3d); }
						if (*src >= 0x26) { jitCompPutByte1(w.dst, 0xa9); }
					} else {
						if (*src <= 0x25) { jitCompPutByte2(w.dst, 0x81, 0xf8 | reg0); }
						if (*src >= 0x26) { jitCompPutByte2(w.dst, 0xf7, 0xc0 | reg0); }
					}
					jitCompPutImm32(&w, w.r3f);
					goto cmpcc1;
				}
			#endif
			if (src[3] >= 0x40) w.err = JITC_ERR_PREGNUM;
			if (reg1 >= 0) {
				if (*src <= 0x25) { jitCompPutByte2(w.dst, 0x39, 0xc0 | reg1 << 3 | reg0); }
				if (*src >= 0x26) { jitCompPutByte2(w.dst, 0x85, 0xc0 | reg1 << 3 | reg0); }
			} else {
				if (*src <= 0x25) { jitCompPutByte1(w.dst, 0x3b); }
				if (*src >= 0x26) { jitCompPutByte1(w.dst, 0x85); }
				jitCompA0001_85DispN(&w, src[3] * 4, reg0);
			}
cmpcc1:
			if (w.err != 0) goto err_w;
			static UCHAR cmpcc_table0[] = {
				0x04, 0x05, 0x0c, 0x0d, 0x0e, 0x0f, 0x04, 0x05,	/* CMPcc, TSTcc */
				0x04, 0x05, 0x02, 0x03, 0x06, 0x07				/* PCMPcc */
			};
			#if (jitCompA0001_USE_R3F_CMPJMP != 0)
				if (src[1] == 0x3f) {
					/* 特殊構文を利用した最適化 */
					jitCompPutByte2(w.dst, 0x0f, 0x80 | cmpcc_table0[*src - 0x20]);
					src += 6;
					i = jitCompGetLabelNum(&w, src + 2);
					if ((flags & JITC_PHASE1) != 0 && w.err != 0) {
						if (label[i].typ == 0) { w.err = JITC_ERR_LABELNODEF; goto err_w; }
					//	if (label[i].typ != 1) { w.err = JITC_ERR_LABELTYP; goto err_w; }
					}
					j = 0;
					if ((flags & JITC_PHASE1) != 0 || ((flags & JITC_PHASE1) == 0) && label[i].typ != 0)
						j = label[i].p - (w.dst + 4);
					jitCompPutImm32(&w, j);
					#if (jitCompA0001_OPTIMIZE_JMP != 0)
						if (-128 - 4 <= j && j < 0) {
							j += 4;
							w.dst -= 6;
							jitCompPutByte2(w.dst, w.dst[1] ^ 0xf0, j & 0xff);
						}
					#endif
					src += 6;
					if (w.err != 0) goto err_w;
					continue;
				}
			#endif
			/* 一般的なJITC */
			reg0 = jitCompA000_selectRegCache(src[1], 0 /* EAX */);
			jitCompPutByte3(w.dst, 0x0f, 0x90 | cmpcc_table0[*src - 0x20], 0xc0 | reg0);	/* SETcc(BYTE(reg0)); */
			jitCompPutByte3(w.dst, 0x0f, 0xb6, 0xc0 | reg0 << 3 | reg0);	/* MOVZX(reg0, BYTE(reg0)); */
			jitCompPutByte2(w.dst, 0xf7, 0xd8 | reg0);	/* NEG(reg0); */
			if (reg0 == 0)
				jitCompA0001_movRxxEax(&w, src[1]);
			cmp0reg = src[2];
			cmp0lev = 1;
			break;

		case 0x28:	/* PCMPE */
		case 0x29:	/* PCMPNE */
		case 0x2a:	/* PCMPL */
		case 0x2b:	/* PCMPGE */
		case 0x2c:	/* PCMPLE */
		case 0x2d:	/* PCMPG */
			if (src[1] == 0x3f) {
				/* 特殊構文チェック */
				if (w.prefix != 0) { w.err = JITC_ERR_PREFIX; goto err_w; }
				if (src[4] != 0x04 || src[5] != 0x3f || src[6] != 0x03 || src[7] != 0x3f) {
					w.err = JITC_ERR_IDIOM; goto err_w;
				}
			}
			if (src[2] >= 0x40) w.err = JITC_ERR_PREGNUM;
			jitCompA000_storePRegCacheAll(&w); // 手抜き.
			if (src[3] != 0xff)
				jitCompA0001_checkCompPtr(&w, src[2], src[3]);
			jitCompA0001_movReg32EbpDisp(&w, 0 /* EAX */, 256 + src[2] * 32 + 0); /* MOV(EAX, [EBP+?]); */
			if (src[3] != 0xff) {
				jitCompPutByte1(w.dst, 0x3b);	/* CMP(EAX, [EBP+?]); */
				jitCompA0001_85DispN(&w, 256 + src[3] * 32 + 0, 0);
			} else {
				/* ヌルポインタとの比較はこれでいいのか？たぶんよくない */
				jitCompPutByte3(w.dst, 0x83, 0xf8, 0x00);	/* CMP(EAX, 0); */
			}
			cmp0reg = -1;
			goto cmpcc1;

		case 0x30:	/* talloc(old:F4) */
		case 0x31:	/* tfree(old:F5) */
		case 0x32:	/* malloc(old:F6) */
		case 0x33:	/* mfree(old:F7) */
			jitCompA000_storeRegCacheAll(&w); // 手抜き.
			jitCompA000_storePRegCacheAll(&w); // 手抜き.
			jitCompPutByte2(w.dst, 0x6a, src[3]);	/* PUSH(?); */
			jitCompPutByte2(w.dst, 0x6a, src[2]);	/* PUSH(?); */
			jitCompPutByte2(w.dst, 0x6a, src[1]);	/* PUSH(?); */
 			jitCompPutByte1(w.dst, 0x55);	/* PUSH(EBP); */
			jitCompPutByte1(w.dst, 0xe8);
			if (*src == 0x30) j = ((UCHAR *) &funcf4) - (w.dst + 4);
			if (*src == 0x31) j = ((UCHAR *) &funcf5) - (w.dst + 4);
			if (*src == 0x32) j = ((UCHAR *) &funcf6) - (w.dst + 4);
			if (*src == 0x33) j = ((UCHAR *) &funcf7) - (w.dst + 4);
			jitCompPutImm32(&w, j);
 			jitCompPutByte3(w.dst, 0x83, 0xc4, 0x10);	/* ADD(ESP,16); */
			jitCompA000_loadRegCacheAll(&w); // 手抜き.
			jitCompA000_loadPRegCacheAll(&w); // 手抜き.
			cmp0reg = -1;
			break;

		case 0x34:	/* data (暫定) */
			cmp0reg = -1;
			if (w.prefix != 0) { w.err = JITC_ERR_PREFIX; goto err_w; }
			int k = jitCompGetImm32(&src[1]), tmpData, bitCount, dataWidth = jitCompA000_dataWidth(k);
			i = jitCompA000_convTyp(k);
			if (i < 2 || i > 7) { w.err = JITC_ERR_BADTYPE; goto err_w; }
			j = jitCompGetImm32(&src[5]);
			oldsrc = src;
			src += 9;
			bitCount = 7;
			while (j > 0) {
				if (src >= src1) { w.err = JITC_ERR_SRC1; src = oldsrc; goto err_w; }
				if (w.dst + 256 > dst1) { w.err = JITC_ERR_DST1; src = oldsrc; goto err_w; }
				tmpData = 0;
				for (k = 0; k < dataWidth; k++) {
					tmpData = tmpData << 1 | ((*src >> bitCount) & 1);
					bitCount--;
					if (bitCount < 0) {
						bitCount = 7;
						src++;
					}
				}
				if ((i & 1) == 0 && dataWidth <= 31 && (tmpData >> (dataWidth - 1)) != 0) {
					tmpData -= 1 << dataWidth;
				}
				if (i == 2 || i == 3) { jitCompPutByte1(w.dst, tmpData & 0xff); }
				if (i == 4 || i == 5) { jitCompPutByte2(w.dst, tmpData & 0xff, (tmpData >> 8) & 0xff); }
				if (i == 6 || i == 7) { jitCompPutByte4(w.dst, tmpData & 0xff, (tmpData >> 8) & 0xff, (tmpData >> 16) & 0xff, (tmpData >> 24) & 0xff); }
				j--;
			}
			continue;

		case 0x3c:	/* ENTER */
			jitCompA000_storeRegCacheAll(&w); // 手抜き.
			jitCompA000_storePRegCacheAll(&w); // 手抜き.
			jitCompPutByte2(w.dst, 0x6a, src[6]);	/* PUSH(?); */
			jitCompPutByte2(w.dst, 0x6a, src[5]);	/* PUSH(?); */
			jitCompPutByte2(w.dst, 0x6a,  src[4]       & 0x0f);	/* PUSH(?); */
			jitCompPutByte2(w.dst, 0x6a, (src[4] >> 4) & 0x0f);	/* PUSH(?); */
 			jitCompPutByte2(w.dst, 0x6a, src[3]);	/* PUSH(?); */
 			jitCompPutByte2(w.dst, 0x6a, src[2]);	/* PUSH(?); */
 			jitCompPutByte2(w.dst, 0x6a, src[1]);	/* PUSH(?); */
 			jitCompPutByte1(w.dst, 0x55);	/* PUSH(EBP); */
			jitCompPutByte1(w.dst, 0xe8);
			j = ((UCHAR *) &func3c) - (w.dst + 4);
			jitCompPutImm32(&w, j);
 			jitCompPutByte3(w.dst, 0x83, 0xc4, 0x20);	/* ADD(ESP,32); */
			jitCompA000_loadRegCacheAll(&w); // 手抜き.
			jitCompA000_loadPRegCacheAll(&w); // 手抜き.
			cmp0reg = -1;
			break;

		case 0x3d:	/* LEAVE */
			jitCompA000_storeRegCacheAll(&w); // 手抜き.
			jitCompA000_storePRegCacheAll(&w); // 手抜き.
			jitCompPutByte2(w.dst, 0x6a, src[6]);	/* PUSH(?); */
			jitCompPutByte2(w.dst, 0x6a, src[5]);	/* PUSH(?); */
			jitCompPutByte2(w.dst, 0x6a,  src[4]       & 0x0f);	/* PUSH(?); */
			jitCompPutByte2(w.dst, 0x6a, (src[4] >> 4) & 0x0f);	/* PUSH(?); */
 			jitCompPutByte2(w.dst, 0x6a, src[3]);	/* PUSH(?); */
 			jitCompPutByte2(w.dst, 0x6a, src[2]);	/* PUSH(?); */
 			jitCompPutByte2(w.dst, 0x6a, src[1]);	/* PUSH(?); */
 			jitCompPutByte1(w.dst, 0x55);	/* PUSH(EBP); */
			jitCompPutByte1(w.dst, 0xe8);
			j = ((UCHAR *) &func3d) - (w.dst + 4);
			jitCompPutImm32(&w, j);
 			jitCompPutByte3(w.dst, 0x83, 0xc4, 0x20);	/* ADD(ESP,32); */
			jitCompA000_loadRegCacheAll(&w); // 手抜き.
			jitCompA000_loadPRegCacheAll(&w); // 手抜き.
			cmp0reg = -1;
			break;

		case 0xfe:	/* remark */
			if (src[1] == 0x01 && src[2] == 0x00) {	// DBGINFO1
				if (level <= JITC_LV_SLOWER) {
					jitCompPutByte1(w.dst, 0xb8);	/* MOV(EAX, ?); */
					jitCompPutImm32(&w, debugInfo1);
					jitCompA0001_movEbpDispReg32(&w, 2304 + 4, 0 /* EAX */); /* MOV(debugInfo1, EAX); */
				}
			}
			if (src[1] == 0x05 && src[2] == 0x00) {	// DBGINFO0
				if (level <= JITC_LV_SLOWEST) {
					debugInfo0 = jitCompGetImm32(src + 3);
				//	jitCompPutByte1(w.dst, 0xbf);	/* MOV(EDI, ?); */
				//	jitCompPutImm32(&w, debugInfo0);
					jitCompPutByte1(w.dst, 0xb8);	/* MOV(EAX, ?); */
					jitCompPutImm32(&w, debugInfo0);
					jitCompA0001_movEbpDispReg32(&w, 2304 + 0, 0 /* EAX */); /* MOV(debugInfo0, EAX); */
				}
			}
			break;

		default:
			w.err = JITC_ERR_OPECODE;
			goto err_w;
		}
		if (w.err != 0) goto err_w;
		jitCompA0001_fixPrefix(&w);
		if (w.err != 0) goto err_w;
		src += jitCompCmdLen(src);
	}
	if (enter0 != NULL) {
		j = w.dst - (enter0 + 4);
		enter0[0] =  j        & 0xff;
		enter0[1] = (j >>  8) & 0xff;
		enter0[2] = (j >> 16) & 0xff;
		enter0[3] = (j >> 24) & 0xff;
	}
	jitCompA000_storeRegCacheAll(&w);
	jitCompA000_storePRegCacheAll(&w);
	jitCompPutByte1(w.dst, 0x61); /* POPAD(); */
	if ((flags & JITC_PHASE1) != 0)
		return w.dst - dst00;
	return 0;

err_w:
	if ((w.err & JITC_ERR_PHASE0ONLY) != 0) {
		if ((flags & JITC_PHASE1) == 0)
			w.err &= ~JITC_ERR_PHASE0ONLY;
	}
	if (w.err == (JITC_ERR_MASK & JITC_ERR_REGNUM))			errmsg = "reg-number error";
	if (w.err == (JITC_ERR_MASK & JITC_ERR_DST1))			errmsg = "dst1 error";
	if (w.err == (JITC_ERR_MASK & JITC_ERR_OPECODE))		errmsg = "opecode error";
	if (w.err == (JITC_ERR_MASK & JITC_ERR_LABELNUM))		errmsg = "label number too large";
	if (w.err == (JITC_ERR_MASK & JITC_ERR_LABELREDEF))		errmsg = "label redefine";
	if (w.err == (JITC_ERR_MASK & JITC_ERR_PREFIX))			{ errmsg = "prefix redefine"; w.dst -= 2; }
	if (w.err == (JITC_ERR_MASK & JITC_ERR_LABELNODEF))		errmsg = "label not defined";
	if (w.err == (JITC_ERR_MASK & JITC_ERR_LABELTYP))		errmsg = "label type error";
	if (w.err == (JITC_ERR_MASK & JITC_ERR_IDIOM))			errmsg = "idiom error";
	if (w.err == (JITC_ERR_MASK & JITC_ERR_PREGNUM))		errmsg = "preg-number error";
	if (w.err == (JITC_ERR_MASK & JITC_ERR_SRC1))			errmsg = "src1 error";
	if (w.err == (JITC_ERR_MASK & JITC_ERR_BADTYPE))		errmsg = "bad type code";
	if (w.err == (JITC_ERR_MASK & JITC_ERR_PREFIXFAR))		errmsg = "prefix internal error";
	if (*errmsg != '\0') {
		fprintf(stderr, "JITC: %s at %06X (debugInfo0=%d)\n    ", errmsg, src - src0, debugInfo0);
		for (i = 0; i < 16; i++)
			fprintf(stderr, "%02X ", src[i]);
		static char *table[0x30] = {
			"NOP", "LB", "LIMM", "PLIMM", "CND", "??", "??", "??",
			"LMEM", "SMEM", "PLMEM", "PSMEM", "LEA", "??", "PADD", "PDIF",
			"CP/OR", "XOR", "AND", "??", "ADD", "SUB", "MUL", "??",
			"SHL", "SAR", "DIV", "MOD", "PLMT0", "PLMT1", "PCP", "PCST",
			"CMPE", "CMPNE", "CMPL", "CMPGE", "CMPLE", "CMPG", "TSTZ", "TSTNZ",
			"PCMPE", "PCMPNE", "PCMPL", "PCMPGE", "PCMPLE", "PCMPG", "??", "EXT" };
		errmsg = "??";
		if (*src < 0x30) errmsg = table[*src];
		fprintf(stderr, "(%s)\n", errmsg);
	}
	return -1;
}

UCHAR *jitCompCallFunc(UCHAR *dst, void *func)
{
	struct JitCompWork w;
	w.dst = dst;
	int i;
	jitCompA000_storeRegCacheAll(&w);
	jitCompA000_storePRegCacheAll(&w);
	jitCompPutByte1(w.dst, 0x60);	/* PUSHAD(); */
	jitCompPutByte1(w.dst, 0x50);	/* PUSH(EAX); */	/* for 16byte-align（win32では不要なのだけど、MacOSには必要らしい） */
	jitCompPutByte1(w.dst, 0x55);	/* PUSH(EBP); */
	jitCompPutByte1(w.dst, 0xe8);	/* CALL(func); */
	int j = ((UCHAR *) func) - (w.dst + 4);
	jitCompPutImm32(&w, j);
	jitCompPutByte1(w.dst, 0x58);	/* POP(EAX); */		/* （win32では不要なのだけど、MacOSには必要らしい） */
	jitCompPutByte1(w.dst, 0x58);	/* POP(EAX); */
	jitCompPutByte1(w.dst, 0x61);	/* POPAD(); */
	jitCompA000_loadRegCacheAll(&w);
	jitCompA000_loadPRegCacheAll(&w);
	jitCompA0001_movReg32EbpDisp(&w, 0 /* EAX */, 256 + 0x30 * 32 + 0); /* MOV(EAX, [EBP+?]); */
	jitCompPutByte2(w.dst, 0xff, 0xe0);	/* JMP(EAX); */
	return w.dst;
}

void func3c(char *ebp, int opt, int r1, int p1, int lenR, int lenP, int r0, int p0)
{
	struct Regs *r = (struct Regs *) (ebp - jitCompA0001_EBP128);
	int i, *pi;
	struct Ptr *pp;
	pi = (void *) r->junkStack; r->junkStack += r1 * 4;
	for (i = 0; i < r1; i++)
		pi[i] = r->ireg[i];
	pp = (void *) r->junkStack; r->junkStack += p1 * 32;
	for (i = 0; i < p1; i++)
		pp[i] = r->preg[i];
	pp = (void *) r->junkStack; r->junkStack += 32;
	*pp = r->preg[0x30];
	pi = (void *) r->junkStack; r->junkStack += 4;
	*pi = opt << 16 | r1 << 8 | p1;
	for (i = 0; i < lenR; i++)
		r->ireg[r0 + i] = r->ireg[0x30 + i];
	for (i = 0; i < lenP; i++)
		r->preg[p0 + i] = r->preg[0x31 + i];
	return;
}

void func3d(char *ebp, int opt, int r1, int p1, int lenR, int lenP, int r0, int p0)
{
	struct Regs *r = (struct Regs *) (ebp - jitCompA0001_EBP128);
	int i;
	r->junkStack -= 4;
	r->junkStack -= 32; struct Ptr *pp = (void *) r->junkStack;
	r->preg[0x30] = *pp;
	r->junkStack -= p1 * 32; pp = (void *) r->junkStack;
	for (i = 0; i < p1; i++)
		r->preg[i] = pp[i];
	r->junkStack -= r1 * 4; int *pi = (void *) r->junkStack;
	for (i = 0; i < r1; i++)
		r->ireg[i] = pi[i];
	return;
}

void funcf4(char *ebp, int pxx, int typ, int len)
{
	struct Regs *r = (struct Regs *) (ebp - jitCompA0001_EBP128);
	int width = jitCompA000_dataWidth(r->ireg[typ]);
	void *p = r->junkStack;
	r->junkStack += width * r->ireg[len];
	r->preg[pxx].p = p;
	r->preg[pxx].p0 = p;
	r->preg[pxx].p1 = (void *) r->junkStack;
	return;
}

void funcf5(char *ebp, int pxx, int typ, int len)
{
	struct Regs *r = (struct Regs *) (ebp - jitCompA0001_EBP128);
	int width = jitCompA000_dataWidth(r->ireg[typ]);
	void *p = r->junkStack;
	r->junkStack -= width * r->ireg[len];
	return;
}

void funcf6(char *ebp, int pxx, int typ, int len)
{
	struct Regs *r = (struct Regs *) (ebp - jitCompA0001_EBP128);
	int width = jitCompA000_dataWidth(r->ireg[typ]);
	void *p = malloc(width * r->ireg[len]);
	r->preg[pxx].p = p;
	r->preg[pxx].p0 = p;
	r->preg[pxx].p1 = p + width * r->ireg[len];
	return;
}

void funcf7(char *ebp, int pxx, int typ, int len)
{
	struct Regs *r = (struct Regs *) (ebp - jitCompA0001_EBP128);
	free(r->preg[pxx].p);
	return;
}

int jitc0(UCHAR **qq, UCHAR *q1, const UCHAR *p0, const UCHAR *p1, int level, struct LabelTable *label)
{
	UCHAR *q = *qq;
	if (p0[0] != 0x05 || p0[1] != 0xe1)
		return 1;

	*q++ = 0x55; /* PUSH(EBP); */
	*q++ = 0x8b; *q++ = 0x6c; *q++ = 0x24; *q++ = 0x08; /* MOV(EBP,[ESP+8]); */

	int i;
	for (i = 0; i < JITC_MAXLABELS; i++)
		label[i].typ = 0;

	i = jitCompiler(q, q1, p0 + 2, p1, p0, label, JITC_MAXLABELS, level, 0, 0);
	if (i != 0) return 2;
	i = jitCompiler(q, q1, p0 + 2, p1, p0, label, JITC_MAXLABELS, level, 0, JITC_PHASE1+0);
	if (i < 0) return 2;
	q += i;

	*q++ = 0x5d; /* POP(EBP); */
	*q++ = 0xc3; /* RET(); */

	*qq = q;
	return 0;
}

#endif

/* OS依存部 */

#if (DRV_OSNUM == 0x0001)

#include <windows.h>
#include <setjmp.h>

#define TIMER_ID			 1
#define TIMER_INTERVAL		10

struct BLD_WORK {
	HINSTANCE hi;
	HWND hw;
	BITMAPINFO bmi;
	int tmcount1, tmcount2, flags, smp; /* bit0: 終了 */
	HANDLE mtx;
	char *winClosed;
};

struct BLD_WORK bld_work;

struct BL_WIN {
	int xsiz, ysiz, *buf;
};

struct BL_WORK {
	struct BL_WIN win;
	jmp_buf jb;
	int csiz_x, csiz_y, cx, cy, col0, col1, tabsiz, slctwin;
	int tmcount, tmcount0, mod, rand_seed;
	int *cbuf;
	unsigned char *ftyp;
	unsigned char **fptn;
	int *ccol, *cbak;
	int *kbuf, kbuf_rp, kbuf_wp, kbuf_c;
};

struct BL_WORK bl_work;

#define BL_SIZ_KBUF			8192

#define BL_WAITKEYF		0x00000001
#define BL_WAITKEYNF	0x00000002
#define BL_WAITKEY		0x00000003
#define BL_GETKEY		0x00000004
#define BL_CLEARREP		0x00000008
#define BL_DELFFF		0x00000010

#define	BL_KEYMODE		0x00000000	// 作りかけ, make/remake/breakが見えるかどうか

#define w	bl_work
#define dw	bld_work

void bld_openWin(int x, int y, char *winClosed);
void bld_flshWin(int sx, int sy, int x0, int y0);
LRESULT CALLBACK WndProc(HWND hw, unsigned int msg, WPARAM wp, LPARAM lp);
void bl_cls();
int bl_iCol(int i);
void bl_readyWin(int n);

static HANDLE threadhandle;

int main(int argc, const UCHAR **argv)
{
	return OsecpuMain(argc, argv);
}

void *mallocRWE(int bytes)
{
	void *p = malloc(bytes);
	DWORD dmy;
	VirtualProtect(p, bytes, PAGE_EXECUTE_READWRITE, &dmy);
	return p;
}

static int winthread(void *dmy)
{
	WNDCLASSEX wc;
	RECT r;
	unsigned char *p, *p0, *p00;
	int i, x, y;
	MSG msg;

	x = dw.bmi.bmiHeader.biWidth;
	y = - dw.bmi.bmiHeader.biHeight;

	wc.cbSize = sizeof (WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = dw.hi;
	wc.hIcon = (HICON) LoadImage(NULL, MAKEINTRESOURCE(IDI_APPLICATION),
		IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = (HCURSOR)LoadImage(NULL, MAKEINTRESOURCE(IDC_ARROW),
		IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wc.hbrBackground = (HBRUSH) COLOR_APPWORKSPACE;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "WinClass";
	if (RegisterClassEx(&wc) == 0)
		return 1;
	r.left = 0;
	r.top = 0;
	r.right = x;
	r.bottom = y;
	AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
	x = r.right - r.left;
	y = r.bottom - r.top;

#if 0
	static unsigned char t[32];
	p00 = p0 = p = GetCommandLineA();
	if (*p == 0x22) {
		p00 = p0 = ++p;
		while (*p != '\0' && *p != 0x22) {
			if (*p == '\\')
				p0 = p + 1;
			p++;
		}
	} else {
		while (*p > ' ') {
			if (*p == '\\')
				p0 = p + 1;
			p++;
		}
	}
	if (p - p0 > 4 && p[-4] == '.' && p[-3] == 'e' && p[-2] == 'x' && p[-1] == 'e')
		p -= 4;
	for (i = 0; i < 32 - 1; i++) {
		if (p <= &p0[i])
			break;
		t[i] = p0[i];
	}
	t[i] = '\0';
#endif
	char *t = "osecpu";

	dw.hw = CreateWindowA(wc.lpszClassName, t, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, x, y, NULL, NULL,	dw.hi, NULL);
	if (dw.hw == NULL)
		return 1;
	ShowWindow(dw.hw, SW_SHOW);
	UpdateWindow(dw.hw);
	SetTimer(dw.hw, TIMER_ID,     TIMER_INTERVAL,       NULL);
	SetTimer(dw.hw, TIMER_ID + 1, TIMER_INTERVAL * 10,  NULL);
	SetTimer(dw.hw, TIMER_ID + 2, TIMER_INTERVAL * 100, NULL);
	dw.flags |= 2 | 4;

	for (;;) {
		i = GetMessage(&msg, NULL, 0, 0);
		if (i == 0 || i == -1)	/* エラーもしくは終了メッセージ */
			break;
		/* そのほかはとりあえずデフォルト処理で */
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
//	PostQuitMessage(0);
	dw.flags |= 1; /* 終了, bld_waitNF()が見つける */
	if (dw.winClosed != NULL)
		*dw.winClosed = 1;
	return 0;
}

void bld_openWin(int sx, int sy, char *winClosed)
{
	static int i;

	dw.bmi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
	dw.bmi.bmiHeader.biWidth = sx;
	dw.bmi.bmiHeader.biHeight = - sy;
	dw.bmi.bmiHeader.biPlanes = 1;
	dw.bmi.bmiHeader.biBitCount = 32;
	dw.bmi.bmiHeader.biCompression = BI_RGB;
	dw.winClosed = winClosed;

	threadhandle = CreateThread(NULL, 0, (void *) &winthread, NULL, 0, (void *) &i);

	return;
}

void drv_flshWin(int sx, int sy, int x0, int y0)
{
	InvalidateRect(dw.hw, NULL, FALSE);
	UpdateWindow(dw.hw);
	return;
}

LRESULT CALLBACK WndProc(HWND hw, unsigned int msg, WPARAM wp, LPARAM lp)
{
	int i;
	if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(dw.hw, &ps);
		SetDIBitsToDevice(hdc, 0, 0, w.win.xsiz, w.win.ysiz,
			0, 0, 0, w.win.ysiz, w.win.buf, &dw.bmi, DIB_RGB_COLORS);
		EndPaint(dw.hw, &ps);
	}
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}
	if (msg == WM_TIMER && wp == TIMER_ID) {
		w.tmcount += TIMER_INTERVAL;
		return 0;
	}
	if (msg == WM_TIMER && wp == TIMER_ID + 1) {
		dw.tmcount1 += TIMER_INTERVAL * 10;
		w.tmcount = dw.tmcount1;
		return 0;
	}
	if (msg == WM_TIMER && wp == TIMER_ID + 2) {
		dw.tmcount2 += TIMER_INTERVAL * 100;
		w.tmcount = dw.tmcount1 = dw.tmcount2;
		return 0;
	}
	if (msg == WM_KEYDOWN) {
		i = -1;
#if 0
		int s_sht = GetKeyState(VK_SHIFT);
		int s_ctl = GetKeyState(VK_CONTROL);
		int s_cap = GetKeyState(VK_CAPITAL);
		int s_num = GetKeyState(VK_NUMLOCK);
		if ('A' <= wp && wp <= 'Z') {
			i = wp;
			if (((s_sht < 0) ^ (s_cap & 1)) == 0)
				i += 'a' - 'A';
		}
		if (wp == VK_SPACE)		i = ' ';
#endif
		if (wp == VK_RETURN)	i = KEY_ENTER;
		if (wp == VK_ESCAPE)	i = KEY_ESC;
		if (wp == VK_BACK)		i = KEY_BACKSPACE;
		if (wp == VK_TAB)		i = KEY_TAB;
		if (wp == VK_PRIOR)		i = KEY_PAGEUP;
		if (wp == VK_NEXT)		i = KEY_PAGEDWN;
		if (wp == VK_END)		i = KEY_END;
		if (wp == VK_HOME)		i = KEY_HOME;
		if (wp == VK_LEFT)		i = KEY_LEFT;
		if (wp == VK_RIGHT)		i = KEY_RIGHT;
		if (wp == VK_UP)		i = KEY_UP;
		if (wp == VK_DOWN)		i = KEY_DOWN;
		if (wp == VK_INSERT)	i = KEY_INS;
		if (wp == VK_DELETE)	i = KEY_DEL;
		if (i != -1) {
			putKeybuf(i);
//			bl_putKeyB(1, &i);
			return 0;
		}
	}
	if (msg == WM_KEYUP) {
		i = 0xfff;
//		bl_putKeyB(1, &i);
	}
	if (msg == WM_CHAR) {
		i = 0;
		if (' ' <= wp && wp <= 0x7e) {
			i = wp;
			putKeybuf(i);
//			bl_putKeyB(1, &i);
			return 0;
		}
	}
	return DefWindowProc(hw, msg, wp, lp);
}

void drv_openWin(int sx, int sy, UCHAR *buf, char *winClosed)
{
	int i, x, y;
	if (sx <= 0 || sy <= 0) return;
	if (sx < 160) return;
	w.win.buf = (int *) buf;
	w.win.xsiz = sx;
	w.win.ysiz = sy;
	bld_openWin(sx, sy, winClosed);
	return;
}

void drv_sleep(int msec)
{
	Sleep(msec);
//	MsgWaitForMultipleObjects(1, &threadhandle, FALSE, msec, QS_ALLINPUT);
	/* 勉強不足でまだ書き方が分かりません! */
	return;
}

#endif

#if (DRV_OSNUM == 0x0002)

// by Liva, 2013.05.29-

#include <mach/mach.h>
#include <Cocoa/Cocoa.h>

void *mallocRWE(int bytes)
{
	void *p = malloc(bytes);
	vm_protect(mach_task_self(), (vm_address_t) p, bytes, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
	return p;
}

NSApplication* app;

@interface OSECPUView : NSView {
  UCHAR *_buf;
  int _sx;
  int _sy;
	CGContextRef _context;
}

- (id)initWithFrame:(NSRect)frameRect
  buf : (UCHAR *) buf
  sx : (int) sx
  sy : (int) sy;

- (void)drawRect:(NSRect)rect;
@end

@implementation OSECPUView
- (id)initWithFrame:(NSRect)frameRect
  buf : (UCHAR *) buf
  sx : (int) sx
  sy : (int) sy
{
  self = [super initWithFrame:frameRect];
  if (self) {
    _buf = buf;
    _sx = sx;
    _sy = sy;
  }
  return self;
}
 
- (void)drawRect:(NSRect)rect {
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  _context = CGBitmapContextCreate (_buf, _sx, _sy, 8, 4 * _sx, colorSpace, (kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst));
  CGImageRef image = CGBitmapContextCreateImage(_context);
  CGContextRef currentContext = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
  CGContextDrawImage(currentContext, NSRectToCGRect(rect), image);
}
 
@end

@interface Main : NSObject<NSWindowDelegate> {
  int argc;
  const UCHAR **argv;
  char *winClosed;
  OSECPUView *_view;
}

- (void)runApp;
- (void)createThread : (int)_argc
args : (const UCHAR **)_argv;
- (BOOL)windowShouldClose:(id)sender;
- (void)openWin : (UCHAR *)buf
sx : (int) sx
sy : (int) sy
winClosed : (char *)_winClosed;
- (void)flushWin : (NSRect) rect;
@end

@implementation Main
- (void)runApp
{
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  osecpuMain(argc,argv);
  [NSApp terminate:self];
	[pool release];
}

- (void)createThread : (int)_argc
  args : (const UCHAR **)_argv
{
  argc = _argc;
  argv = _argv;
  NSThread *thread = [[[NSThread alloc] initWithTarget:self selector:@selector(runApp) object:nil] autorelease];
  [thread start];
}

- (BOOL)windowShouldClose:(id)sender
{
  *winClosed = 1;
  return YES;
}

- (void)openWin : (UCHAR *)buf
  sx : (int) sx
  sy : (int) sy
  winClosed : (char *)_winClosed
{

	NSWindow* window = [[NSWindow alloc] initWithContentRect: NSMakeRect(0, 0, sx, sy) styleMask: NSTitledWindowMask | NSMiniaturizableWindowMask | NSClosableWindowMask backing: NSBackingStoreBuffered defer: NO];
	[window setTitle: @"osecpu"];
	[window center];
	[window makeKeyAndOrderFront:nil];
	[window setReleasedWhenClosed:YES];
	window.delegate = self;
	winClosed = _winClosed;

	_view = [[OSECPUView alloc] initWithFrame:NSMakeRect(0,0,sx,sy) buf:buf sx:sx sy:sy];
	[window.contentView addSubview:_view];
}

- (void)flushWin : (NSRect) rect
  {
    [_view drawRect:rect];
  }

@end

id objc_main;

int main(int argc, const UCHAR **argv)
{
  objc_main = [[Main alloc] init];
  
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  app = [[NSApplication alloc] init];
  [objc_main createThread:argc args:argv];
  [app run];
	[pool release];
  return 0;
}

void drv_openWin(int sx, int sy, UCHAR *buf, char *winClosed)
{
  [objc_main openWin:buf sx:sx sy:sy winClosed:winClosed];
}

void drv_flshWin(int sx, int sy, int x0, int y0)
{
  [objc_main flushWin:NSMakeRect(x0,y0,sx,sy)];
}

void drv_sleep(int msec)
{
  [NSThread sleepForTimeInterval:0.001*msec];
   return;
}

#endif

