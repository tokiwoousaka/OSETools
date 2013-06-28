#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char UCHAR;

#define	BUFSIZE		1024 * 1024
#define	JBUFSIZE	64 * 1024

#define MAXLABEL	4096
#define MAXIDENTS	4096

struct Element {
	UCHAR typ, subTyp[3];
		// [0]:�D�揇��.
		// [1]:�E�����t���O.
		// [2]:�e��t���O.
	union {
		const void *pValue;
		int iValue;
	};
	int subValue;
};

struct VarName {
	int len;
	const UCHAR *p;
	struct Element ele;
};

struct Work {
	int argc, flags, sp, isiz;
	const UCHAR **argv, *argIn, *argOut;
	struct Element ele0[10000], ele1[10000];
	UCHAR ibuf[BUFSIZE];
	UCHAR obuf[BUFSIZE];
	UCHAR jbuf[JBUFSIZE];
};

const UCHAR *searchArg(struct Work *w, const UCHAR *tag);
int putBuf(const UCHAR *filename, const UCHAR *p0, const UCHAR *p1);
int getConstInt(const UCHAR **pp, char *errflg, int flags);
int getConstInt1(const UCHAR **pp);
int aska(struct Work *w);
int prepro(struct Work *w);
int lbstk(struct Work *w);
int db2bin(struct Work *w);
int disasm(struct Work *w);
int appack(struct Work *w);
int maklib(struct Work *w);

#define PUT_USAGE				-999
#define	FLAGS_PUT_MANY_INFO		1
#define FLAGS_NO_DEL_ARGOUT		2

int main(int argc, const UCHAR **argv)
{
	struct Work *w = malloc(sizeof (struct Work));
	w->argc = argc;
	w->argv = argv;
	w->argIn  = searchArg(w, "in:" );
	w->argOut = searchArg(w, "out:");
	w->flags = 0;
	int r;
	const UCHAR *cs = searchArg(w, "flags:");
	if (cs != NULL) {
		w->flags = getConstInt1(&cs);
	}
	if (w->argOut != NULL && (w->flags & FLAGS_NO_DEL_ARGOUT) == 0) {
		remove(w->argOut);
	}
	if (w->argIn != NULL) {
		FILE *fp = fopen(w->argIn, "rb");
		if (fp == NULL) {
			fprintf(stderr, "fopen error: '%s'\n", w->argIn);
			goto fin;
		}
		r = fread(w->ibuf, 1, BUFSIZE - 2, fp);
		fclose(fp);
		if (r >= BUFSIZE - 2) {
			fprintf(stderr, "too large infile: '%s'\n", w->argIn);
			goto fin;
		}
		w->ibuf[r] = '\0';
		w->isiz = r;
	}

	cs = searchArg(w, "tool:");
	r = PUT_USAGE;
	if (cs != NULL && w->argIn != NULL) {
		if (strcmp(cs, "aska")   == 0) { r = aska(w);   }
		if (strcmp(cs, "prepro") == 0) { r = prepro(w); }
		if (strcmp(cs, "lbstk")  == 0) { r = lbstk(w);  }
		if (strcmp(cs, "db2bin") == 0) { r = db2bin(w); }
		if (strcmp(cs, "disasm") == 0) { r = disasm(w); }
		if (strcmp(cs, "appack") == 0) { r = appack(w); }
		if (strcmp(cs, "maklib") == 0) { r = maklib(w); }
	}
	if (cs != NULL && strcmp(cs, "getint") == 0) {
		cs = searchArg(w, "exp:");
		if (cs != NULL) {
			char errflg = 0;
			r = getConstInt(&cs, &errflg, w->flags);
			printf("getConstInt=%d, errflg=%d\n", r, errflg);
			r = 0;
			cs = NULL;
		}
	} 
	if (cs != NULL && strcmp(cs, "osastr") == 0) {
		cs = searchArg(w, "str:");
		if (cs != NULL) {
			r = strlen(cs);
			while (r > 0) {
				if (r >= 8) {
					printf("%02X ",  cs[0] | ((cs[7] >> 6) & 1) << 7);
					printf("%02X ",  cs[1] | ((cs[7] >> 5) & 1) << 7);
					printf("%02X ",  cs[2] | ((cs[7] >> 4) & 1) << 7);
					printf("%02X ",  cs[3] | ((cs[7] >> 3) & 1) << 7);
					printf("%02X ",  cs[4] | ((cs[7] >> 2) & 1) << 7);
					printf("%02X ",  cs[5] | ((cs[7] >> 1) & 1) << 7);
					printf("%02X\n", cs[6] | ( cs[7]       & 1) << 7);
					r -= 8;
					cs += 8;
					continue;
				}
				printf("%02X ", *cs);
				cs++;
				r--;
			}
			r = 0;
			cs = NULL;
		}
	}
	if (cs != NULL && strcmp(cs, "binstr") == 0) {
		cs = searchArg(w, "str:");
		if (cs != NULL) {
			r = strlen(cs);
			while (r > 0) {
				printf("%d%d%d%d%d%d%d%d ", (*cs >> 7) & 1, (*cs >> 6) & 1,
					(*cs >> 5) & 1, (*cs >> 4) & 1, (*cs >> 3) & 1, (*cs >> 2) & 1, (*cs >> 1) & 1, *cs & 1);
				cs++;
				r--;
			}
			r = 0;
			cs = NULL;
			putchar('\n');
		}
	}


	if (r == PUT_USAGE) {
		puts("usage>osectols tool:aska   in:file [out:file] [flags:#]\n"
			 "      osectols tool:prepro in:file [out:file] [flags:#]\n"
			 "      osectols tool:lbstk  in:file [out:file] [flags:#] [lst:debuginfo]\n"
			 "      osectols tool:db2bin in:file [out:file] [flags:#]\n"
			 "      osectols tool:disasm in:file [out:file] [flags:#]\n"
			 "      osectols tool:appack in:file [out:file] [flags:#]\n"
			 "      osectols tool:maklib in:file [out:file] [flags:#]\n"
			 "      osectols tool:getint exp:expression [flags:#]\n"
			 "      osectols tool:osastr str:string\n"
			 "      osectols tool:binstr str:string\n"
			 "  aska   ver.0.16\n"
			 "  prepro ver.0.01\n"
			 "  lbstk  ver.0.03\n"
			 "  db2bin ver.0.10\n"
			 "  disasm ver.0.02\n"
			 "  appack ver.0.08\n"
			 "  maklib ver.0.00\n"
			 "  getint ver.0.05\n"
			 "  osastr ver.0.00\n"
			 "  binstr ver.0.00"
		);
		r = 1;
	}
fin:
	return r;
}

int strncmp1(const UCHAR *s, const UCHAR *t)
{
	int r = 0;
	while (*s == *t) {
		s++;
		t++;
	}
	if (*t != '\0')
		r = *s - *t;
	return r;
}

UCHAR *strcpy1(UCHAR *q, const UCHAR *p)
{
	while (*p != '\0')
		*q++ = *p++;
	return q;
}

const UCHAR *searchArg(struct Work *w, const UCHAR *tag)
{
	int i, l;
	const UCHAR *r = NULL;
	for (i = 1; i < w->argc; i++) {
		if (strncmp1(w->argv[i], tag) == 0) {
			r = w->argv[i] + strlen(tag);
			break;
		}
	}
	return r;
}

int putBuf(const UCHAR *filename, const UCHAR *p0, const UCHAR *p1)
{
	int r = 0;
	if (filename != NULL && p1 != NULL) {
		FILE *fp = fopen(filename, "wb");
		if (fp == NULL) {
			fprintf(stderr, "fopen error: '%s'\n", filename);
			r = 1;
		} else {
			fwrite(p0, 1, p1 - p0, fp);
			fclose(fp);
		}
	}
	return r;
}

int hexChar(UCHAR c)
{
	int r = -1;
	if ('0' <= c && c <= '9') r = c - '0';
	if ('A' <= c && c <= 'F') r = c - ('A' - 10);
	if ('a' <= c && c <= 'f') r = c - ('a' - 10);
	return r;
}

int isSymbolChar(UCHAR c)
{
	int r = 0;
	if (c == '!') r = 1;
	if ('%' <= c && c <= '&') r = 1;
	if (0x28 <= c && c <= 0x2f) r = 1;
	if (0x3a <= c && c <= 0x3f) r = 1;
	if (0x5b <= c && c <= 0x5e) r = 1;
	if (0x7b <= c && c <= 0x7e) r = 1;
	// @��_�͋L���ł͂Ȃ�.
	return r;
}

#define ELEMENT_TERMINATE		0
#define ELEMENT_ERROR			1
#define ELEMENT_REG				2
#define ELEMENT_PREG			3
#define ELEMENT_CONST			4
	#define	ELEMENT_INT				1
#define ELEMENT_PLUS			8
#define ELEMENT_PLUS1			9
#define ELEMENT_PLUS2			10
#define ELEMENT_MINUS			12
#define ELEMENT_MINUS1			13
#define ELEMENT_MINUS2			14
#define ELEMENT_PPLUS			16
#define ELEMENT_PPLUS0			17
#define ELEMENT_PPLUS1			18
#define ELEMENT_MMINUS			20
#define ELEMENT_MMINS0			21
#define ELEMENT_MMINS1			22
#define ELEMENT_PRODCT			24
#define ELEMENT_PRDCT1			25
#define ELEMENT_PRDCT2			26
#define ELEMENT_COMMA			28
#define ELEMENT_COMMA0			29	// �����Z�p���[�^.
#define ELEMENT_COMMA1			30	// ���Z�q.
#define ELEMENT_PRNTH0			32	// ������.
#define ELEMENT_PRNTH1			33
#define ELEMENT_OR				34
#define ELEMENT_OROR			35
#define ELEMENT_AND				36
#define ELEMENT_ANDAND			37
#define ELEMENT_XOR				38
#define ELEMENT_SHL				40
#define ELEMENT_SAR				41
#define ELEMENT_SLASH			42
#define	ELEMENT_PERCNT			43
#define	ELEMENT_EQUAL			44
#define	ELEMENT_OREQUL			45
#define	ELEMENT_XOREQL			46
#define	ELEMENT_ANDEQL			47
#define	ELEMENT_PLUSEQ			48
#define	ELEMENT_MINUSE			49
#define	ELEMENT_PRDCTE			50
#define	ELEMENT_SHLEQL			52
#define	ELEMENT_SAREQL			53
#define ELEMENT_SLASHE			54
#define ELEMENT_PRCNTE			55
#define ELEMENT_SEMCLN			56
#define ELEMENT_COLON			57
#define ELEMENT_BRACE0			58
#define ELEMENT_BRACE1			59
#define	ELEMENT_EEQUAL			60
#define	ELEMENT_NOTEQL			61
#define	ELEMENT_LESS			62
#define	ELEMENT_GRTREQ			63
#define	ELEMENT_LESSEQ			64
#define	ELEMENT_GREATR			65
#define	ELEMENT_NOT				66
#define	ELEMENT_TILDE			67
#define	ELEMENT_DOT				68
#define	ELEMENT_ARROW			69
#define ELEMENT_BRCKT0			70
#define ELEMENT_BRCKT1			71

#define ELEMENT_IDENT			96
#define ELEMENT_FOR				97
#define ELEMENT_IF				98
#define ELEMENT_ELSE			99
#define ELEMENT_BREAK			100
#define ELEMENT_CONTIN			101
#define ELEMENT_GOTO			102
#define ELEMENT_INT32S			104
#define ELEMENT_VDPTR			105

const UCHAR *stringToElements(struct Element *q, const UCHAR *p, int mode0)
// mode0: 1:�R���}�̓R���}���Z�q�Ƃ��Ă͉��߂��Ȃ��A���̋�؂�Ƃ݂Ȃ�.
// �ǂ��ł�߂邩:
//   for�Ŏn�܂��Ă���ꍇ�́Anest����;�ł��~�܂�Ȃ�. �� ����~�܂�.
//   {�Ŏ~�܂�. {���܂�. �� ����܂܂Ȃ�.
//   for�Ŏn�܂��ĂȂ���΁A;�ł��~�܂�. ;���܂�. �� ����܂܂Ȃ�.
//	 }�Ŏ~�܂�. }���܂�. �� ����܂܂Ȃ�.
//   �J���}��؂�₩�����̕��߂��ɂ���~�ł́A,��)���܂܂��ɏI���B
{
	int i, j, k, l, nest = 0, stack[100];
	stack[0] = mode0;
	struct Element *q0 = q;
	q->typ = ELEMENT_TERMINATE; // �Ƃ肠����for�ȊO�����Ă���.
	for (;;) {
		if (q - 1 > q0 && q[-2].typ == ELEMENT_MINUS && q[-1].typ == ELEMENT_CONST) {
			if ((q - 2 == q0) || (q - 2 > q0 && q[-3].typ != ELEMENT_REG && q[-3].typ != ELEMENT_PREG && q[-3].typ != ELEMENT_CONST)) {
				q[-2] = q[-1];
				q[-2].iValue *= -1;
				q--;
			}
		}
		if (*p == '\0' || *p == ';' || *p == '{' || *p == '}') break;
		if (*p == ',' && nest == 0 && (stack[0] & 1) != 0) break;
		if (*p == ')' && nest <= 0) break;
		if (*p == ' ' || *p == '\t') {
			p++;
			continue;
		}
		if (p[0] == '0' && p[1] == 'x') {
			i = 0;
			p += 2;
			for (;;) {
				if (*p != '_') {
					j = hexChar(*p);
					if (j < 0) break;
					i = i << 4 | j;
				}
				p++;
			}
			q->typ = ELEMENT_CONST;
			q->subTyp[0] = ELEMENT_INT;
			q->iValue = i;
			q++;
			continue;
		}
		if ('0' <= *p && *p <= '9') {
			i = 0;
			for (;;) {
				if (*p != '_') {
					if ('0' <= *p && *p <= '9')
						i = i * 10 + (*p - '0');
					else
						break;
				}
				p++;
			}
			q->typ = ELEMENT_CONST;
			q->subTyp[0] = ELEMENT_INT;
			q->iValue = i;
			q++;
			continue;
		}
		if (*p == '\'') {
			i = 0;
			p++;
			while (*p != '\'' && *p >= ' ') {
				if (*p != '\\') {
					j = *p++;
get8bit:
					i = i << 8 | j;
					continue;
				}
				if (p[1] == '.') {
					p += 2;
					continue;
				}
				if (p[1] == '\n') {
					p += 2;
					if (*p == '\r') p++;
					continue;
				}
				if (p[1] == '\r') {
					p += 2;
					if (*p == '\n') p++;
					continue;
				}
				if (p[1] == '\\' || p[1] == '\"' || p[1] == '\'') {
					j = p[1];
					p += 2;
					goto get8bit;
				}
				if (p[1] == 'n') { j = '\n'; p += 2; goto get8bit; }
				if (p[1] == 'r') { j = '\r'; p += 2; goto get8bit; }
				if (p[1] == 't') { j = '\t'; p += 2; goto get8bit; }
				if ('0' <= p[1] && p[1] <= '7') {
					// ������2���ȏ�͎󂯓���Ȃ�.
					// �����\o�ł���ׂ����Ǝv������.
					j = p[1] - '0';
					p += 2;
					if (*p == '.') p++;
					goto get8bit;
				}
				if (p[1] == 'x') {
					p += 2;
					j = 0;
					for (;;) {
						k = hexChar(*p);
						if (k < 0) break;
						j = j << 4 | k;
						p++;
					}
					if (*p == '.') p++;
					goto get8bit;
				}
				goto err;
			}
			if (*p != '\'') goto err;
			p++;
			q->typ = ELEMENT_CONST;
			q->subTyp[0] = ELEMENT_INT;
			q->iValue = i;
			q++;
			continue;
		}
		if (*p == '(') {
			p++;
			q->typ = ELEMENT_PRNTH0;
			q++;
			stack[++nest] = 0;
			continue;
		}
		if (*p == ')') {
			p++;
			q->typ = ELEMENT_PRNTH1;
			q++;
			nest--;
			continue;
		}
		static struct {
			UCHAR typ, s[3];
		} table_s[] = {
			{ ELEMENT_PLUS,   "+  " },
			{ ELEMENT_MINUS,  "-  " },
			{ ELEMENT_PPLUS,  "++ " },
			{ ELEMENT_MMINUS, "-- " },
			{ ELEMENT_PRODCT, "*  " },
			{ ELEMENT_COMMA,  ",  " },
			{ ELEMENT_OR,     "|  " },
			{ ELEMENT_OROR,   "|| " },
			{ ELEMENT_AND,    "&  " },
			{ ELEMENT_ANDAND, "&& " },
			{ ELEMENT_XOR,    "^  " },
			{ ELEMENT_SHL,    "<< " },
			{ ELEMENT_SAR,    ">> " },
			{ ELEMENT_SLASH,  "/  " },
			{ ELEMENT_PERCNT, "%  " },
			{ ELEMENT_EQUAL,  "=  " },
			{ ELEMENT_OREQUL, "|= " },
			{ ELEMENT_XOREQL, "^= " },
			{ ELEMENT_ANDEQL, "&= " },
			{ ELEMENT_PLUSEQ, "+= " },
			{ ELEMENT_MINUSE, "-= " },
			{ ELEMENT_PRDCTE, "*= " },
			{ ELEMENT_SHLEQL, "<<=" },
			{ ELEMENT_SAREQL, ">>=" },
			{ ELEMENT_SLASHE, "/= " },
			{ ELEMENT_PRCNTE, "%= " },
		//	{ ELEMENT_SEMCLN, ";  " },
			{ ELEMENT_COLON,  ":  " },
		//	{ ELEMENT_BRACE0, "{  " },
		//	{ ELEMENT_BRACE1, "}  " },
			{ ELEMENT_EEQUAL, "== " },
			{ ELEMENT_NOTEQL, "!= " },
			{ ELEMENT_LESS,   "<  " },
			{ ELEMENT_GRTREQ, ">= " },
			{ ELEMENT_LESSEQ, "<= " },
			{ ELEMENT_GREATR, ">  " },
			{ ELEMENT_NOT,    "!  " },
			{ ELEMENT_TILDE,  "~  " },
			{ ELEMENT_DOT,    ".  " },
			{ ELEMENT_ARROW,  "-> " },
			{ ELEMENT_BRCKT0, "[  " },
			{ ELEMENT_BRCKT1, "]  " },
			{ 0, 0, 0, 0 }
		};
		for (i = j = k = 0; table_s[i].typ != 0; i++) {
			l = 3;
			while (table_s[i].s[l - 1] <= ' ') l--;
			if (k >= l) continue;
			if (strncmp(p, table_s[i].s, l) == 0) {
				k = l;
				j = i;
			}
		}
		if (k > 0) {
			p += k;
			q->typ = table_s[j].typ;
			q++;
			continue;
		}

		if (!isSymbolChar(*p) && *p > ' ') {
			q->typ = ELEMENT_IDENT;
			q->pValue = p;
			do {
				p++;
			} while (!isSymbolChar(*p) && *p > ' ');
			q->subValue = p - (const UCHAR *) q->pValue;
			if (q->subValue == 3) {
				i = hexChar(p[-2]);
				j = hexChar(p[-1]);
				if (p[-3] == 'R' && 0 <= i && i <= 3 && j >= 0) {
					q->typ = ELEMENT_REG;
					q->iValue = i << 4 | j;
					q++;
					continue;
				}
				if (p[-3] == 'P' && 0 <= i && i <= 3 && j >= 0) {
					q->typ = ELEMENT_PREG;
					q->iValue = i << 4 | j;
					q++;
					continue;
				}
			}
			static struct {
				UCHAR typ, s[10];
			} table_k[] = {
				{ ELEMENT_FOR,    "3for"      },
				{ ELEMENT_IF,     "2if"       },
				{ ELEMENT_ELSE,   "4else"     },
				{ ELEMENT_BREAK,  "5break"    },
				{ ELEMENT_CONTIN, "8continue" },
				{ ELEMENT_GOTO,   "4goto"     },
				{ ELEMENT_INT32S, "6SInt32"   },
				{ ELEMENT_VDPTR,  "7VoidPtr"  },
				{ 0, "" }
			};
			for (i = 0; table_k[i].typ != 0; i++) {
				l = table_k[i].s[0] - '0';
				if (q->subValue != l) continue;
				if (strncmp((const UCHAR *) q->pValue, &table_k[i].s[1], l) == 0) break;
			}
			if (table_k[i].typ != 0) {
				q->typ = table_k[i].typ;
			}
			q++;
			continue;
		}
err:
		q->typ = ELEMENT_ERROR;
		q++;
		break;
	}
	q->typ = ELEMENT_TERMINATE;
	return p;
}

const struct Element *shuntingYard(struct Element *q, const struct Element *p)
{
	char isLastValue = 0, subTyp2[100];
	struct Element stack[100], *q0 = q;
	int sp = 0, sp0, nest = 0, i;
	subTyp2[0] = 0;
	for (;;) {
		stack[sp].subTyp[1] = 0; // �f�t�H���g�ł͍�����.
		if (p->typ == ELEMENT_ERROR) goto err;
		if (p->typ == ELEMENT_CONST || p->typ == ELEMENT_REG || p->typ == ELEMENT_PREG) {
			if (isLastValue != 0) goto err;
			*q++ = *p++;
			isLastValue = 1;
			continue;
		}
		stack[sp].subTyp[2] = subTyp2[nest];
		if (isLastValue == 0) {
			static struct {
				int typ0, typ1;
				UCHAR subTyp0, subTyp1;
			} table[] = {
				{ ELEMENT_PLUS,   ELEMENT_PLUS1,   2, 0 }, 
				{ ELEMENT_MINUS,  ELEMENT_MINUS1,  2, 0 },
				{ ELEMENT_PRODCT, ELEMENT_PRDCT1,  2, 0 },
				{ ELEMENT_PPLUS,  ELEMENT_PPLUS0,  2, 0 },
				{ ELEMENT_MMINUS, ELEMENT_MMINS0,  2, 0 },
				{ ELEMENT_NOT,    ELEMENT_NOT,     2, 0 },
				{ ELEMENT_TILDE,  ELEMENT_TILDE,   2, 0 },
				{ 0, 0, 0, 0 }
			};
			for (i = 0; table[i].typ0 != 0; i++) {
				if (p->typ == table[i].typ0) break;
			}
			if (table[i].typ0 != 0) {
				p++;
				stack[sp].typ = table[i].typ1;
				stack[sp].subTyp[0] = table[i].subTyp0;
				stack[sp].subTyp[1] = table[i].subTyp1;
				sp++;
				continue;
			}
			if (p->typ == ELEMENT_PRNTH0) {
				p++;
				stack[sp].typ = ELEMENT_PRNTH0;
				stack[sp].subTyp[0] = 99;
				sp++;
				nest++;
				subTyp2[nest] = subTyp2[nest - 1];
				continue;
			}
			if (p->typ == ELEMENT_PERCNT && p[1].typ == ELEMENT_PRNTH0) {
				p += 2;
				stack[sp].typ = ELEMENT_PRNTH0;
				stack[sp].subTyp[0] = 99;
				sp++;
				nest++;
				subTyp2[nest] = 1;
				continue;
			}
		} else {
			if (p->typ == ELEMENT_PPLUS) {
				p++;
				stack[sp].typ = ELEMENT_PPLUS1;
				stack[sp].subTyp[0] = 1;
				sp++;
				continue;
			}
			if (p->typ == ELEMENT_MMINUS) {
				p++;
				stack[sp].typ = ELEMENT_MMINS1;
				stack[sp].subTyp[0] = 1;
				sp++;
				continue;
			}
			if (p->typ == ELEMENT_PRNTH1) {
				p++;
				while (stack[sp - 1].typ != ELEMENT_PRNTH0) {
					*q++ = stack[--sp];
				}
				sp--;
				nest--;
			//	isLastValue = 1;
				/* �����Ɋ֐��Ăяo�����K�v���ǂ����̔�������� */
				continue;
			}
			static struct {
				int typ0, typ1;
				UCHAR subTyp0, subTyp1;
			} table[] = {
				{ ELEMENT_PRODCT, ELEMENT_PRDCT2,  4, 0 },
				{ ELEMENT_SLASH,  ELEMENT_SLASH,   4, 0 },
				{ ELEMENT_PERCNT, ELEMENT_PERCNT,  4, 0 },
				{ ELEMENT_PLUS,   ELEMENT_PLUS2,   5, 0 },
 				{ ELEMENT_MINUS,  ELEMENT_MINUS2,  5, 0 },
				{ ELEMENT_SHL,    ELEMENT_SHL,     6, 0 },
				{ ELEMENT_SAR,    ELEMENT_SAR,     6, 0 },
				{ ELEMENT_LESS,   ELEMENT_LESS,    7, 0 },
				{ ELEMENT_GRTREQ, ELEMENT_GRTREQ,  7, 0 },
				{ ELEMENT_LESSEQ, ELEMENT_LESSEQ,  7, 0 },
				{ ELEMENT_GREATR, ELEMENT_GREATR,  7, 0 },
				{ ELEMENT_EEQUAL, ELEMENT_EEQUAL,  8, 0 },
				{ ELEMENT_NOTEQL, ELEMENT_NOTEQL,  8, 0 },
				{ ELEMENT_AND,    ELEMENT_AND,     9, 0 },
				{ ELEMENT_XOR,    ELEMENT_XOR,    10, 0 },
				{ ELEMENT_OR,     ELEMENT_OR,     11, 0 },
				{ ELEMENT_ANDAND, ELEMENT_ANDAND, 12, 0 },
				{ ELEMENT_OROR,   ELEMENT_OROR,   13, 0 },
				{ ELEMENT_EQUAL,  ELEMENT_EQUAL,  15, 1 }, // �E����.
				{ ELEMENT_OREQUL, ELEMENT_OREQUL, 15, 1 }, // �E����.
				{ ELEMENT_XOREQL, ELEMENT_XOREQL, 15, 1 }, // �E����.
				{ ELEMENT_ANDEQL, ELEMENT_ANDEQL, 15, 1 }, // �E����.
				{ ELEMENT_PLUSEQ, ELEMENT_PLUSEQ, 15, 1 }, // �E����.
				{ ELEMENT_MINUSE, ELEMENT_MINUSE, 15, 1 }, // �E����.
				{ ELEMENT_PRDCTE, ELEMENT_PRDCTE, 15, 1 }, // �E����.
				{ ELEMENT_SHLEQL, ELEMENT_SHLEQL, 15, 1 }, // �E����.
				{ ELEMENT_SAREQL, ELEMENT_SAREQL, 15, 1 }, // �E����.
				{ ELEMENT_SLASHE, ELEMENT_SLASHE, 15, 1 }, // �E����.
				{ ELEMENT_PRCNTE, ELEMENT_PRCNTE, 15, 1 }, // �E����.
				{ ELEMENT_COMMA,  ELEMENT_COMMA1, 16, 0 },
				{ 0, 0, 0, 0 }
			};
			for (i = 0; table[i].typ0 != 0; i++) {
				if (p->typ == table[i].typ0) break;
			}
			if (table[i].typ0 != 0) {
				p++;
				stack[sp].typ = table[i].typ1;
				stack[sp].subTyp[0] = table[i].subTyp0;
				stack[sp].subTyp[1] = table[i].subTyp1;
				sp0 = sp;
				while (sp > 0 && (
						(stack[sp0].subTyp[1] == 0 && stack[sp0].subTyp[0] >= stack[sp - 1].subTyp[0])
						|| (stack[sp0].subTyp[0] > stack[sp - 1].subTyp[0])))
				{
					*q++ = stack[--sp];
				}
				stack[sp++] = stack[sp0];	
				isLastValue = 0;
				continue;
			}
		}
		if (p->typ == ELEMENT_TERMINATE || p->typ == ELEMENT_SEMCLN) break;
		goto err;
	}
	while (sp > 0) {
		sp--;
		if (stack[sp].typ == ELEMENT_PRNTH0) goto err;
		*q++ = stack[sp];
	}
	q->typ = ELEMENT_TERMINATE;
	goto fin;
err:
	q0->typ = ELEMENT_ERROR;
fin:
	return p;
}

void dumpElements(const struct Element *e)
{
	int i;
	if (e->typ != ELEMENT_ERROR) {
		for (i = 0; e[i].typ != ELEMENT_TERMINATE; i++) {
			if (e[i].typ == ELEMENT_CONST)	{ printf("%d ",    e[i].iValue); continue; }
			if (e[i].typ == ELEMENT_REG)	{ printf("R%02X ", e[i].iValue); continue; }
			if (e[i].typ == ELEMENT_PREG)	{ printf("P%02X ", e[i].iValue); continue; }
			if (e[i].typ == ELEMENT_MINUS1)	{ printf("-a ");    continue; }
			if (e[i].typ == ELEMENT_MINUS2)	{ printf("a-b ");   continue; }
			if (e[i].typ == ELEMENT_MMINS0)	{ printf("--a ");   continue; }
			if (e[i].typ == ELEMENT_MMINS1)	{ printf("a-- ");   continue; }
			if (e[i].typ == ELEMENT_PLUS1)	{ printf("+a ");    continue; }
			if (e[i].typ == ELEMENT_PLUS2)	{ printf("a+b ");   continue; }
			if (e[i].typ == ELEMENT_PPLUS0)	{ printf("++a ");   continue; }
			if (e[i].typ == ELEMENT_PPLUS1)	{ printf("a++ ");   continue; }
			if (e[i].typ == ELEMENT_PRDCT2)	{ printf("a*b ");   continue; }
			if (e[i].typ == ELEMENT_EQUAL)	{ printf("a=b ");   continue; }
			if (e[i].typ == ELEMENT_PLUSEQ)	{ printf("a+=b ");  continue; }
			if (e[i].typ == ELEMENT_MINUSE)	{ printf("a-=b ");  continue; }
			if (e[i].typ == ELEMENT_SHL)	{ printf("a<<b ");  continue; }
			if (e[i].typ == ELEMENT_SAR)	{ printf("a>>b ");  continue; }
			printf("?(%d) ", e[i].typ);
		}
		putchar('\n');
	}
	return;
}

char canEval(const UCHAR typ)
// 4:����n, 8:���n, 16:���Z�n, 32:��r�n.
{
	char r = 0;
	if (typ == ELEMENT_PLUS1)  r = 1;
	if (typ == ELEMENT_PLUS2)  r = 2 + 8;
	if (typ == ELEMENT_MINUS1) r = 1;
	if (typ == ELEMENT_MINUS2) r = 2;
	if (typ == ELEMENT_PPLUS0) r = 1;
	if (typ == ELEMENT_PPLUS1) r = 1;
	if (typ == ELEMENT_MMINS0) r = 1;
	if (typ == ELEMENT_MMINS1) r = 1;
	if (typ == ELEMENT_PRDCT2) r = 2 + 8;
	if (typ == ELEMENT_COMMA1) r = 2;
	if (typ == ELEMENT_OR)     r = 2 + 8;
	if (typ == ELEMENT_OROR)   r = 2 + 8;
	if (typ == ELEMENT_AND)    r = 2 + 8;
	if (typ == ELEMENT_ANDAND) r = 2 + 8;
	if (typ == ELEMENT_XOR)    r = 2 + 8;
	if (typ == ELEMENT_SHL)    r = 2;
	if (typ == ELEMENT_SAR)    r = 2;
	if (typ == ELEMENT_SLASH)  r = 2 + 16;
	if (typ == ELEMENT_PERCNT) r = 2 + 16;
	if (typ == ELEMENT_EQUAL)  r = 2 + 4;
	if (typ == ELEMENT_OREQUL) r = 2 + 4;
	if (typ == ELEMENT_XOREQL) r = 2 + 4;
	if (typ == ELEMENT_ANDEQL) r = 2 + 4;
	if (typ == ELEMENT_PLUSEQ) r = 2 + 4;
	if (typ == ELEMENT_MINUSE) r = 2 + 4;
	if (typ == ELEMENT_PRDCTE) r = 2 + 4;
	if (typ == ELEMENT_SHLEQL) r = 2 + 4;
	if (typ == ELEMENT_SAREQL) r = 2 + 4;
	if (typ == ELEMENT_SLASHE) r = 2 + 4 + 16;
	if (typ == ELEMENT_PRCNTE) r = 2 + 4 + 16;
	if (typ == ELEMENT_EEQUAL) r = 2 + 8 + 32;
	if (typ == ELEMENT_NOTEQL) r = 2 + 8 + 32;
	if (typ == ELEMENT_LESS)   r = 2 + 32;
	if (typ == ELEMENT_GRTREQ) r = 2 + 32;
	if (typ == ELEMENT_LESSEQ) r = 2 + 32;
	if (typ == ELEMENT_GREATR) r = 2 + 32;
	if (typ == ELEMENT_NOT)    r = 1;
	if (typ == ELEMENT_TILDE)  r = 1;
	return r;
}

int evalConstInt1(const UCHAR typ, int a)
{
	// �������Ȃ�: ELEMENT_PLUS1, ELEMENT_PPLUS1, ELEMENT_MMINS1
	if (typ == ELEMENT_MINUS1) a = - a;
	if (typ == ELEMENT_PPLUS0) a++;
	if (typ == ELEMENT_MMINS0) a--;
	if (typ == ELEMENT_NOT)    a = - ((a & 1) != 0);
	if (typ == ELEMENT_TILDE)  a = ~a;
	return a;
}

int evalConstInt2(const UCHAR typ, int a, const int b)
// �Ăяo�����Ń[�����`�F�b�N�����邱��.
{
	if (typ == ELEMENT_PLUS2)  a +=  b;
	if (typ == ELEMENT_MINUS2) a -=  b;
	if (typ == ELEMENT_PRDCT2) a *=  b;
	if (typ == ELEMENT_COMMA1) a =   b;
	if (typ == ELEMENT_OR)     a |=  b;
	if (typ == ELEMENT_AND)    a &=  b;
	if (typ == ELEMENT_XOR)    a ^=  b;
	if (typ == ELEMENT_SHL)    a <<= b;
	if (typ == ELEMENT_SAR)    a >>= b;
	if (typ == ELEMENT_SLASH)  a /=  b;
	if (typ == ELEMENT_PERCNT) a %=  b;
	if (typ == ELEMENT_EQUAL)  a =   b;
	if (typ == ELEMENT_OREQUL) a |=  b;
	if (typ == ELEMENT_XOREQL) a ^=  b;
	if (typ == ELEMENT_ANDEQL) a &=  b;
	if (typ == ELEMENT_PLUSEQ) a +=  b;
	if (typ == ELEMENT_MINUSE) a -=  b;
	if (typ == ELEMENT_SHLEQL) a <<= b;
	if (typ == ELEMENT_SAREQL) a >>= b;
	if (typ == ELEMENT_SLASHE) a /=  b;
	if (typ == ELEMENT_PRCNTE) a %=  b;
	if (typ == ELEMENT_OROR)   a = - (((a | b) & 1) != 0);
	if (typ == ELEMENT_ANDAND) a = - (((a & b) & 1) != 0);
	if (typ == ELEMENT_EEQUAL) a = - (a == b); 
	if (typ == ELEMENT_NOTEQL) a = - (a != b);
	if (typ == ELEMENT_LESS)   a = - (a <  b);
	if (typ == ELEMENT_GRTREQ) a = - (a >= b);
	if (typ == ELEMENT_LESSEQ) a = - (a <= b);
	if (typ == ELEMENT_GREATR) a = - (a >  b);
	if (typ == ELEMENT_NOT)    a ^= -1;
	if (typ == ELEMENT_TILDE)  a ^= -1;
	return a;
}

int getConstInt(const UCHAR **pp, char *errflg, int flags)
// flags = 1:shuntingYard�̌��ʂ�\��.
//         2:Rxx��Pxx��萔�ϊ�����.
//         4:stringToElements�̌��ʂ�\��.
{
	struct Element e[10000], f[10000];
	int r = 0, i, stk[100], sp = 0;
	*pp = stringToElements(e, *pp, flags >> 8);
	if ((flags & 128) != 0) dumpElements(e);
	shuntingYard(f, e);
	if ((flags & 1) != 0) dumpElements(f);

	if (f[0].typ == ELEMENT_ERROR) {
err:
		*errflg = 1;
	} else {
		for (i = 0; f[i].typ != ELEMENT_TERMINATE; i++) {
			if (f[i].typ == ELEMENT_CONST) {
				stk[sp++] = f[i].iValue;
				continue;
			}
			if ((flags & 2) != 0) {
				if (f[i].typ == ELEMENT_REG) {
					stk[sp++] = f[i].iValue;
					continue;
				}
				if (f[i].typ == ELEMENT_PREG) {
					stk[sp++] = f[i].iValue + 0x40;
					continue;
				}
			}
			char ce = canEval(f[i].typ) & 3;
			if (ce == 1 && sp >= 1) {
				stk[sp - 1] = evalConstInt1(f[i].typ, stk[sp - 1]);
				continue;
			}
			if (ce == 2 && sp >= 2) {
				stk[sp - 2] = evalConstInt2(f[i].typ, stk[sp - 2], stk[sp - 1]);
				sp--;
				continue;
			}
			goto err;
		}
	}
	if (sp != 1)
		*errflg = 1;
	r = stk[0];
	return r;
}

int getConstInt1(const UCHAR **pp)
// �J���}�Ŏ~�܂�.
{
	char errflg = 0;
	return getConstInt(pp, &errflg, 1 << 8);
}

/*** aska ***/

struct Ident {
	int len0, len1;
	const UCHAR *p0, *p1;
	int depth;
	struct Element ele;
};

struct Brace {
	UCHAR typ, flg0;
	const UCHAR *p0, *p1;
};

UCHAR *askaPass1(struct Work *w);
int idendef(const char *p, void *dmy);
const UCHAR *idenref(const UCHAR *p, UCHAR *q, void *iden);
int isRxx(const UCHAR *p);
int isPxx(const UCHAR *p);
int isConst(const UCHAR *p);
const UCHAR *copyConst(const UCHAR *p, UCHAR **qq);
const UCHAR *skipSpace(const UCHAR *p);

int aska(struct Work *w)
{
	return putBuf(w->argOut, w->obuf, askaPass1(w));
}

int askaAllocTemp(UCHAR *a, int i1)
{
	int i, j = -1;
	for (i = 0; i < i1; i++) {
		if (a[i] < 0xfe) {
			j = a[i];
			a[i] = 0xfe;
			break;
		}
	}
	return j;
}

const char *getOpecode(const UCHAR typ)
{
	const char *r = "?";
	if (typ == ELEMENT_PLUS2)  r = "ADD";
	if (typ == ELEMENT_MINUS2) r = "SUB";
	if (typ == ELEMENT_PPLUS0) r = "ADD";
	if (typ == ELEMENT_PPLUS1) r = "ADD";
	if (typ == ELEMENT_MMINS0) r = "SUB";
	if (typ == ELEMENT_MMINS1) r = "SUB";
	if (typ == ELEMENT_PRDCT2) r = "MUL";
	if (typ == ELEMENT_OR)     r = "OR";
	if (typ == ELEMENT_AND)    r = "AND";
	if (typ == ELEMENT_XOR)    r = "XOR";
	if (typ == ELEMENT_SHL)    r = "SHL";
	if (typ == ELEMENT_SAR)    r = "SAR";
	if (typ == ELEMENT_SLASH)  r = "DIV";
	if (typ == ELEMENT_PERCNT) r = "MOD";
	if (typ == ELEMENT_OREQUL) r = "OR";
	if (typ == ELEMENT_XOREQL) r = "XOR";
	if (typ == ELEMENT_ANDEQL) r = "AND";
	if (typ == ELEMENT_PLUSEQ) r = "ADD";
	if (typ == ELEMENT_MINUSE) r = "SUB";
	if (typ == ELEMENT_PRDCTE) r = "MUL";
	if (typ == ELEMENT_SHLEQL) r = "SHL";
	if (typ == ELEMENT_SAREQL) r = "SAR";
	if (typ == ELEMENT_SLASHE) r = "DIV";
	if (typ == ELEMENT_PRCNTE) r = "MOD";
	if (typ == ELEMENT_EEQUAL) r = "CMPE";
	if (typ == ELEMENT_NOTEQL) r = "CMPNE";
	if (typ == ELEMENT_LESS)   r = "CMPL";
	if (typ == ELEMENT_GRTREQ) r = "CMPGE";
	if (typ == ELEMENT_LESSEQ) r = "CMPLE";
	if (typ == ELEMENT_GREATR) r = "CMPG";
	return r;
}

int askaPass1AllocStk(UCHAR *tmpR, UCHAR *tmpP, int typ)
{

}

char askaPass1FreeStk(struct Element *ele, UCHAR *tmpR, UCHAR *tmpP)
{
	char r = 0;
	int i;
	if ((ele->subTyp[2] & 0x03) == 0x01) {
		ele->subTyp[2] |= 0x02;
		if (ele->typ == ELEMENT_REG) {
			if (tmpR == NULL)
				r = 1;
			else {
				for (i = 0; tmpR[i] < 0xfe; i++);
				tmpR[i] = ele->iValue;
			}
		}
		if (ele->typ == ELEMENT_PREG) {
			if (tmpP == NULL)
				r = 1;
			else {
				for (i = 0; tmpP[i] < 0xfe; i++);
				tmpP[i] = ele->iValue;
			}
		}
	}
	return r;
}

char askaPass1Optimize(struct Element **pstk, struct Element *stk0, struct Element **pele, UCHAR *tmpR, UCHAR *tmpP);

char askaPass1PrefechConst(struct Element **pstk, struct Element **pele)
{
	struct Element *stk = *pstk, *ele = *pele, *stk0 = stk;
	char r = askaPass1Optimize(&stk, stk, &ele, NULL, NULL);
	if (r == 0 && stk == stk0 + 1 && stk[-1].typ == ELEMENT_CONST && (canEval(ele->typ) & 3) == 2) {
		*pstk = stk;
		*pele = ele;
	} else
		r = 1;
	return r;
}

int askaLog2(int i)
{
	int r = -1;
	if (i > 0 && ((i - 1) & i) == 0) {
		r = 0;
		while (i >= 2) {
			i >>= 1;
			r++;
		}
	}
	return r;
}

char askaPass1Optimize(struct Element **pstk, struct Element *stk0, struct Element **pele, UCHAR *tmpR, UCHAR *tmpP)
// ���j: ���Z�q��(ele)�������������ɍςޔ͈͂ł̂ݍœK��.
{
	struct Element *stk = *pstk, *ele = *pele, tmpEle, *tele, *tstk;
	char r = 0, rr;
	int i;
	for (;;) {
		if (r != 0) break;
		UCHAR et = ele->typ, ce = canEval(et);
		if (et == ELEMENT_CONST || et == ELEMENT_REG || et == ELEMENT_PREG) {
			*stk = *ele;
			ele++;
			stk->subTyp[2] = 0;
			stk++;
			continue;
		}
		if ((ce & 4) != 0 && stk[-2].typ == ELEMENT_CONST) break; /* �萔�ɑ���͂ł��Ȃ� */
		if ((ce & 3) == 1 && &stk[-1] < stk0) break; /* ��������Ȃ� */
		if ((ce & 3) == 2 && &stk[-2] < stk0) break; /* ��������Ȃ� */
		if ((ce & 3) == 1 && stk[-1].typ == ELEMENT_CONST) {
			stk[-1].iValue = evalConstInt1(et, stk[-1].iValue);
			ele++;
			continue;
		}
		if ((ce & 3) == 2 && stk[-2].typ == ELEMENT_CONST && stk[-1].typ == ELEMENT_CONST) {
			if ((ce & 16) != 0 && stk[-1].iValue == 0) break;
			stk[-2].iValue = evalConstInt2(et, stk[-2].iValue, stk[-1].iValue);
			ele++;
			stk--;
			continue;
		}
		if ((ce & (3 | 8)) == (2 | 8)) {
			rr = 0; // ��2�����Z�q.
			if (stk[-2].typ == ELEMENT_CONST && stk[-1].typ != ELEMENT_CONST)  rr = 1; // �萔���͉E����.
			if (stk[-2].typ != ELEMENT_PREG  && stk[-1].typ == ELEMENT_PREG)   rr = 1; // �|�C���^���W�X�^���������Ă���Ƃ��͍�����.
			if (stk[-2].typ == stk[-1].typ && stk[-2].iValue > stk[-1].iValue) rr = 1; // ���^�̏ꍇ�̓��W�X�^�ԍ����������ق���������.
			if (rr != 0) {
				tmpEle  = stk[-2];
				stk[-2] = stk[-1];
				stk[-1] = tmpEle;
				continue;
			}
		}
		if ((ce & 3) == 1) {
			if (stk[-1].typ != ELEMENT_PREG && et == ELEMENT_PRDCT1) break;
			if (stk[-1].typ == ELEMENT_PREG) {
				if (et == ELEMENT_MINUS1 || et == ELEMENT_NOT || et == ELEMENT_TILDE) break;
			}
		}
		if ((ce & 3) == 2 && et != ELEMENT_COMMA1 && stk[-2].typ == ELEMENT_PREG) {
			if (stk[-1].typ == ELEMENT_PREG) {
				if ((ce & 32) == 0 && et != ELEMENT_MINUS2 && et != ELEMENT_EQUAL) break;
			}
			if (stk[-1].typ != ELEMENT_PREG) {
				if (et != ELEMENT_PLUS2 && et != ELEMENT_MINUS2 && et != ELEMENT_PLUSEQ && et != ELEMENT_MINUSE) break;
			}
		}
		if ((ce & 3) == 2 && stk[-2].typ == stk[-1].typ && stk[-2].iValue == stk[-1].iValue) {
			// ���ӂƉE�ӂœ������̂������ꍇ.
			static struct {
				int typ, value;
			} table0[] = {
				{ ELEMENT_MINUS2,  0 },
				{ ELEMENT_XOR,     0 },
				{ ELEMENT_EEQUAL, -1 },
				{ ELEMENT_NOTEQL,  0 },
				{ ELEMENT_LESS,    0 },
				{ ELEMENT_GRTREQ, -1 },
				{ ELEMENT_LESSEQ, -1 },
				{ ELEMENT_GREATR,  0 },
				{ 0, 0 }
			};
			for (i = 0; table0[i].typ != 0; i++) {
				if (table0[i].typ == et)
					break;
			}
			if (table0[i].typ != 0) {
				// ���ʂ��萔�ɂȂ�p�^�[��.
				ele++;
				r |= askaPass1FreeStk(&stk[-2], tmpR, tmpP);
				r |= askaPass1FreeStk(&stk[-1], tmpR, tmpP);
				stk--;
				stk[-1].typ = ELEMENT_CONST;
				stk[-1].subTyp[0] = ELEMENT_INT;
				stk[-1].iValue = table0[i].value;
				continue;
			}
			rr = 0;
			if (et == ELEMENT_EQUAL || et == ELEMENT_OREQUL || et == ELEMENT_ANDEQL) rr = 1;
			if (et == ELEMENT_OR || et == ELEMENT_AND) rr = 1;
			if (rr != 0) {
				// ���̂܂܌��ʂɂȂ�p�^�[��.
				ele++;
				r |= askaPass1FreeStk(&stk[-1], tmpR, tmpP);
				stk--;
				continue;
			}
		}
		if ((ce & 3) == 2 && stk[-1].typ == ELEMENT_CONST) {
			// ����̒萔�Ƃ̉��Z.
			static struct {
				int typ, value;
			} table0[] = {
				{ ELEMENT_PLUS2,   0 },
				{ ELEMENT_MINUS2,  0 },
				{ ELEMENT_PRDCT2,  1 },
				{ ELEMENT_OR,      0 },
				{ ELEMENT_AND,    -1 },
				{ ELEMENT_XOR,     0 },
				{ ELEMENT_SHL,     0 },
				{ ELEMENT_SAR,     0 },
				{ ELEMENT_SLASH,   1 },
				{ ELEMENT_OREQUL,  0 },
				{ ELEMENT_XOREQL,  0 },
				{ ELEMENT_ANDEQL, -1 },
				{ ELEMENT_PLUSEQ,  0 },
				{ ELEMENT_MINUSE,  0 },
				{ ELEMENT_PRDCTE,  1 },
				{ ELEMENT_SHLEQL,  0 },
				{ ELEMENT_SAREQL,  0 },
				{ ELEMENT_SLASHE,  1 },
				{ 0, 0 }
			};
			for (i = 0; table0[i].typ != 0; i++) {
				if (table0[i].typ == et && table0[i].value == stk[-1].iValue)
					break;
			}
			if (table0[i].typ != 0) {
				// ���Z���Ă������̒l���s�ςɂȂ�p�^�[��.
				ele++;
				stk--;
				continue;
			}
			static struct {
				int typ, v0, v1;
			} table1[] = {
				{ ELEMENT_PRDCT2,  0,  0 },
				{ ELEMENT_OR,     -1, -1 },
				{ ELEMENT_AND,     0,  0 },
				{ ELEMENT_PERCNT,  1,  0 },
				{ ELEMENT_PERCNT, -1,  0 },
				{ 0, 0, 0 }
			};
			for (i = 0; table1[i].typ != 0; i++) {
				if (table1[i].typ == et && table1[i].v0 == stk[-1].iValue)
					break;
			}
			if (table1[i].typ != 0) {
				// ���ʂ��萔�ɂȂ�p�^�[��.
				ele++;
				stk--;
				stk[-1].typ = ELEMENT_CONST;
				stk[-1].subTyp[0] = ELEMENT_INT;
				stk[-1].iValue = table1[i].v1;
				continue;
			}
			// �萔�̘A�����Z�̌���.
			tele = ele + 1;
			tstk = stk;
			rr = askaPass1PrefechConst(&tstk, &tele); // �㑱�����]������.
			if (rr == 0) {
				if (et == ELEMENT_PLUS2  && tele->typ == ELEMENT_PLUS2 ) {
					rr = 1;
					stk[-1].iValue += stk[0].iValue;
				}
				if (et == ELEMENT_PLUS2  && tele->typ == ELEMENT_MINUS2) {
					rr = 1;
					stk[-1].iValue = - (stk[-1].iValue - stk[0].iValue);
				}
				if (et == ELEMENT_MINUS2 && tele->typ == ELEMENT_PLUS2 ) {
					rr = 1;
					stk[-1].iValue = - stk[-1].iValue + stk[0].iValue;
				}
				if (et == ELEMENT_MINUS2 && tele->typ == ELEMENT_MINUS2) {
					rr = 1;
					stk[-1].iValue += stk[0].iValue;
				}
				if (et == ELEMENT_PRDCT2 && tele->typ == ELEMENT_PRDCT2) {
					rr = 1;
					stk[-1].iValue *= stk[0].iValue;
				}
				if (et == ELEMENT_PRDCT2 && tele->typ == ELEMENT_SHL   ) {
					i = askaLog2(stk[-1].iValue);
					if (i >= 0) {
						rr = 1;
						stk[-1].iValue = i + stk[0].iValue;
					}
				}
				if (et == ELEMENT_OR     && tele->typ == ELEMENT_OR    ) {
					rr = 1;
					stk[-1].iValue |= stk[0].iValue;
				}
				if (et == ELEMENT_AND    && tele->typ == ELEMENT_AND   ) {
					rr = 1;
					stk[-1].iValue &= stk[0].iValue;
				}
				if (et == ELEMENT_XOR    && tele->typ == ELEMENT_XOR   ) {
					rr = 1;
					stk[-1].iValue |= stk[0].iValue;
				}
				if (et == ELEMENT_SHL    && tele->typ == ELEMENT_SHL   ) {
					if (stk[-1].iValue >= 0 && stk[0].iValue >= 0) {
						rr = 1;
						stk[-1].iValue += stk[0].iValue;
					}
				}
				if (et == ELEMENT_SHL    && tele->typ == ELEMENT_PRDCT2) {
					if (stk[-1].iValue >= 0) {
						rr = 1;
						stk[-1].iValue = (1 << stk[-1].iValue) * stk[0].iValue;
					}
				}
				if (et == ELEMENT_SAR    && tele->typ == ELEMENT_SAR   ) {
					if (tstk[-2].iValue >= 0 && tstk[-1].iValue >= 0) {
						rr = 1;
						tstk[-2].iValue += tstk[-1].iValue;
					}
				}
				if (et == ELEMENT_SAR    && tele->typ == ELEMENT_SLASH ) {
					if (stk[-1].iValue >= 0) {
						rr = 1;
						stk[-1].iValue = (1 << stk[-1].iValue) * stk[0].iValue;
					}
				}
				if (rr != 0) {
					ele = tele;
					continue;
				}
			}
		}
		if (et == ELEMENT_PLUS1) {
			ele++;
			continue;
		}
		if (et == ELEMENT_COMMA1) {
			ele++;
			r |= askaPass1FreeStk(&stk[-2], tmpR, tmpP);
			stk[-2] = stk[-1];
			stk--;
			continue;
		}
		if (et == ELEMENT_SLASH) {
			// ������: �E�V�t�g�Ƃ̕��p.
		}
		if (et == ELEMENT_PERCNT) {
			// ������: &�Ƃ̕��p
		}
		// �܂��l���ĂȂ�: ELEMENT_PRDCT1, ELEMENT_OROR, ELEMENT_ANDAND
		break;
	}
	if (r == 0) {
		*pstk = stk;
		*pele = ele;
	}
	return r;
}

void askaPass1Optimize1(struct Element *stk, struct Element *ele)
{
	if (stk[0].typ == ELEMENT_REG && stk[1].typ == ELEMENT_CONST) {
		if (stk[1].iValue == 0 && (ele->typ == ELEMENT_PRDCTE || ele->typ == ELEMENT_ANDEQL)) {
			ele->typ = ELEMENT_EQUAL;
		}
		if (stk[1].iValue == -1 && ele->typ == ELEMENT_OREQUL) {
			ele->typ = ELEMENT_EQUAL;
		}
		if (ele->typ == ELEMENT_PRCNTE && (stk[1].iValue == 1 || stk[1].iValue == -1)) {
			ele->typ = ELEMENT_EQUAL;
			stk[1].iValue = 0;
		}
	}
	if (stk[0].typ == ELEMENT_REG && stk[1].typ == ELEMENT_REG && stk[0].iValue == stk[1].iValue) {
		if (ele->typ == ELEMENT_XOREQL || ele->typ == ELEMENT_MINUSE) {
			ele->typ = ELEMENT_EQUAL;
			stk[1].typ = ELEMENT_CONST;
			stk[1].subTyp[0] = ELEMENT_INT;
			stk[1].iValue = 0;
		}
		if (ele->typ == ELEMENT_SLASHE) {
			ele->typ = ELEMENT_EQUAL;
			stk[1].typ = ELEMENT_CONST;
			stk[1].subTyp[0] = ELEMENT_INT;
			stk[1].iValue = 1;
		}
	}
	return;
}

void askaPass1OptimizeOpecode(UCHAR *q, struct Element *stk)
{
	if (stk->typ == ELEMENT_CONST) {
		int i = askaLog2(stk->iValue);
		if (strcmp(q, "SUB") == 0) {
			strcpy(q, "ADD");
			stk->iValue *= -1;
		}
		if (strcmp(q, "MUL") == 0 && i >= 0) {
			strcpy(q, "SHL");
			stk->iValue = i;
		}
		if (strcmp(q, "DIV") == 0 && stk->iValue == -1) {
			strcpy(q, "MUL");
		}
		if (strcmp(q, "DIV") == 0 && i >= 0) {
			strcpy(q, "SAR");
			stk->iValue = i;
		}
		// MOD��AND�ϊ��͂ł��Ȃ�.
		// -123%8 �� -123&7 �͈قȂ錋�ʂȂ̂ŁA��ʂ�x%8==x&7�Ƃ͌����Ȃ�����.
	}
}

int substitute(struct Element *ele, struct Ident *ident)
{
	int i, j;
	int r = -1;
	for (i = 0; ele[i].typ != ELEMENT_TERMINATE && ele[i].typ != ELEMENT_ERROR; i++) {
		if (ele[i].typ == ELEMENT_IDENT) {
			for (j = 0; j < MAXIDENTS; j++) {
				if (ident[j].len0 == ele[i].subValue && strncmp(ident[j].p0, ele[i].pValue, ele[i].subValue) == 0)
					break;
			}
			if (j >= MAXIDENTS) {
				r = i;
				break;
			}
			ele[i] = ident[j].ele;
		}
	}
	return r;
}

const UCHAR *askaPass1Sub(struct Work *w, const UCHAR *p, UCHAR **qq, struct Ident *ident, int flags)
{
	int i, j, dsp;
	UCHAR tmpR[16], tmpP[16];
	for (i = 0; i < 16; i++)
		tmpP[i] = tmpR[i] = 0xff;

	w->sp = -2;
	UCHAR *q = *qq;
	struct Element *ele0 = w->ele0, *ele1 = w->ele1, tmpEle, *prm0, *next;
	const UCHAR *r = stringToElements(ele0, p, flags);
	if (ele0[0].typ == ELEMENT_FOR || ele0[0].typ == ELEMENT_IF ||
		ele0[0].typ == ELEMENT_BREAK || ele0[0].typ == ELEMENT_CONTIN || ele0[0].typ == ELEMENT_GOTO) goto fin;
	i = substitute(ele0, ident);
	if (i >= 0) {
		if (i == 0 && ele0[1].typ == ELEMENT_PRNTH0) goto fin;
		fprintf(stderr, "aska: undeclared: %.*s\n", ele0[i].subValue, ele0[i].pValue);
		goto exp_err;
	}
	j = 0;
	if (ele0[0].typ == ELEMENT_REG && ele0[1].typ == ELEMENT_EQUAL) {
		j = 1;
		for (i = 2; ele0[i].typ != ELEMENT_TERMINATE && ele0[i].typ != ELEMENT_ERROR; i++) {
			if (ele0[i].typ == ele0[0].typ && ele0[i].iValue == ele0[0].iValue)
				j = 0;
		}
	}
	for (i = 1; i <= 12; i++)
		tmpR[i] = tmpP[i] = 0x3c - i;
	tmpR[0] = 0xfe;
	if (j != 0)
		tmpR[0] = ele0[0].iValue;
	j = 0;
	if (ele0[0].typ == ELEMENT_PREG && ele0[1].typ == ELEMENT_EQUAL) {
		j = 1;
		for (i = 2; ele0[i].typ != ELEMENT_TERMINATE && ele0[i].typ != ELEMENT_ERROR; i++) {
			if (ele0[i].typ == ele0[0].typ && ele0[i].iValue == ele0[0].iValue)
				j = 0;
		}
	}
	tmpP[0] = 0xfe;
	if (j != 0)
		tmpP[0] = ele0[0].iValue;
	const struct Element *c_ele = shuntingYard(ele1, ele0);
	if ((flags & 256) != 0) {
		for (i = 0; ele1[i].typ != ELEMENT_TERMINATE && ele1[i].typ != ELEMENT_ERROR; i++);
		ele1[i + 1] = ele1[i];
		ele1[i].typ = ELEMENT_NOT;
	}
	if (ele1->typ != ELEMENT_ERROR && c_ele->typ == ELEMENT_TERMINATE) {
		int sp = 0;
		for (i = 0; ele1[i].typ != ELEMENT_TERMINATE; i++) {
			struct Element *tstk = &ele0[sp], *tele = &ele1[i];
			askaPass1Optimize(&tstk, ele0, &tele, tmpR, tmpP);
			sp = tstk - ele0;
			i = tele - ele1;
			if (tele->typ == ELEMENT_TERMINATE) break;
			char ce = canEval(ele1[i].typ);
			if (ce == 1 && sp > 0) {
				if (ele0[sp - 1].typ == ELEMENT_REG && ele1[i].typ == ELEMENT_PLUS1) continue;
				if (ele1[i].subTyp[2] != 0) goto exp_giveup;
				if (ele0[sp - 1].typ == ELEMENT_REG) {
					if (ele1[i].typ == ELEMENT_MINUS1) {
						askaPass1FreeStk(&ele0[sp - 1], tmpR, tmpP);
						j = askaAllocTemp(tmpR, 16);
						if (j < 0) goto exp_giveup;
						sprintf(q, "MULI(R%02X,R%02X,-1);", j, ele0[sp - 1].iValue);
						q += 17;
						ele0[sp - 1].subTyp[2] = 1;
						ele0[sp - 1].iValue = j;
						continue;
					}
					if (ele1[i].typ == ELEMENT_NOT || ele1[i].typ == ELEMENT_TILDE) {
						askaPass1FreeStk(&ele0[sp - 1], tmpR, tmpP);
						j = askaAllocTemp(tmpR, 16);
						if (j < 0) goto exp_giveup;
						sprintf(q, "XORI(R%02X,R%02X,-1);", j, ele0[sp - 1].iValue);
						q += 17;
						ele0[sp - 1].subTyp[2] = 1;
						ele0[sp - 1].iValue = j;
						continue;
					}
				}
			}
			if ((ce & 16) != 0 && ele0[sp - 1].typ == ELEMENT_CONST && ele0[sp - 1].iValue == 0) goto exp_div0;
			// OROR, ANDAND�͏��������.�Ƃ������߂�ǂ�����.
			if (sp >= 2)
				askaPass1Optimize1(&ele0[sp - 2], &ele1[i]);
			if ((ce & 3) == 2 && sp >= 2 && ele1[i].typ != ELEMENT_EQUAL) {
				prm0 = NULL;
				next = &ele1[i + 1];
				dsp = 1;
				if ((ce & 4) != 0 && ele0[sp - 2].typ != ELEMENT_CONST)
					prm0 = &ele0[sp - 2]; // ����n.
				if ((ce & 32) != 0) {
					// ��r�n
					if ((ce & 8) == 0) {
						j = 0;
						if (ele0[sp - 2].typ == ELEMENT_CONST && ele0[sp - 1].typ == ELEMENT_REG) j |= 1;
						if (ele0[sp - 2].typ == ELEMENT_REG   && ele0[sp - 1].typ == ELEMENT_REG  && ele0[sp - 2].iValue > ele0[sp - 1].iValue) j |= 1;
						if (ele0[sp - 2].typ == ELEMENT_PREG  && ele0[sp - 1].typ == ELEMENT_PREG && ele0[sp - 2].iValue > ele0[sp - 1].iValue) j |= 1;
						if (j != 0) {
							tmpEle = ele0[sp - 2];
							ele0[sp - 2] = ele0[sp - 1];
							ele0[sp - 1] = tmpEle;
							ele1[i].typ ^= 127;
						}
					}
					while (next->typ == ELEMENT_NOT) {
						next++;
						ele1[i].typ ^= 1;
					}
				}
				if (sp >= 3 && next->typ == ELEMENT_EQUAL && ele0[sp - 3].typ == ELEMENT_REG) {
					prm0 = &ele0[sp - 3];
					next++;
					dsp++;
				}
				if (prm0 == NULL) {
					if (ele1[i].subTyp[2] != 0) goto exp_giveup;
					askaPass1FreeStk(&ele0[sp - 2], tmpR, tmpP);
					askaPass1FreeStk(&ele0[sp - 1], tmpR, tmpP);
					j = askaAllocTemp(tmpR, 16);
					if (j < 0) goto exp_giveup;
					sp++;
					ele0[sp - 1] = ele0[sp - 2];
					ele0[sp - 2] = ele0[sp - 3];
					ele0[sp - 3].typ = ELEMENT_REG;
					ele0[sp - 3].subTyp[2] = 1;
					ele0[sp - 3].iValue = j;
					dsp++;
					prm0 = &ele0[sp - 3];
				}
				if (ele0[sp - 2].typ == ELEMENT_CONST) {
					sprintf(q, "LIMM(R3F,0x%X);", ele0[sp - 2].iValue);
					while (*q != 0) q++;
					ele0[sp - 2].typ = ELEMENT_REG;
					ele0[sp - 2].subTyp[2] = 0;
					ele0[sp - 2].iValue = 0x3f;
				}
				if (ele0[sp - 2].typ == ELEMENT_PREG) {
					*q = 'P';
					strcpy(q + 1, getOpecode(ele1[i].typ));
				} else {
					strcpy(q, getOpecode(ele1[i].typ));
					askaPass1OptimizeOpecode(q, &ele0[sp - 1]);
				}
				while (*q != 0) q++;
				if (ele0[sp - 2].typ == ELEMENT_REG  && ele0[sp - 1].typ == ELEMENT_CONST)
					sprintf(q, "I(R%02X,R%02X,0x%X);", prm0->iValue, ele0[sp - 2].iValue, ele0[sp - 1].iValue);
				if (ele0[sp - 2].typ == ELEMENT_REG  && ele0[sp - 1].typ == ELEMENT_REG)
					sprintf(q, "(R%02X,R%02X,R%02X);", prm0->iValue, ele0[sp - 2].iValue, ele0[sp - 1].iValue);
				if (ele0[sp - 2].typ == ELEMENT_PREG && ele0[sp - 1].typ == ELEMENT_PREG)
					sprintf(q, "(R%02X,P%02X,P%02X);", prm0->iValue, ele0[sp - 2].iValue, ele0[sp - 1].iValue);
				while (*q != 0) q++;
				do {
					askaPass1FreeStk(&ele0[sp - 1], tmpR, tmpP);
					sp--;
				} while (--dsp != 0);
				i = next - ele1 - 1;
				continue;
			}
			if (ele1[i].typ == ELEMENT_EQUAL && sp >= 2) {
				if (ele0[sp - 2].typ == ELEMENT_REG && ele0[sp - 1].typ == ELEMENT_CONST) {
					sprintf(q, "LIMM(R%02X,0x%X);", ele0[sp - 2].iValue, ele0[sp - 1].iValue);
					while (*q != 0) q++;
					ele0[sp - 2] = ele0[sp - 1];
					sp--;
					continue;
				}
				if (ele0[sp - 2].typ == ELEMENT_REG && ele0[sp - 1].typ == ELEMENT_REG) {
					// ������: �ǂ������c�����A�����Ƃ��܂����f��������...���Ƃ��΃��W�X�^�ԍ��̎Ⴂ���Ƃ�.
					if (ele0[sp - 1].subTyp[2] != 0) tmpR[ele0[sp - 1].iValue]--;
					if (ele0[sp - 2].iValue != ele0[sp - 1].iValue) {
						sprintf(q, "CP(R%02X,R%02X);", ele0[sp - 2].iValue, ele0[sp - 1].iValue);
						q += 12;
						ele0[sp - 2] = ele0[sp - 1];
					}
					sp--;
					continue;
				}
				if (ele0[sp - 2].typ == ELEMENT_PREG && ele0[sp - 1].typ == ELEMENT_PREG) {
					// ������: �ǂ������c�����A�����Ƃ��܂����f��������...���Ƃ��΃��W�X�^�ԍ��̎Ⴂ���Ƃ�.
					if (ele0[sp - 1].subTyp[2] != 0) tmpP[ele0[sp - 1].iValue]--;
					sprintf(q, "PCP(P%02X,P%02X);", ele0[sp - 2].iValue, ele0[sp - 1].iValue);
					q += 13;
					ele0[sp - 2] = ele0[sp - 1];
					sp--;
					continue;
				}
			}
			if ((ele1[i].typ == ELEMENT_PPLUS0 || ele1[i].typ == ELEMENT_MMINS0) && sp > 0 && ele0[sp - 1].typ == ELEMENT_REG) {
pplus1:
				j = 1;
				if (ele1[i].typ == ELEMENT_MMINS0 || ele1[i].typ == ELEMENT_MMINS1) j = -1;
				sprintf(q, "ADDI(R%02X,R%02X,0x%08X);", ele0[sp - 1].iValue, ele0[sp - 1].iValue, j);
				q += 25;
				continue;
			}
			if ((ele1[i].typ == ELEMENT_PPLUS1 || ele1[i].typ == ELEMENT_MMINS1) && sp > 0 && ele0[sp - 1].typ == ELEMENT_REG) {
				if (ele1[i + 1].typ == ELEMENT_TERMINATE || ele1[i + 1].typ == ELEMENT_COMMA1) goto pplus1;
				/* �e���|�������g���ď��� */
			}
			goto exp_err;
		}
		w->sp = sp;
		if (sp >= 2) {
			q = strcpy1(q, "DB(0xef);");
		}
		goto fin;
exp_giveup:
		/* giveup�R�[�h���o�� */
		q = strcpy1(q, "DB(0xed);");
		goto exp_skip;
exp_div0:
		q = strcpy1(q, "DB(0xee);");
		goto exp_skip;
exp_err:
		q = strcpy1(q, "DB(0xef);");
exp_skip:
		w->sp = -1;
	}
fin:
	*qq = q;
	return r;
}

void askaPass1UseR3F(struct Element *stk, UCHAR *q, UCHAR *q0)
{
	if (stk->typ == ELEMENT_REG && (stk->subTyp[2] & 1) != 0 && q - 17 >= q0 && q[-2] == ')' && q[-1] == ';') {	// CMP(R00,R00,R00);
		q -= 3;
		while (*q != '(' && q > q0) q--;
		if (*q == '(') {
			q--;
			while (q > q0 && *q != ';' && *q > ' ') q--;
			if (strncmp1(q + 1, "CMP") == 0 || strncmp1(q + 1, "PCMP") == 0) {
				while (*q != '(') q++;
				if (q[1] == 'R') {
					q[2] = '3';
					q[3] = 'F';
					stk->iValue = 0x3f;
				}
			}
		}
	}
	return;
}

UCHAR *askaPass1(struct Work *w)
{
	char code = 0, underscore = 1, flag, isWaitBrase0 = 0;
	const UCHAR *p = w->ibuf, *r, *s, *t, *braceP;
	UCHAR *q = w->obuf, *qq;
	struct Ident *ident = malloc(MAXIDENTS * sizeof (struct Ident));
	struct Brace *brase = malloc(128 * sizeof (struct Brace));
	int i, j, k, braseDepth = 0;
	struct Element *ele0 = w->ele0;
	brase[0].typ = 0;
	for (i = 0; i < MAXIDENTS; i++) {
		ident[i].len0 = 0;
	}
	if (strstr(p, "/*") != NULL) {
		// �����prepro�ŏ���������.
		fputs("find: '/*'\n", stderr);
		goto err;
	}
	while (*p != '\0') {
		p = skipSpace(p);
		if (*p == '\n' || *p == '\r') {
			*q++ = *p++;
			underscore = 1;
			continue;
		}
		if (*p == '#') {
			do {
				while (*p != '\n' && *p != '\r' && *p != '\0')
					*q++ = *p++;
				flag = 0;
				if (p[-1] == '\\')
					flag = 1;
				while (*p == '\n' || *p == '\r')
					*q++ = *p++;
			} while (flag != 0 && *p != '\0');
			underscore = 1;
			continue;
		}
		if (p[0] == '/' && p[1] == '/') {
			while (*p != '\n' && *p != '\r' && *p != '\0')
				*q++ = *p++;
			while (*p == '\n' || *p == '\r')
				*q++ = *p++;
			underscore = 1;
			continue;
		}
		if (strncmp1(p, "OSECPU_HEADER();") == 0) {
			code = 1;
			memcpy(q, p, 16);
			p += 16;
			q += 16;
			continue;
		}
		if (strncmp1(p, "%include") == 0 || strncmp1(p, "%define") == 0 || strncmp1(p, "%undef") == 0) {
			*q++ = '#';
			p++;
			while (*p != '\n' && *p != '\r' && *p != '\0')
				*q++ = *p++;
			while (*p == '\n' || *p == '\r')
				*q++ = *p++;
			underscore = 1;
			continue;
		}
		if (strncmp1(p, "aska_code(1);") == 0) {
			code = 1;
			p += 13;
			continue;
		}
		if (strncmp1(p, "aska_code(0);") == 0) {
			code = 0;
			p += 13;
			continue;
		}
		if (code != 0 && underscore != 0) {
			*q++ = '_';
			*q++ = ' ';
			underscore = 0;
		}

		if (isWaitBrase0 != 0) {
			if (*p == '{')
				p++;
			else {
				q = strcpy1(q, "DB(0xef);");
			}
			isWaitBrase0 = 0;
			continue;
		}
		if (*p == '}') {
			p = skipSpace(p + 1);
			if (braseDepth > 0 && brase[braseDepth].typ == ELEMENT_FOR) {
				if ((brase[braseDepth].flg0 & 2) != 0) { // using remark
					q = strcpy1(q, "DB(0xfe,0x01,0x01);");
				}
				if (brase[braseDepth].p1[0] != ')') {
					askaPass1Sub(w, brase[braseDepth].p1, &q, ident, 0);
				}
				askaPass1Sub(w, brase[braseDepth].p0, &q, ident, 0);
				if (w->sp == 0) {
					ele0[0].typ = ELEMENT_CONST;
					ele0[0].iValue = -1;
					w->sp++;
				}
				askaPass1UseR3F(&ele0[0], q, w->obuf);
				if (ele0[0].typ == ELEMENT_CONST) {
					if ((ele0[0].iValue & 1) != 0) {
						q = strcpy1(q, "JMP(CONTINUE);");
					}
				} else if (ele0[0].typ == ELEMENT_REG) {
					sprintf(q, "CND(R%02X);JMP(CONTINUE);", ele0[0].iValue);
					q += 23;
				} else {
					q = strcpy1(q, "DB(0xef);");
				}
				if ((brase[braseDepth].flg0 & 1) != 0)
					q = strcpy1(q, "ENDLOOP();");
				else
					q = strcpy1(q, "ENDLOOP0();");
			}
			if (braseDepth > 0 && brase[braseDepth].typ == ELEMENT_IF) {
				if (strncmp1(p, "else") == 0) {
					p += 4;
					braseDepth--;
					// �ϐ���`�̔j��.
					for (i = 0; i < MAXIDENTS; i++) {
						if (ident[i].len0 > 0 && ident[i].depth > braseDepth)
							ident[i].len0 = 0;
					}
					braseDepth++;
					brase[braseDepth].typ = ELEMENT_ELSE;
					isWaitBrase0 = 1;
					q = strcpy1(q, "JMP(lbstk1(1,1));LB(0,lbstk1(1,0));");
					continue;
				} else {
					q = strcpy1(q, "LB(0,lbstk1(1,0));lbstk3(1);");
				}
			}
			if (braseDepth > 0 && brase[braseDepth].typ == ELEMENT_ELSE) {
				q = strcpy1(q, "LB(0,lbstk1(1,1));lbstk3(1);");
			}
			if (braseDepth > 0) {
				braseDepth--;
				// �ϐ���`�̔j��.
				for (i = 0; i < MAXIDENTS; i++) {
					if (ident[i].len0 > 0 && ident[i].depth > braseDepth)
						ident[i].len0 = 0;
				}
			} else {
				q = strcpy1(q, "DB(0xef);");
			}
			continue;
		}

		flag = 0;
#if 1
		for (r = p; *r != '\0' && *r != '\r' && *r != '\n' && *r != ';'; r++) {
			if (*r == '\"')
				flag = 1;
		}
		if (/* *r != ';' || */ flag != 0) {
			while (p < r)
				*q++ = *p++;
			continue;
		}
#endif

#if 0
		if (strncmp1(p, "int") == 0 && p[3] <= ' ') {
			i = idendef(p + 4, iden);
			if (i != 0) goto err;
			r = p;
			continue;
		}
#endif

		if (strncmp1(p, "SInt32") == 0 && p[6] <= ' ') {
			p += 6;
			k = 'R';
identLoop:
			for (;;) {
				p = skipSpace(p);
				r = p;
				for (;;) {
					if (*p <= ' ' || isSymbolChar(*p)) break;
					p++;
				}
				j = p - r;
				p = skipSpace(p);
				if (*p != ':') goto errSkip;
				p = skipSpace(p + 1);
				if (j == 0) goto errSkip;
				if (*p != k || hexChar(p[1]) < 0 || hexChar(p[2]) < 0 || (!isSymbolChar(p[3]) && p[3] > ' ')) goto errSkip;
				for (i = 0; i < MAXIDENTS; i++) {
					if (ident[i].len0 == j && strncmp(ident[i].p0, r, j) == 0) break;
				}
				if (i < MAXIDENTS) {
					fprintf(stderr, "aska: SInt32/VoidPtr: redefine: %.*s\n", j, r);
					goto identSkip;
				} else {
					for (i = 0; i < MAXIDENTS; i++) {
						if (ident[i].len0 == 0) break;
					}
					if (i >= MAXIDENTS) {
						fputs("aska: SInt32/VoidPtr: too many idents\n", stderr);
						exit(1);
					}
					ident[i].len0 = j;
					ident[i].p0 = r;
					ident[i].ele.iValue = hexChar(p[1]) << 4 | hexChar(p[2]);
					ident[i].depth = braseDepth;
					if (*p == 'R')
						ident[i].ele.typ = ELEMENT_REG;
					if (*p == 'P')
						ident[i].ele.typ = ELEMENT_PREG;
				}
identSkip:
				p = skipSpace(p + 3);
				if (*p == ';') break;
				if (*p != ',') goto errSkip;
				p++;
			}
			continue;
		}
		if (strncmp1(p, "VoidPtr") == 0 && p[7] <= ' ') {
			p += 7;
			k = 'P';
			goto identLoop;
		}
		static UCHAR *table_a[] = {
			"PLIMM", "CND", "LMEM", "SMEM", "PADD", "PADDI", "PDIF", NULL
			// �����ƒǉ����Ă��������A�Ƃ肠���������I�ɂ����܂łƂ���.
		};
		for (j = 0; (r = table_a[j]) != NULL; j++) {
			k = strlen(r);
			if (strncmp(p, r, k) == 0 && isSymbolChar(p[k]) != 0) break;
		}
		if (r != NULL) {
			if (p[k] == '%' && p[k + 1] == '(') {
				strcpy(q, r);
				q += k;
				p += k + 1;
				goto through;
			}
			if (p[k] == '(') {
				strcpy(q, r);
				q += k;
				p += k;
				j = 0;
				while (*p != ';' && *p != '\n' && *p != '\r' && *p != '\0') {
					if (j == 1 && isSymbolChar(*p) == 0 && *p > ' ') {
						for (i = 0; i < MAXIDENTS; i++) {
							k = ident[i].len0;
							if (k == 0) continue;
							if (strncmp(ident[i].p0, p, k) == 0 && (isSymbolChar(p[k]) != 0 || p[k] <= ' ')) break;
						}
						if (i < MAXIDENTS && k > 0) {
							p += k;
							if (ident[i].ele.typ == ELEMENT_REG) {
								sprintf(q, "R%02X", ident[i].ele.iValue);
								q += 3;
							}
							if (ident[i].ele.typ == ELEMENT_PREG) {
								sprintf(q, "P%02X", ident[i].ele.iValue);
								q += 3;
							}
							j = 0;
							continue;
						}
					}
					if (isSymbolChar(*p) != 0) j = 1;
					if (isSymbolChar(*p) == 0 && *p > ' ') j = 0;
					*q++ = *p++;
				}
				if (*p == ';') *q++ = *p++;
				continue;
			}
		}
		r = askaPass1Sub(w, p, &q, ident, 0);
		if (w->sp <= -2) {
			s = p;
			if (w->ele0[0].typ == ELEMENT_BREAK && strncmp1(p, "break") == 0) {
				p = skipSpace(p + 5);
				if (*p != ';') goto errSkip;
				p++;
				q = strcpy1(q, "JMP(BREAK);");
				for (j = braseDepth; j >= 0 && brase[j].typ != ELEMENT_FOR; j--);
				if (j >= 0) brase[j].flg0 |= 1; // using break
				continue;
			}
			if (w->ele0[0].typ == ELEMENT_CONTIN && strncmp1(p, "continue") == 0) {
				p = skipSpace(p + 8);
				if (*p != ';') goto errSkip;
				p++;
				q = strcpy1(q, "JMP(CONTINUE);");
				continue;
			}
			if (w->ele0[0].typ == ELEMENT_GOTO && strncmp1(p, "goto") == 0) {
				p = skipSpace(p + 4);
				q = strcpy1(q, "JMP(L_");
				while (*p > ' ' && !isSymbolChar(*p)) *q++ = *p++;
				if (*p != ';') goto errSkip;
				p++;
				*q++ = ')';
				*q++ = ';';
				continue;
			}
			if (w->ele0[0].typ == ELEMENT_IF && strncmp1(p, "if") == 0) {
				p = skipSpace(p + 2);
				if (*p != '(') goto errSkip;
				t = skipSpace(p + 1);
				p = stringToElements(w->ele0, t, 0);
				if (*p != ')') goto errSkip;
				p = skipSpace(p + 1);
				j = 0;
				if (strncmp1(p, "continue") == 0) { j = 1; p += 8; }
				if (strncmp1(p, "break")    == 0) { j = 2; p += 5; }
				if (strncmp1(p, "goto")     == 0) { j = 3; p += 4; }
				if (j > 0) {
					askaPass1Sub(w, t, &q, ident, 0);
					if (w->sp <= 0) goto errSkip;
					askaPass1UseR3F(&ele0[0], q, w->obuf);
					if (ele0[0].typ == ELEMENT_CONST) {
						if ((ele0[0].iValue & 1) == 0)
							j |= 8;
					} else if (ele0[0].typ == ELEMENT_REG) {
						sprintf(q, "CND(R%02X);", ele0[0].iValue);
						q += 9;
					} else
						goto errSkip;
					p = skipSpace(p);
					if (j == 1) q = strcpy1(q, "JMP(CONTINUE");
					if (j == 2) q = strcpy1(q, "JMP(BREAK");
					if (j == 3) q = strcpy1(q, "JMP(L_");
					while (*p > ' ' && !isSymbolChar(*p)) *q++ = *p++;
					if (*p != ';') goto errSkip;
					p++;
					*q++ = ')';
					*q++ = ';';
					if (j == 2) {
						for (j = braseDepth; j >= 0 && brase[j].typ != ELEMENT_FOR; j--);
						if (j >= 0) brase[j].flg0 |= 1; // using break
					}
					continue;
				}
				q = strcpy1(q, "lbstk2(1,2);");
				askaPass1Sub(w, t, &q, ident, 256); // 256:�ے�^�ɂ���.
				if (w->sp <= 0) goto errSkip;
				askaPass1UseR3F(&ele0[0], q, w->obuf);
				if (ele0[0].typ == ELEMENT_CONST) {
					if ((ele0[0].iValue & 1) == 0) {
						strcpy(q, "JMP(lbstk1(1,0));");
						q += 17;
					}
				} else if (ele0[0].typ == ELEMENT_REG) {
					sprintf(q, "CND(R%02X);JMP(lbstk1(1,0));", ele0[0].iValue);
					q += 9+17;
				} else
					goto errSkip;
				brase[braseDepth + 1].typ = ELEMENT_IF;
				isWaitBrase0 = 1;
				braseDepth++;
				continue;
			}
			if (w->ele0[0].typ == ELEMENT_FOR && strncmp1(p, "for") == 0) {
				p = skipSpace(p + 3);
				if (*p != '(') goto errSkip;
				p = skipSpace(p + 1);
				qq = q;
				p = askaPass1Sub(w, p, &q, ident, 0);
				if (*p != ';') goto errSkip;
				i = -1;
				j = 0;
				if (w->ele1[0].typ == ELEMENT_REG && w->ele1[1].typ == ELEMENT_CONST && w->ele1[2].typ == ELEMENT_EQUAL && w->ele1[3].typ == ELEMENT_TERMINATE) {
					i = w->ele1[0].iValue;
					j = w->ele1[1].iValue;
				}
				p = skipSpace(p + 1);
				brase[braseDepth + 1].typ = ELEMENT_FOR;
				brase[braseDepth + 1].flg0 = 0;
				brase[braseDepth + 1].p0 = p;
				p = stringToElements(w->ele0, p, 0);
				substitute(w->ele0, ident);
				if (*p != ';') goto errSkip;
				k = 0;
				if (i >= 0) {
					if (w->ele0[0].typ == ELEMENT_REG && w->ele0[0].iValue == i &&
						w->ele0[1].typ == ELEMENT_NOTEQL && w->ele0[2].typ == ELEMENT_CONST && w->ele0[3].typ == ELEMENT_TERMINATE)
					{
						k = w->ele0[2].iValue;
						j = k - j;
					} else
						i = -1;
				}
				p = skipSpace(p + 1);
				brase[braseDepth + 1].p1 = p;
				p = stringToElements(w->ele0, p, 0);
				substitute(w->ele0, ident);
				if (*p != ')') goto errSkip;
				p = skipSpace(p + 1);
				if (i >= 0) {
					if (j > 0) {
						if (w->ele0[0].typ == ELEMENT_REG && w->ele0[0].iValue == i && w->ele0[1].typ == ELEMENT_PPLUS)
							;
						else if (w->ele0[1].typ == ELEMENT_PPLUS && w->ele0[1].typ == ELEMENT_REG && w->ele0[1].iValue == i)
							;
						else
							i = -1;
					}
					if (j < 0) {
						if (w->ele0[0].typ == ELEMENT_REG && w->ele0[0].iValue == i && w->ele0[1].typ == ELEMENT_MMINUS)
							;
						else if (w->ele0[1].typ == ELEMENT_MMINUS && w->ele0[1].typ == ELEMENT_REG && w->ele0[1].iValue == i)
							;
						else
							i = -1;
					}
				}
				if (i >= 0) {
					brase[braseDepth + 1].flg0 |= 2; // using remark
					for (j = q - qq; j >= 0; j--)
						qq[j + 36] = qq[j];
					char tmpStr[40];
					sprintf(tmpStr, "DB(0xfe,0x05,0x02);DDBE(0x%08X);", k);
					memcpy(qq, tmpStr, 36);
					q += 36;
				}
				isWaitBrase0 = 1;
				braseDepth++;
				q = strcpy1(q, "LOOP();");
				continue;
			}
#if 0
			if (strncmp1(p, "askaDirect") == 0) {
				p = skipSpace(p + 10);
				if (*p != '(') goto errSkip;
				p = skipSpace(p + 1);
				i = 0;
				while ('0' <= *p && *p <= '9') {
					i = i * 10 + (*p - '0');
					p++;
				}
				p = skipSpace(p);
				if (p != ',') goto errSkip;
				p = askaPass1Sub(w, p, &q, ident, 0);
				if (*p != ')') goto errSkip;


				if (strncmp1(p, "32") == 0) { i = 32; p += 2; }
				else if (strncmp1(p, "32") == 0) { i = 32; p += 2; }

			}
#endif

errSkip:
			w->sp = -2;
			p = s;
		}
		if (w->sp <= -2 || p >= r) {
through:
			while (*p != ';' && *p != '\n' && *p != '\r' && *p != '\0')
				*q++ = *p++;
			if (*p == ';') *q++ = *p++;
			continue;
#if 0
			if (!(q - 9 >= w->obuf && strncmp1(q - 9, "DB(0xef);") == 0)) {
				strcpy(q, "DB(0xef);");
				q += 9;
			}
			r++; // �˔j�ł��Ȃ��������˔j������
#endif
		}
		p = r;
		if (*p == ';')
			p++;
		else {
			q = strcpy1(q, "DB(0xef);");
		}
	}
	if (braseDepth != 0)
		q = strcpy1(q, "DB(0xef);\n");
	goto fin;
err:
	q = NULL;
fin:
	return q;
}

int idendef(const char *p, void *dmy)
{
	return 1;
}

const UCHAR *idenref(const UCHAR *p, UCHAR *q, void *iden)
{
	while (*p != ';') {
		if (*p <= ' ') {
			p++;
			continue;
		}
		if ('0' <= *p && *p <= '9') {
			while (*p > ' ' && !isSymbolChar(*p))
				*q++ = *p++;
			continue;
		}
		if (isSymbolChar(*p)) {
			*q++ = *p++;
			if (*p == ';') {
				*q++ = ' ';
				*q++ = ' ';
				break;
			}
			if (isSymbolChar(*p))
				*q++ = *p++;
			else
				*q++ = ' ';
			if (*p == ';') {
				*q++ = ' ';
				break;
			}
			if (isSymbolChar(*p))
				*q++ = *p++;
			else
				*q++ = ' ';
			continue;
		}
		while (*p > ' ' && !isSymbolChar(*p))
			*q++ = *p++;
		continue;
	}
	*q++ = ';';
	*q++ = ' ';
	*q++ = ' ';
	*q = '\0';
	return p + 1;
}

const UCHAR *skipSpace(const UCHAR *p)
{
	while (*p == ' ' || *p == '\t')
		p++;
	return p;
}

/*** prepro ***/

int prepro(struct Work *w)
{
	return 0;
}

/*** lbstk ***/

#define	LBSTK_LINFOSIZE	1024

struct LbstkLineInfo {
	unsigned int maxline, line0;
	UCHAR file[1024];
};

int lbstkPass0(struct Work *w, struct LbstkLineInfo *linfo);
UCHAR *lbstkPass1(struct Work *w, struct LbstkLineInfo *linfo);
const UCHAR *skip0(const UCHAR *p);
const UCHAR *skip1(const UCHAR *p);
const UCHAR *skip2(const UCHAR *p);

int lbstk(struct Work *w)
{
	struct LbstkLineInfo *lineInfo = malloc(LBSTK_LINFOSIZE * sizeof (struct LbstkLineInfo));
	const UCHAR *cs;
	int i, r;

	for (i = 0; i < LBSTK_LINFOSIZE; i++) {
		lineInfo[i].maxline = 0;
		lineInfo[i].line0 = 0;
		lineInfo[i].file[0] = '\0';
	}
	lineInfo[LBSTK_LINFOSIZE - 1].line0 = 0xffffffff;

	r = lbstkPass0(w, lineInfo);
	if (r != 0) goto fin;
	cs = searchArg(w, "lst:" );
	if (cs != NULL) {
		FILE *fp = fopen(cs, "w");
		for (i = 0; lineInfo[i].file[0] != '\0'; i++) {
			fprintf(fp, "%6u - %6u : \"", lineInfo[i].line0, lineInfo[i].line0 + lineInfo[i].maxline);
			cs = strchr(lineInfo[i].file, '\"');
			fwrite(lineInfo[i].file, 1, cs - lineInfo[i].file, fp);
			fputs("\"\n", fp);
		}
		fclose(fp);
	}
	r = putBuf(w->argOut, w->obuf, lbstkPass1(w, lineInfo));
fin:
	return r;
}

int lbstkPass0(struct Work *w, struct LbstkLineInfo *linfo)
{
	const UCHAR *p = w->ibuf, *q;
	int i, r = 0;
	unsigned int line;
	for (;;) {
		p = strstr(p, "lbstk6(");
		if (p == NULL) break;
		p = strchr(p + 7, '\"');
		if (p == NULL) goto err1;
		p++;
		q = strchr(p, '\"');
		if (q == NULL) goto err1;
		q++;
		if (q - p > 1024) goto err2;
		for (i = 0; linfo[i].line0 != 0xffffffff; i++) {
			if (linfo[i].file[0] == '\0') {
				memcpy(linfo[i].file, p, q - p);
				break;
			}
			if (memcmp(linfo[i].file, p, q - p) == 0)
				break;
		}
		if (linfo[i].line0 == 0xffffffff) goto err3;
		p = strchr(q, ',');
		if (p == NULL) goto err1;
		p++;
		line = getConstInt1(&p);
		if (linfo[i].maxline < line)
			linfo[i].maxline = line;
	}
	line = 0;
	for (i = 0; linfo[i].file[0] != '\0'; i++) {
		linfo[i].line0 = line;
		line += linfo[i].maxline + 100;
		line = ((line + 999) / 1000) * 1000; /* 1000�P�ʂɐ؂�グ�� */
	}
	goto fin;

err1:
	r = 1;
	fputs("lbstk5: syntax error\n", stderr);
	goto fin;

err2:
	r = 2;
	fputs("lbstk5: too long filename\n", stderr);
	goto fin;

err3:
	r = 3;
	fputs("lbstk5: too many filenames\n", stderr);
	goto fin;

fin:
	return r;
}

UCHAR *lbstkPass1(struct Work *w, struct LbstkLineInfo *linfo)
{
	int *stk[4], stkp[4], i;
	int nextlb, nowlb0, nowlbsz;
	const UCHAR *p, *r;
	for (i = 0; i < 4; i++) {
		stk[i] = malloc(65536 * sizeof (int));
		stkp[i] = 0;
	}

	nextlb = 0;
	p = strstr(w->ibuf, "lbstk0(");
	if (p != NULL) {
		p += 7;
		nextlb = getConstInt1(&p);
	}
	nowlb0 = -1;
	nowlbsz = -1;
	if ((w->flags & FLAGS_PUT_MANY_INFO) != 0)
		printf("lbstk0=%d, flags=%d\n", nextlb, w->flags);

	p = w->ibuf;
	UCHAR *q = w->obuf;
	int j, sz, ofs;
	unsigned int line;
	while (*p != '\0') {
		if (*p == 'l' && p[1] == 'b') {
			if (strncmp1(p, "lbstk0(") == 0) {
				p = skip2(p + 7);
				continue;
			}
			if (strncmp1(p, "lbstk2(") == 0) {
				p += 7;
				j = getConstInt1(&p); if (*p == ',') p++;
				sz = getConstInt1(&p); p = skip2(p);
				stk[j][(stkp[j])++] = nextlb;
				nextlb += sz;
				continue;
			}
			if (strncmp1(p, "lbstk1(") == 0) {
				p += 7;
				j = getConstInt1(&p); if (*p == ',') p++;
				ofs = getConstInt1(&p); p = skip1(p);
				if (stkp[j] <= 0) {
					fprintf(stderr, "lbstk1: error: stkp[%d] < 0\n", j);
					goto err;
				}
				i = stk[j][stkp[j] - 1];
				sprintf(q, "%d", i + ofs);
				while (*q != '\0') q++;
				continue;
			}
			if (strncmp1(p, "lbstk3(") == 0) {
				p += 7;
				j = getConstInt1(&p); p = skip1(p);
				if (stkp[j] <= 0) {
					fprintf(stderr, "lbstk3: error: stkp[%d] < 0\n", j);
					goto err;
				}
				i = (stkp[j])--;
				continue;
			}
			if (strncmp1(p, "lbstk4(") == 0) {
				p += 7;
				nowlbsz = getConstInt1(&p);
				nowlb0 = nextlb;
				nextlb += nowlbsz;
				p = skip2(p);
				continue;
			}
			if (strncmp1(p, "lbstk5(") == 0) {
				p += 7;
				i = getConstInt1(&p);
				p = skip1(p);
				if (i >= nowlbsz) {
					fprintf(stderr, "lbstk5: error: i=%d (sz=%d)\n", i, nowlbsz);
					goto err;
				}
				sprintf(q, "%d", nowlb0 + i);
				while (*q != '\0') q++;
				continue;
			}
			if (strncmp1(p, "lbstk6(") == 0) {
				p = strchr(p + 7, '\"') + 1;
				r = strchr(p, '\"') + 1;
				for (i = 0; memcmp(linfo[i].file, p, r - p) != 0; i++);
				p = strchr(r, ',') + 1;
				line = getConstInt1(&p) + linfo[i].line0;
				sprintf(q, "DB(0x%02X,0x%02X,0x%02X,0x%02X",
					(line >> 24) & 0xff,
					(line >> 16) & 0xff,
					(line >>  8) & 0xff,
					 line        & 0xff
				);
				while (*q != '\0') q++;
				continue;
			}
		}
		*q++ = *p++;
	}
	for (i = 0; i < 4; i++) {
		if (stkp[i] != 0) {
			fprintf(stderr, "lbstk: error: stkp[%d] != 0\n", i);
			goto err;
		}
	}
	goto fin;

err:
	q = NULL;
fin:
	return q;
}


const UCHAR *skip0(const UCHAR *p)
{
	while (*p != '\0' && *p != ',') p++;
	if (*p == ',') p++;
	return p;
}

const UCHAR *skip1(const UCHAR *p)
{
	while (*p != '\0' && *p != ')') p++;
	if (*p == ')') p++;
	return p;
}

const UCHAR *skip2(const UCHAR *p)
{
	p = skip1(p);
	while (*p != '\0' && *p != ';') p++;
	if (*p == ';') p++;
	return p;
}

/*** db2bin ***/

int db2binSub0(struct Work *w, const UCHAR **pp, UCHAR **qq);
UCHAR *db2binSub1(const UCHAR **pp, UCHAR *q, int width);
UCHAR *db2binSub2(UCHAR *p0, UCHAR *p1); // ���x���ԍ��̕t������.

int db2bin(struct Work *w)
{
	const UCHAR *pi = w->ibuf;
	UCHAR *q = w->obuf, *pj = w->jbuf + JBUFSIZE - 1;
	int r = 0, i = 0, j;
	*pj = '\0';
	while ((*pi != '\0' || i > 0) && q != NULL) {
		if (i > 0) {
			pj -= i;
			for (j = 0; j < i; j++)
				pj[j] = w->jbuf[j];
		}
		if (*pj != '\0')
			i = db2binSub0(w, (const UCHAR **) &pj, &q);
		else
			i = db2binSub0(w, &pi, &q);
	}
	putchar(0); // ���ꂪ�Ȃ���GCC�����������Ȃ�.
	if ((w->flags & 1) == 0 && q != NULL)
		q = db2binSub2(w->obuf + 2, q);
	if (*pi == '\0' && q != NULL /* && (w->obuf[4] & 0xf0) != 0 */) {
	//	fputs("osecpu binary first byte error.\n", stderr);
		for (r = q - w->obuf - 1; r >= 2; r--)
			w->obuf[r + 1] = w->obuf[r];
		w->obuf[2] = 0x00;
		q++;
	}
	r = 1;
	if (*pi == '\0' && q != NULL /* && (w->obuf[4] & 0xf0) == 0 */)
		r = putBuf(w->argOut, w->obuf, q);
fin:
	return r;
}

int db2binSub0(struct Work *w, const UCHAR **pp, UCHAR **qq)
{
	const UCHAR *p = *pp, *r;
	UCHAR *qj = w->jbuf, *q = *qq;
	int i, j;
	while (*p != '\0' && q != NULL) {
		if (*p <= ' ') {
			p++;
			continue;
		}
		if (qj > w->jbuf) break;
		
		static UCHAR *table0[] = {
			" 08DB(", " 16DWBE(", " 32DDBE(", "-32DDLE(", "-32DD(",
			" 01D1B(", " 02D2B(", " 04D4B(", " 08D8B(", " 12D12B(",
			" 16D16B(", " 20D20B(", " 24D24B(", " 28D28B(", " 32D32B(",
			NULL
		};
		for (i = 0; table0[i] != NULL; i++) {
			if (strncmp1(p, table0[i] + 3) == 0) break;
		}
		if (table0[i] != NULL) {
			p += strlen(table0[i] + 3);
			j = (table0[i][1] - '0') * 10 + (table0[i][2] - '0');
			if (table0[i][0] == '-')
				j = - j;
			q = db2binSub1(&p, q, j);
			continue;
		}

		static UCHAR *table1[] = {
			"0OSECPU_HEADER(",		"DB(0x05,0xe1);",
			"0NOP(",				"DB(0x00);",
			"2LB(",					"DB(0x01,%0); DDBE(%1);",
			"2LIMM(",				"DB(0x02,%0); DDBE(%1);",
			"2PLIMM(",				"DB(0x03,%0-0x40); DDBE(%1);",
			"1CND(",				"DB(0x04,%0);",
			"4LMEM(",				"DB(0x08,%0); DDBE(%1); DB(%2-0x40,%3);",
			"4SMEM(",				"DB(0x09,%0); DDBE(%1); DB(%2-0x40,%3);",
			"4PADD(",				"DB(0x0e,%0-0x40); DDBE(%1); DB(%2-0x40,%3);",
			"4PDIF(",				"DB(0x0f,%0); DDBE(%1); DB(%2-0x40,%3-0x40);",
			"2CP(",					"DB(0x10,%0,%1,0xff);",
			"3OR(",					"DB(0x10,%0,%1,%2);",
			"3XOR(",				"DB(0x11,%0,%1,%2);",
			"3AND(",				"DB(0x12,%0,%1,%2);",
			"3SBX(",				"DB(0x13,%0,%1,%2);",
			"3ADD(",				"DB(0x14,%0,%1,%2);",
			"3SUB(",				"DB(0x15,%0,%1,%2);",
			"3MUL(",				"DB(0x16,%0,%1,%2);",
			"3SHL(",				"DB(0x18,%0,%1,%2);",
			"3SAR(",				"DB(0x19,%0,%1,%2);",
			"3DIV(",				"DB(0x1a,%0,%1,%2);",
			"3MOD(",				"DB(0x1b,%0,%1,%2);",
			"2PCP(",				"DB(0x1e,%0-0x40,%1-0x40);",
			"3CMPE(",				"DB(0x20,%0,%1,%2);",
			"3CMPNE(",				"DB(0x21,%0,%1,%2);",
			"3CMPL(",				"DB(0x22,%0,%1,%2);",
			"3CMPGE(",				"DB(0x23,%0,%1,%2);",
			"3CMPLE(",				"DB(0x24,%0,%1,%2);",
			"3CMPG(",				"DB(0x25,%0,%1,%2);",
			"3TSTZ(",				"DB(0x26,%0,%1,%2);",
			"3TSTNZ(",				"DB(0x27,%0,%1,%2);",
			"3PCMPE(",				"DB(0x28,%0,%1-0x40,%2-0x40);",
			"3PCMPNE(",				"DB(0x29,%0,%1-0x40,%2-0x40);",
			"3PCMPL(",				"DB(0x2a,%0,%1-0x40,%2-0x40);",
			"3PCMPGE(",				"DB(0x2b,%0,%1-0x40,%2-0x40);",
			"3PCMPLE(",				"DB(0x2c,%0,%1-0x40,%2-0x40);",
			"3PCMPG(",				"DB(0x2d,%0,%1-0x40,%2-0x40);",
			"2EXT(",				"DB(0x2f,%0); DWBE(%1);",

			"1DBGINFO0(",			"DB(0xfe,0x05,0x00); DDBE(%0);",
			"0DBGINFO1(",			"DB(0xfe,0x01,0x00);",
			"1DBGINFO(",			"DBGINFO0(%0);",
			"1JMP(",				"PLIMM(P3F, %0);",
			"1PJMP(",				"PCP(P3F, %0);",
			"4PADDI(",				"LIMM(R3F, %3); PADD(%0, %1, %2, R3F);",
			"3ORI(",				"LIMM(R3F, %2); OR( %0, %1, R3F);",
			"3XORI(",				"LIMM(R3F, %2); XOR(%0, %1, R3F);",
			"3ANDI(",				"LIMM(R3F, %2); AND(%0, %1, R3F);",
			"3SBXI(",				"LIMM(R3F, %2); SBX(%0, %1, R3F);",
			"3ADDI(",				"LIMM(R3F, %2); ADD(%0, %1, R3F);",
			"3SUBI(",				"LIMM(R3F, %2); SUB(%0, %1, R3F);",
			"3MULI(",				"LIMM(R3F, %2); MUL(%0, %1, R3F);",
			"3SHLI(",				"LIMM(R3F, %2); SHL(%0, %1, R3F);",
			"3SARI(",				"LIMM(R3F, %2); SAR(%0, %1, R3F);",
			"3DIVI(",				"LIMM(R3F, %2); DIV(%0, %1, R3F);",
			"3MODI(",				"LIMM(R3F, %2); MOD(%0, %1, R3F);",
			"3CMPEI(",				"LIMM(R3F, %2); CMPE( %0, %1, R3F);",
			"3CMPNEI(",				"LIMM(R3F, %2); CMPNE(%0, %1, R3F);",
			"3CMPLI(",				"LIMM(R3F, %2); CMPL( %0, %1, R3F);",
			"3CMPGEI(",				"LIMM(R3F, %2); CMPGE(%0, %1, R3F);",
			"3CMPLEI(",				"LIMM(R3F, %2); CMPLE(%0, %1, R3F);",
			"3CMPGI(",				"LIMM(R3F, %2); CMPG( %0, %1, R3F);",
			NULL
		};
		for (i = 0; table1[i] != NULL; i += 2) {
			if (strncmp1(p, table1[i] + 1) == 0) break;
		}
		if (table1[i] != NULL) {
			const UCHAR *p0[10], *p1[10], *p00 = p;
			p += strlen(table1[i] + 1);
			for (j = 0; j < table1[i][0] - '0'; j++) {
				while(*p == ' ' || *p == '\t') p++;
				if (*p == ')') { p = p00; goto err; }
				p0[j] = p;
				while (*p != ',' && *p != ')' && *p != '(' && *p != '\'' && *p != '\"' && *p != '/') p++;
				if (*p != ',' && *p != ')') {
					p = stringToElements(w->ele0, p0[j], 1);
				}
				p1[j] = p;
				if (*p == ',') p++;
			}
			while(*p == ' ' || *p == '\t') p++;
			if (*p != ')') { p = p00; goto err; }
			p++;
			if (*p != ';') { p = p00; goto err; }
			p++;
			for (r = table1[i + 1]; *r != '\0'; ) {
				if (*r != '%') {
					*qj++ = *r++;
					continue;
				}
				j = r[1] - '0';
				r += 2;
				memcpy(qj, p0[j], p1[j] - p0[j]);
				qj += p1[j] - p0[j];
			}
			continue;
		}
		if (*p == ';') {
			p++;
			continue;
		}
err:
		fputs("db2bin: error: ", stderr);
		for (i = 0; p[i] != '\0'; i++) {
			if (i < 40) fputc(p[i], stderr);
			if (p[i] == ';') break;
		}
		fputc('\n', stderr);
		q = NULL;
		break;
	}
	*pp = p;
	*qq = q;
	return qj - w->jbuf;
}

UCHAR *db2binSub1(const UCHAR **pp, UCHAR *q, int width)
{
	const UCHAR *p = *pp;
	char errflg = 0;
	int i, j;
	while (*p != '\0' && *p != ')') {
		if (*p <= ' ' || *p == ',') { p++; continue; }
		if (width == 8 && *p == '\'') {
			p++;
			while (*p != '\0' && *p != '\'') {
				if (*p == '\\') {
					p++;
					if (*p == 'n') { p++; *q++ = '\n'; continue; }
					if (*p == 't') { p++; *q++ = '\t'; continue; }
					if (*p == 'r') { p++; *q++ = '\r'; continue; }
				}
				*q++ = *p++;
			}
			if (*p == '\'') p++;
			continue;
		}
		static UCHAR *table[] = {
			"02T_SINT8", "03T_UINT8", "04T_SINT16", "05T_UINT16", "06T_SINT32", "07T_UINT32",
			"08T_SINT4", "09T_UINT4", "10T_SINT2", "11T_UINT2", "12T_SINT1", "13T_UINT1",
			"14T_SINT12", "15T_UINT12", "16T_SINT20", "17T_UINT20", "18T_SINT24", "19T_UINT24",
			"20T_SINT28", "21T_UINT28", NULL
		};
		for (i = 0; table[i] != NULL; i++) {
			if (strncmp1(p, table[i] + 2) == 0) {
				j = strlen(table[i] + 2);
				if (p[j] <= ' ' || isSymbolChar(p[j])) {
					p += j;
					i = (table[i][0] - '0') * 10 + (table[i][1] - '0');
					goto put_i;
				}
			}
		}
		*pp = p;
		i = getConstInt(&p, &errflg, 1 << 8 | 2); // �J���}�Ŏ~�܂�, R00�𐔎��ɓW�J.
		if (errflg != 0) {
			fprintf(stderr, "db2binSub: error: %.20s\n", *pp);
			exit(1);
			break;
		}
put_i:
		if (width == -32) {
			*q++ =  i        & 0xff;
			*q++ = (i >>  8) & 0xff;
			*q++ = (i >> 16) & 0xff;
			*q++ = (i >> 24) & 0xff;
		}
		if (width > 0) {
			static int buf = 1;
			for (j = width - 1; j >= 0; j--) {
				buf = buf << 1 | ((i >> j) & 1);
				if (buf >= 0x100) {
					*q++ = buf & 0xff;
					buf = 1;
				}
			}
		}
	}
	if (*p == ')') p++;
	*pp = p;
	return q;
}

int db2binDataWidth(int i)
{
	int r = -1;
	i >>= 1;
	if (i == 0x0002 / 2) r =  8;
	if (i == 0x0004 / 2) r = 16;
	if (i == 0x0006 / 2) r = 32;
	if (i == 0x0008 / 2) r =  4;
	if (i == 0x000a / 2) r =  2;
	if (i == 0x000c / 2) r =  1;
	if (i == 0x000e / 2) r = 12;
	if (i == 0x0010 / 2) r = 20;
	if (i == 0x0012 / 2) r = 24;
	if (i == 0x0014 / 2) r = 28;
	return r;
}

int db2binSub2Len(const UCHAR *src)
{
	int i = 0, j;
	if (*src == 0x00) i = 1;
	if (0x01 <= *src && *src < 0x04) i = 6;
	if (*src == 0x04) i = 2;
	if (0x08 <= *src && *src < 0x0d) i = 8 + src[7] * 4;
	if (0x0e <= *src && *src < 0x10) i = 8;
	if (0x10 <= *src && *src < 0x2e) i = 4;
	if (0x1c <= *src && *src < 0x1f) i = 3;
	if (*src == 0x1f) i = 11;
	if (*src == 0x2f) i = 4 + src[1];
	if (0x3c <= *src && *src <= 0x3d) i = 7;
	if (*src == 0xf0 || *src == 0x34) {
		j = src[1] << 24 | src[2] << 16 | src[3] << 8 | src[4];
		i = src[5] << 24 | src[6] << 16 | src[7] << 8 | src[8];
		j = db2binDataWidth(j);
		if (j <= 0)
			fputs("db2binSub2Len: error: F0\n", stderr);
		if (((i * j) & 7) != 0)
			fprintf(stderr, "db2binSub2Len: error: F0 (align 8bit) bit=%d len=%d\n", j, i);
		i = 9 + ((i * j) >> 3);
	}
	if (0xf4 <= *src && *src <= 0xf7 || 0x30 <= *src && *src <= 0x33) i = 4; // �b�菈�u.
	if (0xec <= *src && *src <= 0xef) i = 1;
	if (*src == 0xfe) i = 2 + src[1];
	if (*src == 0xff) {
		if (src[1] == 0x00) { i = strlen(src + 3) + 4; }
		if (src[1] == 0x01) {
			i = 3;
			while ((src[i + 0] | src[i + 1] | src[i + 2] | src[i + 3]) != 0) i += 4;
			i += 4;
		}
	}
	if (i == 0) {
		fprintf(stderr, "db2binSub2Len: unknown opecode: %02X\n", *src);
		exit(1);
	}
	return i;
}

UCHAR *db2binSub2(UCHAR *p0, UCHAR *p1)
{
	int t[MAXLABEL], i, j;
	UCHAR *p, *q, *r;
	for (j = 0; j < MAXLABEL; j++)
		t[j] = -1;

	for (p = p0; p < p1; ) {
		if (*p == 0x01 || *p == 0x03) {
			j = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
			if (j >= MAXLABEL || j < 0) {
				fputs("label number over error. check LMEM/SMEM/PLMEM/PSMEM.\n", stderr);
				exit(1);
			}
			if (*p == 0x03)
				t[j]--;
		}
		p += db2binSub2Len(p);
	}
	i = 0;
	for (p = p0; p < p1; ) {
		if (*p == 0x01) {
			j = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
			if (t[j] >= 0) {
//printf("redefine label:%06X at %06X\n", j, p - p0);
			}
			if (t[j] < -1) {
//printf("LB:%06X->%06X at %06X\n", j, i, p - p0);
				t[j] = i;
				i++;
			}
		}
		p += db2binSub2Len(p);
	}
	for (p = p0; p < p1; ) {
		if (*p == 0x01 || *p == 0x03) {
			j = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
			i = t[j];
			if (i >= 0 || *p == 0x03) {
				p[2] = (i >> 24) & 0xff;
				p[3] = (i >> 16) & 0xff;
				p[4] = (i >>  8) & 0xff;
				p[5] =  i        & 0xff;
			} else {
				// �Q�Ƃ���Ȃ����x�����߂͏�������.
//printf("delete label:%06X at %06X\n", j, p - p0);
				q = p;
				r = p + 6;
				while (r < p1)
					*q++ = *r++;
				p1 = q;
				continue;
			}
		}
		p += db2binSub2Len(p);
	}
	return p1;
}

int disasm0(struct Work *w)
{
	const UCHAR *p = w->ibuf + 4;
	UCHAR *q = w->obuf;
	while (*p != 0x00) {
		UCHAR c = *p;
		sprintf(q, "%06X: ", p - w->ibuf);
		q += 8;
		sprintf(q, "error: %02X\n", c);
		if (c == 0x01) {
			sprintf(q, "LB(%02X, %08X)", p[1], p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]);
			p += 6;
		}
		if (c == 0x02) {
			sprintf(q, "LIMM(R%02X, %08X)", p[1], p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]);
			p += 6;
		}
		if (c == 0x03) {
			sprintf(q, "PLIMM(P%02X, %08X)", p[1], p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]);
			p += 6;
		}
		if (c == 0x1e) {
			sprintf(q, "PCP(P%02X, P%02X)", p[1], p[2]);
			p += 3;
		}
		if (c == 0x2f) {
			int i = p[1];
			sprintf(q, "EXT(%02X, %04X, ...)", p[1], p[2] << 8 | p[3]);
			p += 4 + i;
		}

		while (*q != '\0') q++;
		if (q[-1] != ')')
			break;
		*q++ = ';';
		*q++ = '\n';
	}
	return putBuf(w->argOut, w->obuf, q);
}

int disasm1(struct Work *w)
// http://ref.x86asm.net/coder32.html
{
	const UCHAR *p = w->ibuf;
	UCHAR *q = w->obuf;
	while (*p != 0x00) {
		UCHAR c = *p, b = 0, f = 0;
		if (c <= 0x3f && c != 0x0f) {
			if ((c & 7) <= 3) { b = 1; f = 1; }
			if ((c & 7) == 4) { b = 1; f = 4; }
			if ((c & 7) == 5) { b = 1; f = 2; }
			if ((c & 7) >= 6) b = 1;
		}
		if (0x40 <= c && c <= 0x61)   b = 1;
		if (0x70 <= c && c <= 0x7f) { b = 1; f = 4; }
		if (0x88 <= c && c <= 0x8b) { b = 1; f = 1; }
		if (0x90 <= c && c <= 0x9a)   b = 1;
		if (0xb8 <= c && c <= 0xbf) { b = 1; f = 2; }
		if (c == 0xff) { b = 1; f = 1; }
		if (c == 0x0f) {
			c = p[1];
			if (0x80 <= c && c <= 0x8f) { b = 2; f = 2; }
			if (0xb6 <= c && c <= 0xb7) { b = 2; f = 1; }
		}
		sprintf(q, "%06X: ", p - w->ibuf);
		q += 8;
		if (b == 0) {
			sprintf(q, "error: %02X\n", c, p[1]);
			q += 13;
			break;
		}
		do {
			sprintf(q, "%02X ", *p++);
			q += 3;
		} while (--b != 0);
		if ((f & 1) != 0) { /* mod-nnn-r/m */
			c = *p & 0xc7;
			sprintf(q, "%02X ", *p++);
			q += 3;
			if (c < 0xc0 && (c & 7) == 4) {	// sib
				sprintf(q, "sib:%02X ", *p++);
				q += 7;
			} else if (c == 5) { // disp32
				goto disp32;
			}
			if ((c & 0xc0) == 0x40) {
				sprintf(q, "disp:%02X ", *p++);
				q += 8;
			}
			if ((c & 0xc0) == 0x80) {
disp32:
				sprintf(q, "disp:%08X ", p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24);
				p += 4;
				q += 14;
			}
		}
		if ((f & 2) != 0) { /* imm32 */
			sprintf(q, "imm:%08X ", p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24);
			p += 4;
			q += 13;
		}
		if ((f & 4) != 0) { /* imm8 */
			sprintf(q, "imm:%02X ", *p++);
			q += 7;
		}
		*q++ = '\n';
	}
	return putBuf(w->argOut, w->obuf, q);
}

int disasm(struct Work *w)
{
	int r = 0;
	if (w->flags == 0) r = disasm0(w);
	if (w->flags == 1) r = disasm1(w);
}

void appackSub0(UCHAR **qq, char *pof, int i)
// 4bit�o��.
{
	UCHAR *q = *qq;
	if (*pof == 0) {
		*q++ = (i << 4) & 0xf0;
		*qq = q;
	} else {
		q[-1] |= i & 0x0f;
	}
	*pof ^= 1;
	return;
}

void appackSub1(UCHAR **qq, char *pof, int i, char uf)
{
	if (uf != 0) {
		if (i <= 0x6)
			appackSub0(qq, pof, i);
		if (0x7 <= i && i <= 0x3f) {
			appackSub0(qq, pof, i >> 4 | 0x8);
			appackSub0(qq, pof, i);
		}
		if (0x40 <= i && i <= 0x1ff) {
			appackSub0(qq, pof, i >> 8 | 0xc);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		}
		if (0x200 <= i && i <= 0xfff) {
			appackSub0(qq, pof, 0xe);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		}
		if (0x1000 <= i && i <= 0xffff) {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x4);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		}
		if (0x10000 <= i && i <= 0xfffff) {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x5);
			appackSub0(qq, pof, i >> 16);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		}
		if (0x100000 <= i && i <= 0xffffff) {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x6);
			appackSub0(qq, pof, i >> 20);
			appackSub0(qq, pof, i >> 16);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		}
		if (0x1000000 <= i && i <= 0xfffffff) {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x8);
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, i >> 24);
			appackSub0(qq, pof, i >> 20);
			appackSub0(qq, pof, i >> 16);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		}
		if (0x10000000 <= i) {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x8);
			appackSub0(qq, pof, 0x8);
			appackSub0(qq, pof, i >> 28);
			appackSub0(qq, pof, i >> 24);
			appackSub0(qq, pof, i >> 20);
			appackSub0(qq, pof, i >> 16);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		}
	} else {
		if (-0x3 <= i && i <= 0x3) {
			// -3, -2, -1, 0, 1, 2, 3
			if (i < 0) i--;
			i &= 0x7;
			appackSub0(qq, pof, i);
		} else if (-0x20 <= i && i <= 0x1f) {
			i &= 0x3f;
			appackSub0(qq, pof, i >> 4 | 0x8);
			appackSub0(qq, pof, i);
		} else if (-0x100 <= i && i <= 0xff) {
			i &= 0x1ff;
			appackSub0(qq, pof, i >> 8 | 0xc);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		} else if (-0x800 <= i && i <= 0x7ff) {
			appackSub0(qq, pof, 0xe);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		} else if (-0x8000 <= i && i <= 0x7fff) {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x4);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		} else if (-0x80000 <= i && i <= 0x7ffff) {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x5);
			appackSub0(qq, pof, i >> 16);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		} else if (-0x800000 <= i && i <= 0x7fffff) {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x6);
			appackSub0(qq, pof, i >> 20);
			appackSub0(qq, pof, i >> 16);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		} else if (-0x8000000 <= i && i <= 0x7ffffff) {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x8);
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, i >> 24);
			appackSub0(qq, pof, i >> 20);
			appackSub0(qq, pof, i >> 16);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		} else {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x8);
			appackSub0(qq, pof, 0x8);
			appackSub0(qq, pof, i >> 28);
			appackSub0(qq, pof, i >> 24);
			appackSub0(qq, pof, i >> 20);
			appackSub0(qq, pof, i >> 16);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		}
	}
	return;
}

int appackEncodeConst(int i)
{
	if (i <= -16) i -= 0x50; /* -0x60�ȉ��� */
	if (0x100 <= i && i <= 0x10000 && (i & (i - 1)) == 0) {
		i = - askaLog2(i) - 0x48; /* -0x50����-0x58�� (-0x59����-0x5f�̓��U�[�u) */
	}
	return i;
}

char fe0501a(const UCHAR *p, int lastLabel, char mod, int p3f)
{
	char r = 0;
	if (mod == 1) {
		if (p[0] == 0x03 && p[1] == 0x30 && (p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]) == lastLabel + 1 &&
			p[6] == 0x1e && p[7] == 0x3f && p[8] == p3f && p[9] == 0x01 && p[10] == 0x01 &&
			(p[11] << 24 | p[12] << 16 | p[13] << 8 | p[14]) == lastLabel + 1 &&
			p[15] == 0xfe && p[16] == 0x01 && p[17] == 0x00)
		{
			r = 1;
		}
	}
	return r;
}

const UCHAR *fe0501b(UCHAR **qq, char *pof, int b0, int b1, const UCHAR *p)
{
	int r3f;
	if (b0 >= 0x30) {
		while (b0 <= b1) {
			if (*p == 0x02 && p[1] == b0) {
				r3f = appackEncodeConst(p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]);
				appackSub1(qq, pof, r3f, 0);
				p += 6;
			} else if (*p == 0x10 && p[1] == b0 && p[3] == 0xff) {
				appackSub1(qq, pof, - p[2] - 16, 0);
				p += 4;
			} else {
				appackSub1(qq, pof, - b0 - 16, 0);
			}
			b0++;
		}
	}
	return p;
}

const UCHAR *fe0501c(UCHAR **qq, char *pof, int c1, const UCHAR *p)
{
	int c0 = 0x31;
	while (c0 <= c1) {
		if (*p == 0x1e && p[1] == c0) {
			appackSub1(qq, pof, - p[2] - 16, 0);
			p += 3;
		} else {
			appackSub1(qq, pof, - c0 - 16, 0);
		}
		c0++;
	}
	return p;
}

const UCHAR *fe0501d(UCHAR **qq, char *pof, int d, const UCHAR *p)
{
	int i, j;
	UCHAR f;
	while (d > 0) {
		if (*p != 0xff) break;
		if (p[1] == 0x00 && p[2] == 0x00) {	// len+UINT8
			appackSub1(qq, pof, 0, 1);
			p += 3;
			int len = strlen(p);
			appackSub1(qq, pof, len, 1);
			do {
				i = *p++;
				appackSub0(qq, pof, (i >> 4) & 0x0f);
				appackSub0(qq, pof,  i       & 0x0f);
			} while (--len > 0);
			p++;
		} else if (p[1] == 0x00 && p[2] == 0x03) {	// UINT8+term(zero)
			appackSub1(qq, pof, 3, 1);
			p += 3;
			do {
				i = *p++;
				appackSub0(qq, pof, (i >> 4) & 0x0f);
				appackSub0(qq, pof,  i       & 0x0f);
			} while (i != 0);
		} else if (p[1] == 0x01 && p[2] == 0x04) {	// hh4(signed)+term(zero)
			appackSub1(qq, pof, 4, 1);
			p += 3;
			do {
				i = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
				p += 4;
				appackSub1(qq, pof, i, 0);
			} while (i != 0);
		} else if (p[1] == 0x00 && p[2] == 0x05) {	// special: len(-4)+UINT7
			appackSub1(qq, pof, 5, 1);
			p += 3;
			int len = strlen(p);
			if (len < 4 || 19 < len) { fprintf(stderr, "fe0501d: mode=5, len=%d\n", len); exit(1); }
			appackSub0(qq, pof, len - 4);
			f = 1;
			do {
				i = *p++;
				for (j = 6; j >= 0; j--) {
					f = f << 1 | ((i >> j) & 1);
					if (f >= 0x10) {
						appackSub0(qq, pof, f & 0x0f);
						f = 1;
					}
				}
			} while (--len > 0);
			p++;
			while (f > 1) {
				f <<= 1;
				if (f >= 0x10) {
					appackSub0(qq, pof, f & 0x0f);
					f = 1;
				}
			}
		} else if (p[1] == 0x00 && p[2] == 0x06) {	// special: UINT7+term(zero)
			appackSub1(qq, pof, 6, 1);
			p += 3;
			f = 1;
			do {
				i = *p++;
				for (j = 6; j >= 0; j--) {
					f = f << 1 | ((i >> j) & 1);
					if (f >= 0x10) {
						appackSub0(qq, pof, f & 0x0f);
						f = 1;
					}
				}
			} while (i != 0);
			while (f > 1) {
				f <<= 1;
				if (f >= 0x10) {
					appackSub0(qq, pof, f & 0x0f);
					f = 1;
				}
			}
		} else
			break;
		d--;
	}
	return p;
}

const UCHAR *fe0501e(UCHAR **qq, char *pof, int e1, const UCHAR *p)
{
	int e0 = 0x30;
	while (e0 <= e1) {
		if (*p == 0x10 && p[2] == e0 && p[3] == 0xff) {
			appackSub1(qq, pof, p[1], 1);
			p += 4;
		} else {
			appackSub1(qq, pof, e0, 1);
		}
		e0++;
	}
	return p;
}

const UCHAR *fe0501f(UCHAR **qq, char *pof, int f1, const UCHAR *p, int *pxxTyp)
{
	int f0 = 0x31;
	while (f0 <= f1) {
		if (*p == 0x1e && p[2] == f0) {
			appackSub1(qq, pof, p[1], 1);
			pxxTyp[p[1]] = -2; // �����Ɗ撣��ׂ���������Ȃ����ǁA�Ƃ肠����.
			p += 3;
		} else {
			appackSub1(qq, pof, f0, 1);
		}
		f0++;
	}
	return p;
}

const UCHAR *fe0501bb(int b0, int b1, const UCHAR *p)
{
	int r3f, rxx = 0;
	if (b0 >= 0x30) {
		while (b0 <= b1) {
			if (*p == 0x10 && p[1] == b0 && p[2] == rxx && p[3] == 0xff) {
				p += 4;
			} else {
				p = NULL;
				break;
			}
			b0++;
			rxx++;
		}
	}
	return p;
}

const UCHAR *fe0501cc(int c0, int c1, const UCHAR *p)
{
	int pxx = 1;
	while (c0 <= c1) {
		if (*p == 0x1e && p[1] == c0 && p[2] == pxx) {
			p += 3;
		} else {
			p = NULL;
			break;
		}
		c0++;
		pxx++;
	}
	return p;
}

int appack0(struct Work *w)
{
	const UCHAR *p = w->ibuf, *pp;
	UCHAR *q = w->obuf, *qq;
	*q++ = *p++;
	*q++ = *p++;
//	*q++ = *p++;
//	*q++ = *p++;
	char of = 0, f, oof, ff;
	int r3f = 0, lastLabel = -1, i, j, wait7 = 0, firstDataLabel = -1;
	int hist[0x40], pxxTyp[64], labelTyp[MAXLABEL];
	for (i = 0; i < 0x40; i++)
		hist[i] = 0;
	for (i = 0; i < 0x40; i++) {
		pxxTyp[i] = -1;
	}
	for (i = 0; i < MAXLABEL; i++) {
		labelTyp[i] = -1;
	}
	qq = db2binSub2(w->ibuf + 2, w->ibuf + w->isiz);
	w->isiz = qq - w->ibuf;
	*qq = '\0';
	while (*p != 0x00 || p == &w->ibuf[2]) {
		if (*p == 0x01) {
			i = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
			labelTyp[i] = 1;
			if (p[6] == 0xf0) {
				labelTyp[i] = p[7] << 24 | p[8] << 16 | p[9] << 8 | p[10];
				if (pxxTyp[1] == -1) {
					pxxTyp[1] = labelTyp[i];
					firstDataLabel = i;
				}
			}
		}
		p += db2binSub2Len(p);
	}
	p = w->ibuf + 2;
	while (*p != 0x00 || p == &w->ibuf[2]) {
//printf("%02X at %06X\n", *p, p - w->ibuf);
		if (*p == 0x00) {
			p++;
		} else if (*p == 0x01) {
			f = 0;
			i = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
			lastLabel++;
			if (p[1] == 0x01 && i == lastLabel && p[6] == 0xf0) {
				appackSub1(&q, &of, 0x2e, 1);
				hist[0x2e]++;
				p += 6;
				goto op_f0;
			}
			if (p[1] == 0x01 && i == lastLabel &&
				p[6] == 0xfe && p[7] == 0x01 && p[8] == 0x00 &&
				p[9] == 0x3c && p[10] == 0x00 && p[11] == 0x20 && p[12] == 0x20 && p[13] == 0x00 && p[14] == 0x00 && p[15] == 0x00)
			{
				appackSub1(&q, &of, 0x3c, 1);
				hist[0x3c]++;
				p += 6 + 3 + 7;
				continue;
			}
			if (p[1] != 0x00 || i != lastLabel) {
				f = 1;
				appackSub1(&q, &of, 0x04, 1);
				hist[0x04]++;
			}
			appackSub1(&q, &of, *p, 1);
			hist[0x01]++;
			if (f != 0) {
				appackSub1(&q, &of, p[1] | (i - lastLabel) << 8, 0);
				lastLabel = i;
			}
			p += 6;
		} else if (*p == 0x02) {
			if (p[1] != 0x3f) {
				hist[0x02]++;
				appackSub1(&q, &of, *p, 1);
				appackSub1(&q, &of, p[1], 1);
				appackSub1(&q, &of, appackEncodeConst(p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]), 0);
			} else {
				r3f = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
			}
			p += 6;
		} else if (*p == 0x03) {
			if (p[1] == 0x3f) {
				hist[0x03]++;
				appackSub1(&q, &of, 0x03, 1);
				appackSub1(&q, &of, (p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]) - lastLabel, 0);
				p += 6;
			} else if (p[1] == 0x30 && p[6] == 0x03 && p[7] == 0x3f && p[12] == 0x01 && p[13] == 0x01 &&
				p[14] == p[2] && p[15] == p[3] && p[16] == p[4] && p[17] == p[5] &&
				p[18] == 0xfe && p[19] == 0x01 && p[20] == 0x00)
			{
				lastLabel++;
				i = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
				j = p[8] << 24 | p[9] << 16 | p[10] << 8 | p[11];
				if (i != lastLabel) {
					appackSub1(&q, &of, 0x04, 1);
					hist[0x04]++;
				}
				appackSub1(&q, &of, 0x3e, 1);
				hist[0x3e]++;
				if (i != lastLabel) {
					// ������0�������ΐ�Ύw�胂�[�h�ɕύX�ł���.
					appackSub1(&q, &of, i - lastLabel, 0);
					lastLabel = i;
				}
				appackSub1(&q, &of, j - lastLabel, 0);
				pxxTyp[0x30] = 1;
				for (i = 0x31; i <= 0x3e; i++)
					pxxTyp[i] = -1;
				p += 21;
			} else if (p[1] == 0x30 && p[6] == 0x1e && p[7] == 0x3f && p[9] == 0x01 && p[10] == 0x01 &&
				p[11] == p[2] && p[12] == p[3] && p[13] == p[4] && p[14] == p[5] &&
				p[15] == 0xfe && p[16] == 0x01 && p[17] == 0x00)
			{
				lastLabel++;
				i = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
				if (i != lastLabel) {
					appackSub1(&q, &of, 0x04, 1);
					hist[0x04]++;
				}
				appackSub1(&q, &of, 0x3f, 1);
				hist[0x3f]++;
				if (i != lastLabel) {
					// ������0�������ΐ�Ύw�胂�[�h�ɕύX�ł���.
					appackSub1(&q, &of, i - lastLabel, 0);
					lastLabel = i;
				}
				appackSub1(&q, &of, p[8], 1);
				lastLabel = i;
				pxxTyp[0x30] = 1;
				for (i = 0x31; i <= 0x3e; i++)
					pxxTyp[i] = -1;
				p += 18;
			} else {
				i = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
				pxxTyp[p[1]] = labelTyp[i];
				if (p == &w->ibuf[3] && p[1] == 1 && firstDataLabel == i) {
					p += 6;
					continue;
				}
				appackSub1(&q, &of, 0x04, 1);
				hist[0x04]++;
				appackSub1(&q, &of, *p, 1);
				hist[0x03]++;
				appackSub1(&q, &of, p[1], 1);
				appackSub1(&q, &of, i - lastLabel, 0);
				p += 6;
			}
		} else if (*p == 0x04) {
			hist[0x4] += 2;
			appackSub1(&q, &of, 0x04, 1);
			appackSub1(&q, &of, 0x04, 1);
			appackSub1(&q, &of, p[1], 1);
			p += 2;
		} else if (0x08 <= *p && *p <= 0x09) {
			static UCHAR bytes[] = { 0x02, 0x3f, 0x00, 0x00, 0x00, 0x01, 0x0e };
			f = ff = 0;
			if (p[7] == 0x00 && memcmp(&p[8], bytes, 7) == 0 && p[15] == p[6] && p[20] == p[6] && p[21] == 0x3f)
				f = 1;
			if (f == 0) {
				appackSub1(&q, &of, 0x4, 1);
				hist[0x04]++;
			}
			if (pxxTyp[p[6]] != -1 && (p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]) != pxxTyp[p[6]]) {
				ff = 1;
				appackSub1(&q, &of, 0x0d, 1);
				hist[0x0d]++;
			}
			hist[*p]++;
			appackSub1(&q, &of, *p, 1);
			appackSub1(&q, &of, p[1], 1);
			appackSub1(&q, &of, p[6], 1);
			if (ff != 0 || pxxTyp[p[6]] == -1) {
				appackSub1(&q, &of, p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5], 1);
				pxxTyp[p[6]] = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
			}
			appackSub1(&q, &of, p[7], 1); // 0������.
			p += 8;
			if (f != 0) p += 6 + 8;
		} else if (*p == 0x0e) {
			f = ff = 0;
			if (pxxTyp[p[6]] != -1 && (p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]) != pxxTyp[p[6]])
				ff = 1;
			if (p[1] == 0x3f && 0x08 <= p[8] && p[8] <= 0x0b && p[14] == 0x3f && p[15] == 0) {
				if (ff != 0) {
					appackSub1(&q, &of, 0x0d, 1);
					hist[0x0d]++;
				}
				appackSub1(&q, &of, p[8] + 0x30, 1);
				hist[p[8] + 0x30]++;
				appackSub1(&q, &of, p[9], 1);
				appackSub1(&q, &of, p[6], 1);
				if (ff != 0 || pxxTyp[p[6]] == -1) {
					appackSub1(&q, &of, p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5], 1);
					pxxTyp[p[6]] = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
				}
				appackSub1(&q, &of, p[7], 1);
				appackSub1(&q, &of, 0, 1);
				p += 16;
				continue;
			}
			if (p[1] != p[6]) {
				f = 1;
				appackSub1(&q, &of, 0x4, 1);
				hist[0x04]++;
			}
			if (ff != 0) {
				appackSub1(&q, &of, 0x0d, 1);
				hist[0x0d]++;
			}
			hist[0x0e]++;
			appackSub1(&q, &of, *p, 1);
			appackSub1(&q, &of, p[1], 1);
			if (f != 0)
				appackSub1(&q, &of, p[6], 1);
			if (ff != 0 || pxxTyp[p[6]] == -1) {
				appackSub1(&q, &of, p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5], 1);
				pxxTyp[p[6]] = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
			}
			pxxTyp[p[1]] = pxxTyp[p[6]];
			if (p[7] != 0x3f)
				appackSub1(&q, &of, - p[7] - 16, 0);
			else {
				appackSub1(&q, &of, appackEncodeConst(r3f), 0);
			}
			p += 8;
		} else if (*p == 0x0f) {
			ff = 0;
			i = pxxTyp[p[6]];
			if (i == -1)
				i = pxxTyp[p[7]];
		//	if (pxxTyp[p[6]] != -1 && pxxTyp[p[7]] != -1 && pxxTyp[p[6]] != pxxTyp[p[7]])
		//		i = -1;
			if (i != -1 && (p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]) != i) {
				ff = 1;
				appackSub1(&q, &of, 0x0d, 1);
				hist[0x0d]++;
			}
			hist[0x0f]++;
			appackSub1(&q, &of, *p, 1);
			appackSub1(&q, &of, p[1], 1);
			appackSub1(&q, &of, p[6], 1);
			appackSub1(&q, &of, p[7], 1);
			if (ff != 0 || i == -1) {
				appackSub1(&q, &of, p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5], 1);
				i = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
			}
			pxxTyp[p[6]] = pxxTyp[p[7]] = i;
			p += 8;
		} else if (0x10 <= *p && *p <= 0x1b && *p != 0x17) {
			if (*p == 0x10 && p[3] == 0xff) {
				hist[0x17]++;
				appackSub1(&q, &of, 0x17, 1);
				appackSub1(&q, &of, p[1], 1);
				appackSub1(&q, &of, p[2], 1);
			} else {
				f = 0;
				if (p[1] != p[2]) {
					f = 1;
					appackSub1(&q, &of, 0x4, 1);
					hist[0x04]++;
				}
				hist[*p]++;
				appackSub1(&q, &of, *p, 1);
				appackSub1(&q, &of, p[1], 1);
				if (f != 0)
					appackSub1(&q, &of, p[2], 1);
				if (p[3] != 0x3f)
					appackSub1(&q, &of, - p[3] - 16, 0);
				else {
					appackSub1(&q, &of, appackEncodeConst(r3f), 0);
				}
				if (p[2] == 0x3f)
					appackSub1(&q, &of, r3f, 0);
			}
			p += 4;
		} else if (0x1c <= *p && *p <= 0x1e) {
			hist[*p]++;
			appackSub1(&q, &of, *p, 1);
			appackSub1(&q, &of, p[1], 1);
			appackSub1(&q, &of, p[2], 1);
			if (*p == 0x1e) {
				pxxTyp[p[1]] = pxxTyp[p[2]];
			}
			p += 3;
		} else if (0x20 <= *p && *p <= 0x27) {
			f = 0;
			if (p[1] != 0x3f) {
				f = 1;
				appackSub1(&q, &of, 0x4, 1);
				hist[0x04]++;
			}
			hist[*p]++;
			appackSub1(&q, &of, *p, 1);
			if (f != 0)
				appackSub1(&q, &of, p[1], 1);
			appackSub1(&q, &of, p[2], 1);
			if (p[3] != 0x3f)
				appackSub1(&q, &of, - p[3] - 16, 0);
			else {
				appackSub1(&q, &of, appackEncodeConst(r3f), 0);
			}
			if (p[2] == 0x3f)
				appackSub1(&q, &of, r3f, 0);
			p += 4;
			if (f == 0) {
				p += 2 + 6;
				appackSub1(&q, &of, (p[-4] << 24 | p[-3] << 16 | p[-2] << 8 | p[-1]) - lastLabel, 0);
			}
		} else if (0x30 <= *p && *p <= 0x33 || 0xf4 <= *p && *p <= 0xf7) {
			appackSub1(&q, &of, (*p & 3) | 0x30, 1);
			appackSub1(&q, &of, p[1], 1);
			appackSub1(&q, &of, p[2], 1);
			appackSub1(&q, &of, p[3], 1);
			if ((*p & 1) == 0) pxxTyp[p[1]] = -1;
			p += 4;
		} else if (*p == 0x3c) {
			f = 1;
	//		if (p[1] == 0x00 && p[2] == 0x20 && p[3] == 0x20 && p[4] == 0xcb && p[5] == 0x04 && p[6] == 0x04)
	//			f = 0;
			if (f != 0) {
				appackSub1(&q, &of, 0x4, 1);
				hist[0x04]++;
			}
			hist[*p]++;
			appackSub1(&q, &of, *p, 1);
			if (f != 0) {
				appackSub1(&q, &of, p[1], 1);
				appackSub1(&q, &of, p[2], 1);
				appackSub1(&q, &of, p[3], 1);
				appackSub1(&q, &of, (p[4] >> 4) & 0x0f, 1);
				appackSub1(&q, &of,  p[4]       & 0x0f, 1);
				appackSub1(&q, &of, p[5], 1);
				appackSub1(&q, &of, p[6], 1);
			}
			p += 7;
		} else if (*p == 0x3d) {
			if (p[7] == 0x1e && p[8] == 0x3f && p[9] == 0x30 && p[10] == 0x00) { p += 7 + 3; continue; }
			f = 0;
			if (p[7] == 0x1e && p[8] == 0x3f && p[9] == 0x30) {
				f = 1;
				hist[0x04]++;
				appackSub1(&q, &of, 0x04, 1);
			}
			hist[*p]++;
			appackSub1(&q, &of, *p, 1);
			p += 7;
			if (f != 0) p += 3;
		} else if (*p == 0xf0 || *p == 0x34) {
			appackSub1(&q, &of, 0x34, 1);
op_f0:
			i = p[1] << 24 | p[2] << 16 | p[3] << 8 | p[4];
			int len = p[5] << 24 | p[6] << 16 | p[7] << 8 | p[8];
			appackSub1(&q, &of, i, 1);
			appackSub1(&q, &of, len, 1);
			p += 9;
			len *= db2binDataWidth(i);
			while (len > 0) {
				appackSub0(&q, &of, *p >> 4);
				appackSub0(&q, &of, *p);
				p++;
				len -= 8;
			}
		} else if (*p == 0xfe && p[1] == 0x05 && p[2] == 0x02 && p[7] == 0x02 &&
			p[13] == 0x01 && p[14] == 0x00 && (p[15] << 24 | p[16] << 16 | p[17] << 8 | p[18]) == lastLabel + 1)
		{
			lastLabel++;
			appackSub1(&q, &of, 0x06, 1);
			appackSub1(&q, &of, p[8], 1);
			appackSub1(&q, &of, appackEncodeConst(p[9] << 24 | p[10] << 16 | p[11] << 8 | p[12]), 0);
			appackSub1(&q, &of, appackEncodeConst(p[3] << 24 | p[4] << 16 | p[5] << 8 | p[6]), 0);
			p += 19;
			/* �{���Ȃ�Ή�����fe0101�̂��߂ɁA�G���R�[�h�����̂��ǂ������X�^�b�N�ɋL�^����ׂ� */
		} else if (*p == 0xfe && p[1] == 0x01 && p[2] == 0x01) {
			wait7++;
			if (p[31] != 0x00 && !(p[31] == 0xfe && p[32] == 0x01 && p[33] == 0x01) && p[31] != 0x3d) {
				do {
					appackSub1(&q, &of, 0x07, 1);
				} while (--wait7 > 0);
			}
			p += 31;
		} else if (*p == 0xfe && p[1] == 0x01 && p[2] == 0xff) {
			appackSub0(&q, &of, 0x0f);
			p += 3;
		} else if (*p == 0xfe && p[1] == 0x01) {
			appackSub1(&q, &of, *p, 1);
			appackSub1(&q, &of, p[1], 1);
			appackSub1(&q, &of, p[2], 1);
			p += 3;
		} else if (*p == 0xfe && p[1] == 0x05) {
			i = p[3] << 24 | p[4] << 16 | p[5] << 8 | p[6];
			if (p[2] == 0x01) {
				qq = q; oof = of;
				static struct {
					int i, skip0, b0, b1, c1, d, mod, p3f, e1, f1, b2, c2, e2, f2;
				} table_fe0501[] = {
					{ 0x0010, 13, 0x31, 0x32, 0,    0, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_openWin
					{ 0x0011, 13, 0x31, 0x34, 0,    0, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_flushWin
					{ 0x0009, 13, 0x31, 0x32, 0,    0, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_sleep
					{ 0x000d, 13, 0x31, 0x31, 0,    0, 1, 0x28, 0x30, 0,    0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_inkey
					{ 0x0002, 13, 0x31, 0x34, 0,    0, 1, 0x28, 0,    0,    3, 0, 0, 0 }, 	// junkApi_drawPoint
					{ 0x0003, 13, 0x31, 0x36, 0,    0, 1, 0x28, 0,    0,    5, 0, 0, 0 }, 	// junkApi_drawLine
					{ 0x0004, 13, 0x31, 0x36, 0,    0, 1, 0x28, 0,    0,    5, 0, 0, 0 }, 	// junkApi_fillRect/drawRect
					{ 0x0005, 13, 0x31, 0x36, 0,    0, 1, 0x28, 0,    0,    5, 0, 0, 0 }, 	// junkApi_fillOval/draeOval
					{ 0x0006, 13, 0x31, 0x36, 0,    1, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f }, 	// junkApi_drawString
					{ 0x0740, 13, 0x31, 0x31, 0,    0, 1, 0x28, 0x30, 0x31, 0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_fopenRead
					{ 0x0741, 13, 0x31, 0x32, 0x31, 0, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_fopenWrite
					{ 0x0742, 13, 0x31, 0,    0,    0, 1, 0x28, 0,    0x31, 0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_allocBuf
					{ 0x0743, 13, 0x31, 0x31, 0x31, 0, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_writeStdout
					{ 0x0001, 13, 0x31, 0,    0,    1, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f }, 	//
					{ 0x0013, 13, 0x31, 0,    0,    0, 1, 0x28, 0x30, 0,    0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_rand
					{ 0x0014, 13, 0x31, 0x31, 0,    0, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_randSeed
					{ 0x0015, 13, 0x31, 0,    0,    0, 1, 0x28, 0x30, 0,    0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_getSeed
					{ 0,      0,  0,    0,    0,    0, 0, 0,    0,    0,    0x3f, 0x3f, 0x3f, 0x3f }
				};
				for (j = 0; table_fe0501[j].skip0 > 0; j++) {
					if (i == table_fe0501[j].i) break;
				}
				if (table_fe0501[j].skip0 > 0) {
					appackSub1(&qq, &oof, 0x04, 1);
					appackSub1(&qq, &oof, 0x05, 1);
					appackSub1(&qq, &oof, i, 0);
					pp = fe0501b(&qq, &oof, table_fe0501[j].b0, table_fe0501[j].b1 - table_fe0501[j].b2, p + table_fe0501[j].skip0);
					pp = fe0501bb(table_fe0501[j].b1 - table_fe0501[j].b2 + 1, table_fe0501[j].b1, pp);
					if (pp != NULL) {
						pp = fe0501c(&qq, &oof, table_fe0501[j].c1 - table_fe0501[j].c2, pp);
						pp = fe0501cc(table_fe0501[j].c1 - table_fe0501[j].c2 + 1, table_fe0501[j].c1, pp);
						if (pp != NULL) {
							if (fe0501a(pp, lastLabel, table_fe0501[j].mod, table_fe0501[j].p3f) != 0) {
								lastLabel++;
								if (table_fe0501[j].mod == 1) pp += 18;
								pp = fe0501e(&qq, &oof, table_fe0501[j].e1, pp);
								pp = fe0501f(&qq, &oof, table_fe0501[j].f1, pp, pxxTyp);
								q = qq;
								of = oof;
								p = pp;
								pxxTyp[0x30] = 1;
								for (i = 0x31; i <= 0x3e; i++)
									pxxTyp[i] = -2;	// �����͂����Ɗ撣��ׂ�.
								continue;
							}
						}
					}
					qq = q; oof = of;
					appackSub1(&qq, &oof, 0x05, 1);
					appackSub1(&qq, &oof, i, 0);
					pp = fe0501b(&qq, &oof, table_fe0501[j].b0, table_fe0501[j].b1, p + table_fe0501[j].skip0);
					pp = fe0501c(&qq, &oof, table_fe0501[j].c1, pp);
					pp = fe0501d(&qq, &oof, table_fe0501[j].d, pp);
					if (fe0501a(pp, lastLabel, table_fe0501[j].mod, table_fe0501[j].p3f) != 0) {
						lastLabel++;
						q = qq;
						of = oof;
						p = pp;
						if (table_fe0501[j].mod == 1) p += 18;
						p = fe0501e(&q, &of, table_fe0501[j].e1, p);
						p = fe0501f(&q, &of, table_fe0501[j].f1, p, pxxTyp);
						pxxTyp[0x30] = 1;
						for (i = 0x31; i <= 0x3e; i++)
							pxxTyp[i] = -2;	// �����͂����Ɗ撣��ׂ�.
						continue;
					}
				}
			}
			appackSub1(&q, &of, *p, 1);
			appackSub1(&q, &of, p[1], 1);
			appackSub1(&q, &of, p[2], 1);
			appackSub1(&q, &of, i, 0);
			p += 7;
		} else {
			printf("appack: error: %02X (at %06X)\n", *p, p - w->ibuf);
			break;
		}
	}
	if (q[-1] == 0x00) q--;
	if (q[-1] == 0x00) q--;
	if ((w->flags & 1) != 0) {
		printf("size=%d\n", q - w->obuf);
		for (i = 0; i < 0x40; i++) {
			printf("%02X:%05d ", i, hist[i]);
			if ((i & 0x07) == 0x07) putchar('\n');
		}
	}
	return putBuf(w->argOut, w->obuf, q);
}

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

int appackSub3(const UCHAR **pp, char *pif, char uf)
{
	int r = 0, i, j;
	if (uf != 0) {
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
			i = appackSub3(pp, pif, 1);
			r = 0;
			while (i > 0) {
				r = r << 4 | appackSub2(pp, pif);
				i--;
			}
		}
	} else {
		r = appackSub2(pp, pif);
		if (r <= 6) {
			if (r >= 4) r -= 7;
		} else if (0x8 <= r && r <= 0xb) {
			r = r << 4 | appackSub2(pp, pif);
			r &= 0x3f;
			if (r >= 0x20) r -= 0x40;
		} else if (0xc <= r && r <= 0xd) {
			r = r << 4 | appackSub2(pp, pif);
			r = r << 4 | appackSub2(pp, pif);
			r &= 0x1ff;
			if (r >= 0x100) r -= 0x200;
		} else if (r == 0xe) {
			r = appackSub2(pp, pif);
			r = r << 4 | appackSub2(pp, pif);
			r = r << 4 | appackSub2(pp, pif);
			if (r >= 0x800) r -= 0x1000;
		} else if (r == 0x7) {
			i = appackSub3(pp, pif, 1) - 1;
			r = appackSub2(pp, pif);
			j = 0x8;
			while (i > 0) {
				r = r << 4 | appackSub2(pp, pif);
				j <<= 4;
				i--;
			}
			if (r >= j) r -= j + j;
		}
	}
	return r;
}

UCHAR *appackSub4(UCHAR *q, int i)
{
	q[0] = (i >> 24) & 0xff;
	q[1] = (i >> 16) & 0xff;
	q[2] = (i >>  8) & 0xff;
	q[3] =  i        & 0xff;
	return q + 4;
}

char appackSub5(const UCHAR **pp, char *pif, UCHAR *preg, int *pr3f)
{
	char f = 0;
	int i = appackSub3(pp, pif, 0);
	if (-0x4f <= i && i <= -0x10) {
		f = 1;
		*preg = -0x10 - i;
	} else if (i >= -0x0f) {
		*pr3f = i;
		*preg = 0x3f;
	} else if (i <= -0x50) {
		*pr3f = i + 0x40;
		*preg = 0x3f;
	}
	return f;
}

int appack1(struct Work *w)
{
	const UCHAR *p = w->ibuf;
	UCHAR *q = w->obuf;
	*q++ = *p++;
	*q++ = *p++;
//	*q++ = *p++;
//	*q++ = *p++;
	char inf = 0, f = 0;
	int r3f = 0, lastLabel = -1, i, j;
	for (;;) {
		*q = appackSub3(&p, &inf, 1);
		if (*q == 0x00) break;
		if (*q == 0x01) {
			lastLabel++;
			q[1] = 0x00;
			if (f != 0) {
				i = appackSub3(&p, &inf, 0);
				q[1] = i & 0xff;
				lastLabel += i >> 8;
			}
			q = appackSub4(q + 2, lastLabel);
			f = 0;
		} else if (*q == 0x02) {
			q[1] = appackSub3(&p, &inf, 1);
			i = appackSub3(&p, &inf, 0);
			q = appackSub4(q + 2, i);
			f = 0;
		} else if (*q == 0x06) {
			f = 1;
		} else if (0x10 <= *q && *q <= 0x1b && *q != 0x17) {
			q[1] = appackSub3(&p, &inf, 1);
			if (f == 0)
				q[2] = q[1];
			else
				q[2] = appackSub3(&p, &inf, 1);
			f = appackSub5(&p, &inf, &q[3], &r3f);
			if (f != 0 && (q[2] == 0x3f || q[3] == 0x3f)) {
				f = 0;
				r3f = appackSub3(&p, &inf, 0);
			}
			if (f == 0) {
				q[6] = *q;
				q[7] = q[1];
				q[8] = q[2];
				q[9] = q[3];
				*q = 0x02;
				q[1] = 0x3f;
				q = appackSub4(q + 2, r3f);
			}
			q += 4;
			f = 0;
		} else if (0x20 <= *q && *q <= 0x27) {
			q[1] = 0x3f;
			if (f != 0)
				q[1] = appackSub3(&p, &inf, 1);
			q[2] = appackSub3(&p, &inf, 1);
			f = appackSub5(&p, &inf, &q[3], &r3f);
			if (f != 0 && (q[2] == 0x3f || q[3] == 0x3f)) {
				f = 0;
				r3f = appackSub3(&p, &inf, 0);
			}
			if (f != 0) {
				q[6] = *q;
				q[7] = q[1];
				q[8] = q[2];
				q[9] = q[3];
				*q = 0x02;
				q[1] = 0x3f;
				q = appackSub4(q + 2, r3f);
			}
			if (q[1] == 0x3f) {
				q[4] = 0x04;
				q[5] = 0x3f;
				q[6] = 0x03;
				q[7] = 0x00;
				i = appackSub3(&p, &inf, 0) + lastLabel;
				q = appackSub4(q + 8, i);
			} else
				q += 4;
			f = 0;
		} else {
			printf("err: %02X (at %06X, %06X)\n", *q, q - w->obuf, p - w->ibuf);
			break;
		}
	}
	return putBuf(w->argOut, w->obuf, q);
}

int appack2(struct Work *w, char level)
{
	FILE *fp;
	int i = 0;
	UCHAR *p, *q = NULL, *pp;
	fp = fopen("$tmp.org", "wb");
	fwrite(w->ibuf + 2, 1, w->isiz - 2, fp);
	fclose(fp);
	w->obuf[0] = w->ibuf[0];
	w->obuf[1] = w->ibuf[1];
	if (level == 0)
		i = system("bim2bin -osacmp in:$tmp.org out:$tmp.tk5");
	else
		i = system("bim2bin -osacmp in:$tmp.org out:$tmp.tk5 eopt:@ eprm:@");
	if (i == 0) {
		printf("org:%d ", w->isiz);
		fp = fopen("$tmp.tk5", "rb");
		w->isiz = fread(w->ibuf, 1, BUFSIZE - 4, fp);
		fclose(fp);
		p = w->ibuf + 16;
		i = 0;
		do {
			i = i << 7 | *p++ >> 1;
		} while ((p[-1] & 1) == 0);
		//printf("org.size=%d\n", i);
		w->obuf[2] = 0xf0;
		q = &w->obuf[3];
		char of = 0;
		pp = p;
		for (;;) {
			if (*p == 0x15)      { appackSub1(&q, &of, 1, 1); p++; }
			else if (*p == 0x19) { appackSub1(&q, &of, 2, 1); p++; }
			else if (*p == 0x21) { appackSub1(&q, &of, 3, 1); p++; }
			else                 { appackSub1(&q, &of, 0, 1);      }
			appackSub1(&q, &of, i >> 8, 1);
			if (of == 0) break;
			q = &w->obuf[3];
			of = 0;
			appackSub0(&q, &of, 0x08);
			p = pp;
		}
		*q++ = i & 0xff;
		while (p < w->ibuf + w->isiz)
			*q++ = *p++;
		printf("tk5:%d\n", q - w->obuf);
	} else {
		fputs("appack2: bim2bin error\n", stderr);
	}
	remove("$tmp.org");
	remove("$tmp.tk5");
	return putBuf(w->argOut, w->obuf, q);
}


int appack(struct Work *w)
{
	int r = 0;
	if (w->flags == 0) r = appack0(w);
	if (w->flags == 1) r = appack1(w);
	if (w->flags == 2) r = appack2(w, 0);
	if (w->flags == 3) r = appack2(w, 1);
}

int maklib(struct Work *w)
{
	UCHAR *q = w->obuf;
	q[0] = 0x05;
	q[1] = 0x1b;
	q[2] = 0x00;
	q[3] = 0x00;
	q[4] = 0;
	q[5] = 0;
	q[6] = 0;
	q[7] = 24;
	int i, j;
	for (j = 0; j < 1; j++) {
		for (i = 0; i < 16; i++) {
			q[8 + j * 20 + 4 + i] = 0;
		}
	}
	strcpy(&q[8 + 20 * 0 + 4], "decoder");
	q[8 + 20 * 0 + 0] = 0;
	q[8 + 20 * 0 + 1] = 0;
	q[8 + 20 * 0 + 2] = 0;
	q[8 + 20 * 0 + 3] = 32;
	q[8 + 20 * 1 + 0] = 0xff;
	q[8 + 20 * 1 + 1] = 0xff;
	q[8 + 20 * 1 + 2] = 0xff;
	q[8 + 20 * 1 + 3] = 0xff;
	FILE *fp = fopen("decoder.ose", "rb");
	if (fp == NULL) {
		fputs("fopen error: decoder.ose", fp);
		return 1;
	}
	i = fread(&q[8 + 20 * 1 + 4], 1, 1024 * 1024 - 256, fp);
	fclose(fp);
	q[8 + 20 * 1 + 4 + i] = 0xff;
	return putBuf(w->argOut, w->obuf, q + 8 + 20 * 1 + 4 + i);
}

