#ifndef _INTERPRETER_INCLUDES_H
#define _INTERPRETER_INCLUDES_H


/***********************************************
*                 includes.h
*
* Sorts out processor family specific includes and defines.
*
* Started: 6 December 2007 - Michael Pearce
*
* Currently supported:
*  Microchip PIC18, dsPIC33, PIC32
*
************************************************
*         Version Information
************************************************
* 20071206 - Start of file - Michael Pearce
* 20071206 - readapted to C18 - Dario Greggio
* 20190811 - readapted to C32 - Dario Greggio
* 20200214 - readapted to XC16/forgetIVrea - Dario/C Greggio
* 20210902 - readapted to XC32/PC_PIC motherboard - Dario/Katia Greggio
* 20220722 - readapted/improved for KommissarRexx AND MiniBasic
***********************************************/


#include <xc.h>

typedef const char * STRINGFARPTR;
typedef const char * STRINGPTR;
#define BSTATIC static

#include <stdio.h>
#include <stdlib.h>
#include <string.h>   //??
#include <stdarg.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>


//#include <malloc.h>


#define _H_USER ((FILE*)((int)-1))
#define _H_USART ((FILE*)((int)-2));
  


//---------------- System Hardware Defs ---------------------------



#define IDLENGTH 31    /* maximum length of variable */
#define MAXFORS 32     /* maximum number of nested fors */
#define MAXDIMS 5     /* maximum indices for dimensions */
#define MAXGOSUB 32    /* maximum number of gosubs */
#define MAXFILES 10     // 


typedef uint16_t LINE_NUMBER_TYPE;
typedef union __attribute__ ((__packed__)) {
    struct __attribute__ ((__packed__)) {
        uint16_t line;
        uint8_t pos;
        };
    uint32_t d;
    } LINE_NUMBER_STMT_POS_TYPE;

#undef SMALLTOKENS    /* use shortened tokens to reduce program size */

typedef double NUM_TYPE;

#define NEXTLG 3   /* controls chunk size for allocation of variables */

#undef BASICASSERT

#ifdef BASICASSERT
#define ASSERTDEBUG(x) x
#else
#define ASSERTDEBUG(x) {}
#endif

typedef void (*addbasictoken)(int chv, void *v);


// error codes (in BASIC and REXX script) defined 
enum __attribute__ ((__packed__)) INTERPRETER_ERRORS {
	ERR_CLEAR = 0,
	ERR_SYNTAX,
	ERR_OUTOFMEMORY,
	ERR_IDTOOLONG,
	ERR_NOSUCHVARIABLE,
	ERR_BADSUBSCRIPT,
	ERR_TOOMANYDIMS,
	ERR_TOOMANYINITS,
    ERR_BADTYPE,
	ERR_TOOMANYFORS,
	ERR_NONEXT,   //10
	ERR_NOFOR,
	ERR_NOSUCHLINE,
	ERR_DIVIDEBYZERO,
	ERR_NEGLOG,
	ERR_NEGSQRT,
	ERR_BADSINCOS,
	ERR_EOF,
	ERR_ILLEGALOFFSET,
	ERR_TYPEMISMATCH,
	ERR_INPUTTOOLONG,    //20
	ERR_BADVALUE,
	ERR_NOTINT,
	ERR_TOOMANYGOSUB,
	ERR_NORETURN,
	ERR_FORMULATOOCOMPLEX,
	ERR_FILE,
	ERR_NETWORK,
	ERR_STOP,
	ERR_INTERNAL    //28 per gestione eccezioni 
	};


//======================================================================================================


void outstrint(char *s, int x);
void outstrhex(char *s, unsigned int x);
void outstr(char *x);
void outint(int x);
int instrn(char *x, int len);
int innum(NUM_TYPE *val);
char *myitoa(int n);
char *myitob(unsigned int n);
char *myitohex(unsigned int n);
uint8_t myhextob(char c);
unsigned int myhextoi(const char *p);
unsigned int mybtoi(const char *p);

int8_t stricmp(const char *, const char *);

typedef struct  __attribute((packed)) {
  uint8_t inquote;
  uint8_t notlastspace;
	} TOKSTAT;

typedef struct  __attribute((packed)) {
  char *s;
  uint16_t toknum;
  uint16_t charnum;
	} UNTOKSTAT;

void inittokenizestate(TOKSTAT *t);
void tokenizeline(char *str, char **end, TOKSTAT *tk, addbasictoken abt, void *v);
void inituntokenizestate(UNTOKSTAT *utk, char *str);
int untokenizecode(UNTOKSTAT *utk);


void Delay_uS(unsigned char );
//void Delay_mS_free(WORD);


typedef uint16_t DIM_SIZE;
typedef uint8_t TOKEN_NUM;
typedef uint8_t IDENT_LEN;

typedef struct __attribute((packed)) {
    const char *str;				// points to start of line
    LINE_NUMBER_TYPE no;          // line number
	} LINE;

enum __attribute((packed)) DATATYPE {
	FLTID=1,
	INTID=2,
	STRID=8
	};
    

typedef struct __attribute((packed)) {
	void *handle;
	uint8_t number;
	uint8_t type;
	} FILE_DESCR;

enum _FILE_TYPES {
	FILE_COM,
#ifdef USA_USB_SLAVE_CDC
	FILE_CDC,
#endif
	FILE_TCP,
	FILE_UDP,
	FILE_DISK
	};


typedef struct __attribute((packed)) {
	const char *tokenname;
	uint8_t length;
	} TOKEN_LIST;

typedef uint32_t COLORREF;
/*typedef struct _COLORREF {
	char red;
	char green;
	char blue;
	} COLORREF; mah, no.. */


struct __attribute((packed)) EVENT_HANDLER {
	LINE_NUMBER_TYPE handler;
	LINE_NUMBER_STMT_POS_TYPE line;
	enum INTERPRETER_ERRORS errorcode;
	};

    
extern char theScript[32767];


#endif   // _INCLUDES_H



