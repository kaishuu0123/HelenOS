#include "pass2.h"
static int op0[] = { -1 };
static int op1[] = { -1 };
static int op2[] = { 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 232, 234, -1 };
static int op3[] = { -1 };
static int op4[] = { 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 232, 234, -1 };
static int op5[] = { 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 232, 234, -1 };
static int op6[] = { 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 232, 234, -1 };
static int op7[] = { 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 232, 234, -1 };
static int op8[] = { 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 232, 234, -1 };
static int op9[] = { 233, 234, -1 };
static int op10[] = { 68, 69, 70, 71, 72, 73, 74, 75, 76, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 234, -1 };
static int op11[] = { 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 96, 234, -1 };
static int op12[] = { 151, 152, 153, 154, 155, 156, 157, 234, -1 };
static int op13[] = { 158, 159, 160, 161, 162, 234, -1 };
static int op14[] = { 163, 164, 165, 166, 167, 168, 234, -1 };
static int op15[] = { 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 183, 184, 185, 186, 187, 188, 189, 234, -1 };
static int op16[] = { 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 234, -1 };
static int op17[] = { 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 234, -1 };
static int op18[] = { 97, 98, 99, 100, 101, 102, 103, 234, -1 };
static int op19[] = { 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 234, -1 };
static int op20[] = { 211, 212, 213, 214, 233, 234, -1 };
static int op21[] = { -1 };
static int op22[] = { -1 };
static int op23[] = { 169, 170, 171, 172, 173, 174, 175, 228, 233, 234, -1 };
static int op24[] = { 206, 207, 208, 209, 210, 233, 234, -1 };
static int op25[] = { 176, 177, 178, 179, 180, 181, 182, 234, -1 };
static int op26[] = { 176, 177, 178, 179, 180, 181, 182, 234, -1 };
static int op27[] = { 176, 177, 178, 179, 180, 181, 182, 234, -1 };
static int op28[] = { 176, 177, 178, 179, 180, 181, 182, 234, -1 };
static int op29[] = { 176, 177, 178, 179, 180, 181, 182, 234, -1 };
static int op30[] = { 176, 177, 178, 179, 180, 181, 182, 234, -1 };
static int op31[] = { 176, 177, 178, 179, 180, 181, 182, 234, -1 };
static int op32[] = { 176, 177, 178, 179, 180, 181, 182, 234, -1 };
static int op33[] = { 176, 177, 178, 179, 180, 181, 182, 234, -1 };
static int op34[] = { 176, 177, 178, 179, 180, 181, 182, 234, -1 };
static int op35[] = { 234, -1 };
static int op36[] = { 231, 233, 234, -1 };
static int op37[] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 233, 234, -1 };
static int op38[] = { 1, 233, 234, -1 };
static int op39[] = { 234, -1 };
static int op40[] = { 234, -1 };
static int op41[] = { 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 234, -1 };
static int op42[] = { 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 233, 234, -1 };
static int op43[] = { 234, -1 };
static int op44[] = { 233, 234, -1 };
static int op45[] = { 65, 66, 67, 234, -1 };
static int op46[] = { 62, 63, 64, 233, 234, -1 };
static int op47[] = { 232, 234, -1 };
static int op48[] = { 234, -1 };
static int op49[] = { 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 229, -1 };
static int op50[] = { 150, 230, -1 };
static int op51[] = { 227, 233, 234, -1 };
static int op52[] = { 233, 234, -1 };
static int op53[] = { 234, -1 };
static int op54[] = { 190, 191, 233, 234, -1 };
static int op55[] = { -1 };
static int op56[] = { 234, -1 };
static int op57[] = { 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 233, 234, -1 };
static int op58[] = { 233, 234, -1 };

int *qtable[] = { 
	op0,
	op1,
	op2,
	op3,
	op4,
	op5,
	op6,
	op7,
	op8,
	op9,
	op10,
	op11,
	op12,
	op13,
	op14,
	op15,
	op16,
	op17,
	op18,
	op19,
	op20,
	op21,
	op22,
	op23,
	op24,
	op25,
	op26,
	op27,
	op28,
	op29,
	op30,
	op31,
	op32,
	op33,
	op34,
	op35,
	op36,
	op37,
	op38,
	op39,
	op40,
	op41,
	op42,
	op43,
	op44,
	op45,
	op46,
	op47,
	op48,
	op49,
	op50,
	op51,
	op52,
	op53,
	op54,
	op55,
	op56,
	op57,
	op58,
};
int tempregs[] = { 0, 1, 2, -1 };
int permregs[] = { 3, 4, 5, -1 };
bittype validregs[] = {
	0xffffff3f,
	0x0000007f,
};
static int amap[MAXREGS][NUMCLASS] = {
	/* 0 */{ 0x1,0x3,0x1f,0x0 },
	/* 1 */{ 0x2,0xc,0x1e1,0x0 },
	/* 2 */{ 0x4,0x30,0xe22,0x0 },
	/* 3 */{ 0x8,0xc0,0x3244,0x0 },
	/* 4 */{ 0x10,0x0,0x5488,0x0 },
	/* 5 */{ 0x20,0x0,0x6910,0x0 },
	/* 6 */{ 0x0,0x0,0x0,0x0 },
	/* 7 */{ 0x0,0x0,0x0,0x0 },
	/* 8 */{ 0x1,0x1,0x1f,0x0 },
	/* 9 */{ 0x1,0x2,0x1f,0x0 },
	/* 10 */{ 0x2,0x4,0x1e1,0x0 },
	/* 11 */{ 0x2,0x8,0x1e1,0x0 },
	/* 12 */{ 0x4,0x10,0xe22,0x0 },
	/* 13 */{ 0x4,0x20,0xe22,0x0 },
	/* 14 */{ 0x8,0x40,0x3244,0x0 },
	/* 15 */{ 0x8,0x80,0x3244,0x0 },
	/* 16 */{ 0x3,0xf,0x1ff,0x0 },
	/* 17 */{ 0x5,0x33,0xe3f,0x0 },
	/* 18 */{ 0x9,0xc3,0x325f,0x0 },
	/* 19 */{ 0x11,0x3,0x549f,0x0 },
	/* 20 */{ 0x21,0x3,0x691f,0x0 },
	/* 21 */{ 0x6,0x3c,0xfe3,0x0 },
	/* 22 */{ 0xa,0xcc,0x33e5,0x0 },
	/* 23 */{ 0x12,0xc,0x55e9,0x0 },
	/* 24 */{ 0x22,0xc,0x69f1,0x0 },
	/* 25 */{ 0xc,0xf0,0x3e66,0x0 },
	/* 26 */{ 0x14,0x30,0x5eaa,0x0 },
	/* 27 */{ 0x24,0x30,0x6f32,0x0 },
	/* 28 */{ 0x18,0xc0,0x76cc,0x0 },
	/* 29 */{ 0x28,0xc0,0x7b54,0x0 },
	/* 30 */{ 0x30,0x0,0x7d98,0x0 },
	/* 31 */{ 0x0,0x0,0x0,0x1 },
	/* 32 */{ 0x0,0x0,0x0,0x2 },
	/* 33 */{ 0x0,0x0,0x0,0x4 },
	/* 34 */{ 0x0,0x0,0x0,0x8 },
	/* 35 */{ 0x0,0x0,0x0,0x10 },
	/* 36 */{ 0x0,0x0,0x0,0x20 },
	/* 37 */{ 0x0,0x0,0x0,0x40 },
	/* 38 */{ 0x0,0x0,0x0,0x80 },
};
int
aliasmap(int class, int regnum)
{
	return amap[regnum][class-1];
}
static int rmap[NUMCLASS][15] = {
	{ 0, 1, 2, 3, 4, 5, },
	{ 8, 9, 10, 11, 12, 13, 14, 15, },
	{ 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, },
	{ 31, 32, 33, 34, 35, 36, 37, 38, },
};

int
color2reg(int color, int class)
{
	return rmap[class-1][color];
}
int regK[] = { 0, 6, 8, 15, 8, 0, 0, 0 };
int
classmask(int class)
{
	if(class == CLASSA) return 0x3f;
	if(class == CLASSB) return 0xff;
	if(class == CLASSC) return 0x7fff;
	if(class == CLASSD) return 0xff;
	if(class == CLASSE) return 0x0;
	if(class == CLASSF) return 0x0;
	return 0x0;
}
static bittype ovlarr[MAXREGS][2] = {
{ 0x1f0301, 0x0,  },
{ 0x1e10c02, 0x0,  },
{ 0xe223004, 0x0,  },
{ 0x3244c008, 0x0,  },
{ 0x54880010, 0x0,  },
{ 0x69100020, 0x0,  },
{ 0x40, 0x0,  },
{ 0x80, 0x0,  },
{ 0x1f0101, 0x0,  },
{ 0x1f0201, 0x0,  },
{ 0x1e10402, 0x0,  },
{ 0x1e10802, 0x0,  },
{ 0xe221004, 0x0,  },
{ 0xe222004, 0x0,  },
{ 0x32444008, 0x0,  },
{ 0x32448008, 0x0,  },
{ 0x1ff0f03, 0x0,  },
{ 0xe3f3305, 0x0,  },
{ 0x325fc309, 0x0,  },
{ 0x549f0311, 0x0,  },
{ 0x691f0321, 0x0,  },
{ 0xfe33c06, 0x0,  },
{ 0x33e5cc0a, 0x0,  },
{ 0x55e90c12, 0x0,  },
{ 0x69f10c22, 0x0,  },
{ 0x3e66f00c, 0x0,  },
{ 0x5eaa3014, 0x0,  },
{ 0x6f323024, 0x0,  },
{ 0x76ccc018, 0x0,  },
{ 0x7b54c028, 0x0,  },
{ 0x7d980030, 0x0,  },
{ 0x80000000, 0x0,  },
{ 0x0, 0x1,  },
{ 0x0, 0x2,  },
{ 0x0, 0x4,  },
{ 0x0, 0x8,  },
{ 0x0, 0x10,  },
{ 0x0, 0x20,  },
{ 0x0, 0x40,  },
};
int
interferes(int reg1, int reg2)
{
return (TESTBIT(ovlarr[reg1], reg2)) != 0;
}
