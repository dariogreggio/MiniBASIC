#ifndef _MINIBASIC_H
#define _MINIBASIC_H

/*
  Minibasic header file
  By Malcolm Mclean
  Small changes for PIC18, by DarioG 29/11/07
  Start Changes for Dspic33fj256GP710  JeffD 25/10/08
  Start Changes for PIC24FJ256GA010  CG 14/5/14 (la morte sia con rivoli e giaveno)
  Start Changes for dsPIC33EP512GP502  DG/CG 29/9/14 (after a lovely skirty day!)
	Changes for VGA24, 07.15
  Start Changes for PIC32MZ  GD 11/8/19 FUCK HUMANS! DIE!!
  Breakthrough version 2021
*/


#include <stdio.h>
#include "../../generictypedefs.h"
#include "../../Compiler.h"

#include "../pc_pic_cpu.h"
#include "../breakthrough.h"
#include "interpreter_includes.h"



//================================== Basic SCRIPT to run in script.c ===================================
extern const char demoscript[];

typedef double NUM_TYPE;


typedef union __attribute((packed)) {
  NUM_TYPE dval;			  /* its value if a real */
  char    *sval;				/* its value if a string (malloc'ed) */
  int16_t  ival;	        /* its value if an int */
	} VARIABLEDATA;

typedef struct __attribute((packed)) {
  VARIABLEDATA d;               /* data in variable */
  char id[IDLENGTH];				/* id of variable */
//#warning fatto IDLEN a 31 e type uint8 !
  enum DATATYPE  /*ALIGNMENT in alloc dinamica! */   type;		    /* its type, STRID or FLTID or INTID */
	// unsigned int * fixed_address??
	} VARIABLE;
STATIC_ASSERT(!(sizeof(VARIABLE) % 4),0);

typedef union __attribute((packed)) {
  char        **str;	        /* pointer to string data */
  NUM_TYPE     *dval;	        /* pointer to real data */
  int16_t      *ival;	        /* pointer to int data */
	} DIMVARPTR;

typedef struct __attribute((packed)) {
  char id[IDLENGTH];			/* id of dimensioned variable */
  enum DATATYPE type;					/* its type, STRID or FLTID (or INTID)*/
  uint32_t /*uint8_t  ALIGNMENT in alloc dinamica! */  ndims;			/* number of dimensions */
  DIM_SIZE dim[MAXDIMS];			/* dimensions in x y order */
  DIMVARPTR     d;              /* pointers to string/real data */
	} DIMVAR;

typedef struct __attribute((packed)) {
  char id[IDLENGTH];			/* id of dimensioned variable */
  uint8_t  ndims;			/* number of dimensions */
  DIM_SIZE dim[MAXDIMS];			/* dimensions in x y order */
  DIMVARPTR     d;              /* pointers to string/real data */
  char **str;                   /* its value if a string (malloc'ed) */
	} DIMVARSTRING;

typedef union __attribute((packed)) {
  char        **sval;			/* pointer to string data */
  NUM_TYPE     *dval;		    /* pointer to real data */
  int16_t      *ival;	        /* pointer to int data */
	} LVALUEDATA;

typedef struct __attribute((packed)) {
  LVALUEDATA    d;              /* data pointed to by LVALUE */
  uint8_t type;			/* type of variable (STRID or FLTID or INTID or B_ERROR) */   
	} LVALUE;

typedef struct __attribute((packed)) {
  char id[IDLENGTH];			/* id of control variable */
  uint8_t filler;
  LINE_NUMBER_TYPE nextline;	/* line below FOR to which control passes */
  NUM_TYPE toval;			/* terminal value */
  NUM_TYPE step;			/* step size */
	} FORLOOP;
#warning USARE PUNTATORE allo Script al posto del line_number, cosi dovrebbe gestire multistatement :

typedef struct __attribute((packed)) _MINIBASIC {
  //OCCHIO ALLINEAMENTI!
  FORLOOP forStack[MAXFORS];   // stack for FOR loop control 
  LINE_NUMBER_TYPE gosubStack[MAXGOSUB];		// GOSUB stack
  LINE_NUMBER_TYPE doStack[MAXFORS];		// DO stack - same # allowed as for's
  uint8_t nfors;					// number of fors on stack
  uint8_t ngosubs;
  uint8_t ndos;
  uint8_t inBlock;

  FILE_DESCR openFiles[MAXFILES];		// open files descriptors


  VARIABLE *variables;			// the script's variables
  DIMVAR *dimVariables;		// dimensioned arrays
  LINE *lines;					// list of line starts

  uint16_t nvariables;				// number of variables
  uint16_t ndimVariables;			// number of dimensioned arrays
  uint16_t nlines;					// number of BASIC lines in program
  int16_t curline;      // usa -1 come marker di fine...

  const char *string;        // string we are parsing

  HWND hWnd;
  THREAD *threadID;

  enum INTERPRETER_ERRORS errorFlag;           // set when error in input encountered
  uint8_t lastExprType;
  uint8_t nfiles;
  uint8_t incomingChar[2];
  
  struct __attribute((packed)) EVENT_HANDLER errorHandler,irqHandler,timerHandler;
  uint8_t filler[3];
  
  COLORREF Color,ColorBK;
  POINT Cursor;
  GFX_COLOR ColorPalette,ColorPaletteBK;

  TOKEN_NUM token;          // current token (lookahead)
  
  } MINIBASIC;
STATIC_ASSERT(!(sizeof(struct _MINIBASIC) % 4),0);

int basic(MINIBASIC *instance,const char *script,BYTE where);

#endif


