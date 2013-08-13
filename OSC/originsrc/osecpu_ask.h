#define junkApi_openWin(xsiz, ysiz)					DB(0xfe,0x05,0x01); DDBE(0x0010); R30=0xff40; R31=xsiz; R32=ysiz; PCALL(P28)
#define junkApi_flushWin(xsiz, ysiz, x0, y0)		DB(0xfe,0x05,0x01); DDBE(0x0011); R30=0xff41; R31=xsiz; R32=ysiz; R33=x0; R34=y0; PCALL(P28)
#define junkApi_sleep(opt, msec)					DB(0xfe,0x05,0x01); DDBE(0x0009); R30=0xff42; R31=opt; R32=msec; PCALL(P28)
#define junkApi_inkey(_i, mod)						DB(0xfe,0x05,0x01); DDBE(0x000d); R30=0xff43; R31=mod; PCALL(P28); _i=R30 
#define junkApi_drawPoint(mod, x, y, c)				DB(0xfe,0x05,0x01); DDBE(0x0002); R30=0xff44; R31=mod; R32=x; R33=y; R34=c; PCALL(P28) 
#define junkApi_drawLine(mod, x0, y0, x1, y1, c)	DB(0xfe,0x05,0x01); DDBE(0x0003); R30=0xff45; R31=mod; R32=x0; R33=y0; R34=x1; R35=y1; R36=c; PCALL(P28) 
#define junkApi_fillRect(mod, xsiz, ysiz, x0, y0, c)	DB(0xfe,0x05,0x01); DDBE(0x0004); R30=0xff46; R31=mod;      R32=xsiz; R33=ysiz; R34=x0; R35=y0; R36=c; PCALL(P28) 
#define junkApi_drawRect(mod, xsiz, ysiz, x0, y0, c)	DB(0xfe,0x05,0x01); DDBE(0x0004); R30=0xff46; R31=mod|0x20; R32=xsiz; R33=ysiz; R34=x0; R35=y0; R36=c; PCALL(P28) 
#define junkApi_fillOval(mod, xsiz, ysiz, x0, y0, c)	DB(0xfe,0x05,0x01); DDBE(0x0005); R30=0xff47; R31=mod;      R32=xsiz; R33=ysiz; R34=x0; R35=y0; R36=c; PCALL(P28) 
#define junkApi_drawOval(mod, xsiz, ysiz, x0, y0, c)	DB(0xfe,0x05,0x01); DDBE(0x0005); R30=0xff47; R31=mod|0x20; R32=xsiz; R33=ysiz; R34=x0; R35=y0; R36=c; PCALL(P28) 
#define junkApi_drawString0(mod, xsiz, ysiz, x0, y0, c, s)	DB(0xfe,0x05,0x01); DDBE(0x0006); R30=0xff48; R31=mod; R32=xsiz; R33=ysiz; R34=x0; R35=y0; R36=c; DB(0xff,0x00,0x00); DB(s,0x00); PCALL(P28) 
#define junkApi_drawString3(mod, xsiz, ysiz, x0, y0, c, s)	DB(0xfe,0x05,0x01); DDBE(0x0006); R30=0xff48; R31=mod; R32=xsiz; R33=ysiz; R34=x0; R35=y0; R36=c; DB(0xff,0x00,0x03); DB(s,0x00); PCALL(P28) 
#define junkApi_drawString5(mod, xsiz, ysiz, x0, y0, c, s)	DB(0xfe,0x05,0x01); DDBE(0x0006); R30=0xff48; R31=mod; R32=xsiz; R33=ysiz; R34=x0; R35=y0; R36=c; DB(0xff,0x00,0x05); DB(s,0x00); PCALL(P28) 
#define junkApi_drawString6(mod, xsiz, ysiz, x0, y0, c, s)	DB(0xfe,0x05,0x01); DDBE(0x0006); R30=0xff48; R31=mod; R32=xsiz; R33=ysiz; R34=x0; R35=y0; R36=c; DB(0xff,0x00,0x06); DB(s,0x00); PCALL(P28) 
#define junkApi_drawString(mod, xsiz, ysiz, x0, y0, c, s)	junkApi_drawString6(mod, xsiz, ysiz, x0, y0, c, s)
#define junkApi_rand(_r)							DB(0xfe,0x05,0x01); DDBE(0x0013); R30=0xff49; PCALL(P28); _r=R30

#define junkApi_fopenRead(_filesize, _p, arg)		DB(0xfe,0x05,0x01); DDBE(0x0740); R30=0xff01; R31=arg; PCALL(P28); _filesize=R30; _p=P31
#define junkApi_fopenWrite(arg, filesize, p)		DB(0xfe,0x05,0x01); DDBE(0x0741); R30=0xff02; R31=arg; R32=filesize; P31=p; PCALL(P28)
#define junkApi_allocBuf(_p)						DB(0xfe,0x05,0x01); DDBE(0x0742); R30=0xff03; PCALL(P28); _p=P31
#define junkApi_writeStdout(len, p)					DB(0xfe,0x05,0x01); DDBE(0x0743); R30=0xff05; R31=len; P31=p; PCALL(P28)
#define jnukApi_exit(i)								DB(0xfe,0x05,0x01); DDBE(0x0008); R30=0xff06; R31=i; PCALL(P28)
#define junkApi_putConstString0(s)					DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x00,0x00); DB(s,0x00); PCALL(P28)
#define junkApi_putConstString3(s)					DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x00,0x03); DB(s,0x00); PCALL(P28)
#define junkApi_putConstString5(s)					DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x00,0x05); DB(s,0x00); PCALL(P28)
#define junkApi_putConstString6(s)					DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x00,0x06); DB(s,0x00); PCALL(P28)
#define junkApi_putcharRxx(reg)						DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x01,0x04); DDBE(-16-(reg&0x3f),0); PCALL(P28)
#define junkApi_putchar(c)							R38=c;                          DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x01,0x04); DDBE(-16-0x38,0); PCALL(P28)
#define junkApi_putchar2(c0, c1)					R38=c0; R39=c1;                 DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x01,0x04); DDBE(-16-0x38,-16-0x39,0); PCALL(P28)
#define junkApi_putchar3(c0, c1, c2)				R38=c0; R39=c1; R3A=c2;         DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x01,0x04); DDBE(-16-0x38,-16-0x39,-16-0x3a,0); PCALL(P28)
#define junkApi_putchar4(c0, c1, c2, c3)			R38=c0; R39=c1; R3A=c2; R3B=c3; DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x01,0x04); DDBE(-16-0x38,-16-0x39,-16-0x3a,-16-0x3b,0); PCALL(P28)
#define junkApi_putConstString(s)					junkApi_putConstString6(s)
#define junkApi_malloc(_p, typ, len)				R38=typ; R39=len; DB(0x32,_p&0x3f,0x38,0x39)

#define	LMEM0(reg, typ, preg)	LMEM(reg, typ, preg, 0)
#define SMEM0(reg, typ, preg)	SMEM(reg, typ, preg, 0)

#define	LMEM0PP(reg, typ, preg)	LMEM(reg, typ, preg, 0); PADDI(preg, typ, preg, 1)
#define SMEM0PP(reg, typ, preg)	SMEM(reg, typ, preg, 0); PADDI(preg, typ, preg, 1)

#define PALMEM(reg0, typ32, preg0, reg1, mclen)	PADD(P3F, typ32, preg0, reg1); LMEM(reg0, typ32, P3F, mclen)
#define PASMEM(reg0, typ32, preg0, reg1, mclen)	PADD(P3F, typ32, preg0, reg1); SMEM(reg0, typ32, P3F, mclen)
#define PALMEM0(reg0, typ32, preg0, reg1)		PALMEM(reg0, typ32, preg0, reg1, 0)
#define PASMEM0(reg0, typ32, preg0, reg1)		PASMEM(reg0, typ32, preg0, reg1, 0)

#define beginFunc(label)							ELB(label); DB(0x3c, 0x00, 0x20, 0x20, 0x00, 0x00, 0x00)
#define endFunc()									DB(0x3d, 0x00, 0x20, 0x20, 0x00, 0x00, 0x00); RET()

#define int32s										SInt32
#define do											for (;0;)

#define T_SINT8     0x02 // 8bit�̕����t��, ������ signed char.
#define T_UINT8     0x03
#define T_SINT16    0x04 // 16bit�̕����t��, ������ short.
#define T_UINT16    0x05
#define T_SINT32    0x06
#define T_UINT32    0x07
#define T_SINT4     0x08
#define T_UINT4     0x09
#define T_SINT2     0x0a
#define T_UINT2     0x0b
#define T_SINT1     0x0c // ����ł���̂�0��-1�̂�.
#define T_UINT1     0x0d
#define T_SINT12    0x0e
#define T_UINT12    0x0f
#define T_SINT20    0x10
#define T_UINT20    0x11
#define T_SINT24    0x12
#define T_UINT24    0x13
#define T_SINT28    0x14
#define T_UINT28    0x15

%include "osecpu_asm.h"
OSECPU_HEADER();
