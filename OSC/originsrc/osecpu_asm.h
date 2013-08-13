#if (defined(NODBGINFO0))
	#define	DI()						/* none */
#else
	#define	DI()						DB(0xfe,0x05,0x00); lbstk6(__FILE__,__LINE__)
#endif

#define	_								DI();
#define	LB0(imm)						LB(0, imm); _
#define	GLB(imm)						LB(1, imm); DBGINFO1(); 
#define	ELB(imm)						LB(1, imm); DBGINFO1(); 

#define PALMEM(reg0, typ32, preg0, reg1, mclen)	PADD(P3F, typ32, preg0, reg1); LMEM(reg0, typ32, P3F, mclen)
#define PASMEM(reg0, typ32, preg0, reg1, mclen)	PADD(P3F, typ32, preg0, reg1); SMEM(reg0, typ32, P3F, mclen)

#define	CMPJE(reg1, reg2, label)		CMPE(  R3F, reg1, reg2); CND(R3F); JMP(label)
#define	CMPIJE(reg1, imm, label)		CMPEI( R3F, reg1, imm);  CND(R3F); JMP(label)
#define	CMPJNE(reg1, reg2, label)		CMPNE( R3F, reg1, reg2); CND(R3F); JMP(label)
#define	CMPIJNE(reg1, imm, label)		CMPNEI(R3F, reg1, imm);  CND(R3F); JMP(label)
#define	CMPIJLE(reg1, imm, label)		CMPLEI(R3F, reg1, imm);  CND(R3F); JMP(label)
#define	PCMPJNE(reg1, reg2, label)		PCMPNE(R3F, reg1, reg2); CND(R3F); JMP(label)
#define	PCMPJE(reg1, reg2, label)		PCMPE (R3F, reg1, reg2); CND(R3F); JMP(label)

#define LOOP()   	  					lbstk2(0,2); LB0(lbstk1(0,0))
#define CONTINUE						lbstk1(0,0)
#define BREAK							lbstk1(0,1)
#define ENDLOOP0()						lbstk3(0)					/* BREAK���g��Ȃ��ꍇ�p */
#define ENDLOOP()						LB0(lbstk1(0,1)); lbstk3(0)	/* BREAK���g���ꍇ�p */
#define SWITCH()						lbstk2(0,2)
#define ENDSWITCH()						LB0(lbstk1(0,1)); lbstk3(0)	/* BREAK���g���ꍇ�p */
#define	GLOBALLABELS(n)					lbstk0(n)
#define	LOCALLABELS(n)					lbstk4(n)
#define	LOCAL(i)						lbstk5(i)
#define	CALL(label)						lbstk2(0,1); PLIMM(P30, lbstk1(0,0)); JMP(label); ELB(lbstk1(0,0)); lbstk3(0)
#define	PCALL(preg)						lbstk2(0,1); PLIMM(P30, lbstk1(0,0)); PJMP(preg); ELB(lbstk1(0,0)); lbstk3(0)
#define RET()							PJMP(P30)
#define	IF()							lbstk2(1,3)
#define	THEN()							LB0(lbstk1(1,0))
#define	THEN0()							/* TOTHEN���g��Ȃ��ꍇ�p�A�ȗ��\ */
#define	ELSE()							LB0(lbstk1(1,1))
#define	ELSE0()							/* TOELSE���g��Ȃ��ꍇ�p�A�ȗ��\ */
#define	ENDIF()							LB0(lbstk1(1,2)); lbstk3(1)	/* TOENDIF���g���ꍇ�p */
#define	ENDIF0()						lbstk3(1)					/* TOENDIF���g��Ȃ��ꍇ�p */
#define	TOTHEN							lbstk1(1,0)
#define	TOELSE							lbstk1(1,1)
#define	TOENDIF							lbstk1(1,2)

#define	DAT_SA(label, typ32, length)		LB(1, label); DB(0x34); DDBE(typ32, length)	/* simple-array */

#define PADDINGPREFIX()					DB(0xfe,0x01,0xff)
