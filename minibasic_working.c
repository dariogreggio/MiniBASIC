/*****************************************************************
*                           Mini BASIC                           *
*                        by Malcolm McLean                       *
*                           version 1.0                          *
*                                                                *
*                  adapted to PIC18 etc  by  Dario Greggio       *
*                           29/11/2007  
*						started port to C30 Dspic33fJ256GP710    *
*						26/10/08 JeffD							*
* 					2014-2015 porting to dsPIC33 / XC16 (free)
*						2017: PIC24EP
*						2019: PIC32MZ, anche con USB host
 *          2020: forgetIvrea con display TFT ST7735, e ILI9341
 *          2021: PC_PIC motherboard, PIC32MZ
*****************************************************************/

// if con doppia condizione non va... e/o dà syntax error. v. MANDEL
// inserire righe non va ancora/è da fare e run pure (v.)
//   ci vogliono PARENTESI se doppia condizione! ossia pare <= ha priorità inferiore a AND... SISTEMARE
// cose tipo BEEP non danno errore se manca parametro...
// In generale, se c'è un errore in uno statement, non si ferma fino a fine statement e rischia pure di eseguirlo con parametro a cazzo...
//  bisognerebbe STRONCARE, forse con un longjmp da setError?
//  verificare se string$ possono restituire NULL o serve emptyString
//  verificare che succede se SDOK, HDOK ecc non sono validi quando si accede ai vari dischi.. mettere controlli
//    (anche in command.c)

// REM  SW1 è IND(1,11) , il Led è OUTD(2,14) , PGood è come SW1 (in caso) e EXCEP è OUTD(1,10) .. (partono da 1)

#include "../at_winc1500.h"
#include <ctype.h>
#include <sys/kmem.h>

#include "../genericTypedefs.h"
#include "../pc_pic_cpu.h"

#if defined(USA_USB_HOST_MSD)
#include "../harmony_pic32mz/usb_host_msd.h"
#include "../harmony_pic32mz/usb_host_scsi.h"
#endif

#include "fat_sd/fsconfig.h"
#include "fat_sd/fsio.h"
#include "fat_ram/ramfsio.h"
#include "fat_ide/idefsio.h"
#include "fat_ide/fdc.h"
//#include "../../pc_pic_video.X/Adafruit_GFX.h"
#include "../../pc_pic_video.X/Adafruit_colors.h"
#include "../../pc_pic_video.X/gfxfont.h"
#include "minibasic.h"
#include "iohw.h"

#include "harmony_app.h"
#include "superfile.h"

#if defined(USA_USB_HOST_UVC)
#include "../harmony_pic32mz/usb_host_uvc.h"
#include "picojpeg.h"
unsigned char pjpeg_need_bytes_callback(unsigned char* pBuf, unsigned char buf_size, unsigned char *pBytes_actually_read, void *pCallback_data);
#endif
extern enum CAPTURING_VIDEO capturingVideo;
extern APP_DATA appData;

extern BYTE SDcardOK,USBmemOK,HDOK,FDOK;
extern BYTE *RAMdiscArea;

#define MINIBASIC_COPYRIGHT_STRING "MiniBasic for PIC32MZ v2.2.5 - 6/12/2022\n"


#undef stricmp    // per at_winc1500 ... £$%@#
#undef strnicmp

extern BYTE eventReceived;
extern struct KEYPRESS keypress;
extern SIZE Screen,ScreenText;
extern BYTE fontFace,fontSize,fontStyle;
extern BYTE powerState;
extern signed char currDrive;

BYTE byteRecMidi,byteRec232;
extern DWORD now;
#ifdef USA_WIFI
extern BYTE myRSSI;
extern Ipv4Addr myIp;
extern uint8_t internetBuffer[256];
extern uint8_t rxBuffer[1536];
#endif


char getDrive(const char *s,SUPERFILE *f,char **filename);
BOOL hitCtrlC(BOOL);
BOOL isCtrlS(void);

extern DWORD extRAMtot;


/* tokens defined */
#define EOS 23
#define VALUE 24
#define PITOK 40
#define ETOK 41


#define B_ERROR 20
#define EOL 21
#define EQUALS 22
#define DIMFLTID 26
#define DIMSTRID 27
#define DIMINTID 28
#define QUOTE 29
#define GREATER 30
#define LESS 31
#define SEMICOLON 32
#define DIESIS 33
//#define AMPERSAND 38

#define STMT_CONTINUE 0   // in teoria LINE=0 e POS=0 non arrivano mai...
#define STMT_STOP (-1)

enum __attribute((packed)) {
	DIV=90,
	MULT,
	OPAREN,
  CPAREN,
  PLUS,
  MINUS,
  SHRIEK,
  AMPERSAND,
  COMMA,
  COLON,
  BITNOT,
  MOD,

	PRINT,PRINT2,
	LET,
  CLR,
	DEF,
	DIM,
	IF,
	THEN,
	ELSE,
	AND,
	OR,
	XOR,
	NOT,
	GOTO,
	INPUTTOK,
  OUTPUTTOK,   //123 v. doOpen
	GET,
	PUT,
  APPEND,
  SERVER,
  CLIENT,
	REM,REM2,
	FOR,
	TO,
	NEXT,
	STEP,
	GOSUB,
	RETURN,
	DO,
	LOOP,
	WHILE,
	UNTIL,
	ON,
	ERRORTOK,
	BEGIN,
	BEND,
	IRQ,
	END,
	RUN,
	SHELL,
	CONTINUE,
	STOP,
	SLEEP,
  POWER,
	SYS,
	POKE,
	EEPOKE,
	SETPIN,
	OUTD,
	OUTDAC,
	SOUND,
	TONE,
	PLAY,
	BEEP,
	OPEN,
	CLOSE,
	AS,
	DIRTOK,CD,MD,RD,
	RENAME,DELETE,FORMAT,
	CLS,LOCATE,
	LINETOK,RECTANGLE,CIRCLE,ELLIPSE,ARC,TRIANGLE,POINTTOK,FILL,SETCOLOR,SETBKCOLOR,BITBLT,
  LIST,NEW,LOAD,SAVE,QUIT,
	WIFICONNECT,
	MODEMCMD,
#if defined(USA_USB_HOST_UVC)
	WEBCAM,
#endif
  
	SIN,
  FIRST_NUM_FUNCTION=SIN,
	COS,
	TAN,
	LOG,
	EXP,
	POW,
	SQR,
	ERRTOK,
	STATUSTOK,
	PEEK,
	TIMER,
  // timezone?? o in timestring?
	ABS,
	MIN,
	MAX,
	IIF,
	LEN,
	ASC,
	ASIN,
	ACOS,
	ATAN,
	INTTOK,
	RND,
	VAL,
	VALLEN,
	MEM,
	USR,
	IND,
	INADC,
	INSTR,
	PENX,PENY,
	CURSORX,CURSORY,
	SCREENX,SCREENY,
	GETPIXEL,GETCOLOR,GETPEN,
	JOYSTICK,
	DISKSPACE,
	EEPEEK,
	TEMPERATURE,
	RSSI,
  BIOS,
  LAST_NUM_FUNCTION=BIOS,

	CHRSTRING,
  FIRST_CHAR_FUNCTION=CHRSTRING,
	STRSTRING,
	LEFTSTRING,
	RIGHTSTRING,
	MIDSTRING,
	STRINGSTRING,
	HEXSTRING,
	TRIMSTRING,
	TIMESTRING,
	INKEYSTRING,
	GETKEYSTRING,
	MIDISTRING,
	MODEMSTRING,
#ifdef USA_BREAKTHROUGH
	MENUKEYSTRING,
#endif
	ERRORSTRING,
  VERSTRING,
  DIRSTRING,
  CURDIRSTRING,
  CURDRIVESTRING,
	IPADDRESSSTRING,
	ACCESSPOINTSSTRING,
  LAST_CHAR_FUNCTION=ACCESSPOINTSSTRING,
	};


/* relational operators defined */

enum REL_OPS {
	ROP_EQ=1,         // equals 
	ROP_NEQ,	        // doesn't equal 
	ROP_LT,		        // less than 
	ROP_LTE,         // less than or equals 
	ROP_GT,          // greater than 
	ROP_GTE          // greater than or equals 
	};


BSTATIC void doLine(MINIBASIC *);
BSTATIC void doRectangle(MINIBASIC *);
BSTATIC void doCircle(MINIBASIC *);
BSTATIC void doEllipse(MINIBASIC *);
BSTATIC void doArc(MINIBASIC *);
BSTATIC void doTriangle(MINIBASIC *);
BSTATIC void doPoint(MINIBASIC *);
BSTATIC void doFill(MINIBASIC *);
BSTATIC void doSetcolor(MINIBASIC *);
BSTATIC void doSetBkcolor(MINIBASIC *);
BSTATIC void doBitblt(MINIBASIC *);


//BSTATIC int inkey(void);
  
BSTATIC const char * const error_msgs[] = {
	"",
	"Syntax error",
	"Out of memory",
	"Identifier too long",
	"No such variable",
	"Bad subscript", 
	"Too many dimensions",
	"Too many initialisers",
	"Illegal type",
	"Too many nested FOR or DO",
	"FOR/DO without matching NEXT/LOOP",    //10
	"NEXT/LOOP without matching FOR/DO",
	"No such line",
	"Divide by zero",
	"Negative logarithm",
	"Negative square root",
	"Sine or cosine out of range",
	"End of input file",
	"Illegal offset",
	"Type mismatch",
	"Input too long",   //20
	"Bad value",
	"Not an integer",
	"Too many GOSUB",
	"RETURN without GOSUB",
	"Formula too complex",
	"File error",
	"Network error",
	"BREAKPOINT (press C to continue or ESC to stop)",
	"Fatal internal error"
	};



BSTATIC const char * const EmptyString="";

BSTATIC TOKEN_LIST const tl[] = {
	{ "PRINT",5 },
	{ "?",1 },
	{ "LET",3 },
	{ "CLR",3 },
	{ "DEF",3 },
	{ "DIM",3 },
	{ "IF",2 },
	{ "THEN",4 },
	{ "ELSE",4 },
	{ "AND",3 },
	{ "OR",2 },
	{ "XOR",3 },
	{ "NOT",3 },
	{ "GOTO",4 },
	{ "INPUT",5 },
	{ "OUTPUT",6 },
	{ "GET",3 },
	{ "PUT",3 },
	{ "APPEND",6 },
	{ "SERVER",6 },
	{ "CLIENT",6 },
	{ "REM",3 },
	{ "'",1 },
	{ "FOR",3 },
	{ "TO",2 },
	{ "NEXT",4 },
	{ "STEP",4 },
	{ "GOSUB",5 },
	{ "RETURN",6 },
	{ "DO",2 },
	{ "LOOP",4 },
	{ "WHILE",5 },
	{ "UNTIL",5 },
	{ "ON",2 },
	{ "ERROR",5 },
	{ "BEGIN",5 },
	{ "BEND",4 },
	{ "IRQ",3 },
	{ "END",3 },
	{ "RUN",3 },
	{ "SHELL",5 },
	{ "CONTINUE",8 },
	{ "STOP",4 },
	{ "SLEEP",5 },
  { "POWER",5 },
	{ "SYS",3 },
	{ "POKE",4 },
	{ "EEPOKE",6 },
	{ "SETPIN",6 },
	{ "OUTD",4 },
	{ "OUTDAC",6 },
	{ "SOUND",5 },
	{ "TONE",4 },
	{ "PLAY",4 },
	{ "BEEP",4 },
	{ "OPEN",4 },
	{ "CLOSE",5 },
	{ "AS",2 },
	{ "DIR",6 }, { "CD",2 }, { "MD",2 }, { "RD",2 }, { "RENAME",3 }, { "DELETE",6 }, { "FORMAT",6 },
	{ "CLS",3 }, { "LOCATE",6 },
	{ "LINE",4 }, { "RECTANGLE",9 }, { "CIRCLE",6 }, { "ELLIPSE",7 }, { "ARC",3 }, { "TRIANGLE",8 }, { "POINT",5 }, 
  { "FILL",4 }, { "SETCOLOR",8 }, { "SETBKCOLOR",10 }, { "BITBLT",6 },
	{ "LIST",4 },
	{ "NEW",3 },
	{ "LOAD",4 },	{ "SAVE",4 },
	{ "QUIT",4 },
	{ "WIFICONNECT",11 },
	{ "MODEMCMD",8 },
#if defined(USA_USB_HOST_UVC)
	{ "WEBCAM",6 },
#endif
  
	{ "SIN",3 },
	{ "COS",3 },
	{ "TAN",3 },
	{ "LOG",3 },
	{ "EXP",3 },
	{ "POW",3 },
	{ "SQR",3 },
	{ "ERR",3 },
	{ "STATUS",6 },
	{ "PEEK",4 },
	{ "TIMER",5 },
	{ "ABS",3 },
	{ "MIN",3 },
	{ "MAX",3 },
	{ "IIF",3 },
	{ "LEN",3 },
	{ "ASC",3 },
	{ "ASIN",4 },
	{ "ACOS",4 },
	{ "ATAN",4 },
	{ "INT",3 },
	{ "RND",3 },
	{ "VAL",3 },
	{ "VALLEN",6 },
	{ "MEM",3 },
	{ "USR",3 },
	{ "IND",3 },
	{ "INADC",5 },
	{ "INSTR",5 },
	{ "PENX",4 }, { "PENY",4 }, { "CURSORX",7 },{ "CURSORY",7 }, { "SCREENX",7 },{ "SCREENY",7 }, 
	{ "GETPIXEL",8 }, { "GETCOLOR",8 }, { "GETPEN",6 },

  { "JOYSTICK",8 },
  { "DISKSPACE",9 },
	{ "EEPEEK",6 },
	{ "TEMPERATURE",11 },
	{ "RSSI",4 },
	{ "BIOS",4 },

	{ "CHR$",4 },
	{ "STR$",4 },
	{ "LEFT$",5 },
	{ "RIGHT$",6 },
	{ "MID$",4 },
	{ "STRING$",7 },
	{ "HEX$",4 },
	{ "TRIM$",5 },
	{ "TIME$",5 },
	{ "INKEY$",6 },
	{ "GETKEY$",7 },
	{ "MIDI$",5 },
	{ "MODEM$",6 },
#ifdef USA_BREAKTHROUGH
	{ "MENUKEY$",8 },
#endif
	{ "ER$",3 },
	{ "VER$",4 },
	{ "DIR$",4 },
	{ "CURDIR$",7 },
  { "CURDRIVE$",9 },
  { "IPADDRESS$",10 },
	{ "ACCESSPOINTS$",13 },
	};


#define basicAssert(x) //myTextOut(mInstance,"assert!") //assert((x))



BSTATIC int setup(MINIBASIC *,char *script);
BSTATIC void cleanup(MINIBASIC *,uint8_t);
BSTATIC void subClose(MINIBASIC *,int f);

BSTATIC void reportError(MINIBASIC *,int16_t lineno);
BSTATIC int16_t findLine(MINIBASIC *,LINE_NUMBER_TYPE no);
BSTATIC int16_t findDef(MINIBASIC *,const char *id);
BSTATIC VARIABLE *addNewVar(MINIBASIC *mInstance,const char *id, char type);

BSTATIC LINE_NUMBER_STMT_POS_TYPE line(MINIBASIC *);
BSTATIC void doPrint(MINIBASIC *);
BSTATIC void doLet(MINIBASIC *,int8_t);
BSTATIC void doClr(MINIBASIC *);
BSTATIC void doDef(MINIBASIC *);
BSTATIC void doDim(MINIBASIC *);
BSTATIC LINE_NUMBER_STMT_POS_TYPE doIf(MINIBASIC *);
BSTATIC LINE_NUMBER_STMT_POS_TYPE subGoto(MINIBASIC *,uint16_t);
BSTATIC LINE_NUMBER_STMT_POS_TYPE doGoto(MINIBASIC *);
BSTATIC LINE_NUMBER_STMT_POS_TYPE doGosub(MINIBASIC *);
BSTATIC LINE_NUMBER_STMT_POS_TYPE doReturn(MINIBASIC *);
BSTATIC LINE_NUMBER_STMT_POS_TYPE doOn(MINIBASIC *);
BSTATIC void doInput(MINIBASIC *);
BSTATIC void doGet(MINIBASIC *);
BSTATIC void doPut(MINIBASIC *);
BSTATIC void doRem(MINIBASIC *);
BSTATIC LINE_NUMBER_STMT_POS_TYPE doFor(MINIBASIC *);
BSTATIC LINE_NUMBER_STMT_POS_TYPE doNext(MINIBASIC *);
BSTATIC LINE_NUMBER_STMT_POS_TYPE doDo(MINIBASIC *);
BSTATIC LINE_NUMBER_STMT_POS_TYPE doLoop(MINIBASIC *);
BSTATIC void doPoke(MINIBASIC *);
BSTATIC void doEEPoke(MINIBASIC *);
BSTATIC void doOpen(MINIBASIC *);
BSTATIC void doClose(MINIBASIC *);
BSTATIC void doSleep(MINIBASIC *);
BSTATIC void doPower(MINIBASIC *);
BSTATIC void doSetpin(MINIBASIC *);
BSTATIC void doSound(MINIBASIC *);
BSTATIC void doTone(MINIBASIC *);
BSTATIC void doPlay(MINIBASIC *);
BSTATIC void doBeep(MINIBASIC *);
BSTATIC void doOutd(MINIBASIC *);
BSTATIC void doOutdac(MINIBASIC *);
BSTATIC void doSys(MINIBASIC *);
BSTATIC void doStop(MINIBASIC *);
BSTATIC void doError(MINIBASIC *);
BSTATIC void doDir(MINIBASIC *);
BSTATIC void doFormat(MINIBASIC *);
BSTATIC void doDelete(MINIBASIC *);
BSTATIC void doRename(MINIBASIC *);
BSTATIC void doCD(MINIBASIC *);
BSTATIC void doRD(MINIBASIC *);
BSTATIC void doMD(MINIBASIC *);
BSTATIC void doCls(MINIBASIC *);
BSTATIC void doLocate(MINIBASIC *);
BSTATIC int16_t doRun(MINIBASIC *);
BSTATIC void doShell(MINIBASIC *);
BSTATIC void doContinue(MINIBASIC *);
BSTATIC void doList(MINIBASIC *);
BSTATIC void doNew(MINIBASIC *);
BSTATIC void doLoad(MINIBASIC *);
BSTATIC void doSave(MINIBASIC *);
// inutile! void doQuit(MINIBASIC *);
BSTATIC void doWificonnect(MINIBASIC *);
BSTATIC void doModemcmd(MINIBASIC *);
#if defined(USA_USB_HOST_UVC)
void doWebcam(MINIBASIC *);
#endif

BSTATIC void lvalue(MINIBASIC *,LVALUE *lv);
BSTATIC int subLoad(const char *,char *);

BSTATIC signed char boolExpr(MINIBASIC *);
BSTATIC signed char rop(signed char, NUM_TYPE, NUM_TYPE);
BSTATIC signed char iRop(signed char, int16_t, int16_t);
BSTATIC signed char strRop(signed char, const char *, const char *);
BSTATIC signed char boolFactor(MINIBASIC *);
BSTATIC enum REL_OPS relOp(MINIBASIC *);


BSTATIC NUM_TYPE expr(MINIBASIC *);
BSTATIC NUM_TYPE term(MINIBASIC *);
BSTATIC NUM_TYPE factor(MINIBASIC *);
BSTATIC NUM_TYPE instr(MINIBASIC *);
BSTATIC NUM_TYPE variable(MINIBASIC *);
BSTATIC int16_t ivariable(MINIBASIC *);
BSTATIC NUM_TYPE dimVariable(MINIBASIC *);
BSTATIC int16_t dimivariable(MINIBASIC *);


BSTATIC VARIABLE *findVariable(MINIBASIC *,const char *id);
BSTATIC DIMVAR *findDimVar(MINIBASIC *,const char *id);
BSTATIC DIMVAR *dimension(MINIBASIC *,const char *id, BYTE ndims, ...);
BSTATIC void *getDimVar(MINIBASIC*,DIMVAR *dv, ...);
BSTATIC VARIABLE *addFloat(MINIBASIC *,const char *id);
BSTATIC VARIABLE *addString(MINIBASIC *,const char *id);
BSTATIC VARIABLE *addInt(MINIBASIC *,const char *id);
BSTATIC DIMVAR *adddimvar(MINIBASIC *,const char *id);

BSTATIC char *stringExpr(MINIBASIC *);
BSTATIC char *chrString(MINIBASIC *);
BSTATIC char *strString(MINIBASIC *);
BSTATIC char *leftString(MINIBASIC *);
BSTATIC char *rightString(MINIBASIC *);
BSTATIC char *midString(MINIBASIC *);
BSTATIC char *stringString(MINIBASIC *);
BSTATIC char *hexString(MINIBASIC *);
BSTATIC char *trimString(MINIBASIC *);
BSTATIC char *timeString(MINIBASIC *);
BSTATIC char *inkeyString(MINIBASIC *);
BSTATIC char *getkeyString(MINIBASIC *);
BSTATIC char *midiString(MINIBASIC *);
BSTATIC char *modemString(MINIBASIC *);
#ifdef USA_BREAKTHROUGH
BSTATIC char *menukeyString(MINIBASIC *);
#endif
BSTATIC char *errorString(MINIBASIC *);
BSTATIC char *verString(MINIBASIC *);
BSTATIC char *stringDimVar(MINIBASIC *);
BSTATIC char *stringVar(MINIBASIC *);
BSTATIC char *stringLiteral(MINIBASIC *);
BSTATIC char *dirString(MINIBASIC *);
BSTATIC char *curdirString(MINIBASIC *);
BSTATIC char *curdriveString(MINIBASIC *);
BSTATIC char *ipaddressString(MINIBASIC *);
BSTATIC char *accesspointsString(MINIBASIC *);

BSTATIC int integer(MINIBASIC *,NUM_TYPE x);

BSTATIC void match(MINIBASIC *,TOKEN_NUM tok);
BSTATIC void setError(MINIBASIC *,enum INTERPRETER_ERRORS errorcode);
BSTATIC LINE_NUMBER_TYPE getNextLine(MINIBASIC *,const char *str);
BSTATIC LINE_NUMBER_STMT_POS_TYPE getNextStatement(MINIBASIC *,const char *str);
BSTATIC TOKEN_NUM getToken(const char *str);
BSTATIC uint8_t tokenLen(MINIBASIC *,const char *str, TOKEN_NUM token);

BSTATIC signed char isString(TOKEN_NUM token);
BSTATIC NUM_TYPE getValue(const char *str, IDENT_LEN *len);
BSTATIC void getId(MINIBASIC *,const char *str, char *out, IDENT_LEN *len);

static void mystrgrablit(char *dest, const char *src);
static const char *mystrend(const char *str, char quote);
static unsigned short int myStrCount(const char *str, char ch);
static char *myStrConcat(const char *str, const char *cat);
BSTATIC double factorial(double x);

BSTATIC char myitoabuf[17];
BSTATIC const char *skipSpaces(const char *);
int8_t strnicmp(const char *, const char *, size_t);

const char MiniBasicCopyrightString[]= {MINIBASIC_COPYRIGHT_STRING};
static char *scriptAllocated=NULL;

extern SIZE Screen;

BSTATIC int myTextOut(MINIBASIC *mInstance,const char *s);
BSTATIC int myCR(MINIBASIC *mInstance);


//====================================================== MAIN ===================================================================
int minibasic(MINIBASIC *hInstance,const char *scriptName) {
  BYTE ch;
  int i;
  char *cmdPointer,*parmsPointer;
  char commandLine[128]={0},lastCommandLine[128]={0};
  int commandLineCnt=0;

    
/*	static const char rom greeting[] = 
		"Minibasic-PIC 0.1\n"
#if !defined(__18CXX)
		"R-Run\n"
		"L-List\n"
		#ifdef EDITOR
		"E-Edit\n"
		#endif
		"P-Program\n\n";
#else
		"\n";
#endif
		*/

  cleanup(hInstance,0);

//------------------------------------------------------------- Send CopyRight Message ----------------------------------------------------------
  hInstance->incomingChar[0]=hInstance->incomingChar[1]=0;
  hInstance->errorFlag=0;
#ifdef USA_BREAKTHROUGH
  HDC hDC;
  GetDC(hInstance->hWnd,&hDC);
	SetTextColor(&hDC,BRIGHTGREEN);
 	TextOut(&hDC,0,0,MiniBasicCopyrightString);  // c'è anche in WM_CREATE di là, unire...
#else
	SetColors(BRIGHTGREEN,BLACK);
  Cls();
  hInstance->Cursor.x=0; hInstance->Cursor.y=0;
 	myTextOut(hInstance,MiniBasicCopyrightString/*greeting*/);  // 
  myCR(hInstance);
  sprintf(commandLine,"%u Bytes free.", getTotRAM());
  myTextOut(hInstance,commandLine);
  *commandLine=0;
#endif
  
#ifdef USA_BREAKTHROUGH
  // usare eventi..
#else
  T6CON=0;
  T6CONbits.TCS = 0;            // clock from peripheral clock
  T6CONbits.TCKPS = 0b111;      // 1:256 prescaler 
  T6CONbits.T32=1;      // 32bit
  PR6 = 15625;                  // 1Hz x Timer Event ON TIMER 
  PR7 = 25;                  // 
  
  T6CONbits.TON = 1;    // start timer (for pwm)
#endif

  myCR(hInstance);
  myTextOut(hInstance,"Ready.");
  myCR(hInstance); /*deve/devono dipendere da xsize.. */

  keypress.key=keypress.modifier=0;

//------------------------------------------------------------- Run Script -----------------------------------------------------------------------
  
  do {
    
    handle_events();
    hInstance->incomingChar[0]=keypress.key;
    hInstance->incomingChar[1]=keypress.modifier;
    KBClear();
    
#if defined(USA_USB_HOST) || defined(USA_USB_SLAVE_CDC)
    SYS_Tasks();
#ifdef USA_USB_HOST_MSD
    SYS_FS_Tasks();
#endif
#else
    APP_Tasks();
#endif

#ifdef USA_WIFI
  	m2m_wifi_handle_events(NULL);
#endif
#ifdef USA_ETHERNET
    StackTask();
    StackApplications();
#endif

    if(hInstance->incomingChar[0]) {

     if((hInstance->incomingChar[0]>=' ' && hInstance->incomingChar[0]<=0x7e) ||
      hInstance->incomingChar[0]=='\r' || hInstance->incomingChar[0]=='\n' || hInstance->incomingChar[0]=='\x8') {

      ch=hInstance->incomingChar[0];
      hInstance->incomingChar[0]=0;

      if(commandLineCnt<sizeof(commandLine)) {
        commandLine[commandLineCnt++]=ch;
        commandLine[commandLineCnt]=0;
        hInstance->Cursor.x++;
        if(hInstance->Cursor.x >= ScreenText.cx) {
          myCR(hInstance);
          }
        
        switch(ch) {
          default:
            if(isprint(ch))
              putchar(ch);
            break;
          case '\x8':
            // backspace
            commandLine[--commandLineCnt]=0;    // xché cmq l'ho messo in commandline...
            hInstance->Cursor.x--;
            if(commandLineCnt>0) {
              commandLine[--commandLineCnt]=0;
              hInstance->Cursor.x--;
              putchar('\x8');
              }
//            SetXY(1,hInstance->Cursor.x*1 /*textsize*/ ,hInstance->Cursor.y*1 /*textsize*/ );
//            putchar(' ');   // o mandare 0x08 e far fare alla scheda video??
            break;
          case '\r':
            commandLine[commandLineCnt--]=0;            // pulisco cmq il successivo, in caso di prec. comando lungo..
            commandLine[commandLineCnt]=0;            // tolgo LF
            break;
          case '\n':
            commandLine[--commandLineCnt]=0;            // tolgo CR NO! compatibilità con minibasic-script
            myCR(hInstance);
  
            cmdPointer=&commandLine[0];   // per warning...
//            myTextOut(hInstance,cmdPointer);

            hInstance->errorFlag=0;
            hInstance->nfors=hInstance->ngosubs=hInstance->ndos=hInstance->inBlock=0;
            
            strcpy(lastCommandLine,commandLine);

            if(!strnicmp(commandLine,"RUN",3)) {
              parmsPointer=(char *)skipSpaces(cmdPointer+3);
              if(*parmsPointer=='\"') {
                SUPERFILE f;
                char buf[128],*p,*filename;
                // OVVIAMENTE andare anche su C: E: !
                
                strncpy(buf,parmsPointer+1,120);
                if(p=strchr(buf,'\"'))
                  *p=0;
                if(!strchr(buf,'.'))
                  strcat(buf,".BAS");

                getDrive(buf,&f,&filename);
                p=theScript;
                if(SuperFileOpen(&f,filename,'r')) { 
                  while(1) {
                    if(SuperFileRead(&f,p,1) == 1) {  // o FSfgets??
                      if(*p != 13 && *p != 9)    // cmq tolgo CR e TAB
                        p++;
                      }
                    else
                      break;
                    }
                  *p=0;
                  SuperFileClose(&f);
                  i=basic(hInstance,theScript,0);
  //                if(!i)
  //                  break;
                  goto ready2;
                  }
                else {
                  myTextOut(hInstance,"Program not found");
                  myCR(hInstance);
                  goto ready;
                  }
                }     // se nomefile..
              else {
                goto execline;
                }
              }
            else if(!strnicmp(commandLine,"QUIT",4)) {    // ev. returnCode/errorLevel?
              goto fine;
              break;
              }
            else {
          		if(*cmdPointer) {
execline:
                hInstance->string = cmdPointer;
                hInstance->token = getToken(hInstance->string);
                hInstance->errorFlag = 0;
                /*i = (int)*/line(hInstance);
                if(hInstance->errorFlag) {
                  myTextOut(hInstance,error_msgs[hInstance->errorFlag]);
                  ErrorBeep(200);
                  hInstance->errorFlag=0;
                  myCR(hInstance);
                  }
ready2:
                myCR(hInstance);
ready:
                myTextOut(hInstance,"Ready.");
                }
              else {
//                putchar('\n'); 
                }
              }
            myCR(hInstance);
            
            commandLineCnt=0;
            memset(commandLine,0,sizeof(commandLine));;

            break;
          }

        }
      }     // char accepted
    else {
      switch(hInstance->incomingChar[0]) {
        case 0xa1: // F1
//          strcpy(commandLine,lastCommandLine);
//          commandLineCnt=strlen(commandLine);
//          myTextOut(hInstance,commandLine);
          if(commandLineCnt<strlen(lastCommandLine)) {
            char buf[2];
            commandLine[commandLineCnt]=lastCommandLine[commandLineCnt];
            buf[0]=commandLine[commandLineCnt]; buf[1]=0;
            commandLine[++commandLineCnt]=0;
            myTextOut(hInstance,buf);
            }
          break;
        case 0xa3: // F3
          while(commandLineCnt<strlen(lastCommandLine)) {
            char buf[2];
            commandLine[commandLineCnt]=lastCommandLine[commandLineCnt];
            buf[0]=commandLine[commandLineCnt]; buf[1]=0;
            commandLine[++commandLineCnt]=0;
            myTextOut(hInstance,buf);
            }
          break;
        case 0x94: // up, 
          break;
        case 0x91: // right
          // gestire frecce qua?
          break;
        }
      }
      }
    
//    handleWinTimers();
//    manageWindows(i);    // fare ogni mS

    } while(1);


//------------------------------------------------------------- Finished running Basic Script ---------------------------------------------------
fine:
          
#ifdef USA_BREAKTHROUGH
 	do_print(&hDC,"\nPress a key to restart");
  //compila ma non esiste???!!! 2/1/22
#else
#endif
  
  if(scriptAllocated) {
    free((void *)scriptAllocated);
    scriptAllocated=NULL;
    }

  cleanup(hInstance,0);

#ifdef USA_BREAKTHROUGH
  ReleaseDC(hInstance->hWnd,&hDC);
#else
#endif

  SetCursorMode(1,1);

  
	return 0;
	}



//============================= Basic Interpreter Start ================================
/*
  Interpret a BASIC script

  Params: script - the script to run
  Returns: 0 on success, 1 on error condition.
*/

int basic(MINIBASIC *mInstance,const char *script,BYTE where) {
  LINE_NUMBER_STMT_POS_TYPE nextstmt;
  int answer = 0;

  mInstance->incomingChar[0]=mInstance->incomingChar[1]=0;
  mInstance->errorFlag=ERR_CLEAR;
  
  mInstance->curline=0;

  mInstance->ColorPaletteBK=textColors[0] /*BLACK*/;
  mInstance->ColorBK=ColorRGB(mInstance->ColorPaletteBK);
	mInstance->ColorPalette=textColors[7] /*WHITE*/;
  mInstance->Color=ColorRGB(mInstance->ColorPalette);
//	setTextColorBG(textColors[mInstance->ColorPalette],textColors[mInstance->ColorPaletteBK]); 
#ifndef USING_SIMULATOR
  Cls();
	mInstance->Cursor.x=0;
  mInstance->Cursor.y=0;
  SetCursorMode(0,0);
#endif
 	
  if(where) {
    subLoad((char *)script,theScript);
    }
  else {
    strcpy(theScript,script);
    }

  if(setup(mInstance,(char *)theScript) == -1) {				//Run setup Function to extract data from the Script
    answer=-1;										//Script error END
    goto end_all;
    }

#ifdef USA_BREAKTHROUGH
#else
  T6CON=0;
  T6CONbits.TCS = 0;            // clock from peripheral clock
  T6CONbits.TCKPS = 0b111;      // 1:256 prescaler 
  T6CONbits.T32=1;      // 32bit
  PR6 = 15625;                  // 1Hz x Timer Event ON TIMER 
  PR7 = 25;                  // 
  
  T6CONbits.TON = 1;    // start timer (for pwm)
#endif
  
  
  mInstance->string = mInstance->lines[mInstance->curline].str;
 	for(;;) {			//Loop through Script Running each Line

		mInstance->token = getToken(mInstance->string);
		
		mInstance->errorFlag=ERR_CLEAR;
		
    if(mInstance->token == VALUE)   // se siamo a inizio di una riga...
      match(mInstance,VALUE);
		nextstmt = line(mInstance);
    
    if(mInstance->errorFlag != ERR_CLEAR) {
			if(!mInstance->errorHandler.handler)
				reportError(mInstance,mInstance->lines[mInstance->curline].no);

//      printf("errorhandler: %u,%u\r\n",mInstance->errorFlag,mInstance->errorHandler.handler);
      
      if(mInstance->errorFlag == ERR_STOP)	{
        int i;

#if 0
        do {
          i=mInstance->incomingChar;
          Yield(mInstance->threadID);
          ClrWdt();
          }	while(i != 'C' && i != '\x1b');
#else
//          SYS_Tasks();
          goto handle_error2;
#endif

        if(hitCtrlC(TRUE))	{ 
//        if(i == '\x1b')	{     // usare CTRL-C ....
          goto handle_error2;
          }
        }
      else	{
//handle_error:
        if(((int16_t)mInstance->errorHandler.handler)==-1)	{   // ON ERROR CONTINUE
          mInstance->errorFlag=ERR_CLEAR;
//      		nextstmt.d = STMT_CONTINUE;
          // se no SALTA ALLA RIGA DOPO!! unire con ricerca COLON in line() come anche REM ELSE ecc
      		nextstmt.line=mInstance->curline;
          nextstmt.pos=mInstance->string-mInstance->lines[mInstance->curline].str;
          if(*mInstance->string == '\n') {   // questo lo gestisco qua, perché ERRORHANDLER va prima del resto e CONTINUE può causare un loop!
            nextstmt.line++; nextstmt.pos=0;
            }
          }
        else if(mInstance->errorHandler.handler)	{
//          LINE_NUMBER_STMT_POS_TYPE oldnextstmt=nextstmt;
          nextstmt=subGoto(mInstance,mInstance->errorHandler.handler);
          if(nextstmt.line == -1)   // errore nell'errore :)
            goto handle_error2;
          mInstance->errorHandler.errorcode=mInstance->errorFlag;
          mInstance->errorFlag=ERR_CLEAR;
          mInstance->errorHandler.line.line=mInstance->curline;
          mInstance->errorHandler.line.pos=mInstance->string-mInstance->lines[mInstance->curline].str;
          }
        else	{
handle_error2:
          answer=1;
          break;
          }
        }
      }

		if((int32_t)nextstmt.d == STMT_STOP)			// p.es. comando END
	  	break;

		if(nextstmt.d == STMT_CONTINUE)	{				//Line increment from 0 to 1
  /*    
 	  if(mInstance->token == ELSE || mInstance->token==REM || mInstance->token==REM2) {
			goto do_rem;
			}

	  if(mInstance->token != EOS) {
			str = mInstance->string;
			while(isspace(*str)) {
			  if(*str == '\n' || *str == ':')
			    break;
			  str++;
				}
	
			if(*str == ':') {   // questa roba potrebbe andare sopra nel loop principale... quando si restituisce 0... ?!
				match(mInstance,COLON);
				if(!mInstance->errorFlag)			// specie per STOP
			  	goto rifo;
				else
					goto fine;
				}
			if(*str != '\n')
			  setError(mInstance,ERR_SYNTAX);
	  	}
*/
    	mInstance->curline++;
 			if(mInstance->curline == mInstance->nlines)			//check if last line has been read
   			break;
   		}
		else {							//find next line
      if((int16_t)nextstmt.line != mInstance->curline) {
        if((int16_t)nextstmt.line == -1) {
          char buf[32];
          mInstance->ColorPaletteBK=textColors[0] /*BLACK*/;    // unire con reportError...
          mInstance->ColorBK=ColorRGB(mInstance->ColorPaletteBK);
          mInstance->ColorPalette=textColors[7] /*WHITE*/;
          mInstance->Color=ColorRGB(mInstance->ColorPalette);
          // v. anche ERR_NOSUCHLINE 2022
          sprintf(buf,"line %u not found", mInstance->lines[mInstance->curline].no);		// QUESTO NON viene trappato.. non ha senso..!
//#warning RIMETTERE NUM RIGA GIUSTO, PER TUTTI
          myTextOut(mInstance,buf);
          myCR(mInstance);
          goto handle_error2;
          }
        else
set_line:
          mInstance->curline = nextstmt.line;
        }
   		}
    mInstance->string = mInstance->lines[mInstance->curline].str + nextstmt.pos;
		ClrWdt();
    
#ifndef USING_SIMULATOR
		if(IFS0bits.INT0IF)	{     // boh attivare/usare se serve...
      IFS0bits.INT0IF=0;
			if(mInstance->irqHandler.handler && !mInstance->irqHandler.line.line) {   // evito rientro
        nextstmt=subGoto(mInstance,mInstance->irqHandler.handler);
        if(nextstmt.line == -1)   // errore 
          goto handle_error2;
        mInstance->irqHandler.line.line=mInstance->curline;
        mInstance->irqHandler.line.pos=mInstance->string-mInstance->lines[mInstance->curline].str;
        goto set_line;
        }
      }
#endif
		if(IFS1bits.T7IF)	{
      IFS1bits.T7IF=0;
			if(mInstance->timerHandler.handler && !mInstance->timerHandler.line.line) {   // evito rientro
        nextstmt=subGoto(mInstance,mInstance->timerHandler.handler);
        if(nextstmt.line == -1)   // errore 
          goto handle_error2;
        mInstance->timerHandler.line.line=mInstance->curline;
        mInstance->timerHandler.line.pos=mInstance->string-mInstance->lines[mInstance->curline].str;
        goto set_line;
        }
      }
    
    Yield(mInstance->threadID);
    
    mLED_1 ^= 1;     //  

//    if(mInstance->incomingChar == '\x1b')
//      if(mInstance->incomingChar[0] =='c' && mInstance->incomingChar[1] == 0b00000001)
#warning FINIRE modifier qua!
//        break;
      if(hitCtrlC(TRUE)) {    // opp. così...
        setError(mInstance,ERR_STOP);
        break;
      }
      

    handle_events();
    mInstance->incomingChar[0]=keypress.key; mInstance->incomingChar[1]=keypress.modifier;
    KBClear();

#ifndef USING_SIMULATOR
#if defined(USA_USB_HOST) || defined(USA_USB_SLAVE_CDC)
    SYS_Tasks();
#ifdef USA_USB_HOST_MSD
    SYS_FS_Tasks();
#endif
#else
    APP_Tasks();
#endif

#ifdef USA_WIFI
  	m2m_wifi_handle_events(NULL);
#endif
#ifdef USA_ETHERNET
    StackTask();
    StackApplications();
#endif
#endif

  	} //While finish


end_all:
  cleanup(mInstance,1);
  SetCursorMode(1,1);
  KBClear();
  
  return answer;
	}

//===============================================================================================================================================================



/*
  Sets up all our globals, including the list of lines.
  Params: script - the script passed by the user
  Returns: 0 on success, -1 on failure
*/
int setup(MINIBASIC *mInstance,char *script) {
  int i;
  char buf[64];

  mInstance->nlines = myStrCount(script,'\n');						//Count the lines in the Basic Script by counting \n
  mInstance->lines = (LINE *)malloc(mInstance->nlines * sizeof(LINE));
  if(!mInstance->lines) {
    myTextOut(mInstance,"Out of memory");
    myCR(mInstance);
		return -1;
  	}
  for(i=0; i<mInstance->nlines; i++) {
		if(isdigit(*script)) {
      mInstance->lines[i].str = script;
	  	mInstance->lines[i].no = atoi(script);
      if(!mInstance->lines[i].no) {
        sprintf(buf,"line %d not allowed", mInstance->lines[i].no); //:)
        goto errore;
        }
			}
		else {
	  	i--;
	  	mInstance->nlines--;
			}
		script = (char *)strchr((char *)script,'\n');

		script++;
  	}
  if(!mInstance->nlines) {
    sprintf(buf,"Can't read program");
    goto errore;
		return -1;
  	}

  for(i=1; i<mInstance->nlines; i++) {
		if(mInstance->lines[i].no <= mInstance->lines[i-1].no) {
      sprintf(buf,"program lines %d and %d not in order", 
	  		mInstance->lines[i-1].no, mInstance->lines[i].no);
errore:
      myTextOut(mInstance,buf);
      myCR(mInstance);
	  	free(mInstance->lines); mInstance->lines=NULL;
	  	return -1;
  		}
		}

  mInstance->nvariables = 0;
  mInstance->variables = NULL;

  mInstance->ndimVariables = 0;
  mInstance->dimVariables = NULL;

  return 0;
	}


/*
  frees all the memory we have allocated
*/
void cleanup(MINIBASIC *mInstance,uint8_t alsoprogram) {
  int i;
  int ii;
  int size;

  T5CON=0;

	for(i=0; i<mInstance->nfiles; i++) {
    subClose(mInstance,i);
    }
  
  for(i=0; i<mInstance->nvariables; i++) {
		if(mInstance->variables[i].type == STRID) {
			if(mInstance->variables[i].d.sval)
		  	free(mInstance->variables[i].d.sval);
			mInstance->variables[i].d.sval=0;
			}
		}
  if(mInstance->variables)
	 	free(mInstance->variables);
  mInstance->variables = NULL;
  mInstance->nvariables = 0;

  for(i=0; i<mInstance->ndimVariables; i++) {
    if(mInstance->dimVariables[i].type == STRID)	{
	  	if(mInstance->dimVariables[i].d.str) {
				size = 1;
        for(ii=0; ii<mInstance->dimVariables[i].ndims; ii++)
		  		size *= mInstance->dimVariables[i].dim[ii];
	    	for(ii=0; ii<size; ii++) {
		  		if(mInstance->dimVariables[i].d.str[ii])
		    		free(mInstance->dimVariables[i].d.str[ii]);
	    		mInstance->dimVariables[i].d.str[ii]=NULL;
					}
				free(mInstance->dimVariables[i].d.str);
				mInstance->dimVariables[i].d.str=NULL;
	  		}
			}
		else {
		  if(mInstance->dimVariables[i].d.dval)
				free(mInstance->dimVariables[i].d.dval);
		  mInstance->dimVariables[i].d.dval=0;
			}
  	}

  if(mInstance->dimVariables)
		free(mInstance->dimVariables);
  mInstance->dimVariables=NULL;
 
  mInstance->ndimVariables = 0;

	if(alsoprogram) {
	  if(mInstance->lines)
			free(mInstance->lines);
	  mInstance->lines = NULL;
  	mInstance->nlines = 0;
		}

	mInstance->nfors=0;
	mInstance->ngosubs=0;
	mInstance->ndos=0;
	mInstance->inBlock=0;

	mInstance->errorHandler.handler=0;
	mInstance->errorHandler.line.d=0;
	mInstance->errorHandler.errorcode=0;
	mInstance->irqHandler.handler=0;
	mInstance->irqHandler.line.d=0;
	mInstance->irqHandler.errorcode=0;
	mInstance->timerHandler.handler=0;
	mInstance->timerHandler.line.d=0;
	mInstance->timerHandler.errorcode=0;
	}


/*
  error report function.
  for reporting errors in the user's script.
  checks the global errorFlag.
  writes to fperr.
  Params: lineno - the line on which the error occurred
*/
BSTATIC void reportError(MINIBASIC *mInstance, int16_t lineno) {

// era un'idea :)  if(mInstance->Color==mInstance->ColorBK)
//    mInstance->Color = ~mInstance->ColorBK;
  mInstance->ColorPaletteBK=textColors[0] /*BLACK*/;
  mInstance->ColorBK=ColorRGB(mInstance->ColorPaletteBK);
	mInstance->ColorPalette=textColors[7] /*WHITE*/;
  mInstance->Color=ColorRGB(mInstance->ColorPalette);
  
  if(mInstance->errorFlag == ERR_CLEAR) {
	  basicAssert(0);
		}
	else if(mInstance->errorFlag >= ERR_SYNTAX && mInstance->errorFlag <= ERR_STOP) {
// tolto a-capo, specie su display piccoli...
    if(lineno != -1) {
      char buf[64];
      sprintf(buf,"%s at line %u",error_msgs[mInstance->errorFlag],lineno);
      myTextOut(mInstance,buf);
	    goto acapo;
      }
    else {
      myTextOut(mInstance,error_msgs[mInstance->errorFlag]);
	    goto acapo;
      }
		}
	else {
    if(lineno != -1) {
      char buf[32];
  	  sprintf(buf,"ERROR at line %u", lineno);
      myTextOut(mInstance,buf);
      }
    else
      myTextOut(mInstance,"ERROR");
acapo:
    myCR(mInstance);
    myCR(mInstance);
	  }
  ErrorBeep(300);
	}


/*
  binary search for a line
  Params: no - line number to find
  Returns: index of the line, or -1 on fail.
*/
BSTATIC int16_t findLine(MINIBASIC *mInstance, LINE_NUMBER_TYPE no) {    // 
  int high;
  int low;
  int mid;

  low = 0;
  high = mInstance->nlines-1;
  while(high > low + 1) {
    mid = (high + low)/2;
		if(mInstance->lines[mid].no == no)
		  return mid;
		if(mInstance->lines[mid].no > no)
		  high = mid;
		else
		  low = mid;
	  }

  if(mInstance->lines[low].no == no)
		mid = low;
  else if(mInstance->lines[high].no == no)
		mid = high;
  else
		mid = -1;

  return mid;
	}

BSTATIC int16_t findDef(MINIBASIC *mInstance,const char *id) {
  const char *p;
  int i;
  uint8_t t;

  for(i=0; i<mInstance->nlines; i++) {
		p = mInstance->lines[mInstance->curline].str;
		t=getToken(p);    // dev'essere VALUE = #riga
    p += tokenLen(mInstance, p, mInstance->token);
		if(t=getToken(p) == DEF) {    // dev'essere DEF
      p += tokenLen(mInstance, p, mInstance->token);
      p=skipSpaces(p);
      if(!strnicmp(p,id,strlen(id))) {
        return i;
        }
    	}
  	}
  return -1;
  }


/*
  Parse a line. High level parse function
*/
LINE_NUMBER_STMT_POS_TYPE line(MINIBASIC *mInstance) {
  LINE_NUMBER_STMT_POS_TYPE answer;
  const char *str;

rifo:
	answer.d = STMT_CONTINUE;
  switch(mInstance->token) {
    case PRINT:
    case PRINT2:
		  doPrint(mInstance);
		  break;
		case POKE:
			doPoke(mInstance);
			break;
		case EEPOKE:
			doEEPoke(mInstance);
			break;
		case SYS:
			doSys(mInstance);
			break;
		case SLEEP:
			doSleep(mInstance);
			break;
    case POWER:
      doPower(mInstance);
      break;
		case STOP:
			doStop(mInstance);
			break;
    case SETPIN:
		  doSetpin(mInstance);
			break;
		case OUTD:
			doOutd(mInstance);
			break;
		case OUTDAC:
//			dotwoset();
			doOutdac(mInstance);
			break;
		case SOUND:
			doSound(mInstance);
			break;
		case TONE:
//			dothreeset(); doTone()
			doTone(mInstance);    //MessageBeep ..
			break;
		case PLAY:
			doPlay(mInstance);
			break;
		case BEEP:
			doBeep(mInstance);
			break;
		case OPEN:
			doOpen(mInstance);
			break;
		case CLOSE:
			doClose(mInstance);
			break;
		case DIRTOK:     // 
			doDir(mInstance);
			break;
		case RENAME:     // 
			doRename(mInstance);
			break;
		case DELETE:     // 
			doDelete(mInstance);
			break;
		case CD:
			doCD(mInstance);
			break;
		case MD:
			doMD(mInstance);
			break;
		case RD:
			doRD(mInstance);
			break;
		case FORMAT:
			doFormat(mInstance);
			break;
		case CLS:
			doCls(mInstance);
			break;
		case LOCATE:
			doLocate(mInstance);
			break;
    case LET:
		  doLet(mInstance,1);
		  break;
		case DEF:
		  doDef(mInstance);
		  break;
		case DIM:
		  doDim(mInstance);
		  break;
		case IF:
		  answer = doIf(mInstance);
			if(answer.d == STMT_STOP)			// 
				goto rifo;
		  break;
		case END:
      match(mInstance,END);
		  answer.d = STMT_STOP;
		  break;
		case GOTO:
		  answer = doGoto(mInstance);
		  break;
		case GOSUB:
			answer = doGosub(mInstance);
			break;
		case ERRORTOK:
			doError(mInstance);
			break;
		case RETURN:
			answer = doReturn(mInstance);
			break;
		case ON:
		  answer = doOn(mInstance);
		  break;
		case RUN:
			answer.line = doRun(mInstance);
			break;
		case SHELL:
			/*answer = */doShell(mInstance);
			break;
		case CONTINUE:
			doContinue(mInstance);
			break;
		case LIST:
      doList(mInstance);
		  break;
		case NEW:
      doNew(mInstance);
		  answer.d = STMT_STOP;    // per interrompere esecuzione :)
		  break;
		case CLR:
      doClr(mInstance);
		  break;
		case LOAD:
      doLoad(mInstance);
		  answer.d = STMT_STOP;    //
		  break;
		case SAVE:
      doSave(mInstance);
		  answer.d = STMT_STOP;    // e boh..
		  break;
		case QUIT:
//      doQuit(mInstance);
		  answer.d = STMT_STOP;    // per interrompere esecuzione :)
		  break;

		case INPUTTOK:
		  doInput(mInstance);
		  break;
		case GET:
		  doGet(mInstance);
		  break;
		case PUT:
		  doPut(mInstance);
		  break;
		case REM:
		case REM2:
		case ELSE:			// defaults to here, i.e. when a IF statement is TRUE
do_rem:
		  doRem(mInstance);     // REM va a fondo riga...
//		  answer.d=STMT_CONTINUE;
		  answer.line=mInstance->curline;   //..e imposto la riga che segue! (più che altro per On Timer ecc)
		  answer.pos=0;
      goto fine;
		  break;
		case FOR:
		  answer = doFor(mInstance);
		  break;
		case NEXT:
		  answer = doNext(mInstance);
		  break;
		case DO:
		  answer = doDo(mInstance);
		  break;
		case LOOP:
		  answer = doLoop(mInstance);
		  break;
		case BEGIN:
		  mInstance->inBlock++;
		  break;
		case BEND:
			if(mInstance->inBlock>0)
				mInstance->inBlock--;
			else
		    setError(mInstance,ERR_NOFOR);
		  break;
		case COLON:
			goto rifo;
			break;

		case LINETOK:
		  doLine(mInstance);
		  break;
		case RECTANGLE:
		  doRectangle(mInstance);
		  break;
		case TRIANGLE:
		  doTriangle(mInstance);
		  break;
		case CIRCLE:
		  doCircle(mInstance);
		  break;
		case ELLIPSE:
		  doEllipse(mInstance);
		  break;
		case ARC:
		  doArc(mInstance);
		  break;
		case POINTTOK:
		  doPoint(mInstance);
		  break;
		case FILL:
		  doFill(mInstance);
		  break;
		case SETCOLOR:
		  doSetcolor(mInstance);
		  break;
		case SETBKCOLOR:
		  doSetBkcolor(mInstance);
		  break;
		case BITBLT:
		  doBitblt(mInstance);
		  break;

		case WIFICONNECT:
		  doWificonnect(mInstance);
		  break;
		case MODEMCMD:
		  doModemcmd(mInstance);
		  break;
#if defined(USA_USB_HOST_UVC)
		case WEBCAM:
      doWebcam(mInstance);
		  break;
#endif
      
		default:
      // if isdigit o mInstance->token==VALUE  per inserire righe...
      
			doLet(mInstance,0);
		  break;
	  }

	if(answer.d==STMT_CONTINUE) {  
    // CREDO SIA INUTILE ORA, v. REM ELSE sopra!
	  if(mInstance->token == ELSE || mInstance->token==REM || mInstance->token==REM2) {
			goto do_rem;
			}

	  if(mInstance->token != EOS) {

			/*match(mInstance,VALUE);*/
			/* check for a newline */
			str = mInstance->string;
			while(isspace(*str)) {
			  if(*str == '\n' || *str == ':')
			    break;
			  str++;
				}
	
			if(*str == ':') {   // questa roba potrebbe andare sopra nel loop principale... quando si restituisce 0... ?!
				match(mInstance,COLON);
        if(mInstance->errorFlag == ERR_CLEAR) 		// specie per STOP
			  	goto rifo;
				else
					goto fine;
				}
			if(*str != '\n')
			  setError(mInstance,ERR_SYNTAX);
	  	}
		}

fine:
  return answer;
	}


/*
  the PRINT statement
*/
void doPrint(MINIBASIC *mInstance) {
  static char *str;
  static NUM_TYPE x;
	void *ftemp=NULL;
  enum _FILE_TYPES filetype=-1;
	uint8_t pendingCR=0;                 // per semicolon a fine riga

  match(mInstance,mInstance->token);		// e PRINT2 
/*	if(errorFlag) {
		errorFlag=0;			// PATCHina...
	  match(mInstance,PRINT2);
		}*/

  mInstance->string=(char *)skipSpaces(mInstance->string);
//	t=getToken(string); USARE?
  if(*mInstance->string == '#')	{	// per print to file
		unsigned int f,i;
    
//  printf("#%u\r\n",mInstance->token);
		match(mInstance,DIESIS);
    
		f=integer(mInstance,expr(mInstance));
		match(mInstance,COMMA);
//  printf(",%u %u\r\n",f,mInstance->token);


		for(i=0; i<mInstance->nfiles; i++) {
			if(mInstance->openFiles[i].number==f) {
        filetype=mInstance->openFiles[i].type;
				switch(filetype) {
					case FILE_COM:    // si potrebbe usare SUPERFILE anche qua...
//            ftemp=mInstance->openFiles[i].handle;
						break;
#ifdef USA_USB_SLAVE_CDC
					case FILE_CDC:
						break;
#endif
#ifdef USA_WIFI
					case FILE_TCP:
            ftemp=mInstance->openFiles[i].handle;
            *rxBuffer=0;
						break;
					case FILE_UDP:
            ftemp=mInstance->openFiles[i].handle;
            *rxBuffer=0;
						break;
#endif
					case FILE_DISK:
            ftemp=mInstance->openFiles[i].handle;
						break;
					}
        goto print_file_found;
				}
			}
    setError(mInstance,ERR_FILE);   // file not found/not valid
    return;
  	}

print_file_found:
  for(;;) {
		switch(mInstance->token) {
			default:
				if(isString(mInstance->token)) {
					// case STRID QUOTE DIMSTRID, CHRSTRING ecc ma meglio così
					str = stringExpr(mInstance);
		// if errorFlag NON dovrebbe stampare...
	  			if(str) {
						switch(filetype) {
							case FILE_COM:
							{
								char *p=str;
								while(*p)
									WriteSerial(*p++);
							}
								break;
		#ifdef USA_USB_SLAVE_CDC
							case FILE_CDC:
								printf("%s",str);
								break;
		#endif
		#ifdef USA_WIFI
							case FILE_TCP:
								strcat(rxBuffer,str);
								break;
							case FILE_UDP:
								strcat(rxBuffer,str);
								break;
		#endif
							case FILE_DISK:
								if(SuperFileWrite(ftemp,str,strlen(str)) != strlen(str)) {     // 
									setError(mInstance,ERR_FILE);
									}
								break;
							default:
								myTextOut(mInstance,str);
								break;
							}
						pendingCR=1;
						free(str);
	  				}
  				}
        else if(mInstance->token >= FIRST_NUM_FUNCTION && mInstance->token <= LAST_NUM_FUNCTION)
          goto is_value;
        else
          goto fine;
				break;
			case FLTID:
			case DIMFLTID:
			case VALUE:
				{
					static long n;
					static char buf[20];

is_value:
	  			x = expr(mInstance);
		// if errorFlag NON dovrebbe stampare...

		//			x -= (double)(long)x;
		//			n=x*1000000.0;
					n=(long) (fabs(x - (double)(long)x ) * 1000000.0);
		//	  	fprintf(fpout, (STRINGFARPTR)"%g", x);
					if(n)	{	// un trucchetto per stampare interi o float
						unsigned char i;
        
						// metto lo spazio prima dei numeri... se non negativi...
  					if(x>=0)
    					sprintf(buf,(STRINGFARPTR)" %ld.%06lu", (long) x, (long) n); 
						else
    					sprintf(buf,(STRINGFARPTR)"%ld.%06lu", (long) x, (long) n); 
						for(i=strlen(buf)-1; i; i--) {
							if(buf[i] == '0')
								buf[i]=0;
							else
								break;
							}
						}
					else {
  					if(x>=0)
  						sprintf(buf," %ld", (long)x); 	// metto lo spazio prima dei numeri... se non negativi...
						else
							sprintf(buf,"%ld", (long)x); 
						}
					switch(filetype) {
						case FILE_COM:
							{
								char *p=buf;
								while(*p)
									WriteSerial(*p++);
							}
							break;
		#ifdef USA_USB_SLAVE_CDC
						case FILE_CDC:
							printf("%s",buf);
							break;
		#endif
		#ifdef USA_WIFI
						case FILE_TCP:
							strcat(rxBuffer,buf);
							break;
						case FILE_UDP:
							strcat(rxBuffer,buf);
							break;
		#endif
						case FILE_DISK:
							if(SuperFileWrite(ftemp,buf,strlen(buf)) != strlen(buf)) {     // 
								setError(mInstance,ERR_FILE);
								}
							break;
						default:
							myTextOut(mInstance,buf);
							break;
						}

					pendingCR=1;
					}
				break;
			case INTID:
			case DIMINTID:
				{
					int16_t i;

					i=integer(mInstance,expr(mInstance));
					{
					char buf[16];
					if(i>=0)
    				sprintf(buf," %d",i);	// metto lo spazio prima dei numeri... se non negativi...
					else
						sprintf(buf,"%d",i);
					switch(filetype) {
						case FILE_COM:
							{
								char *p=buf;
								while(*p)
									WriteSerial(*p++);
							}
							break;
		#ifdef USA_USB_SLAVE_CDC
						case FILE_CDC:
							printf("%s",buf);
							break;
		#endif
		#ifdef USA_WIFI
						case FILE_TCP:
							strcat(rxBuffer,buf);
							break;
						case FILE_UDP:
							strcat(rxBuffer,buf);
							break;
		#endif
						case FILE_DISK:
							if(SuperFileWrite(ftemp,buf,strlen(buf)) != strlen(buf)) {     // 
								setError(mInstance,ERR_FILE);
								}
							break;
						default:
							myTextOut(mInstance,buf);
							break;
						}
					}
					pendingCR=1;
					}
				break;
			case COMMA:
	//	    putc('\t', fpout);			// should print 8 chars or up to next tab...
				switch(filetype) {
					case FILE_COM:
						WriteSerial(' ');
						break;
	#ifdef USA_USB_SLAVE_CDC
					case FILE_CDC:
						putchar(' ');
						break;
	#endif
	#ifdef USA_WIFI
					case FILE_TCP:
						strcat(rxBuffer," ");
						break;
					case FILE_UDP:
						strcat(rxBuffer," ");
						break;
	#endif
					case FILE_DISK:
						if(SuperFileWrite(ftemp," ",1) != 1) {     // 
							setError(mInstance,ERR_FILE);
							}
						break;
					default:
						do {
							/*mInstance->Cursor.x += */ myTextOut(mInstance," ");
							} while(mInstance->Cursor.x % (ScreenText.cx>25 ? 8 : 4));   // bah, sì
						break;
					}

	  		match(mInstance,COMMA);
				pendingCR=0;
				break;
			case SEMICOLON:
	  		match(mInstance,SEMICOLON);
				pendingCR=0;
	  		break;
			case EOL:
			case EOS:
			case REM:
			case REM2:
        goto fine;
	  		break;
			}
  	}

// naturalmente, se rxBuffer sfora, errore?!  diverso tra tcp e udp..
// ev. cmq anche SuperFile e 
fine:
  
  if(!pendingCR /*token == SEMICOLON*/) {
//		match(mInstance,SEMICOLON);
//    fflush(fpout);
    switch(filetype) {
#ifdef USA_WIFI
      case FILE_TCP:
        send((SOCKET)(int)ftemp, rxBuffer, strlen(rxBuffer), 0);
        break;
      case FILE_UDP:
        send((SOCKET)(int)ftemp, rxBuffer, strlen(rxBuffer), 0);
        break;
#endif
#ifdef USA_ETHERNET
#endif
      default:
        break;
      }
  	}
  else {    // pendingCR
    switch(filetype) {
      case FILE_COM:
        WriteSerial('\r'); WriteSerial('\n');
        break;
#ifdef USA_USB_SLAVE_CDC
      case FILE_CDC:
        putchar('\r'); putchar('\n');
        break;
#endif
#ifdef USA_WIFI
      case FILE_TCP:
        strcat(rxBuffer,"\r\n");
        send((SOCKET)(int)ftemp, rxBuffer, strlen(rxBuffer), 0);
        break;
      case FILE_UDP:
        strcat(rxBuffer,"\r\n");
        send((SOCKET)(int)ftemp, rxBuffer, strlen(rxBuffer), 0);
        break;
#endif
      case FILE_DISK:
        if(SuperFileWrite(ftemp,"\r\n",2) != 2) {     // 
          setError(mInstance,ERR_FILE);
          }
        break;
      default:
#ifndef USING_SIMULATOR
        putchar('\n');
#endif
        mInstance->Cursor.y++; mInstance->Cursor.x=0;
        if(mInstance->Cursor.y >= ScreenText.cy) {
          mInstance->Cursor.y--;
          }
#ifndef USING_SIMULATOR
        SetXYText(mInstance->Cursor.x,mInstance->Cursor.y);
#endif
        break;
      }

    //ScrollWindow()
		}

	}


/*
  --------------------
*/
void doCls(MINIBASIC *mInstance) {
  RECT rc;

  match(mInstance,CLS);
  
#ifdef USA_BREAKTHROUGH
  HDC myDC,*hDC;
  BRUSH b;
  hDC=GetDC(mInstance->hWnd,&myDC);
  mInstance->Cursor.x=0;
  mInstance->Cursor.y=0;
  
//  HDC *myDC=BeginPaint(mInstance->hWnd,NULL,TRUE);
  GetClientRect(mInstance->hWnd,&rc);
  b=CreateSolidBrush(WHITE);
  FillRect(hDC,&rc,b);
//  EndPaint(mInstance->hWnd,NULL,FALSE);   // FALSE per consentire altre operazioni hDC :) (v.)
  // ovvero lo levo del tutto!

  ReleaseDC(mInstance->hWnd,hDC);
  
#else
  mInstance->Cursor.x=0;
  mInstance->Cursor.y=0;
  Cls();    // usa il colore nero e non il bkcolor...
#endif
  
	}



/*
  ------------------
*/
void doLocate(MINIBASIC *mInstance) {
  unsigned char x,y;

	match(mInstance,LOCATE);
  x = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  y = integer(mInstance,expr(mInstance));

  if((x < 0 || x >= ScreenText.cx) || 
    (y < 0 || y >= ScreenText.cy)) {
		setError(mInstance, ERR_BADVALUE);
    return;
    }
  
  mInstance->Cursor.x=x;
  mInstance->Cursor.y=y;
	}


/*
  ------------------
*/
LINE_NUMBER_STMT_POS_TYPE doOn(MINIBASIC *mInstance) {
  unsigned char n;
	TOKEN_NUM t;
	LINE_NUMBER_TYPE x;
  LINE_NUMBER_STMT_POS_TYPE answer;

	match(mInstance,ON);
	t=getToken(mInstance->string);

	if(t==ERRORTOK) {
		match(mInstance,ERRORTOK);
		getToken(mInstance->string);
    // GESTIRE anche GOSUB qua?? con un flag negli handler per tornare? ma in effetti è già gestito così in doReturn
    switch(mInstance->token) {
      case GOTO:
        match(mInstance,GOTO);
        mInstance->errorHandler.handler = integer(mInstance,expr(mInstance));
        break;
      case CONTINUE:
        match(mInstance,CONTINUE);
        mInstance->errorHandler.handler = -1;
        break;
      case STOP:
        match(mInstance,STOP);    // GOTO 0 dovrebbe essere uguale :)
        mInstance->errorHandler.handler = 0;
        break;
      default:
        setError(mInstance,ERR_SYNTAX);
        break;
      }
		mInstance->errorHandler.errorcode = 0;
		mInstance->errorHandler.line.d = 0;
    answer.d=STMT_CONTINUE;
		return answer;
		}
	else if(t==IRQ) {
		match(mInstance,IRQ);
		getToken(mInstance->string);
		match(mInstance,GOTO);
		mInstance->irqHandler.handler = integer(mInstance,expr(mInstance));
		mInstance->irqHandler.line.d = 0;
    answer.d=STMT_CONTINUE;
		return answer;
		}
	else if(t==TIMER) {
    unsigned long long i;
		match(mInstance,TIMER);
		getToken(mInstance->string);
    if(mInstance->token == GOTO) {
      i=1000;   // in mSec
      }
    else
      i=integer(mInstance,expr(mInstance));
    if(i<1)
      i=1;
    if(i>86400000ULL)    // un giorno in millisecondi :)
      i=86400000ULL;
    i=i*((GetPeripheralClock()/1000)/256);    // 
    PR6=LOWORD(i); PR7=HIWORD(i);
    TMR6=TMR7=0;
		match(mInstance,GOTO);
		mInstance->timerHandler.handler = integer(mInstance,expr(mInstance));
		mInstance->timerHandler.line.d = 0;
    answer.d=STMT_CONTINUE;
		return answer;
		}
	else {
		n = integer(mInstance,expr(mInstance));
		t=getToken(mInstance->string);
		if(t==GOSUB) {
			match(mInstance,GOSUB);
			if(mInstance->ngosubs >= MAXGOSUB) {
				setError(mInstance,ERR_TOOMANYGOSUB);
        answer.d=STMT_STOP;
        return answer;
				}
			}
		else
			match(mInstance,GOTO);

		while(n--) {
			x = integer(mInstance,expr(mInstance));
			mInstance->string=(char *)skipSpaces(mInstance->string);
			if(*mInstance->string != ',' /*COMMA*/)
				break;
      if(mInstance->errorFlag != ERR_CLEAR)
				break;
			}
    while(*mInstance->string && *mInstance->string != ':' && *mInstance->string != '\n')  // vado a fine statement
      mInstance->string++;

		if(t==GOSUB) {
		  mInstance->gosubStack[mInstance->ngosubs++] = getNextStatement(mInstance,mInstance->string);
			}

    answer = subGoto(mInstance,x);
   	return answer;
		}

	}


/*
  the OPEN and CLOSE statement
*/
void doOpen(MINIBASIC *mInstance) {		// OPEN filename [FOR mode][ACCESS access][lock] AS [#]file number [LEN=reclen] (stile GWBASIC)
	uint8_t fileno;
	uint8_t mode;
	char *str;

	match(mInstance,OPEN);
  str = stringExpr(mInstance);
//  if(mInstance->token == FOR) { boh a me pare NECESSARIO
		match(mInstance,FOR);

    if(mInstance->token == APPEND)
      mode=2;
    else if(mInstance->token == OUTPUTTOK)
      mode=1;
    else if(mInstance->token == INPUTTOK)
      mode=0;
//    else if(mInstance->token == SERVER)   // per udp e tcp..
//      mode=0;
//    else if(mInstance->token == CLIENT)
//      mode=0;
    else {
      // ev. APPEND ecc
  		setError(mInstance,ERR_SYNTAX);
      goto open_error;
      }
    
    match(mInstance,mInstance->token);

//		}
  match(mInstance,AS);
	fileno = integer(mInstance,expr(mInstance));
  if(fileno<1 || fileno>253 /*mah sì*/) {
 		setError(mInstance,ERR_BADVALUE);
    goto open_error;
    }

//  printf("%s %u %u\r\n",str,mode, fileno);
  
	if(mInstance->nfiles < (MAXFILES-1)) {
		mInstance->openFiles[mInstance->nfiles].number=fileno;
/*A device may be one of the following:
A:, B:, C:... 	Disk Drive
KYBD: 	Keyboard (input only)
SCRN: 	Screen (output only)
LPT1: 	Line Printer 1
LPT2: 	Line Printer 2
LPT3: 	Line Printer 3
COM1: 	RS-232 Communications 1
COMn: 	RS-232 Communications n
USB:    a file on USB Pendrive
CDC:    serial port over USB (slave)
TCP:    
UDP:      */
		if(!strnicmp(str,"COM:",4)) {
			mInstance->openFiles[mInstance->nfiles].type=FILE_COM;
			mInstance->openFiles[mInstance->nfiles].handle=(void *)(int)str[4];
      SetSerialPort(9600,8,0,1,FALSE);
//    	InitUART((BYTE)(DWORD)mInstance->openFiles[mInstance->nfiles].handle,9600); 
      mInstance->nfiles++;
			}
#ifdef USA_USB_SLAVE_CDC
		else if(!strnicmp(str,"CDC:",4)) {
			mInstance->openFiles[mInstance->nfiles].type=FILE_CDC;
			mInstance->openFiles[mInstance->nfiles].handle=0;		// 
      mInstance->nfiles++;
			}
#endif
#ifdef USA_WIFI
		else if(!strnicmp(str,"TCP:",4)) {
      struct sockaddr_in strAddr;
      uint16_t u16ServerPort,tOut;
      char *p;
      if(p=strchr(str+4,':')) {
        u16ServerPort=atoi(p+1);
        *p=0;
        }
      else
        u16ServerPort=80;    //
			mInstance->openFiles[mInstance->nfiles].type=FILE_TCP;
			mInstance->openFiles[mInstance->nfiles].handle=(void*)(int)socket(AF_INET,SOCK_STREAM,0);		// handle tcp
      if(mInstance->openFiles[mInstance->nfiles].handle >= 0) {
        strAddr.sin_family = AF_INET;
        strAddr.sin_port = _htons(u16ServerPort);
        strAddr.sin_addr.s_addr = 0; //INADDR_ANY
        if(0) { // se ACCEPT - FARE!
          bind((SOCKET)(int)(mInstance->openFiles[mInstance->nfiles].handle), (struct sockaddr*)&strAddr, sizeof(struct sockaddr_in));
          }
        else {
          StringToIPAddress(str+4, &strAddr.sin_addr.s_addr);
          if(!strAddr.sin_addr.s_addr) {
            *(unsigned long*)internetBuffer=0;
            tOut=0;
            gethostbyname((uint8_t*)str+4);
            while(!*(unsigned long*)internetBuffer && tOut<DNS_TIMEOUT) {
              m2m_wifi_handle_events(NULL);
              tOut++;
              __delay_ms(1);
              }
            strAddr.sin_addr.s_addr=*(unsigned long*)internetBuffer;
            }
          connect((SOCKET)(int)mInstance->openFiles[mInstance->nfiles].handle, (struct sockaddr*)&strAddr, sizeof(struct sockaddr_in));
          //aspettare msg di CONNECT ed ev. dare errore...
//  setError(mInstance,ERR_NETWORK);
          }
        }
      else
        goto open_error;
      mInstance->nfiles++;
			}
		else if(!strnicmp(str,"UDP:",4)) {
      struct sockaddr_in strAddr;
      uint16_t u16ServerPort,tOut;
      uint32_t u32EnableCallbacks=1;
      char *p;
      if(p=strchr(str+4,':')) {
        u16ServerPort=atoi(p+1);
        *p=0;
        }
      else
        u16ServerPort=12345;    // :)
			mInstance->openFiles[mInstance->nfiles].type=FILE_UDP;
			mInstance->openFiles[mInstance->nfiles].handle=(void*)(int)socket(AF_INET,SOCK_DGRAM,0);		// handle udp
      if(mInstance->openFiles[mInstance->nfiles].handle >= 0) {
        strAddr.sin_family = AF_INET;
        strAddr.sin_port = _htons(u16ServerPort);
        strAddr.sin_addr.s_addr = 0; //INADDR_ANY
        if(0) { // se ACCEPT/SERVER - FARE!
          bind((SOCKET)(int)mInstance->openFiles[mInstance->nfiles].handle, (struct sockaddr*)&strAddr, sizeof(struct sockaddr_in));
          }
        else {
          StringToIPAddress(str+4, &strAddr.sin_addr.s_addr);
          if(!strAddr.sin_addr.s_addr) {
            *(unsigned long*)internetBuffer=0;
            tOut=0;
            gethostbyname((uint8_t*)str+4);
            while(!*(unsigned long*)internetBuffer && tOut<DNS_TIMEOUT) {
              m2m_wifi_handle_events(NULL);
              tOut++;
              __delay_ms(1);
              }
            strAddr.sin_addr.s_addr=*(unsigned long*)internetBuffer;
            }
          sendto((SOCKET)(int)mInstance->openFiles[mInstance->nfiles].handle, rxBuffer, 1, 0, 
                  (struct sockaddr*)&strAddr, sizeof(struct sockaddr_in));
          // serve la sendto per impostare indirizzo... mando un primo byte tanto per...
        	setsockopt((SOCKET)(int)mInstance->openFiles[mInstance->nfiles].handle, SOL_SOCKET, SO_SET_UDP_SEND_CALLBACK, &u32EnableCallbacks, 4);
//  setError(mInstance,ERR_NETWORK);
          }
        }
      else
        goto open_error;
      mInstance->nfiles++;
			}
#endif
#if defined(USA_USB_HOST_MSD)
		else if(!strnicmp(str,"USB:",4)) {
      str[0]='E'; str[1]=':'; str[2]=0;
      goto is_disk;   // bah sì :) :)
			}
		else if(!strnicmp(str,"E:",2)) {
      goto is_disk;   // bah sì :)
			}
#endif
		else if(!strnicmp(str,"A:",2) || !strnicmp(str,"C:",2)) {
      goto is_disk;   // bah sì :)
			}
		else {
      SUPERFILE *f;
      char *filename;
is_disk:
      f=malloc(sizeof(SUPERFILE));
			mInstance->openFiles[mInstance->nfiles].type=FILE_DISK;
//  printf(" %s %u %u\r\n",str,mode, f);
      
      getDrive(str,f,&filename);
      if(SuperFileOpen(f,filename,mode==2 ? 's' : (mode==1 ? 'w' : 'r'))) {
    		mInstance->openFiles[mInstance->nfiles].number=fileno;
        mInstance->openFiles[mInstance->nfiles].handle=f;
        mInstance->nfiles++;
        }
      else {
        free(f);
        goto open_error;
        }
			}
		free(str);
		}
	else {
open_error:
		free(str);
		setError(mInstance,ERR_FILE);
		}
	}

void subClose(MINIBASIC *mInstance,int f) {
  
  switch(mInstance->openFiles[f].type) {
    case FILE_COM:
//          CloseUART((BYTE)(DWORD)mInstance->openFiles[i].handle);
      break;
#ifdef USA_USB_SLAVE_CDC
    case FILE_CDC:
      break;
#endif
#ifdef USA_WIFI
    case FILE_TCP:
      close((SOCKET)(int)mInstance->openFiles[f].handle);
      // se ACCEPTED, gestire...
      break;
    case FILE_UDP:
      close((SOCKET)(int)mInstance->openFiles[f].handle);
      break;
#endif
    case FILE_DISK:
      SuperFileClose(mInstance->openFiles[f].handle);
      free(mInstance->openFiles[f].handle);
      break;
    }
  }
void doClose(MINIBASIC *mInstance) {
	int i;
	unsigned int n;

	match(mInstance,CLOSE);
 	n = integer(mInstance,expr(mInstance));		// 

	for(i=0; i<mInstance->nfiles; i++) {
		if(mInstance->openFiles[i].number==n) {
      subClose(mInstance,i);
			int j;
			for(j=i+1; j<mInstance->nfiles; j++) {
				mInstance->openFiles[j-1].number=mInstance->openFiles[j].number;
				mInstance->openFiles[j-1].handle=mInstance->openFiles[j].handle;
				mInstance->openFiles[j-1].type=mInstance->openFiles[j].type;
				}
			mInstance->nfiles--;
			goto fine;
			}
		}

	setError(mInstance,ERR_FILE);

fine: ;
	}

void doDir(MINIBASIC *mInstance) {
  SearchRec rec;
  char mydrive;
#if defined(USA_USB_HOST_MSD)
  SYS_FS_HANDLE myFileHandle;
  SYS_FS_FSTAT stat;
#endif
  FS_DISK_PROPERTIES disk_properties;
  int i;
  WORD totfiles,totdirs,totsize;
  uint32_t totalSectors,freeSectors,sectorSize;
  uint32_t sn,timestamp;
  char *str,*filter;
  char buf[32];
  
	match(mInstance,DIRTOK);
  str = stringExpr(mInstance);
  // fare opzionale
  if(str)
    mydrive=getDrive(str,NULL,&filter);
  else
    mydrive=currDrive;
  if(!*filter)
    filter="*.*";
      
  switch(mydrive) {
    case 'A':
    case 'C':
#ifdef USA_RAM_DISK 
    case DEVICE_RAMDISK:
#endif
      switch(mydrive) {
        case 'A':
          if(!SDcardOK)
            goto fine;
          i=FindFirst(filter, ATTR_VOLUME, &rec);
          break;
        case 'C':
          if(!HDOK)
            goto fine;
          i=IDEFindFirst(filter, ATTR_VOLUME, &rec);
          break;
#ifdef USA_RAM_DISK 
        case DEVICE_RAMDISK:
#endif
          if(!RAMdiscArea)
            goto fine;
          i=RAMFindFirst(filter, ATTR_VOLUME, &rec);
          break;
        }
      if(!i) {
        sprintf(buf,"Volume in drive %c:%s is %s",mydrive,"\\",rec.filename); 
        myTextOut(mInstance,buf);
        myCR(mInstance);
        }

      switch(mydrive) {
        case 'A':
          i=FindFirst(filter, ATTR_MASK ^ ATTR_VOLUME, &rec);
          break;
        case 'C':
          i=IDEFindFirst(filter, ATTR_MASK ^ ATTR_VOLUME, &rec);
          break;
#ifdef USA_RAM_DISK 
        case DEVICE_RAMDISK:
#endif
          i=RAMFindFirst(filter, ATTR_MASK ^ ATTR_VOLUME, &rec);
          break;
        }
      sprintf(buf,"Directory of %c:%s",mydrive,"\\");    // finire :)
      myTextOut(mInstance,buf);
      myCR(mInstance);

      if(!i) {
        totfiles=0; totdirs=0; totsize=0;
        for(;;) {
          myTextOut(mInstance,rec.filename);
          mInstance->Cursor.x = 13;
          if(rec.attributes & ATTR_DIRECTORY) {
            myTextOut(mInstance,"DIR");
            totdirs++;
            }
          else {
            sprintf(buf,"%-9u",rec.filesize);
            totsize+=rec.filesize;
            myTextOut(mInstance,buf);
            mInstance->Cursor.x = 23;
            sprintf(buf,"%02u/%02u/%04u %02u:%02u:%02u",
              (rec.timestamp >> 16) & 31,
              (rec.timestamp >> (5+16)) & 15,
              (rec.timestamp >> (9+16)) + 1980,
              (rec.timestamp >> 11) & 31,
              (rec.timestamp >> 5) & 63,
              rec.timestamp & 63);
            myTextOut(mInstance,buf);
            }

          mInstance->Cursor.y++; mInstance->Cursor.x=0;
          totfiles++;
          if(hitCtrlC(TRUE))
            break;
          while(isCtrlS());
          switch(mydrive) {
            case 'A':
              if(FindNext(&rec))
                goto fine_dir;
              break;
            case 'C':
              if(IDEFindNext(&rec))
                goto fine_dir;
              break;
#ifdef USA_RAM_DISK 
            case DEVICE_RAMDISK:
#endif
              if(RAMFindNext(&rec))
                goto fine_dir;
              break;
            }
          } 
        }
      else {
        setError(mInstance,ERR_FILE);
        goto fine;
        }

fine_dir:
              // stampare...totdirs e totsize
      disk_properties.new_request=1;
      do {
        switch(mydrive) {
          case 'A':
            FSGetDiskProperties(&disk_properties);
            break;
          case 'C':
            IDEFSGetDiskProperties(&disk_properties);
            break;
#ifdef USA_RAM_DISK 
          case DEVICE_RAMDISK:
#endif
            RAMFSGetDiskProperties(&disk_properties);
            break;
          }
        ClrWdt();
        } while(disk_properties.properties_status == FS_GET_PROPERTIES_STILL_WORKING);
      if(disk_properties.properties_status == FS_GET_PROPERTIES_NO_ERRORS) {
        sprintf(buf,"%u file%c %lu KBytes free",totfiles,totfiles==1 ? ' ' : 's',
                disk_properties.results.free_clusters*disk_properties.results.sectors_per_cluster*disk_properties.results.sector_size/1024); 
        myTextOut(mInstance,buf);
        myCR(mInstance);
        }
      break;
#if defined(USA_USB_HOST_MSD)
    case 'E':
      if(!USBmemOK)
        goto fine;
      i=SYS_FS_DriveLabelGet(NULL, buf, &sn, &timestamp);
      if(i==SYS_FS_RES_SUCCESS ) {
        sprintf(buf,"Volume in drive %c:%s is %s",'E',"\\",buf);    // finire :)
        myTextOut(mInstance,buf);
        myCR(mInstance);
        }

      if((myFileHandle=SYS_FS_DirOpen("/")) != SYS_FS_HANDLE_INVALID) {
        sprintf(buf,"Directory of %c:%s",'E',"\\");    // finire :)
        myTextOut(mInstance,buf);
        myCR(mInstance);
        totfiles=0; totdirs=0; totsize=0;

        while(SYS_FS_DirSearch(myFileHandle,filter,SYS_FS_ATTR_MASK ^ SYS_FS_ATTR_VOL,&stat) == SYS_FS_RES_SUCCESS) {
          myTextOut(mInstance,stat.fname);
          mInstance->Cursor.x = 13;
          if(stat.fattrib & ATTR_DIRECTORY) {
            myTextOut(mInstance,"DIR");
            totdirs++;
            }
          else {
            sprintf(buf,"%-9u",stat.fsize);
            totsize+=stat.fsize;
            myTextOut(mInstance,buf);
            mInstance->Cursor.x = 23;
            sprintf(buf,"%02u/%02u/%04u %02u:%02u:%02u",
              stat.fdate & 31,
              (stat.fdate >> (5)) & 15,
              (stat.fdate >> (9)) + 1980,
              (stat.ftime >> 11) & 31,
              (stat.ftime >> 5) & 63,
              stat.ftime & 63);
            myTextOut(mInstance,buf);
            }

          mInstance->Cursor.y++; mInstance->Cursor.x=0;
          totfiles++;
          if(hitCtrlC(TRUE))
            break;
          while(isCtrlS());
          }
        SYS_FS_DirClose(myFileHandle);
        }
      else {
        setError(mInstance,ERR_FILE);
        goto fine;
        }

      // stampare...totdirs e totsize
      disk_properties.new_request=1;
      if(SYS_FS_DriveSectorGet(NULL,&totalSectors,&freeSectors,&sectorSize)==SYS_FS_RES_SUCCESS) {
        sprintf(buf,"%u file%c %lu KBytes free",totfiles,totfiles==1 ? ' ' : 's',
                freeSectors* MEDIA_SECTOR_SIZE /* boh dov'è??*//1024); 
        myTextOut(mInstance,buf);
        myCR(mInstance);
        }
      break;
#endif
    }

fine:
  if(str)
    free(str);
	}

void doFormat(MINIBASIC *mInstance) {
  int i; 
  char *str,*label;
  char mydrive;
  
	match(mInstance,FORMAT);
  str = stringExpr(mInstance);
  
  if(str) {   // qua obbligatorio!
    mydrive=getDrive(str,NULL,&label);
    switch(mydrive) {
      case 'A':
        if(!SDcardOK)
          goto fine;
        i=!FSformat(1,rand(),label);
        break;
      case 'E':
#if defined(USA_USB_HOST_MSD)
        if(!USBmemOK)
          goto fine;
        i=SYS_FS_DriveFormat(NULL, SYS_FS_FORMAT_SFD /*boh sì*/, 4096 /*??*/) == SYS_FS_RES_SUCCESS;
        SYS_FS_DriveLabelSet(NULL, label);
  //      i=USBFSformat(1,rand(),str);
  //        printf("Directory of %s","E:\\");    // finire :)
        break;
#endif
      case 'C':
        if(!HDOK)
          goto fine;
        i=!IDEFSformat(1,rand(),label);
        break;
#ifdef USA_RAM_DISK 
      case DEVICE_RAMDISK:
        if(!RAMdiscArea)
          goto fine;
        i=!RAMFSformat(1,rand(),label);
        break;
#endif
      default:
        i=0;
        break;
      }
    if(!i)
fine:
      setError(mInstance,ERR_FILE);
    free(str);
    }
  }
  
void doRename(MINIBASIC *mInstance) {
  SUPERFILE f;
  char *str1,*str2,*filename;
  
	match(mInstance,RENAME);
  str1 = stringExpr(mInstance);
	match(mInstance,COMMA);
  str2 = stringExpr(mInstance);
  
  if(str1 && str2) {
    getDrive(str1,&f,&filename);
    if(!filename)
      filename=str1;
    if(!SuperFileRename(&f,filename,str2))
      setError(mInstance,ERR_FILE);
    }
  free(str1);
  free(str2);
  }
  
void doDelete(MINIBASIC *mInstance) {
  char *str;
  SUPERFILE f;
  char *filename;
  
	match(mInstance,DELETE);
  str = stringExpr(mInstance);
  
  if(str) {
    getDrive(str,&f,&filename);
    if(!filename)
      filename=str;
    if(!SuperFileDelete(&f,filename))
      setError(mInstance,ERR_FILE);
    free(str);
    }
  }
  
void doCD(MINIBASIC *mInstance) {
  int i; 
  char *str,*path;
  char mydrive;
  
	match(mInstance,CD);
  str = stringExpr(mInstance);
  
  if(str) {
    mydrive=getDrive(str,NULL,&path);
    switch(mydrive) {
      case 'A':
        if(!SDcardOK)
          goto fine;
        i=!FSchdir(str);
        break;
#if defined(USA_USB_HOST_MSD)
      case 'E':
        if(!USBmemOK)
          goto fine;
        i=SYS_FS_DirectoryChange(path) == SYS_FS_RES_SUCCESS;
        break;
#endif
      case 'C':
        if(!HDOK)
          goto fine;
        i=!IDEFSchdir(str);
        break;
#ifdef USA_RAM_DISK 
      case DEVICE_RAMDISK:
        if(!RAMdiscArea)
          goto fine;
        i=!RAMFSchdir(str);
        break;
#endif
      default:
        i=0;
        break;
      }
    if(!i)
fine:
      setError(mInstance,ERR_FILE);
    free(str);
    }
  }
  
void doMD(MINIBASIC *mInstance) {
  int i; 
  char *str,*path;
  char mydrive;
  
	match(mInstance,MD);
  str = stringExpr(mInstance);
  
  if(str) {
    mydrive=getDrive(str,NULL,&path);
    switch(mydrive) {
      case 'A':
        if(!SDcardOK)
          goto fine;
        i=!FSmkdir(str);
        break;
#if defined(USA_USB_HOST_MSD)
      case 'E':
        if(!USBmemOK)
          goto fine;
        i=SYS_FS_DirectoryMake(path) == SYS_FS_RES_SUCCESS;
        break;
#endif
      case 'C':
        i=!IDEFSmkdir(str);
        break;
#ifdef USA_RAM_DISK 
      case DEVICE_RAMDISK:
        if(!RAMdiscArea)
          goto fine;
        i=!RAMFSmkdir(str);
        break;
#endif
      default:
        i=0;
        break;
      }
    if(!i)
fine:
      setError(mInstance,ERR_FILE);
    free(str);
    }
  }
  
void doRD(MINIBASIC *mInstance) {
  int i; 
  char *str,*path;
  char mydrive;
  
	match(mInstance,RD);
  str = stringExpr(mInstance);
  
  if(str) {
    mydrive=getDrive(str,NULL,&path);
    switch(mydrive) {
      case 'A':
        i=!FSrmdir(str,TRUE);
        break;
#if defined(USA_USB_HOST_MSD)
      case 'E':
        if(!USBmemOK)
          goto fine;
        i=SYS_FS_FileDirectoryRemove(path) == SYS_FS_RES_SUCCESS;
        break;
#endif
      case 'C':
        i=!IDEFSrmdir(str,TRUE);
        break;
#ifdef USA_RAM_DISK 
      case DEVICE_RAMDISK:
        if(!RAMdiscArea)
          goto fine;
        i=!RAMFSrmdir(str,TRUE);
        break;
#endif
      default:
        i=0;
        break;
      }
    if(!i)
fine:
      setError(mInstance,ERR_FILE);
    free(str);
    }
  }
  

/*
  the POKE statement
*/
void doPoke(MINIBASIC *mInstance) {
  unsigned int adr;
  unsigned int dat;
  
  match(mInstance,POKE);
  adr = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  dat = integer(mInstance,expr(mInstance));
  *((unsigned char *)PA_TO_KVA0(adr)) = dat;
	}

void doEEPoke(MINIBASIC *mInstance) {
  unsigned int adr;
  unsigned int dat;
  
  match(mInstance,EEPOKE);
  adr = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  dat = integer(mInstance,expr(mInstance));
  EEwrite(adr,dat);
	}



/*
  the SLEEP statement
*/
void doSleep(MINIBASIC *mInstance) {
  int t;

  match(mInstance,SLEEP);
  t = integer(mInstance,expr(mInstance));
  if(t<0) {
		Sleep();
		}
	else {
    DWORD t2=timeGetTime()+t;
#ifdef USA_BREAKTHROUGH
		while(timeGetTime() < t2) {
//			__delay_ms(1);
			Yield(mInstance->threadID);
      ClrWdt();
      }
/*		while(t--) {
			__delay_ms(1);
			Yield(mInstance->threadID); // ovviamente dopo di questa il tempo sarà sbagliato, va usato un tipo timeGetTime :)
      timeGetTime();
      ClrWdt();
			}*/
#else
		while(t--) {
			__delay_ms(1);
      ClrWdt();
      }
#endif
		}
	}

void doPower(MINIBASIC *mInstance) {
  int i;
  
  match(mInstance,POWER);
  i = integer(mInstance,expr(mInstance));
  
  setPowerMode(i);
  }


/*
  the RUN statement
*/
int16_t doRun(MINIBASIC *mInstance) {
  int t=0;

  match(mInstance,RUN);
	if(mInstance->token==VALUE)
	  t = integer(mInstance,expr(mInstance));			// 
	cleanup(mInstance,0);

	if(t)
		mInstance->curline=findLine(mInstance,t);
	else
		mInstance->curline=0;
  
  if(mInstance->curline>=0) {
    mInstance->string = mInstance->lines[mInstance->curline].str;
    mInstance->token = getToken(mInstance->string);
    mInstance->errorFlag = ERR_CLEAR;
    }
  else
    setError(mInstance,ERR_NOSUCHLINE);   // (sopra è gestita a parte)

#warning RUN non va... bisogna passare da commandline a basic() ...
	return mInstance->curline;
	}

void doShell(MINIBASIC *mInstance) {
  char *str;
  
  match(mInstance,SHELL);
  str=stringExpr(mInstance);
  if(str) {
    execCmd(str,NULL,NULL);
    free(str);
    }
	}

/*
  the CONTINUE statement
*/
void doContinue(MINIBASIC *mInstance) {

  match(mInstance,CONTINUE);
	mInstance->errorFlag=ERR_CLEAR;
	}

void doList(MINIBASIC *mInstance) {
  int i;
  char buf[256],j;
  
  match(mInstance,LIST);
  i=0;
 	while(i < mInstance->nlines) {			//Loop through Script Running each Line
    for(j=0; j<255; j++) {
      buf[j]=mInstance->lines[i].str[j];
      if(buf[j]=='\n')
        break;
      }
    buf[j]=0;
		myTextOut(mInstance,buf);
    myCR(mInstance);
   	i++;
    }
  
    myCR(mInstance);

  // fare...
	// ovvero :)
/*	if(theScript)
    puts(theScript);
 * */
	}
  
void doNew(MINIBASIC *mInstance) {
  
  match(mInstance,NEW);
	*theScript=0;
  cleanup(mInstance,1);   // verificare, se si usa!
	}

void doClr(MINIBASIC *mInstance) {
  
  match(mInstance,CLR);
  cleanup(mInstance,0);
	}

int subLoad(const char *s,char *p) {
  SUPERFILE f;
  int n=0;
  char *filename;
  
  *p=0;
  getDrive(s,&f,&filename);
  if(SuperFileOpen(&f,filename,'r')) { 
    while(1) {
      if(SuperFileRead(&f,p,1) == 1) {  // o FSfgets??
        if(*p != 13 && *p != 9)    // cmq tolgo CR e TAB
          p++;
        n++;
        }
      else
        break;
      }
    *p=0;
    SuperFileClose(&f);
    return n;
    }
  else {
    return -1;
    }
  }
  
void doLoad(MINIBASIC *mInstance) {
	char *filename;
  
  match(mInstance,LOAD);
  filename=stringExpr(mInstance);
  if(filename) {
    if(subLoad(filename,theScript) < 0)
      setError(mInstance,ERR_FILE);
    else
      setup(mInstance,theScript);
    }
  free(filename);
	}

void doSave(MINIBASIC *mInstance) {
  SUPERFILE f;
  int i,n=0;
	char *str,*filename;
  
  match(mInstance,SAVE);
  str=stringExpr(mInstance);
  if(str) {
    getDrive(str,&f,&filename);
    if(SuperFileOpen(&f, filename, 'w')) { 
      i=0;
      while(i < mInstance->nlines) {			//Loop through Script Running each Line
        n+=SuperFileWrite(&f,mInstance->lines[i].str,strlen(mInstance->lines[i].str));
        n+=SuperFileWrite(&f,"\n",1);
        i++;
        }
      SuperFileClose(&f);
  //    return n;
      }
    else
      setError(mInstance,ERR_FILE);
    free(str);
    }
	}


void doWificonnect(MINIBASIC *mInstance) {
#ifdef USA_WIFI
	char *ap,*pasw;
	int auth,chan;

  match(mInstance,WIFICONNECT);

	auth=M2M_WIFI_SEC_WPA_PSK;
	chan=M2M_WIFI_CH_ALL;

  ap=stringExpr(mInstance);
	match(mInstance,COMMA);
  pasw=stringExpr(mInstance);
	if(mInstance->token==COMMA) {
		match(mInstance,COMMA);
		auth=expr(mInstance);
		if(mInstance->token==COMMA) {
			match(mInstance,COMMA);
			chan=expr(mInstance);
			}
		}

  /*mInstance->errorFlag= v. callback ...*/ m2m_wifi_connect(ap,12,M2M_WIFI_SEC_WPA_PSK,pasw,chan);
	while(m2m_wifi_handle_events(NULL) != M2M_SUCCESS) {
    ClrWdt();
		}
  free(ap);
  free(pasw);
#else
  setError(mInstance,ERR_NETWORK);
#endif
  }

void doModemcmd(MINIBASIC *mInstance) {
  char *str;
  static BYTE modemInited=0;

  match(mInstance,MODEMCMD);
  
  if(!modemInited) {
  // QUANDO fare InitModem??
    InitModem(1);
    modemInited=1;
    }

  str=stringExpr(mInstance);
  if(str) {
    WriteModemATCommand(str);
    free(str);
    }
  
  if(!GetReturnCode(AUDIO_CARD))
  	setError(mInstance,ERR_BADVALUE);
  }


#if defined(USA_USB_HOST_UVC)
void doWebcam(MINIBASIC *mInstance) {
  DIMVAR *dimvar;
  uint16_t varsize;
  char id[IDLENGTH];
  IDENT_LEN len;
  SIZE sz;
  BYTE resize;
  uint32_t n;
  WORD divider=0;

  match(mInstance,WEBCAM);
  sz.cx = integer(mInstance,expr(mInstance));
  if(sz.cx < 0 || sz.cx > MAX_HORIZ_SIZE_WEBCAM)    //
		setError(mInstance, ERR_BADVALUE);
  match(mInstance,COMMA);
  sz.cy = integer(mInstance,expr(mInstance));
  if(sz.cy < 0 || sz.cy > MAX_VERT_SIZE_WEBCAM)
		setError(mInstance, ERR_BADVALUE);
	
  match(mInstance,COMMA);
  getId(mInstance,mInstance->string, id, &len);
  match(mInstance,DIMINTID);
  dimvar = findDimVar(mInstance,id);
  if(!dimvar) {
    setError(mInstance,ERR_NOSUCHVARIABLE);
    return;
    }
  match(mInstance,CPAREN);    // diciam così, WEBCAM sx,sy,w%()
  
  if(mInstance->token == COMMA) {   // può seguire size...
    match(mInstance,COMMA);
    resize=integer(mInstance,expr(mInstance));
    }
  else
    resize=1;

  if(dimvar->ndims==1) {
    // GESTIRE SIZE con riduzione!
    APP_VideoDataSubSetDefault(0);   // FINIRE con dim

    int x,y,n;
    BYTE status=false; 
    
    if(appData.deviceIsConnected) {
    capturingVideo=CAPTURE_CAPTURE;
    myTextOut(mInstance,"capturing video...  ");
    x=0;
    while(capturingVideo != CAPTURE_DONE) {
      SYS_Tasks();
      divider++;
      if(divider > 4000) {    // 1/3 1/4 sec
        mInstance->Cursor.x=19;
        id[0]="|/-\\|\\-/"[x++ & 7]; id[1]=0;
        myTextOut(mInstance,id); //troppo frequenti si incasina scheda video... !!!
        handle_events();
        if(hitCtrlC(TRUE))
          break;
        divider=0;
        }
      }
    myCR(mInstance);
    if(capturingVideo==CAPTURE_DONE) {
      capturingVideo=CAPTURE_IDLE;
      if(appData.videoStreamFormat.format==USB_UVC_FORMAT_MJPG) {
      pjpeg_image_info_t JPG_Info;
      int mcu_x;
      int mcu_y;
      if(status=pjpeg_decode_init(&JPG_Info, pjpeg_need_bytes_callback, NULL,0)) {
        goto error_webcam;
        }
      else {
        n=0;
        for(;;) {

          if(status = pjpeg_decode_mcu())
            goto error_compressed;

          for(y=0; y < JPG_Info.m_MCUHeight; y += 8) {
            for(x=0; x < JPG_Info.m_MCUWidth; x += 8) {

              // Compute source byte offset of the block in the decoder's MCU buffer.
              uint32_t src_ofs = (x * 8U) + (y * 16U);
              const uint8_t *pSrcR = JPG_Info.m_pMCUBufR + src_ofs;
              const uint8_t *pSrcG = JPG_Info.m_pMCUBufG + src_ofs;
              const uint8_t *pSrcB = JPG_Info.m_pMCUBufB + src_ofs;

              uint8_t bx, by;
              for(by=0; by<8; by++) {
                for(bx=0; bx<8; bx++) {
                  dimvar->d.ival[n++] = Color565(*pSrcR,*pSrcG,*pSrcB);
                  pSrcR++; pSrcG++; pSrcB++;
                  }
                }
              }
            }
          mcu_x++;      // in x ogni blocco è già 16 pixel (con YUV, pare)
          if(mcu_x == JPG_Info.m_MCUSPerRow) {
            mcu_x = 0;
            mcu_y++;
//                  if(mcu_y == JPG_Info.m_MCUSPerCol) {
//                    break;
 //                   }
            }
          ClrWdt();
          }
        }
      
error_webcam:
error_compressed:
      ;
      }
    else {     // YUY
      extern BYTE *VideoBufferPtr2;
      extern DWORD VideoBufferLen2;
      n=0;
      while(VideoBufferLen2>0) {
        int Y = *VideoBufferPtr2++;
        int U = *VideoBufferPtr2++;
        int Y2= *VideoBufferPtr2++;
        int V = *VideoBufferPtr2++;
        int r,g,b;

        int c = Y-16;
        int d = U-128;
        int e = V-128;
        c *= 298;
#define clip(n) { if(n<0) n=0; else if(n>255) n=255; }
        b = (c + 516*d + 128) >> 8;   // blue
        clip(b);
        g = (c - 100*d - 208*e + 128) >> 8; // green
        clip(g);
        r = (c + 409*e + 128) >> 8;   // red
        clip(r);
          
//        YUV2RGB[Y][U][V];     // USARE!!

        dimvar->d.ival[n++]=Color565(r,g,b);
          
        c = Y2-16;
        c *= 298;
        b = (c + 516*d + 128) >> 8;   // blue
        clip(b);
        g = (c - 100*d - 208*e + 128) >> 8; // green
        clip(g);
        r = (c + 409*e + 128) >> 8;   // red
        clip(r);

        dimvar->d.ival[n++]=Color565(r,g,b);
          
        VideoBufferLen2-=4;
        }
      }
      }
        
    }
    else
  		setError(mInstance, ERR_FILE);    // cambiare...

    }
  else
		setError(mInstance, ERR_BADVALUE);
	}

#endif
      
      
/*
  ------------------
*/
void doSetpin(MINIBASIC *mInstance) {
  unsigned char port, pin, mode;
  
  match(mInstance,SETPIN);
  port = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pin = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  mode = integer(mInstance,expr(mInstance));
	setpin(port,pin,mode);
	}


/*
  ------------------
*/
void doSound(MINIBASIC *mInstance) {    // SOUND canale,frequenza [,TIPO[,volume]]
  int channel,type=QUADRA,freq,stereo,bits,volume=100;
  
  match(mInstance,SOUND);
//  SetAudioMode(SYNTH);    // in effetti credo implicita in setaudiowave SOLO SE SERVE
  channel = expr(mInstance);
  match(mInstance,COMMA);
  freq = expr(mInstance);
  if(mInstance->token == COMMA) {
    match(mInstance,COMMA);
    type = expr(mInstance);
    if(mInstance->token == COMMA) {
      match(mInstance,COMMA);
      volume = expr(mInstance);
      }
    }
  
  SetAudioWave(channel,type,freq,2,8,volume,1 /* mix*/,0);
	}

void doPlay(MINIBASIC *mInstance) {
  int pos;
  int t;
  
  match(mInstance,PLAY);
	t=getToken(mInstance->string);
#ifndef USING_SIMULATOR
  SetAudioMode(SAMPLES);
#endif
  if(t==QUOTE || t==STRID) {
    char *str,*filename;
    str=stringExpr(mInstance);
    if(str) {
      SUPERFILE f;
      char buf[256];

      getDrive(str,&f,&filename);
      if(SuperFileOpen(&f,filename,'r')) { 
        WAVE_SAMPLES_FORMAT wsf;
        struct __attribute((packed)) WAV_HEADER_0 w0;
        struct __attribute((packed)) WAV_HEADER_FMT wfmt;
        struct __attribute((packed)) WAV_HEADER_FACT wfct;
        struct __attribute((packed)) WAV_HEADER_DATA wd;

// LEGGERE WAVEFMT da file! FARE controlli!
        SuperFileRead(&f,&w0,sizeof(struct WAV_HEADER_0)); 
        SuperFileRead(&f,&wfmt,sizeof(struct WAV_HEADER_FMT));
        SuperFileRead(&f,&wfct,sizeof(struct WAV_HEADER_FACT));
        SuperFileRead(&f,&wd,sizeof(struct WAV_HEADER_DATA));
        
        wsf.channels=wfmt.NumChannels;
        wsf.bytesPerSample=wfmt.BitsPerSample/8;
        wsf.samplesPerSec=wfmt.SamplesPerSec;
        wsf.bufferSize=wfmt.SamplesPerSec;    // per ora così
        wsf.buffers=1;
        wsf.active=1; 
        if(mInstance->token == COMMA) {
          match(mInstance,COMMA);
          wsf.loop = expr(mInstance);
          }
        SetAudioSamplesMode(&wsf);
        pos=0;
        do {
          if(SuperFileRead(&f,buf,256)==256) {    // finire, bufferizzare...
            SetAudioSamples(0,pos,buf,256);
            pos+=256;
            }
          else 
            break;
          } while(1);   // anche wd.NumSamples
        SuperFileClose(&f);
        }
      else {
        setError(mInstance,ERR_FILE);
        }
      free(str);
      }
    }
  else if(t==DIMINTID) {
    BYTE *p;
    DIMVAR *dimvar;
    uint16_t varsize;
    char id[IDLENGTH];
    IDENT_LEN len;
    
    WAVE_SAMPLES_FORMAT wsf;
    wsf.channels=1;
    wsf.bytesPerSample=1;
    wsf.samplesPerSec=22050;
    wsf.bufferSize=512;
    wsf.buffers=1;
    wsf.active=1; wsf.loop=1;
#ifndef USING_SIMULATOR
    SetAudioSamplesMode(&wsf);
#endif
        
    getId(mInstance,mInstance->string, id, &len);
    match(mInstance,DIMINTID);
    dimvar = findDimVar(mInstance,id);
    if(!dimvar) {
      setError(mInstance,ERR_NOSUCHVARIABLE);
      return;
      }
  	match(mInstance,CPAREN);    // diciam così, PLAY s%(),1024
    match(mInstance,COMMA);
    varsize=integer(mInstance,expr(mInstance)); // andrebbe controllata dimensione... che è stata malloc'ata...
    if(mInstance->token == COMMA) {
      match(mInstance,COMMA);
      wsf.loop = expr(mInstance);
      }

    switch(dimvar->ndims) {
		  case 1:
        SetAudioSamples2(0,0,&dimvar->d.ival[0],varsize);    // dovrebbe...
        // però gli array sono di WORD e non di byte...
				break;
      default:
        setError(mInstance,ERR_BADVALUE);
				break;
      }

    }
  else {
    setError(mInstance,ERR_BADVALUE);
    }

	}

void doTone(MINIBASIC *mInstance) {
  unsigned char port, pin, freq;
  
  match(mInstance,TONE);
  port = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pin = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  freq = integer(mInstance,expr(mInstance));
	tone(port,pin,freq);
	}

void doBeep(MINIBASIC *mInstance) {
  WORD durata;
  
  
  
//    IFS1bits.T7IF=1; //test

    
    
    
  match(mInstance,BEEP);
  durata = expr(mInstance);
  // usare StdBeep(durata) ??
  if(!durata)   // o dare errore... v sopra
    durata=500;
  OC1CONbits.ON=1;    //
  __delay_ms(durata); 
  OC1CONbits.ON=0;
  ClrWdt();
	}


/*
  ------------------
*/
void doOutd(MINIBASIC *mInstance) {
  unsigned char port, pin, value;
  
  match(mInstance,OUTD);
  port = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pin = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  value = integer(mInstance,expr(mInstance));
	outd(port,pin,value);
	}


/*
  ------------------
*/
void doOutdac(MINIBASIC *mInstance) {
  unsigned char port, value;
  
  match(mInstance,OUTDAC);
  port = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  value = integer(mInstance,expr(mInstance));
	outdac(port,value);
	}


/*
  the LET statement
*/
void doLet(MINIBASIC *mInstance,int8_t matchlet) {
	LVALUE lv;
  char *temp;
	int n,t;
  char id[IDLENGTH];
  IDENT_LEN len;
  const char *p;  
//#warning prende una parola (minuscola o maiuscola ) da sola senza dire nulla (credo cerchi di vederla come variabile)
//#warning e non da nemmeno errore su statement maiuscoli sbagliati...  
//#warning e tipo PI lo prende minuscolo ma le funzioni maiuscole...
  
  if(matchlet) {
		match(mInstance,LET);
		}

  mInstance->string=skipSpaces(mInstance->string);
	getId(mInstance, mInstance->string, id, &len);   // devo andare a vedere cosa c'è dopo la stringa/var/label...
  p=skipSpaces(mInstance->string+len);

  switch(*p) {
    case '=':
      lvalue(mInstance,&lv);
      
is_var:
      switch(lv.type) {
        case FLTID:
          match(mInstance,EQUALS);
          *lv.d.dval = expr(mInstance);
          break;

        case INTID:
          match(mInstance,EQUALS);
    //			if(lv.d.ival)
            *lv.d.ival = integer(mInstance,expr(mInstance));
          break;

        case STRID:
          match(mInstance,EQUALS);
          temp = *lv.d.sval;
          *lv.d.sval = stringExpr(mInstance);
          if(temp)
            free(temp);
          break;

        case TIMER:
          lv.type=INTID;
          match(mInstance,OPAREN);
          t = integer(mInstance,expr(mInstance));
          match(mInstance,CPAREN);
          match(mInstance,EQUALS);
          n=integer(mInstance,expr(mInstance));

          switch(t) {
            case 1:
              TMR1=n;
              break;
            case 2:
    #if defined(USA_USB_HOST) || defined(USA_USB_SLAVE_CDC)
    #else
              TMR2=n;   // OCCHIO USB!!
    #endif
              break;
            case 3:
              TMR3=n;
              break;
            case 4:
              TMR4=n;
              break;
            case 5:
              TMR5=n;
              break;
            case 6:
              TMR6=n;    // OCCHIO se usato con ON TIMER...
              break;
            case 7:
              TMR7=n;
              break;
            case 8:
              TMR8=n;
              break;
            case 9:
              TMR9=n;
              break;
            default:
              setError(mInstance,ERR_BADVALUE);
              break;
            }

    //				*lv.d.ival = n;   // mettere??
          break;  //break timer

        case MIDSTRING:
          {
          char *str2,*str3;
          int len;

          match(mInstance,OPAREN);

    //			fprintf(fperr,"DEBUG2: lv=%x, lv.d=%x\n",lv,lv.d);

    //			/* *lv.d.sval= */ stringVar();		// cercaVar ...
          lvalue(mInstance,&lv);
          if(lv.type != STRID)
            setError(mInstance,ERR_TYPEMISMATCH);
          if(mInstance->errorFlag != ERR_CLEAR)
            break;

          match(mInstance,COMMA);
          n = integer(mInstance,expr(mInstance));
          if(mInstance->token==COMMA) {
            match(mInstance,COMMA);
            t = integer(mInstance,expr(mInstance));
            }
          else
            t=-1;

          match(mInstance,CPAREN);

    //			t= strlen(*lv.d.sval);
          if(t > (int) strlen(*lv.d.sval) || t<n || !t)	{
            setError(mInstance,ERR_ILLEGALOFFSET);
            break;
            }

          len=strlen(*lv.d.sval);
          if(t>0)
            len=len-n+t;
          else
            len=len+n;

          str3 = (char *)malloc(len + 1);
          if(!str3) {
            setError(mInstance,ERR_OUTOFMEMORY);
            break;
            }

          match(mInstance,EQUALS);
          str2=stringExpr(mInstance);

    //			fprintf(fperr,"DEBUG2: str3=%s, str2=%s, org=%s\n",str3,str2,*lv.d.sval);
          strcpy(str3,*lv.d.sval);
          strcpy(str3+n,str2);
    //			fprintf(fperr,"DEBUG2: str3=%s, str2=%s\n",str3,str2);
          if(t > 0)
            {
            strcat(str3,(*lv.d.sval)+t+n);
            }
    //			fprintf(fperr,"DEBUG2: str3=%s, str2=%s\n",str3,str2);

          free(str2);
          temp = *lv.d.sval;
          *lv.d.sval = str3;
          if(temp)
            free(temp);

          }//end MIDSTRING

          break;

        case MIDISTRING:
        {
          char *str2,*str3;
          match(mInstance,EQUALS);
          if(!isString(mInstance->token))
            setError(mInstance,ERR_TYPEMISMATCH);
          else {
            str2=stringExpr(mInstance);

            if(str2) {
              str3=str2;
              while(*str3)
                WriteModem(*str3++);
              free(str2);
              }
      //      else
      //      ;
          }
        }
          break;
    #warning PROVARE mando stringhe intere non singoli char
        case MODEMSTRING:
        {
          char *str2,*str3;
          match(mInstance,EQUALS);
          if(!isString(mInstance->token))
            setError(mInstance,ERR_TYPEMISMATCH);
          else {
            str2=stringExpr(mInstance);

            if(str2) {
              str3=str2;
              while(*str3)
                WriteModem(*str3++);
              free(str2);
              }
      //      else
      //      ;
            }
        }
          break;
        }
  		break;
      
		default:
      if(*(p-1)=='(') {   // VERIFICARE! sembra andare...
        lvalue(mInstance,&lv);
        goto is_var;
        }
      else
        setError(mInstance,ERR_SYNTAX);
			break;
		}
	
	}

void doDef(MINIBASIC *mInstance) {
  
  match(mInstance,DEF);
  // qua ignoro e basta
	}

/*
  the DIM statement
*/
void doDim(MINIBASIC *mInstance) {
  BYTE ndims=0;
  DIM_SIZE dims[MAXDIMS+1];
  char name[IDLENGTH];
  IDENT_LEN len;
  DIMVAR *dimvar;
  unsigned int i;
  unsigned int size=1;

  match(mInstance,DIM);

  switch(mInstance->token) {
    case DIMFLTID:
		case DIMSTRID:
		case DIMINTID:
      getId(mInstance,mInstance->string, name, &len);
			match(mInstance,mInstance->token);
			dims[ndims++] = integer(mInstance,expr(mInstance));
			while(mInstance->token == COMMA) {
				match(mInstance,COMMA);
				dims[ndims++] = integer(mInstance,expr(mInstance));
				if(ndims > MAXDIMS)	{
					setError(mInstance,ERR_TOOMANYDIMS);
					return;
					}
				} 

		  match(mInstance,CPAREN);
	  
			for(i=0; i<ndims; i++) {
				if(dims[i] < 0) {
					setError(mInstance,ERR_BADSUBSCRIPT);
					return;
          }
        }
      switch(ndims) {
        case 1:
          dimvar = dimension(mInstance,name, 1, dims[0]);
          break;
        case 2:
          dimvar = dimension(mInstance,name, 2, dims[0], dims[1]);
          break;
        case 3:
          dimvar = dimension(mInstance,name, 3, dims[0], dims[1], dims[2]);
          break;
        case 4:
          dimvar = dimension(mInstance,name, 4, dims[0], dims[1], dims[2], dims[3]);
          break;
        case 5:
          dimvar = dimension(mInstance,name, 5, dims[0], dims[1], dims[2], dims[3], dims[4]);
          break;
        }
      break;
    default:
      setError(mInstance,ERR_SYNTAX);
      return;
    }
  if(dimvar == NULL) {
  /* out of memory */
    setError(mInstance,ERR_OUTOFMEMORY);
    return;
    }


  for(i=0; i<dimvar->ndims; i++)
    size *= dimvar->dim[i];
  
  if(mInstance->token == EQUALS) {
    match(mInstance,EQUALS);

		switch(dimvar->type) {
      case FLTID:
				i=0;
				dimvar->d.dval[i++] = expr(mInstance);
				while(mInstance->token == COMMA && i < size)	{
					match(mInstance,COMMA);
					dimvar->d.dval[i++] = expr(mInstance);
          if(mInstance->errorFlag != ERR_CLEAR)
						break;
					}
				break;
			case STRID:
				i=0;
				if(dimvar->d.str[i])
					free(dimvar->d.str[i]);
				dimvar->d.str[i++] = stringExpr(mInstance);

				while(mInstance->token == COMMA && i < size)	{
					match(mInstance,COMMA);
					if(dimvar->d.str[i])
						free(dimvar->d.str[i]);
					dimvar->d.str[i++] = stringExpr(mInstance);
          if(mInstance->errorFlag != ERR_CLEAR)
						break;
					}
				break;
      case INTID:
				i=0;
				dimvar->d.ival[i++] = integer(mInstance,expr(mInstance));
				while(mInstance->token == COMMA && i < size)	{
					match(mInstance,COMMA);
					dimvar->d.ival[i++] = integer(mInstance,expr(mInstance));
          if(mInstance->errorFlag != ERR_CLEAR)
						break;
					}
				break;
			}
		
		if(mInstance->token == COMMA)
			setError(mInstance,ERR_TOOMANYINITS);
		}
  
  else {    // il baciapile non inizializzava a 0... ;) !
    
		switch(dimvar->type) {
      case FLTID:
				i=0;
				while(i < size)
  				dimvar->d.dval[i++]=0;
				break;
			case STRID:
				i=0;
				while(i < size)
  				dimvar->d.str[i++]=NULL;
				break;
      case INTID:
				i=0;
				while(i < size)
  				dimvar->d.ival[i++]=0;
				break;
			}
    }

	}


/*
  the IF statement.
  [if jump taken, returns new line no, else returns 0]
*/
LINE_NUMBER_STMT_POS_TYPE doIf(MINIBASIC *mInstance) {
  char condition;
  LINE_NUMBER_STMT_POS_TYPE jumpthen,jumpelse;
  
  jumpelse.d=STMT_CONTINUE;

  match(mInstance,IF);
  condition = boolExpr(mInstance);
  match(mInstance,THEN);
  if(mInstance->token==VALUE) {   // GD 2022
    jumpthen.line=findLine(mInstance,integer(mInstance,expr(mInstance)));
    jumpthen.pos=0;
    if(jumpthen.line == -1) {
      setError(mInstance,ERR_NOSUCHLINE);   // (sopra è gestita a parte)
      }
    if(mInstance->token == ELSE) {
      match(mInstance,ELSE);
      jumpelse.line=findLine(mInstance,integer(mInstance,expr(mInstance)));
      jumpelse.pos=0;
      if(jumpelse.line == -1) {
        setError(mInstance,ERR_NOSUCHLINE);   // (sopra è gestita a parte)
        }
      }
    return condition ? jumpthen : jumpelse;
    }
  else {
    if(condition) {
      jumpthen.d=STMT_STOP;
      return jumpthen;    // forza esecuzione stmt successivo, v. sopra
      }
    else {      // cerco EOL o ELSE, gestisco COLON 
      for(;;) {
        mInstance->token = getToken(mInstance->string);
//	      getId(mInstance,mInstance->string, nextid, &len);
        switch(mInstance->token) {
          case EOS:
//            match(mInstance,EOS);
            jumpthen.d=STMT_CONTINUE;
            return jumpthen;
            break;
          case EOL:
//            match(mInstance,EOL);
            jumpthen.d=STMT_CONTINUE;
            return jumpthen;
            break;
          case COLON:
            match(mInstance,COLON);
            break;
          case ELSE:
            match(mInstance,ELSE);
            jumpthen.d=STMT_STOP;
            return jumpthen;
            break;
          default:
            match(mInstance,mInstance->token);
            break;
          }
        }
          
      }
    }

	}

/*
  the GOTO statement
  returns new line number
*/
LINE_NUMBER_STMT_POS_TYPE subGoto(MINIBASIC *mInstance,uint16_t line) {
  LINE_NUMBER_STMT_POS_TYPE toline;

  toline.line = findLine(mInstance,line);
  if(toline.line == -1)
    setError(mInstance,ERR_NOSUCHLINE);   // (sopra è gestita a parte)
  toline.pos=0;
  return toline;
	}
LINE_NUMBER_STMT_POS_TYPE doGoto(MINIBASIC *mInstance) {
  LINE_NUMBER_STMT_POS_TYPE toline;

  match(mInstance,GOTO);
  return subGoto(mInstance,integer(mInstance,expr(mInstance)));
	}

LINE_NUMBER_STMT_POS_TYPE doGosub(MINIBASIC *mInstance) {
  LINE_NUMBER_STMT_POS_TYPE toline;
  int16_t i;

  match(mInstance,GOSUB);
  if(mInstance->ngosubs >= MAXGOSUB) {
	  setError(mInstance,ERR_TOOMANYGOSUB);
	  toline.d=STMT_STOP;
    return toline;
		}
  i=integer(mInstance,expr(mInstance));
  mInstance->gosubStack[mInstance->ngosubs++] = getNextStatement(mInstance,mInstance->string);
  return subGoto(mInstance,i);
	}

void doError(MINIBASIC *mInstance) {

  match(mInstance,ERRORTOK);
  setError(mInstance,integer(mInstance, expr(mInstance)));
	}

LINE_NUMBER_STMT_POS_TYPE doReturn(MINIBASIC *mInstance) {
  LINE_NUMBER_STMT_POS_TYPE n;

  match(mInstance,RETURN);
  
  // COME scegliere quello valido?? diciam che ci basiamo sulla line salvata
  if(mInstance->errorHandler.line.d) {
    n=mInstance->errorHandler.line;
    mInstance->errorHandler.line.d=0;
    mInstance->errorHandler.errorcode=0;
    return n;
    }
  if(mInstance->irqHandler.line.d) {
    n=mInstance->irqHandler.line;
    mInstance->irqHandler.line.d=0;
    return n;
    }
  if(mInstance->timerHandler.line.d) {
    n=mInstance->timerHandler.line;
    mInstance->timerHandler.line.d=0;
    return n;
    }

  if(mInstance->ngosubs <= 0) {
	  setError(mInstance,ERR_NORETURN);
    n.d=STMT_STOP;
	  return n;
		}
  
  return mInstance->gosubStack[--mInstance->ngosubs];
	}


/*
  The FOR statement.

  Pushes the for stack.
  Returns line to jump to, or -1 to end program
*/
LINE_NUMBER_STMT_POS_TYPE doFor(MINIBASIC *mInstance) {
  LVALUE lv;
  char id[IDLENGTH];
  IDENT_LEN len;
  NUM_TYPE initval;
  NUM_TYPE toval;
  NUM_TYPE stepval;
  LINE_NUMBER_STMT_POS_TYPE answer;

  match(mInstance,FOR);
  getId(mInstance,mInstance->string, id, &len);

  lvalue(mInstance,&lv);
  if(lv.type != FLTID && lv.type != INTID) {
    setError(mInstance,ERR_TYPEMISMATCH);
    answer.d=STMT_STOP;
		return answer;
		}
  match(mInstance,EQUALS);
  initval = expr(mInstance);
  match(mInstance,TO);
  toval = expr(mInstance);
  if(mInstance->token == STEP) {
    match(mInstance,STEP);
		stepval = expr(mInstance);
		}
  else
    stepval = 1.0;

  if(lv.type == INTID)
	  *lv.d.ival = initval;

	else
	  *lv.d.dval = initval;

  if(mInstance->nfors > MAXFORS - 1) {
		setError(mInstance,ERR_TOOMANYFORS);
    answer.d=STMT_STOP;
		return answer;
	  }
 
  if(((stepval < 0) && (initval <= toval)) || ((stepval > 0) && (initval >= toval))) {
    // questa roba serve a skippare completamente un FOR se le condizioni sono invalide... 
    //ragiona solo con le righe quindi dovrebbe gestire il : ...
	  char nextid[IDLENGTH];
    const char *savestring;
		savestring = mInstance->string;
//      mInstance->errorFlag = ERR_CLEAR;
    for(;;) {
      mInstance->token = getToken(mInstance->string);
//	      getId(mInstance,mInstance->string, nextid, &len);
      switch(mInstance->token) {
        case EOL:
          mInstance->curline++;
        case EOS:
        case COLON:
        case VALUE:
          match(mInstance,mInstance->token);
          break;
        case NEXT:
          match(mInstance,NEXT);
          if(mInstance->token == FLTID || mInstance->token == DIMFLTID ||
            mInstance->token == INTID || mInstance->token == DIMINTID) {
            getId(mInstance,mInstance->string, nextid, &len);
            if(!stricmp(id, nextid)) {    // case insens GD 2022
              match(mInstance,mInstance->token);
//              if(mInstance->token == EOL || mInstance->token == EOS || mInstance->token == COLON)
//                match(mInstance,mInstance->token);
              answer = getNextStatement(mInstance,mInstance->string);
//              mInstance->string = savestring;
//              mInstance->token = getToken(mInstance->string);
              if(!answer.d)
                answer.d=STMT_STOP;
              return answer;
              }
            }
          else {
//            if(mInstance->token == EOL || mInstance->token == EOS || mInstance->token == COLON)
//              match(mInstance,mInstance->token);
            answer = getNextStatement(mInstance,mInstance->string);
//            mInstance->string = savestring;
//            mInstance->token = getToken(mInstance->string);
            if(!answer.d)
              answer.d=STMT_STOP;
            return answer;
            }
          break;
        default:    // incluso ' che viene scartato anche senza : , ok
          match(mInstance,mInstance->token);
          break;
        }
      }
	
		setError(mInstance,ERR_NONEXT);
    answer.d=STMT_STOP;
		return answer;
  	}
  else {
		strcpy(mInstance->forStack[mInstance->nfors].id, id);
		mInstance->forStack[mInstance->nfors].nextline = getNextStatement(mInstance,mInstance->string);
		mInstance->forStack[mInstance->nfors].toval = toval;
		mInstance->forStack[mInstance->nfors].step = stepval;
		mInstance->nfors++;
    answer.d=STMT_CONTINUE;
		return answer;
	  }

	}



/*
  the NEXT statement
  updates the counting index, and returns line to jump to
*/
LINE_NUMBER_STMT_POS_TYPE doNext(MINIBASIC *mInstance) {
  char id[IDLENGTH];
  IDENT_LEN len;
  LVALUE lv;
	BYTE n;
  LINE_NUMBER_STMT_POS_TYPE answer;

  match(mInstance,NEXT);

  if(mInstance->nfors>0) {
		n=mInstance->nfors-1;
		if(mInstance->token == EOL || mInstance->token == COLON) {
			VARIABLE *var;
			var = findVariable(mInstance,mInstance->forStack[n].id);
/*			if(!var) {		// IMPOSSIBILE!
				setError(mInstance,ERR_OUTOFMEMORY);
				return 0;
				}*/
			lv.type = var->type;
			lv.d.dval = &var->d.dval;		// both int & real
			}
		else {
	    getId(mInstance,mInstance->string, id, &len);
			if(stricmp(id,mInstance->forStack[n].id)) {   // case GD 2022
		    setError(mInstance,ERR_NOFOR);
        answer.d=STMT_CONTINUE;
        return answer;
				}
	    lvalue(mInstance,&lv);
			}
  	if(lv.type != FLTID && lv.type != INTID) {
		  setError(mInstance,ERR_TYPEMISMATCH);
      answer.d=STMT_STOP;
    	return answer;
			}
	  if(lv.type == FLTID) {
	    *lv.d.dval += mInstance->forStack[n].step;
			if( (mInstance->forStack[n].step < 0 && *lv.d.dval < mInstance->forStack[n].toval) ||
				(mInstance->forStack[n].step > 0 && *lv.d.dval > mInstance->forStack[n].toval)) {
			  mInstance->nfors--;
        answer.d=STMT_CONTINUE;
        return answer;
				}
			else {
	      return mInstance->forStack[n].nextline;
				}
			}
		else {
	    *lv.d.ival += (int)mInstance->forStack[n].step;
			if( (mInstance->forStack[n].step < 0 && *lv.d.ival < mInstance->forStack[n].toval) ||
				(mInstance->forStack[n].step > 0 && *lv.d.ival > mInstance->forStack[n].toval)) {
			  mInstance->nfors--;
        answer.d=STMT_CONTINUE;
        return answer;
				}
			else {
	      return mInstance->forStack[n].nextline;
				}
			}
  	}
  else {
    setError(mInstance,ERR_NOFOR);
    answer.d=STMT_STOP;
    return answer;
  	}
	}


/*
  The DO statement.

  Pushes the do stack.
  Returns line to jump to, or -1 to end program
*/
LINE_NUMBER_STMT_POS_TYPE doDo(MINIBASIC *mInstance) {
  LINE_NUMBER_STMT_POS_TYPE answer;
  int t;

  match(mInstance,DO);
  if(mInstance->token == WHILE) {
	  match(mInstance,WHILE);
		t = boolExpr(mInstance);
		}
  // QUESTE NON VANNO!! finire
  else if(mInstance->token == UNTIL) {
	  match(mInstance,UNTIL);
		t = !boolExpr(mInstance);
		}
  
	else {
    t=1;
		}

  if(mInstance->ndos > MAXFORS-1) {
		setError(mInstance,ERR_TOOMANYFORS);
    answer.d=STMT_STOP;
		return answer;
	  }
  else {
		if(t) {
			mInstance->doStack[mInstance->ndos++] = getNextStatement(mInstance,mInstance->string);
      answer.d=STMT_CONTINUE;
    	return answer;
			}
		else {
			uint8_t n;

			n=0;
			do {
				mInstance->curline++;
				mInstance->string = mInstance->lines[mInstance->curline].str;
				mInstance->token = getToken(mInstance->string);
				switch(mInstance->token) {
          case DO:
            n++;
            break;
          case LOOP:
            n--;
            break;
				// use inBlocks...
          case COLON:
            break;
          case EOS:
            setError(mInstance,ERR_SYNTAX /* or NEXT / LOOP not found... */);
            break;
          default:
            break;
					}
				} while(mInstance->token != LOOP && n==0);
			answer.line = mInstance->curline;
      answer.pos = mInstance->string - mInstance->lines[mInstance->curline].str;
    	return answer;
			}
	  }

	}



/*
  The LOOP statement.

  Uses the do stack.
  Returns line to jump to, or -1 to end program
*/
LINE_NUMBER_STMT_POS_TYPE doLoop(MINIBASIC *mInstance) {
  LINE_NUMBER_STMT_POS_TYPE answer;
  int t;

  match(mInstance,LOOP);
  if(mInstance->token == WHILE) {
	  match(mInstance,WHILE);
		t = boolExpr(mInstance);
		}
  else if(mInstance->token == UNTIL) {
	  match(mInstance,UNTIL);
		t = !boolExpr(mInstance);
		}
	else {
    t=1;
		}

	if(mInstance->ndos>0) {
    if(t) {
      answer=mInstance->doStack[mInstance->ndos-1];
      }
    else {
      --mInstance->ndos;
      answer.d=STMT_CONTINUE;
      }
		}
  else {
    setError(mInstance,ERR_NOFOR);
    answer.d=STMT_STOP;
  	}
  return answer;
	}


/*
  the INPUT statement
*/
void doInput(MINIBASIC *mInstance) {
  LVALUE lv;
  char buff[256];
  char *end,*newbuff;
  enum _FILE_TYPES filetype=-1;
	void *ftemp=NULL;
  int i;

  match(mInstance,INPUTTOK);
  *buff=0;

  mInstance->string=(char *)skipSpaces(mInstance->string);
//	t=getToken(string); USARE?
  if(*mInstance->string == '#')	{	// per input from file
		unsigned int f,i;
    
		match(mInstance,DIESIS);
		f=integer(mInstance,expr(mInstance));
		match(mInstance,COMMA);

		for(i=0; i<mInstance->nfiles; i++) {
			if(mInstance->openFiles[i].number==f) {
        filetype=mInstance->openFiles[i].type;
				switch(filetype) {
					case FILE_COM:    // si potrebbe usare SUPERFILE anche qua...
//            ftemp=mInstance->openFiles[i].handle;
						break;
#ifdef USA_USB_SLAVE_CDC
					case FILE_CDC:
						break;
#endif
#ifdef USA_WIFI
					case FILE_TCP:
            ftemp=mInstance->openFiles[i].handle;
            *rxBuffer=0;
						break;
					case FILE_UDP:
            ftemp=mInstance->openFiles[i].handle;
            *rxBuffer=0;
						break;
#endif
					case FILE_DISK:
            ftemp=mInstance->openFiles[i].handle;
						break;
					}
        goto input_file_found;
				}
			}
    setError(mInstance,ERR_FILE);   // file not found/not valid
    return;
		}

input_file_found:
  
  switch(filetype) {
    case FILE_COM:
    {
      char *p=buff;
      while(*p && *p != '\n')   // finire....
        *p++=ReadSerial();
      *p=0;
    }
      break;
#ifdef USA_USB_SLAVE_CDC
    case FILE_CDC:
      scanf("%s",str);
      break;
#endif
#ifdef USA_WIFI
    case FILE_TCP:
//      strcat(rxBuffer,str);
      // fare
      break;
    case FILE_UDP:
//      strcat(rxBuffer,str);
      // fare
      break;
#endif
    case FILE_DISK:
      if(!SuperFileGets(ftemp,buff,254)) {     // 
        setError(mInstance,ERR_FILE);
        }
      else {
        }
      break;
    default:
      SetCursorMode(1,1);     // fare solo se input NON da file...
//      if(gets(buff) == 0)	{
  		if(mInstance->token == QUOTE) {
        inputString(buff,254,stringExpr(mInstance),0);
      	match(mInstance,COMMA);
        }
      else
        inputString(buff,254,"?",0);
      if(i == 0) {
//				setError(mInstance,ERR_EOF);
//        return;
        }
      myCR(mInstance);
      break;
    }

  if(!*buff) {
//				setError(mInstance,ERR_EOF);
//    goto fine_input;
    }
  end=strchr(buff, '\n');
  if(!end) {
//    setError(mInstance,ERR_INPUTTOOLONG);
// sì?        goto fine_input;
    }
  else *end=0;
  
  lvalue(mInstance,&lv);
	newbuff=strtok(buff,",");
  while(newbuff) {
    switch(lv.type) {
      // dare errore se non-numero??
      case FLTID:
        if(sscanf(newbuff,(char *)"%f", lv.d.dval) == 1) {
          }
				// se no, uscire?
				else
					goto fine_input;
        break;
      case INTID:
      {
        int n;
        if(sscanf(newbuff,(char *)"%d", &n) == 1) {
          *lv.d.ival=n;
          }
				// se no, uscire?
				else
					goto fine_input;
      }
        break;
      case STRID:
        if(*lv.d.sval) {
          free(*lv.d.sval);
          *lv.d.sval = 0;
          }
        *lv.d.sval = strdup(newbuff);
        if(!*lv.d.sval)	{
          setError(mInstance,ERR_OUTOFMEMORY);
  //				return;
          }
        break;
      default:
        goto fine_input;
        break;
      }
		if(mInstance->token != COMMA) {
      break;
      }
  	match(mInstance,COMMA);
		newbuff=strtok(NULL,",");
    lvalue(mInstance,&lv);
    // non è perfetto perchè se ci sono meno valori (con ,) che lista di variabili va in SYNTAX ERROR...
    }

fine_input:
  SetCursorMode(0,0);   // fare solo se in RUN!

	}

void doGet(MINIBASIC *mInstance) {
  char buff[256];
  enum _FILE_TYPES filetype=-1;
	void *ftemp=NULL;
  DIMVAR *dimvar;
  uint16_t varsize;
  char id[IDLENGTH];
  IDENT_LEN len;
  unsigned int f,i,j;

  match(mInstance,GET);
  match(mInstance,DIESIS);
  f=integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  getId(mInstance,mInstance->string, id, &len);
  match(mInstance,DIMINTID);
  dimvar = findDimVar(mInstance,id);
  if(!dimvar) {
    setError(mInstance,ERR_NOSUCHVARIABLE);
    return;
    }
  match(mInstance,CPAREN);    // diciam così, GET #1,s%(),1024
  match(mInstance,COMMA);
  varsize=integer(mInstance,expr(mInstance)); // andrebbe controllata dimensione... che è stata malloc'ata...

  if(dimvar->ndims != 1) {
    setError(mInstance,ERR_BADVALUE);
    return;
    }
      
  for(i=0; i<mInstance->nfiles; i++) {
    if(mInstance->openFiles[i].number==f) {
      filetype=mInstance->openFiles[i].type;
      switch(filetype) {
        case FILE_COM:
//            ftemp=mInstance->openFiles[i].handle;
          break;
#ifdef USA_USB_SLAVE_CDC
        case FILE_CDC:
          break;
#endif
#ifdef USA_WIFI
        case FILE_TCP:
          ftemp=mInstance->openFiles[i].handle;
          *rxBuffer=0;
          break;
        case FILE_UDP:
          ftemp=mInstance->openFiles[i].handle;
          *rxBuffer=0;
          break;
#endif
        case FILE_DISK:
//						FSfwrite("aaa",3,1,mInstance->openFiles[i].handle);		// gestire...
          ftemp=mInstance->openFiles[i].handle;
          break;
        }
      goto get_file_found;
      }
    }
  
  setError(mInstance,ERR_FILE);
  return;

get_file_found:
  switch(filetype) {
    case FILE_COM:
      for(j=0; j<varsize; j++) {
        dimvar->d.ival[j]=ReadSerial();
        }
      break;
#ifdef USA_USB_SLAVE_CDC
    case FILE_CDC:
      for(j=0; j<varsize; j++) {
        dimvar->d.ival[j]=getchar();
        }
      break;
#endif
#ifdef USA_WIFI
    case FILE_TCP:
      for(j=0; j<varsize; j++) {
        dimvar->d.ival[j]=rxBuffer[j];
        }
      break;
    case FILE_UDP:
      for(j=0; j<varsize; j++) {
        dimvar->d.ival[j]=rxBuffer[j];
        }
      break;
#endif
    case FILE_DISK:
      for(j=0; j<varsize; j++) {
        if(SuperFileRead(ftemp,&dimvar->d.ival[j],1) != 1) {
          setError(mInstance,ERR_FILE);
          break;
          }
        }
      break;
    default:
      break;
    }

	}

void doPut(MINIBASIC *mInstance) {
  char buff[256];
  enum _FILE_TYPES filetype=-1;
	void *ftemp=NULL;
  DIMVAR *dimvar;
  uint16_t varsize;
  char id[IDLENGTH];
  IDENT_LEN len;
  unsigned int f,i,j;

  match(mInstance,PUT);
  match(mInstance,DIESIS);
  f=integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  getId(mInstance,mInstance->string, id, &len);
  match(mInstance,DIMINTID);
  dimvar = findDimVar(mInstance,id);
  if(!dimvar) {
    setError(mInstance,ERR_NOSUCHVARIABLE);
    return;
    }
  match(mInstance,CPAREN);    // diciam così, PUT #1,s%(),1024
  match(mInstance,COMMA);
  varsize=integer(mInstance,expr(mInstance)); // andrebbe controllata dimensione... che è stata malloc'ata...

  if(dimvar->ndims != 1) {
    setError(mInstance,ERR_BADVALUE);
    return;
    }

  for(i=0; i<mInstance->nfiles; i++) {
    if(mInstance->openFiles[i].number==f) {
      filetype=mInstance->openFiles[i].type;
      switch(filetype) {
        case FILE_COM:
//            ftemp=mInstance->openFiles[i].handle;
          break;
#ifdef USA_USB_SLAVE_CDC
        case FILE_CDC:
          break;
#endif
#ifdef USA_WIFI
        case FILE_TCP:
          ftemp=mInstance->openFiles[i].handle;
          *rxBuffer=0;
          break;
        case FILE_UDP:
          ftemp=mInstance->openFiles[i].handle;
          *rxBuffer=0;
          break;
#endif
        case FILE_DISK:
//						FSfwrite("aaa",3,1,mInstance->openFiles[i].handle);		// gestire...
          ftemp=mInstance->openFiles[i].handle;
          break;
        }
      goto put_file_found;
      }
    }

  setError(mInstance,ERR_FILE);
  return;
  
put_file_found:

  switch(filetype) {
    case FILE_COM:
      for(j=0; j<varsize; j++)
        WriteSerial(LOBYTE(dimvar->d.ival[j]));
      break;
#ifdef USA_USB_SLAVE_CDC
    case FILE_CDC:
      for(j=0; j<varsize; j++)
        putchar(LOBYTE(dimvar->d.ival[j]));
      break;
#endif
#ifdef USA_WIFI
    case FILE_TCP:
      for(j=0; j<varsize; j++)
        rxBuffer[j]=LOBYTE(dimvar->d.ival[j]);
      send((SOCKET)(int)ftemp, rxBuffer, varsize, 0);
      break;
    case FILE_UDP:
      for(j=0; j<varsize; j++)
        rxBuffer[j]=LOBYTE(dimvar->d.ival[j]);
      send((SOCKET)(int)ftemp, rxBuffer, varsize, 0);
      break;
#endif
    case FILE_DISK:
      for(j=0; j<varsize; j++) {
        if(SuperFileWrite(ftemp,/*LOBYTE*/&dimvar->d.ival[j],1) != 1) {  // magari bufferizzare??
          setError(mInstance,ERR_FILE);
          break;
          }
        }
      break;
    default:

      break;
    }

	}

/*
  the REM statement.
  Note is unique as the rest of the line is not parsed
*/
void doRem(MINIBASIC *mInstance) {

//  match(mInstance,REM);
  
  match(mInstance,mInstance->token);		//
  while(*mInstance->string && *mInstance->string != '\n')
    mInstance->string++;
  if(*mInstance->string == '\n')
    mInstance->string++;
  mInstance->curline++;
	}


/*
  ------------------
*/
__longramfunc__ (*ramfuncEntryPoint)();
void doSys(MINIBASIC *mInstance) {
  uint32_t t;
  void (*funcEntryPoint)();
  
  match(mInstance,SYS);
  t=integer(mInstance,expr(mInstance));
  if(t>=0x1d000000) {     // boh... diciamo v. usr
  // se RAM... e se ROM? serve ASM?
    funcEntryPoint = (void *)t;
    (*funcEntryPoint)();
    }
  else {
    ramfuncEntryPoint = (void *)t;
    (*ramfuncEntryPoint)();
    }

//__asm__("MOV _a,W0");		// tendenzialmente "a" si trova già in W0 ! (v. disassembly)
//__asm__("CALL W0");
//  _jr_hb();
//  __asm__("j": "=c" (a));   // VERIFICARE!
	}



/*
  ------STOP--------
*/
void doStop(MINIBASIC *mInstance) {

  match(mInstance,STOP);
	setError(mInstance,ERR_STOP);
  
// waits for a key, (vedere sopra loop che gestisce keypress/WM_CHAR per errori!)
	}


void doLine(MINIBASIC *mInstance) {
  POINT pt1,pt2;
  BYTE size;
  HDC myDC,*hDC;

  match(mInstance,LINETOK);
  pt1.x = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pt1.y = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pt2.x = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pt2.y = integer(mInstance,expr(mInstance));
  if(mInstance->token == COMMA) {
    match(mInstance,COMMA);
    size=integer(mInstance,expr(mInstance));
    }
  else
    size=1;
  
  if((pt1.x < 0 || pt1.x > Screen.cx) ||
    (pt1.y < 0 || pt1.y > Screen.cy) ||
    (pt2.x < 0 || pt2.x > Screen.cx) ||
    (pt2.y < 0 || pt2.y > Screen.cy)) {
		setError(mInstance, ERR_BADVALUE);
    return;
    }

#ifdef USA_BREAKTHROUGH
  hDC=GetDC(mInstance->hWnd,&myDC);
  hDC->pen=CreatePen(PS_SOLID,size,Color24To565(mInstance->Color));
  MoveTo(hDC,pt1.x,pt1.y);
  LineTo(hDC,pt2.x,pt2.y);
  mInstance->Cursor.x=pt2.x;
  mInstance->Cursor.y=pt2.y;
  ReleaseDC(mInstance->hWnd,hDC);
#else
  BYTE oldSize=fontSize;
  SetCharMode(fontFace,size,fontStyle);
  DrawLine(pt1.x,pt1.y,pt2.x,pt2.y,Color24To565(mInstance->Color));
  SetCharMode(fontFace,oldSize,fontStyle);
// no! text  mInstance->Cursor.x=pt1.x;
//  mInstance->Cursor.y=pt1.y;
#endif
	}

void doRectangle(MINIBASIC *mInstance) {
  POINT pt[2];
  BYTE size,filled;
  HDC myDC,*hDC;

  match(mInstance,RECTANGLE);
  pt[0].x = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pt[0].y = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pt[1].x = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pt[1].y = integer(mInstance,expr(mInstance));
	
  if(mInstance->token == COMMA) {
    match(mInstance,COMMA);
    size=integer(mInstance,expr(mInstance));
    }
  else
    size=1;
  if(mInstance->token == COMMA) {
    match(mInstance,COMMA);
    filled=integer(mInstance,expr(mInstance));
    }
  else
    filled=0;
  
  if((pt[0].x < 0 || pt[0].x > Screen.cx) ||
    (pt[0].y < 0 || pt[0].y > Screen.cy) ||
    (pt[1].x < 0 || pt[1].x > Screen.cx) ||
    (pt[1].y < 0 || pt[1].y > Screen.cy)) {
		setError(mInstance, ERR_BADVALUE);
    return;
    }

#ifdef USA_BREAKTHROUGH
  hDC=GetDC(mInstance->hWnd,&myDC);
  hDC->pen=CreatePen(PS_SOLID,size,Color24To565(mInstance->Color));
  Rectangle(hDC,pt[0].x,pt[0].y,pt[1].x,pt[1].y);
  ReleaseDC(mInstance->hWnd,hDC);
#else
	//qui usa TextSize come size, volendo...
  BYTE oldSize=fontSize;
  SetCharMode(fontFace,size,fontStyle);
  if(filled)
    FillRectangle(pt[0].x,pt[0].y,pt[1].x,pt[1].y,Color24To565(mInstance->Color));
  else
    DrawRectangle(pt[0].x,pt[0].y,pt[1].x,pt[1].y,Color24To565(mInstance->Color));
  SetCharMode(fontFace,oldSize,fontStyle);
#endif
	}

void doTriangle(MINIBASIC *mInstance) {
  POINT pt[4];
  BYTE size,filled;
  HDC myDC,*hDC;

  match(mInstance,TRIANGLE);
  pt[3].x = pt[0].x = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pt[3].y = pt[0].y = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pt[1].x = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pt[1].y = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pt[2].x = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pt[2].y = integer(mInstance,expr(mInstance));
	
  if(mInstance->token == COMMA) {
    match(mInstance,COMMA);
    size=integer(mInstance,expr(mInstance));
    }
  else
    size=1;
  if(mInstance->token == COMMA) {
    match(mInstance,COMMA);
    filled=integer(mInstance,expr(mInstance));
    }
  else
    filled=0;
  
  if((pt[0].x < 0 || pt[0].x > Screen.cx) ||
    (pt[0].y < 0 || pt[0].y > Screen.cy) ||
    (pt[1].x < 0 || pt[1].x > Screen.cx) ||
    (pt[1].y < 0 || pt[1].y > Screen.cy) ||
    (pt[2].x < 0 || pt[2].x > Screen.cx) ||
    (pt[2].y < 0 || pt[2].y > Screen.cy)) {
		setError(mInstance, ERR_BADVALUE);
    return;
    }
  
#ifdef USA_BREAKTHROUGH
  hDC=GetDC(mInstance->hWnd,&myDC);
  hDC->pen=CreatePen(PS_SOLID,size,Color24To565(mInstance->Color));
  Polygon(hDC,pt,4);
  ReleaseDC(mInstance->hWnd,hDC);
#else
  BYTE oldSize=fontSize;
  SetCharMode(fontFace,size,fontStyle);
  DrawLine(pt[0].x,pt[0].y,pt[1].x,pt[1].y,Color24To565(mInstance->Color));
  DrawLine(pt[1].x,pt[1].y,pt[2].x,pt[2].y,Color24To565(mInstance->Color));
  DrawLine(pt[2].x,pt[2].y,pt[3].x,pt[3].y,Color24To565(mInstance->Color));
  if(filled)
    Fill((pt[2].x+pt[1].x+pt[0].x)/3,(pt[2].y+pt[1].y+pt[0].y)/3,
  // https://vivalascuola.studenti.it/come-trovare-il-baricentro-di-un-triangolo-nel-piano-cartesiano-57565.html#steps_3
      Color24To565(mInstance->Color),Color24To565(mInstance->Color),1);
  SetCharMode(fontFace,oldSize,fontStyle);
#endif
	}

void doEllipse(MINIBASIC *mInstance) {
  POINT pt;
  uint16_t rx,ry;
  BYTE size,filled;
  HDC myDC,*hDC;

  match(mInstance,ELLIPSE);
  pt.x = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pt.y = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  rx = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  ry = integer(mInstance,expr(mInstance));
/* boh... non so se è il caso
    if(pt1.x-rx < 0 || pt1.x+rx > Screen.cx)
		setError(mInstance, ERR_BADVALUE);
  if(pt1.y-ry < 0 || pt1.y+ry > Screen.cx)
		setError(mInstance, ERR_BADVALUE);
  */
	
  if(mInstance->token == COMMA) {
    match(mInstance,COMMA);
    size=integer(mInstance,expr(mInstance));
    }
  else
    size=1;
  if(mInstance->token == COMMA) {
    match(mInstance,COMMA);
    filled=integer(mInstance,expr(mInstance));
    }
  else
    filled=0;
  
  if((pt.x < 0 || pt.x > Screen.cx) ||
    (pt.y < 0 || pt.y > Screen.cy)) {
    // rx ry??
		setError(mInstance, ERR_BADVALUE);
    return;
    }

  // FINIRE ellipse!
#ifdef USA_BREAKTHROUGH
  hDC=GetDC(mInstance->hWnd,&myDC);
  hDC->pen=CreatePen(PS_SOLID,size,Color24To565(mInstance->Color));
  hDC->brush=CreateSolidBrush(Color24To565(mInstance->ColorBK));
  Ellipse(hDC,pt.x-rx,pt.y-ry,pt.x+rx,pt.y+ry);
  ReleaseDC(mInstance->hWnd,hDC);
#else
  BYTE oldSize=fontSize;
  SetCharMode(fontFace,size,fontStyle);
  if(filled)
    FillCircle(pt.x,pt.y,rx,Color24To565(mInstance->Color));
  else
    DrawCircle(pt.x,pt.y,rx,Color24To565(mInstance->Color));
  SetCharMode(fontFace,oldSize,fontStyle);
#endif
  
	}

void doCircle(MINIBASIC *mInstance) {
  POINT pt;
  uint16_t r;
  BYTE size,filled;
  HDC myDC,*hDC;

  match(mInstance,CIRCLE);
  pt.x = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pt.y = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  r = integer(mInstance,expr(mInstance));
/* boh... non so se è il caso
    if(pt1.x-r < 0 || pt1.x+r > Screen.cx)
		setError(mInstance, ERR_BADVALUE);
  if(pt1.y-r < 0 || pt1.y+r > Screen.cx)
		setError(mInstance, ERR_BADVALUE);
  */
	
  if(mInstance->token == COMMA) {
    match(mInstance,COMMA);
    size=integer(mInstance,expr(mInstance));
    }
  else
    size=1;
  if(mInstance->token == COMMA) {
    match(mInstance,COMMA);
    filled=integer(mInstance,expr(mInstance));
    }
  else
    filled=0;
  
  if((pt.x < 0 || pt.x > Screen.cx) ||
    (pt.y < 0 || pt.y > Screen.cy)) {
    // r??
		setError(mInstance, ERR_BADVALUE);
    return;
    }
#ifdef USA_BREAKTHROUGH
  hDC=GetDC(mInstance->hWnd,&myDC);
  hDC->pen=CreatePen(PS_SOLID,size,Color24To565(mInstance->Color));
  hDC->brush=CreateSolidBrush(Color24To565(mInstance->ColorBK));
  Ellipse(hDC,pt.x-r,pt.y-r,pt.x+r,pt.y+r);
  ReleaseDC(mInstance->hWnd,hDC);
#else
  BYTE oldSize=fontSize;
  SetCharMode(fontFace,size,fontStyle);
  if(filled)
    FillCircle(pt.x,pt.y,r,Color24To565(mInstance->Color));
  else
    DrawCircle(pt.x,pt.y,r,Color24To565(mInstance->Color));
  SetCharMode(fontFace,oldSize,fontStyle);
#endif
  
	}

void doArc(MINIBASIC *mInstance) {
  uint16_t x1,y1,x2,y2;
  BYTE size;
  HDC myDC,*hDC;

  match(mInstance,ARC);
  x1 = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  y1 = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  x2 = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  y2 = integer(mInstance,expr(mInstance));

  if(mInstance->token == COMMA) {
    match(mInstance,COMMA);
    size=integer(mInstance,expr(mInstance));
    }
  else
    size=1;
  
  if((x1 < 0 || x1 > Screen.cx) ||
    (y1 < 0 || y1 > Screen.cy) ||
    (x2 < 0 || x2 > Screen.cx) ||
    (y2 < 0 || y2 > Screen.cy)) {
		setError(mInstance, ERR_BADVALUE);
    return;
    }
  
#ifdef USA_BREAKTHROUGH
  hDC=GetDC(mInstance->hWnd,&myDC);
  hDC->pen=CreatePen(PS_SOLID,size,Color24To565(mInstance->Color));
// finire...
  ReleaseDC(mInstance->hWnd,hDC);
#else
// finire...
#endif
	}

void doPoint(MINIBASIC *mInstance) {
  POINT pt;
  BYTE size;
  int32_t c;
  HDC myDC,*hDC;

  match(mInstance,POINTTOK);
  pt.x = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pt.y = integer(mInstance,expr(mInstance));
  
  if(mInstance->token == COMMA) {   // può seguire size...
    match(mInstance,COMMA);
    size=integer(mInstance,expr(mInstance));
    }
  else
    size=1;
  if(mInstance->token == COMMA) {   // ..e color
    match(mInstance,COMMA);
    c = expr(mInstance);
    if(c < 0) {	// se negativo, RGB!
      c=-c;
      if(c > 0xffffff)
        setError(mInstance, ERR_BADVALUE);
      }
    else if(c > 15)
      setError(mInstance, ERR_BADVALUE);
    else {
      c=ColorRGB(textColors[c]);
      }
    }
  else
    c=ColorRGB(mInstance->ColorPalette);
  
  if((pt.x < 0 || pt.x > Screen.cx) ||
    (pt.y < 0 || pt.y > Screen.cy)) {
		setError(mInstance, ERR_BADVALUE);
    return;
    }
  
#ifdef USA_BREAKTHROUGH
  hDC=GetDC(mInstance->hWnd,&myDC);
//  hDC.pen=CreatePen(PS_SOLID,size,mInstance->Color);
  mInstance->Cursor.x=pt.x;
  mInstance->Cursor.y=pt.y;

  if(size==1)
    SetPixel(hDC,pt.x,pt.y,Color24To565(c));
  else {
    RECT rc;
    BRUSH b;
    rc.left=pt.x;
    rc.top=pt.y;
    rc.right=rc.left+size;
    rc.bottom=rc.top+size;
    b=CreateSolidBrush(Color24To565(c));
    FillRect(hDC,&rc,b);
    }
	
  ReleaseDC(mInstance->hWnd,hDC);
#else
// no! text  mInstance->Cursor.x=pt.x;
//  mInstance->Cursor.y=pt.y;

#ifndef USING_SIMULATOR
  if(size==1)
    DrawPixel(pt.x,pt.y,Color24To565(c));
  else {
    RECT rc;
    rc.left=pt.x;
    rc.top=pt.y;
    rc.right=rc.left+size;
    rc.bottom=rc.top+size;
    FillRectangle(rc.left,rc.top,rc.right,rc.bottom,Color24To565(c));
    }
#endif
#endif

  }

void doFill(MINIBASIC *mInstance) {
  POINT pt;
  BYTE bordermode=0;
  int32_t c,c1;
  HDC myDC,*hDC;

  match(mInstance,FILL);
  pt.x = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pt.y = integer(mInstance,expr(mInstance));
  
  match(mInstance,COMMA);
  c = expr(mInstance);
  if(c < 0) {	// se negativo, RGB!
    c=-c;
    if(c > 0xffffff)
      setError(mInstance, ERR_BADVALUE);
    }
  else if(c > 15)
    setError(mInstance, ERR_BADVALUE);
  else {
    c=ColorRGB(textColors[c]);
    }

  if(mInstance->token == COMMA) {   // ..colore che delimita??
    match(mInstance,COMMA);
    bordermode=1;
    c1 = expr(mInstance);
    if(c1 < 0) {	// se negativo, RGB!
      c1=-c1;
      if(c1 > 0xffffff)
        setError(mInstance, ERR_BADVALUE);
      }
    else if(c1 > 15)
      setError(mInstance, ERR_BADVALUE);
    else {
      c1=ColorRGB(textColors[c1]);
      }
    }
  
  if((pt.x < 0 || pt.x > Screen.cx) ||
    (pt.y < 0 || pt.y > Screen.cy)) {
		setError(mInstance, ERR_BADVALUE);
    return;
    }

#ifdef USA_BREAKTHROUGH
  hDC=GetDC(mInstance->hWnd,&myDC);
//  hDC.pen=CreatePen(PS_SOLID,size,mInstance->Color);
  mInstance->Cursor.x=pt.x;
  mInstance->Cursor.y=pt.y;

  if(!bordermode)
    FloodFill(hDC,pt.x,pt.y,Color24To565(c));
  else
    FloodFill(hDC,pt.x,pt.y,Color24To565(c));   // QUA non c'è...
	
  ReleaseDC(mInstance->hWnd,hDC);
#else

  if(!bordermode)
    Fill(pt.x,pt.y,Color24To565(c),0,0);
  else
    Fill(pt.x,pt.y,Color24To565(c),Color24To565(c1),1);

#endif

  }

void doSetcolor(MINIBASIC *mInstance) {
  int32_t c;
  
  match(mInstance,SETCOLOR);
  c = expr(mInstance);
  if(c < 0) {	// se negativo, RGB!
		c=-c;
		if(c > 0xffffff)
			setError(mInstance, ERR_BADVALUE);
		mInstance->Color=c;
		mInstance->ColorPalette=Color24To565(c);
		}
  else if(c > 15)
		setError(mInstance, ERR_BADVALUE);
	else {
		mInstance->ColorPalette=textColors[c];
		mInstance->Color=ColorRGB(mInstance->ColorPalette);
		}
  
  SetColors(Color24To565(mInstance->Color),Color24To565(mInstance->ColorBK)); // questo serve cmq per printf/putchar
	}

void doSetBkcolor(MINIBASIC *mInstance) {
  int32_t c;
  
  match(mInstance,SETBKCOLOR);
  c = expr(mInstance);
  if(c < 0) {	// se negativo, RGB!
		c=-c;
		if(c > 0xffffff)
			setError(mInstance, ERR_BADVALUE);
		mInstance->ColorBK=c;
		mInstance->ColorPaletteBK=Color24To565(c);
		}
  else if(c > 15)
		setError(mInstance, ERR_BADVALUE);
	else {
	  mInstance->ColorPaletteBK=textColors[c];
		mInstance->ColorBK=ColorRGB(mInstance->ColorPaletteBK);
		}
  
  SetColors(Color24To565(mInstance->Color),Color24To565(mInstance->ColorBK)); // questo serve cmq per printf/putchar
	}

void doBitblt(MINIBASIC *mInstance) {
  DIMVAR *dimvar;
  uint16_t varsize;
  char id[IDLENGTH];
  IDENT_LEN len;
  POINT pt;
  SIZE sz;
  BYTE size;
  int x,y,n;
  HDC myDC,*hDC;

  match(mInstance,BITBLT);
  pt.x = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  pt.y = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  sz.cx = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  sz.cy = integer(mInstance,expr(mInstance));
	
  match(mInstance,COMMA);
  getId(mInstance,mInstance->string, id, &len);
  match(mInstance,DIMINTID);
  dimvar = findDimVar(mInstance,id);
  if(!dimvar) {
    setError(mInstance,ERR_NOSUCHVARIABLE);
    return;
    }
  match(mInstance,CPAREN);    // diciam così, BITBLT x,y,cx,cy,s%()
  
  if(mInstance->token == COMMA) {   // può seguire size...
    match(mInstance,COMMA);
    size=integer(mInstance,expr(mInstance));
    }
  else
    size=1;

  if((pt.x < 0 || pt.x > Screen.cx) ||
    (pt.y < 0 || pt.y > Screen.cy) ||
    (sz.cx < 0 || sz.cx >= Screen.cx) ||    // + pt.x ...
    (sz.cy < 0 || sz.cy >= Screen.cy)) {
		setError(mInstance, ERR_BADVALUE);
    return;
    }
  
  if(dimvar->ndims==1) {
#ifdef USA_BREAKTHROUGH
    hDC=GetDC(mInstance->hWnd,&myDC);
    n=0;
    for(y=0; y<sz.cy; y++) {
      for(x=0; x<sz.cx; x++) {
        if(size==1)
          SetPixel(hDC,x+pt.x,y+pt.y,(GFX_COLOR)dimvar->d.ival[n++]);
        else {
          RECT rc;
          BRUSH b;
          rc.left=pt.x+x*size;
          rc.top=pt.y+y*size;
          rc.right=rc.left+size;
          rc.bottom=rc.top+size;
          b=CreateSolidBrush((GFX_COLOR)dimvar->d.ival[n++]);
          FillRect(hDC,&rc,b);
          }
        }
      }
    ReleaseDC(mInstance->hWnd,hDC);
#else
    n=0;
    for(y=0; y<sz.cy; y++) {
      for(x=0; x<sz.cx; x++) {
        if(size==1)
          DrawPixel(x+pt.x,y+pt.y,(GFX_COLOR)dimvar->d.ival[n++]);    // dovrebbe...
        else {
          RECT rc;
          rc.left=pt.x+x*size;
          rc.top=pt.y+y*size;
          rc.right=rc.left+size;
          rc.bottom=rc.top+size;
          FillRectangle(rc.left,rc.top,rc.right,rc.bottom,(GFX_COLOR)dimvar->d.ival[n++]);
          }
        }
      }
#endif
    }
  else
		setError(mInstance, ERR_BADVALUE);

	}


/*
  Get an lvalue from the environment
  Params: lv - structure to fill.
  Notes: missing variables (but not out of range subscripts)
         are added to the variable list.
*/
void lvalue(MINIBASIC *mInstance,LVALUE *lv) {
  char name[IDLENGTH];
  IDENT_LEN len;
  VARIABLE *var;
  DIMVAR *dimvar;
  DIM_SIZE index[MAXDIMS];
  void *valptr = 0;
  char type;
  
  lv->type = B_ERROR;
  lv->d.dval = 0;		// clears them all

  switch(mInstance->token) {
    case FLTID:
			getId(mInstance,mInstance->string, name, &len);
			match(mInstance,FLTID);
			var = findVariable(mInstance,name);
			if(!var)
				var = addFloat(mInstance,name);
			if(!var) {
				setError(mInstance,ERR_OUTOFMEMORY);
				return;
				}
			lv->type = FLTID;
			lv->d.dval = &var->d.dval;
			break;
    case STRID:
			getId(mInstance,mInstance->string, name, &len);
			match(mInstance,STRID);
			var = findVariable(mInstance,name);
			if(!var)
				var = addString(mInstance,name);
			if(!var) {
				setError(mInstance,ERR_OUTOFMEMORY);
				return;
				}
			lv->type = STRID;
			lv->d.sval = &var->d.sval;
			break;
    case INTID:
			getId(mInstance,mInstance->string, name, &len);
			match(mInstance,INTID);
			var = findVariable(mInstance,name);
			if(!var)
				var = addInt(mInstance,name);
			if(!var) {
				setError(mInstance,ERR_OUTOFMEMORY);
				return;
				}
			lv->type = INTID;
			lv->d.ival = &var->d.ival;
			break;
		case DIMFLTID:
		case DIMSTRID:
		case DIMINTID:
			type = (mInstance->token == DIMFLTID) ? FLTID : ((mInstance->token == DIMSTRID) ? STRID : INTID);
			getId(mInstance,mInstance->string, name, &len);
			match(mInstance,mInstance->token);
			dimvar = findDimVar(mInstance,name);
			if(dimvar) {
				switch(dimvar->ndims)	{
					case 1:
						index[0] = integer(mInstance,expr(mInstance));
            if(mInstance->errorFlag == ERR_CLEAR)
              valptr = getDimVar(mInstance,dimvar, index[0]);
						break;
					case 2:
						index[0] = integer(mInstance,expr(mInstance));
						match(mInstance,COMMA);
						index[1] = integer(mInstance,expr(mInstance));
            if(mInstance->errorFlag == ERR_CLEAR)
							valptr = getDimVar(mInstance,dimvar, index[0], index[1]);
						break;
					case 3:
						index[0] = integer(mInstance,expr(mInstance));
						match(mInstance,COMMA);
						index[1] = integer(mInstance,expr(mInstance));
						match(mInstance,COMMA);
						index[2] = integer(mInstance,expr(mInstance));
            if(mInstance->errorFlag == ERR_CLEAR)
							valptr = getDimVar(mInstance,dimvar, index[0], index[1], index[2]);
						break;
				  case 4:
						index[0] = integer(mInstance,expr(mInstance));
						match(mInstance,COMMA);
						index[1] = integer(mInstance,expr(mInstance));
						match(mInstance,COMMA);
            index[2] = integer(mInstance,expr(mInstance));
						match(mInstance,COMMA);
						index[3] = integer(mInstance,expr(mInstance));
            if(mInstance->errorFlag == ERR_CLEAR)
							valptr = getDimVar(mInstance,dimvar, index[0], index[1], index[2], index[3]);
						break;
					case 5:
						index[0] = integer(mInstance,expr(mInstance));
						match(mInstance,COMMA);
						index[1] = integer(mInstance,expr(mInstance));
						match(mInstance,COMMA);
						index[2] = integer(mInstance,expr(mInstance));
						match(mInstance,COMMA);
						index[3] = integer(mInstance,expr(mInstance));
						match(mInstance,COMMA);
						index[4] = integer(mInstance,expr(mInstance));
            if(mInstance->errorFlag == ERR_CLEAR)
							valptr = getDimVar(mInstance,dimvar, index[0], index[1], index[2], index[3]);
						break;
					}
				match(mInstance,CPAREN);
				}
			else {
        int16_t defLine;
        const char *oldPos;
        char *p;
        char buf[256];
        defLine=findDef(mInstance,name);
        if(defLine != -1) {
          // CHIAMARE FUNZIONE DEF FN!
          oldPos=mInstance->string;
          // substDefParm(mInstance,mInstance->string,p,buf) FARE!
//          mInstance->curline=defLine;   // serve davvero?
          mInstance->string=buf;
          expr(mInstance);
//          mInstance->curline=oldLine;   // serve davvero?
          mInstance->string=oldPos;
          }
        else {
          setError(mInstance,ERR_NOSUCHVARIABLE);
          return;
          }
				}
	  if(valptr) {
			lv->type = type;
	    switch(type) {
        case FLTID:
          lv->d.dval = valptr;
          break;
        case STRID:
          lv->d.sval = valptr;
          break;
        case INTID:
          lv->d.ival = valptr;
          break;
        default:
          basicAssert(0);
        }
		  }
		  break;
		case TIMER:
			match(mInstance,TIMER);
			lv->type = TIMER;
//			lv->dval = &var->dval;
//			lv->sval = 0;
			break;
		case MIDSTRING:
			match(mInstance,MIDSTRING);
			lv->type = MIDSTRING;
//			lv->dval = &var->dval;
//			lv->sval = 0;
			break;
		case MIDISTRING:
			match(mInstance,MIDISTRING);
			lv->type = MIDISTRING;
//			lv->dval = &var->dval;
//			lv->sval = 0;
			break;
		case MODEMSTRING:
			match(mInstance,MODEMSTRING);
			lv->type = MODEMSTRING;
//			lv->dval = &var->dval;
//			lv->sval = 0;
			break;
		default:
			setError(mInstance,ERR_SYNTAX);
		  break;
		}
	}


/*
  parse a boolean expression
  consists of expressions or strings and relational operators,
  and parentheses
*/
signed char boolExpr(MINIBASIC *mInstance) {
  signed char left,right;
  
  left = boolFactor(mInstance);

  while(1) {
    switch(mInstance->token) {
			case AND:
				match(mInstance,AND);
				right = boolExpr(mInstance);
				return (left && right) ? -1 : 0;
			case OR:
				match(mInstance,OR);
				right = boolExpr(mInstance);
				return (left || right) ? -1 : 0;
			default:
				return left;
			}
		}
	}


/*
  boolean factor, consists of expression relOp expression
    or string relOp string, or ( boolExpr())
*/
signed char boolFactor(MINIBASIC *mInstance) {
  signed char answer;
  NUM_TYPE left,right;
  signed char op;
  char *strleft,*strright;

  switch(mInstance->token) {
    case OPAREN:
			match(mInstance,OPAREN);
			answer = boolExpr(mInstance);
			match(mInstance,CPAREN);
			break;
		default:
			if(isString(mInstance->token)) {
				strleft = stringExpr(mInstance);
				op = relOp(mInstance);
				strright = stringExpr(mInstance);
				if(!strleft || !strright)	{
					if(strleft)
						free(strleft);
					if(strright)
						free(strright);
					return 0;
					}
				answer= strRop(op,strleft,strright);
			
				free(strleft);
				free(strright);
			  }
		  else {
		    left = expr(mInstance);
//				op = relOp();
//				right = expr(mInstance);
//				answer= rop(op,left,right);
				answer=left;
				}
			break;
	  }

  return answer;
	}


/*
  ------------------
*/
signed char rop(signed char op, NUM_TYPE left, NUM_TYPE right) {
	signed char answer;

	switch(op) {
		case ROP_EQ:
			answer = (left == right) ? -1 : 0;
			break;
		case ROP_NEQ:
			answer = (left != right) ? -1 : 0;
			break;
		case ROP_LT:
			answer = (left < right) ? -1 : 0;
			break;
		case ROP_LTE:
			answer = (left <= right) ? -1 : 0;
			break;
		case ROP_GT:
			answer = (left > right) ? -1 : 0;
			break;
		case ROP_GTE:
			answer = (left >= right) ? -1 : 0;
			break;
		default:    // NON deve succedere!
		  return 0;
			break;
		}

	return answer;
	}



/*
  ------------------
*/
signed char strRop(signed char op, const char *left, const char *right) {
	signed char answer;

	answer = strcmp(left, right);
	switch(op) {
		case ROP_EQ:
			answer = answer == 0 ? -1 : 0;
			break;
		case ROP_NEQ:
			answer = answer == 0 ? 0 : -1;
			break;
		case ROP_LT:
			answer = answer < 0 ? -1 : 0;
			break;
		case ROP_LTE:
			answer = answer <= 0 ? -1 : 0;
			break;
		case ROP_GT:
			answer = answer > 0 ? -1 : 0;
			break;
		case ROP_GTE:
			answer = answer >= 0 ? -1 : 0;
			break;
		default:    // NON deve succedere!
			answer = 0;
			break;
		}

	return answer;
	}


/*
  ------------------
*/
signed char iRop(signed char op, int16_t left, int16_t right) {
	char answer;

	switch(op) {
		case ROP_EQ:
			answer = (left == right) ? -1 : 0;
			break;
		case ROP_NEQ:
			answer = (left != right) ? -1 : 0;
			break;
		case ROP_LT:
			answer = (left < right) ? -1 : 0;
			break;
		case ROP_LTE:
			answer = (left <= right) ? -1 : 0;
			break;
		case ROP_GT:
			answer = (left > right) ? -1 : 0;
			break;
		case ROP_GTE:
			answer = (left >= right) ? -1 : 0;
			break;
		default:    // NON deve succedere!
		  return 0;
			break;
		}

	return answer;
	}



/*
  get a relational operator
  returns operator parsed or ERROR
*/
enum REL_OPS relOp(MINIBASIC *mInstance) {

  switch(mInstance->token) {
    case EQUALS:
		  match(mInstance,EQUALS);
			return ROP_EQ;
			break;
    case GREATER:
			match(mInstance,GREATER);
			if(mInstance->token == EQUALS) {
        match(mInstance,EQUALS);
				return ROP_GTE;
				}
			return ROP_GT; 
			break;
		case LESS:
      match(mInstance,LESS);
			if(mInstance->token == EQUALS) {
				match(mInstance,EQUALS);
				return ROP_LTE;
				}
			else if(mInstance->token == GREATER) {
				match(mInstance,GREATER);
				return ROP_NEQ;
				}
			return ROP_LT;
			break;
		default:
			setError(mInstance,ERR_SYNTAX);// TOLTO per la storia dello statement da solo... v. b_error

/* non c'è speranza...
  
     {  int i;
    	match(mInstance,mInstance->token);

    i= getToken(mInstance->string);
      
			if(mInstance->token == EOS)
        return EOS;
			else if(mInstance->token == EOL)
        return EOL;
			else if(mInstance->token == COLON)
        return COLON;
      else*/
        return B_ERROR;
//    }
			break;
		}
	}



/*
  parses an expression
*/
NUM_TYPE expr(MINIBASIC *mInstance) {
  NUM_TYPE left,right;
  uint8_t op;

	mInstance->lastExprType=0;
	
  left = term(mInstance);

  while(1) {
    switch(mInstance->token)	{
			case PLUS:
				match(mInstance,PLUS);
				right = term(mInstance);
				left += right;
				break;
			case MINUS:
				match(mInstance,MINUS);
				right = term(mInstance);
				left -= right;
				break;
			default:
				op=relOp(mInstance);			//Find relationship operator  eg = > < >= <=
				if(op != B_ERROR) {		//If operator found OK
//          if(mInstance->errorFlag==ERR_SYNTAX || mInstance->errorFlag==ERR_NOSUCHVARIABLE)    // bah..
    				mInstance->errorFlag=ERR_CLEAR;
					match(mInstance,op);			//Check Operator
					right = term(mInstance);		//Get Value Data
					left= rop(op,left,right);
          // ci sarebbe anche irop per interi ma è difficile, direi...
					}
				else {
          if(mInstance->errorFlag==ERR_SYNTAX || mInstance->errorFlag==ERR_NOSUCHVARIABLE)    // bah..
    				mInstance->errorFlag=ERR_CLEAR;
					return left;
					}
				break;
			}
		}
	}


/*
  parses a term 
*/
NUM_TYPE term(MINIBASIC *mInstance) {
  NUM_TYPE left,right;

  left = factor(mInstance);
  
  while(1) {
    switch(mInstance->token) {
			case MULT:
				match(mInstance,MULT);
				right = factor(mInstance);
				left *= right;
				break;
			case DIV:
				match(mInstance,DIV);
				right = factor(mInstance);
				if(right != 0.0)
					left /= right;
				else
					setError(mInstance,ERR_DIVIDEBYZERO);
				break;
			case MOD:
				match(mInstance,MOD);
				right = factor(mInstance);
				if(right != 0.0)
  				left = fmod(left,right);
//  				left = (NUM_TYPE) (((int)left) % ((int)right));    //fmod schiantava..
				else
					setError(mInstance,ERR_DIVIDEBYZERO);
				break;
			case OR:
				match(mInstance,OR);
				right = factor(mInstance);
				left = (NUM_TYPE) (((unsigned int)left) | ((unsigned int)right));
				break;
			case AND:
				match(mInstance,AND);
				right = factor(mInstance);
				left = (NUM_TYPE) (((unsigned int)left) & ((unsigned int)right));
				break;
			case XOR:
				match(mInstance,XOR);
				right = factor(mInstance);
				left = (NUM_TYPE) (((unsigned int)left) ^ ((unsigned int)right));
				break;
			case BITNOT:
				match(mInstance,BITNOT);
				right = factor(mInstance);
				left = (NUM_TYPE) ~ ((unsigned int)right);
        //PROVARE!!
				break;
			default:
				return left;
			}
	  }

	}


/*
  parses a factor
*/
NUM_TYPE factor(MINIBASIC *mInstance) {
	NUM_TYPE answer=0;
  char *str;
  char *end;		// NO const
	int t,t1;
  IDENT_LEN len;

  switch(mInstance->token) {
    case OPAREN:
			match(mInstance,OPAREN);
			answer = expr(mInstance);
			match(mInstance,CPAREN);
			break;
		case VALUE:
			answer = getValue(mInstance->string, &len);
			match(mInstance,VALUE);
			break;
		case NOT:
			match(mInstance,NOT);
			answer = (NUM_TYPE) (~( integer(mInstance, factor(mInstance))));
			break;
		case MINUS:
			match(mInstance,MINUS);
			answer = -factor(mInstance);
			break;
		case FLTID:
  		mInstance->lastExprType=FLTID;
			answer = variable(mInstance);
			break;
		case INTID:
			if(!mInstance->lastExprType)
				mInstance->lastExprType=INTID;
			answer = ivariable(mInstance);
			break;
		case DIMFLTID:
			answer = dimVariable(mInstance);
			break;
		case DIMINTID:
			answer = dimivariable(mInstance);
			break;
		case ETOK:
			answer = exp(1.0);
			match(mInstance,ETOK);
			break;
		case PITOK:
			answer = PI;
			match(mInstance,PITOK);
			break;
		case AMPERSAND:   // solo &""... provare, e/o fare &H 
			match(mInstance,AMPERSAND);
			str = stringExpr(mInstance);
			if(str) {
  			answer = myhextoi(str);
				free(str);
	  		}
			else
				answer = 0;
			break;
		case SIN:
			match(mInstance,SIN);
			match(mInstance,OPAREN);
			answer = expr(mInstance);
			match(mInstance,CPAREN);
			answer = sin(answer);
			break;
		case COS:
			match(mInstance,COS);
			match(mInstance,OPAREN);
			answer = expr(mInstance);
			match(mInstance,CPAREN);
			answer = cos(answer);
			break;
		case TAN:
			match(mInstance,TAN);
			match(mInstance,OPAREN);
			answer = expr(mInstance);
			match(mInstance,CPAREN);
			answer = tan(answer);
			break;
		case LOG:
			match(mInstance,LOG);
			match(mInstance,OPAREN);
			answer = expr(mInstance);
			match(mInstance,CPAREN);
			if(answer > 0)
				answer = log(answer);
			else
				setError(mInstance,ERR_NEGLOG);
			break;
		case EXP:
			match(mInstance,EXP);
			match(mInstance,OPAREN);
			answer = expr(mInstance);
			match(mInstance,CPAREN);
			answer = exp(answer);
			break;
		case POW:
			match(mInstance,POW);
			match(mInstance,OPAREN);
			answer = expr(mInstance);
			match(mInstance,COMMA);
			answer = pow(answer, expr(mInstance));
			match(mInstance,CPAREN);
			break;
		case SQR:
			match(mInstance,SQR);
			match(mInstance,OPAREN);
			answer = expr(mInstance);
			match(mInstance,CPAREN);
			if(answer >= 0.0)
				answer = sqrt(answer);
			else
				setError(mInstance,ERR_NEGSQRT);
			break;
    case IIF:
		  match(mInstance,IIF);
		  match(mInstance,OPAREN);
			if(boolExpr(mInstance)) {
			  match(mInstance,COMMA);
				answer = expr(mInstance);
			  match(mInstance,COMMA);
				expr(mInstance);
				}
			else {
			  match(mInstance,COMMA);
				expr(mInstance);
			  match(mInstance,COMMA);
				answer = expr(mInstance);
				}
		  match(mInstance,CPAREN);
			break;
		case ABS:
			match(mInstance,ABS);
			match(mInstance,OPAREN);
			answer = expr(mInstance);
			match(mInstance,CPAREN);
			answer = fabs(answer);
			break;
		case MIN:
			{
			NUM_TYPE answer2;
			match(mInstance,MIN);
			match(mInstance,OPAREN);
			answer = expr(mInstance);
			match(mInstance,COMMA);
			answer2 = expr(mInstance);
			match(mInstance,CPAREN);
			answer = answer < answer2 ? answer : answer2;
			}
			break;
		case MAX:
			{
			NUM_TYPE answer2;
			match(mInstance,MAX);
			match(mInstance,OPAREN);
			answer = expr(mInstance);
			match(mInstance,COMMA);
			answer2 = expr(mInstance);
			match(mInstance,CPAREN);
			answer = answer > answer2 ? answer : answer2;
			}
			break;

		case PEEK:
			match(mInstance,PEEK);
			match(mInstance,OPAREN);
      t = integer(mInstance, expr(mInstance));
			answer = (NUM_TYPE) (*((unsigned char *)PA_TO_KVA0(t)));
			match(mInstance,CPAREN);
			break;
    case USR:
		  match(mInstance,USR);
		  match(mInstance,OPAREN);
		  t = integer(mInstance, expr(mInstance));
      match(mInstance,CPAREN);
		  if(t>=0x1d000000) {     // boh... diciamo v. doSys
        void (*funcEntryPoint)();
      // se RAM... e se ROM? serve ASM?
        funcEntryPoint = (void *)t;
        (*funcEntryPoint)();
        }
      else {
        ramfuncEntryPoint = (void *)t;
        (*ramfuncEntryPoint)();
        }
			{	// asm ...
//  _jr_hb();
//  __asm__("j": "=c" (t));   // VERIFICARE!
			}
			break;

		case TIMER:
			match(mInstance,TIMER);
			match(mInstance,OPAREN);
		  t = integer(mInstance, expr(mInstance));
			switch(t) {
				case 1:
					answer = (NUM_TYPE)TMR1;
					break;
				case 2:
					answer = (NUM_TYPE)TMR2;    // occhio usb!
					break;
				case 3:
					answer = (NUM_TYPE)TMR3;
					break;
				case 4:
					answer = (NUM_TYPE)TMR4;
					break;
				case 5:
					answer = (NUM_TYPE)TMR5;
					break;
				case 6:
					answer = (NUM_TYPE)TMR6;       // OCCHIO se usato con ON TIMER...
					break;
				case 7:
					answer = (NUM_TYPE)TMR7;
					break;
				case 8:
					answer = (NUM_TYPE)TMR8;
					break;
				case 9:
					answer = (NUM_TYPE)TMR9;
					break;
				default:
					setError(mInstance,ERR_BADVALUE);
					break;
				}
			match(mInstance,CPAREN);
			break;
      
		case JOYSTICK:
			match(mInstance,JOYSTICK);
			match(mInstance,OPAREN);
		  t = integer(mInstance, expr(mInstance));
      answer = ReadJoystick(t);
			match(mInstance,CPAREN);
			break;
      
		case ERRTOK:
			match(mInstance,ERRTOK);
			match(mInstance,OPAREN);
		  t = integer(mInstance, expr(mInstance));
			match(mInstance,CPAREN);
			answer = t ? mInstance->errorHandler.line.line : mInstance->errorHandler.errorcode;
			break;
		case STATUSTOK:
			{
			signed char c;
			match(mInstance,STATUSTOK);
			match(mInstance,OPAREN);
		  t = integer(mInstance, expr(mInstance));
			match(mInstance,CPAREN);
			switch(t) {
				case 0:		// Reset
//					answer = RCON;
					answer = RCON;    // bah :)
          RCON=0;
					break;
				case 1:		// Wakeup
					answer = (RCONbits.SLEEP ? 1 : 0) | (RCONbits.WDTO ? 2 : 0) | ((powerState & 1) ? 4 : 0);
					break;
				case 2:
					answer = t;   //:)
					break;
				case 3:
//					c=U1RXREG;
					c=HIBYTE(LOWORD(GetStatus(SOUTH_BRIDGE,NULL))); // meglio, qua :)
					answer = c;
					break;
				case 4:
					c=U1RXREG;
					answer = c;
					break;
				case 5:
					c=LOBYTE(HIWORD(GetStatus(SOUTH_BRIDGE,NULL))); // parallela
					answer = c;
					break;
				case 6:
#ifdef USA_WIFI
          m2m_periph_gpio_get_val(M2M_PERIPH_GPIO18,&c);    // leggo il led!
          c=!c;   // [beh...] v. merda non va
#else
          c=-1;
#endif
					answer = c;
					break;
				case 7:
#ifdef USA_ETHERNET
					c=MACIsLinked();
#else
          c=-1;
#endif
					answer = c;
					break;
				case 8:
					c=GetStatus(SOUTH_BRIDGE,NULL);
					answer = c;
					break;
          
				case 15:
					c=BiosArea.bootErrors;    // farne anche una per tutti i biosarea?
					answer = c;
					break;
          
        case 16:
          {
          SUPERFILE f;
          f.drive=currDrive;    // per ora così...
          answer=SuperFileError(&f);
          }
					break;
          
        default:
          answer=-1;
					break;
				}
			}
			break;
		case LEN:
			match(mInstance,LEN);
			match(mInstance,OPAREN);
			str = stringExpr(mInstance);
			match(mInstance,CPAREN);
			if(str) {
				answer = strlen(str);
				free(str);
	  		}
			else
				answer = 0;
			break;
    case ASC:
			match(mInstance,ASC);
			match(mInstance,OPAREN);
			str = stringExpr(mInstance);
			match(mInstance,CPAREN);
			if(str) {
				answer = *str;
				free(str);
				}
			else
				answer = 0;
			break;
    case ASIN:
			match(mInstance,ASIN);
			match(mInstance,OPAREN);
			answer = expr(mInstance);
			match(mInstance,CPAREN);
			if(answer >= -1 && answer <= 1)
				answer = asin(answer);
			else
				setError(mInstance,ERR_BADSINCOS);
			break;
    case ACOS:
			match(mInstance,ACOS);
			match(mInstance,OPAREN);
			answer = expr(mInstance);
			match(mInstance,CPAREN);
			if(answer >= -1 && answer <= 1)
				answer = acos(answer);
			else
				setError(mInstance,ERR_BADSINCOS);
			break;
    case ATAN:
			match(mInstance,ATAN);
			match(mInstance,OPAREN);
			answer = expr(mInstance);
			match(mInstance,CPAREN);
			answer = atan(answer);
			break;
    case INTTOK:
			match(mInstance,INTTOK);
			match(mInstance,OPAREN);
			answer = expr(mInstance);
			match(mInstance,CPAREN);
			answer = floor(answer);
			break;
    case RND:
			match(mInstance,RND);
			match(mInstance,OPAREN);
		  t = integer(mInstance, expr(mInstance));
			match(mInstance,CPAREN);
#warning			o usare crypto , GenerateRandomDWORD() da helpers.c
			if(t > 1) {
        unsigned long long l=t;
				l *= rand();
				answer = l/RAND_MAX;
				}
			else if(t == 1)
				answer = rand()/(RAND_MAX + 1.0);
			else {
				if(answer < 0)
		  		srand( (unsigned) -answer);
				answer = 0;
	  		}
			break;
    case VAL:
		  match(mInstance,VAL);
		  match(mInstance,OPAREN);
		  str = stringExpr(mInstance);
		  match(mInstance,CPAREN);
	  	if(str) {
	    	answer = strtod(str , NULL);
				free(str);
	  		}
	  	else
				answer = 0;
			break;
		case VALLEN:
			match(mInstance,VALLEN);
			match(mInstance,OPAREN);
			str = stringExpr(mInstance);
			match(mInstance,CPAREN);
			if(str) {
				strtod(str, &end);		// works all the same even if end is ROM - only len is needed!
				answer = end - str;
				free(str);
				}
			else
				answer = 0.0;
			break;
    case MEM:
		  match(mInstance,MEM);
		  match(mInstance,OPAREN);
		  t = integer(mInstance, expr(mInstance));
		  match(mInstance,CPAREN);
	  	if(t<0) {
	    	// fare garbage collection?
        t=-t;
	  		}
  
      switch(t) {
        case 0:
          return getTotRAM();
          // _dump_heap_info per info su memoria dinamica...
          //heap_free() in heap_info.c, provare
//          return heap_free();
          break;
        case 1:
          return extRAMtot;
          break;
        case 2:
    // v. bitlash...            return ((char *)&vstack[vsptr])-stringPool;    // spazio dal fondo di stack (che scende da top) e stringpool (che sale da sotto)
//          break;
        case 3:
    // eeprom..      return E2END*2;
          break;
        case 4:
          goto fai_diskspace;
          break;
        default:
  		    setError(mInstance,ERR_BADVALUE);
          break;
        }
			break;
		case INSTR:
	  	answer = instr(mInstance);
	  	break;
		case PENX:
			match(mInstance,PENX);
//			match(mInstance,OPAREN);
//			match(mInstance,CPAREN);
//    	outd(1,0,1);
//#warning FINIRE TOUCH RESISTIVO che però passa a windows :)
//      __delay_ms(50);
//			answer = inadc(0);
      {
      POINT pt;
#ifdef USA_BREAKTHROUGH
      GetCursorPos(&pt);
#else
#endif
      answer=pt.x;
      }
		  break;
		case PENY:
			match(mInstance,PENY);
//			match(mInstance,OPAREN);
//			match(mInstance,CPAREN);
//    	outd(1,1,1);
//      __delay_ms(50);
//			answer = inadc(1);
      {
      POINT pt;
#ifdef USA_BREAKTHROUGH
      GetCursorPos(&pt);
#else
#endif
      answer=pt.y;
      }
		  break;
		case CURSORX:
			match(mInstance,CURSORX);
#ifdef USA_BREAKTHROUGH
//      answer=mInstance->hWnd->caret   fare;
#else
      answer=mInstance->Cursor.x;
#endif
		  break;
		case CURSORY:
			match(mInstance,CURSORY);
#ifdef USA_BREAKTHROUGH
//      answer=mInstance->hWnd->caret   fare;
#else
      answer=mInstance->Cursor.y;
#endif
		  break;
		case SCREENX:
			match(mInstance,SCREENX);
#ifdef USA_BREAKTHROUGH
      answer=mInstance->hWnd->clientArea.right-mInstance->hWnd->clientArea.left;
#else
      answer=Screen.cx;
#endif
		  break;
		case SCREENY:
			match(mInstance,SCREENY);
#ifdef USA_BREAKTHROUGH
      answer=mInstance->hWnd->clientArea.bottom-mInstance->hWnd->clientArea.top;
#else
      answer=Screen.cy;
#endif
		  break;
      
    case GETPIXEL:
    {
      POINT pt;
      DWORD oldCursorPos;
			match(mInstance,GETPIXEL);
		  match(mInstance,OPAREN);
		  pt.x = integer(mInstance, expr(mInstance));
		  match(mInstance,COMMA);
		  pt.y = integer(mInstance, expr(mInstance));
		  match(mInstance,CPAREN);
#ifdef USA_BREAKTHROUGH
//      answer= FARE ! mInstance->hWnd->clientArea.right-mInstance->hWnd->clientArea.left;
#else
      oldCursorPos=GetXY();
      SetXY(0,pt.x,pt.y);
      answer=GetPixelColor();
      SetXY(0,LOWORD(oldCursorPos),HIWORD(oldCursorPos));
#endif
    }
		  break;
    case GETCOLOR:
			match(mInstance,GETCOLOR);
		  match(mInstance,OPAREN);
		  t = integer(mInstance, expr(mInstance));
		  match(mInstance,CPAREN);
      if(t>=0 && t<16) {
        answer=textColors[t];
        }
      else if(t==-1) {
#ifdef USA_BREAKTHROUGH
        HDC myDC,*hDC=GetDC(mInstance->hWnd,&myDC);
        answer=hDC->foreColor;
        ReleaseDC(mInstance->hWnd,hDC);
#else
        answer=mInstance->ColorPalette;
#endif
        }
      else if(t==-2) {
#ifdef USA_BREAKTHROUGH
        HDC myDC,*hDC=GetDC(mInstance->hWnd,&myDC);
        answer=hDC->backColor;
        ReleaseDC(mInstance->hWnd,hDC);
#else
        answer=mInstance->ColorPaletteBK;
#endif
        }
      else {
				setError(mInstance,ERR_BADVALUE);
        }
		  break;
    case GETPEN:
			match(mInstance,GETPEN);
#ifdef USA_BREAKTHROUGH
      HDC myDC,*hDC=GetDC(mInstance->hWnd,&myDC);
      answer=hDC->pen.size;
      ReleaseDC(mInstance->hWnd,hDC);
#else
      answer=fontSize;
#endif
		  break;
      
		case DISKSPACE:
			match(mInstance,DISKSPACE);
			{
        FS_DISK_PROPERTIES disk_properties;
        char mydrive;
        
  			match(mInstance,OPAREN);
        str = stringExpr(mInstance);    // obbligatorio qua
  			match(mInstance,CPAREN);
        
        if(str)
          mydrive=getDrive(str,NULL,NULL);
        else
          mydrive=currDrive;

fai_diskspace:
        switch(mydrive) {
          case 'A':
            disk_properties.new_request=1;
            do {
              FSGetDiskProperties(&disk_properties);
              ClrWdt();
              } while(disk_properties.properties_status == FS_GET_PROPERTIES_STILL_WORKING);
            if(disk_properties.properties_status == FS_GET_PROPERTIES_NO_ERRORS)
              answer=disk_properties.results.free_clusters*disk_properties.results.sectors_per_cluster*disk_properties.results.sector_size/1024; 
            else
              answer=0;
            break;
          case 'C':
            disk_properties.new_request=1;
            do {
              IDEFSGetDiskProperties(&disk_properties);
              ClrWdt();
              } while(disk_properties.properties_status == FS_GET_PROPERTIES_STILL_WORKING);
            if(disk_properties.properties_status == FS_GET_PROPERTIES_NO_ERRORS)
              answer=disk_properties.results.free_clusters*disk_properties.results.sectors_per_cluster*disk_properties.results.sector_size/1024; 
            else
              answer=0;
            break;
#if defined(USA_USB_HOST_MSD)
          case 'E':
          {
            uint32_t totalSectors,freeSectors,sectorSize;
            if(USBmemOK) {
              if(SYS_FS_DriveSectorGet(NULL, &totalSectors, &freeSectors, &sectorSize) == SYS_FS_RES_SUCCESS) 
                answer=freeSectors* MEDIA_SECTOR_SIZE /* boh dov'è??*//1024; 
              }
            else {
              answer=0;
              }
          }
            break;
#endif
#ifdef USA_RAM_DISK 
          case DEVICE_RAMDISK:
            disk_properties.new_request=1;
            do {
              RAMFSGetDiskProperties(&disk_properties);
              ClrWdt();
              } while(disk_properties.properties_status == FS_GET_PROPERTIES_STILL_WORKING);
            if(disk_properties.properties_status == FS_GET_PROPERTIES_NO_ERRORS)
              answer=disk_properties.results.free_clusters*disk_properties.results.sectors_per_cluster*disk_properties.results.sector_size/1024; 
            else
              answer=0;
            break;
#endif
          }
			}
		  break;
		case EEPEEK:
			match(mInstance,EEPEEK);
			match(mInstance,OPAREN);
			t = integer(mInstance, expr(mInstance));
			match(mInstance,CPAREN);
      return EEread(t);
      break;
		case IND:
			match(mInstance,IND);
			match(mInstance,OPAREN);
			t = integer(mInstance, expr(mInstance));
			match(mInstance,COMMA);
			t1 = integer(mInstance, expr(mInstance));
			match(mInstance,CPAREN);
			answer = ind(t,t1);
			break;
		case INADC:
			match(mInstance,INADC);
			match(mInstance,OPAREN);
			t = integer(mInstance, expr(mInstance));
			match(mInstance,CPAREN);
			answer = inadc(t);
			break;
		case TEMPERATURE:
			match(mInstance,TEMPERATURE);
			answer = ReadTemperature();
      break;
		case RSSI:
			match(mInstance,RSSI);
#ifdef USA_WIFI
      m2m_wifi_req_curr_rssi();
      answer=myRSSI;
#endif
      break;
		case BIOS:
    {
      int t2;
      BYTE buf[16];
      
			match(mInstance,BIOS);
			match(mInstance,OPAREN);
			t = integer(mInstance, expr(mInstance));
			match(mInstance,COMMA);
			t1 = integer(mInstance, expr(mInstance));
			match(mInstance,COMMA);
			t2 = integer(mInstance, expr(mInstance));
			match(mInstance,CPAREN);
      if(t>0 && t<=SOUTH_BRIDGE && /*t1 controllare? && */ t2<15) {
        ReadPMPs(t,t1,buf,t2+1);
        // occhio: con BURST mode BISOGNA leggere tutto quanto previsto, o letture in cascata si incasinano..
        // ev. mettere ritardo
        __delay_ms(1);
        answer = buf[t2];
        }
      else {
    		setError(mInstance, ERR_BADVALUE);
        }
    }
      break;
		default:
		  if(isString(mInstance->token))
				setError(mInstance,ERR_TYPEMISMATCH);
		  else
		    setError(mInstance,ERR_SYNTAX);
	  	break;
  	}

  while(mInstance->token == SHRIEK) {
    match(mInstance,SHRIEK);
		answer = factorial(answer);
  	}

  return answer;
	}



/*
  calculate the INSTR() function.
*/
NUM_TYPE instr(MINIBASIC *mInstance) {
  char *str;
  char *substr;
  char *end;
  int answer = 0;
  int offset;

  match(mInstance,INSTR);
  match(mInstance,OPAREN);
  str = stringExpr(mInstance);
  match(mInstance,COMMA);
  substr = stringExpr(mInstance);
  match(mInstance,COMMA);
  offset = integer(mInstance,expr(mInstance));
  offset--;
  match(mInstance,CPAREN);

  if(!str || !substr) {
    if(str)
			free(str);
		if(substr)
		  free(substr);
		return 0;
	  }

  if(offset >= 0 && offset < (int) strlen(str)) {
    end = strstr(str + offset, substr);
    if(end)
			answer = end - str + 1;
		}

  free(str);
  free(substr);

  return (NUM_TYPE)answer;
	}



/*
  get the value of a scalar variable from string
  matches FLTID
*/
NUM_TYPE variable(MINIBASIC *mInstance) {
  VARIABLE *var;
  char id[IDLENGTH];
  IDENT_LEN len;

  getId(mInstance,mInstance->string, id, &len);
  match(mInstance,FLTID);
  var = findVariable(mInstance,id);
  if(var)
    return var->d.dval;
  else {
		setError(mInstance,ERR_NOSUCHVARIABLE);
		return 0.0;
	  }
	}


/*
  get the value of a scalar variable from string
  matches INTID
*/
int16_t ivariable(MINIBASIC *mInstance) {
  VARIABLE *var;
  char id[IDLENGTH];
  IDENT_LEN len;

  getId(mInstance,mInstance->string, id, &len);
  match(mInstance,INTID);
  var = findVariable(mInstance,id);
  if(var)
    return var->d.ival;
  else {
		setError(mInstance,ERR_NOSUCHVARIABLE);
		return 0;
	  }
	}


/*
  get value of a dimensioned variable from string.
  matches DIMFLTID
*/
NUM_TYPE dimVariable(MINIBASIC *mInstance) {
  DIMVAR *dimvar;
  char id[IDLENGTH];
  IDENT_LEN len;
  DIM_SIZE index[MAXDIMS];
  NUM_TYPE *answer;

  getId(mInstance,mInstance->string, id, &len);
  match(mInstance,DIMFLTID);
  dimvar = findDimVar(mInstance,id);
  if(!dimvar) {
    setError(mInstance,ERR_NOSUCHVARIABLE);
		return 0.0;
  	}

  if(dimvar) {
    switch(dimvar->ndims) {
		  case 1:
		    index[0] = integer(mInstance,expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0]);
				break;
      case 2:
				index[0] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[1] = integer(mInstance,expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0], index[1]);
				break;
		  case 3:
				index[0] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[1] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[2] = integer(mInstance,expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0], index[1], index[2]);
				break;
		  case 4:
				index[0] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[1] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[2] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[3] = integer(mInstance,expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0], index[1], index[2], index[3]);
				break;
		  case 5:
				index[0] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[1] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[2] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[3] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[4] = integer(mInstance,expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0], index[1], index[2], index[3], index[4]);
				break;
			}

		match(mInstance,CPAREN);
  	}

  if(answer)
		return *answer;

  return 0.0;

	}

int16_t dimivariable(MINIBASIC *mInstance) {
  DIMVAR *dimvar;
  char id[IDLENGTH];
  IDENT_LEN len;
  DIM_SIZE index[MAXDIMS];
  NUM_TYPE *answer;

  //VERIFICARE! 2022
  getId(mInstance,mInstance->string, id, &len);
  match(mInstance,DIMINTID);
  dimvar = findDimVar(mInstance,id);
  if(!dimvar) {
    setError(mInstance,ERR_NOSUCHVARIABLE);
		return 0;
  	}

  if(dimvar) {
    switch(dimvar->ndims) {
		  case 1:
		    index[0] = integer(mInstance,expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0]);
				break;
      case 2:
				index[0] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[1] = integer(mInstance,expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0], index[1]);
				break;
		  case 3:
				index[0] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[1] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[2] = integer(mInstance,expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0], index[1], index[2]);
				break;
		  case 4:
				index[0] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[1] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[2] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[3] = integer(mInstance,expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0], index[1], index[2], index[3]);
				break;
		  case 5:
				index[0] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[1] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[2] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[3] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[4] = integer(mInstance,expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0], index[1], index[2], index[3], index[4]);
				break;
			}

		match(mInstance,CPAREN);
  	}

  if(answer)
		return *((int16_t *)answer);

  return 0;
	}


/*
  find a scalar variable in variables list
  Params: id - id to get
  Returns: pointer to that entry, 0 on fail
*/
VARIABLE *findVariable(MINIBASIC *mInstance,const char *id) {
  int i;

  for(i=0; i<mInstance->nvariables; i++) {
		if(!stricmp(mInstance->variables[i].id, id))    // case insensitive GD 2022
			return &mInstance->variables[i];
    }
  return NULL;
	}


/*
  get a dimensioned array by name
  Params: id (includes opening parenthesis)
  Returns: pointer to array entry or 0 on fail
*/
DIMVAR *findDimVar(MINIBASIC *mInstance,const char *id) {
  int i;

  for(i=0; i<mInstance->ndimVariables; i++)
		if(!stricmp(mInstance->dimVariables[i].id, id)) // case insensitive GD 2022
			return &mInstance->dimVariables[i];
  return 0;
	}


/*
  dimension an array.
  Params: id - the id of the array (include leading ()
          ndims - number of dimension (1-5)
		  ... - integers giving dimension size, 
*/
DIMVAR *dimension(MINIBASIC *mInstance,const char *id, BYTE ndims, ...) {
  DIMVAR *dv;
  va_list vargs;
  int size = 1;
  int oldsize = 1;
  unsigned char i;
  DIM_SIZE dimensions[MAXDIMS];
  NUM_TYPE *dtemp;
  char **stemp;
  int16_t *itemp;

  basicAssert(ndims <= MAXDIMS);
  if(ndims > MAXDIMS)
		return NULL;

  dv = findDimVar(mInstance,id);
  if(!dv)
		dv = adddimvar(mInstance,id);
  if(!dv) {
    setError(mInstance,ERR_OUTOFMEMORY);
		return NULL;
  	}

  if(dv->ndims) {
    for(i=0; i<dv->ndims; i++)
			oldsize *= dv->dim[i];
  	}
  else
		oldsize = 0;

  va_start(vargs, ndims);
  for(i=0; i<ndims; i++) {
		dimensions[i] = va_arg(vargs, int) +1;		// da 0 a n compresi, stile-BASIC non C; GD 2022
    size *= dimensions[i];
  	}
  va_end(vargs);

  switch(dv->type) {
    case FLTID:
      dtemp = (double *)realloc(dv->d.dval, size * sizeof(double));
      if(dtemp)
        dv->d.dval = dtemp;
			else {
				setError(mInstance,ERR_OUTOFMEMORY);
				return NULL;
	  		}
			break;
		case STRID:
			if(dv->d.str) {
				for(i=size; i<oldsize; i++)
					if(dv->d.str[i]) {
						free(dv->d.str[i]);
						dv->d.str[i] = NULL;
			  		}
		  	}
			stemp = (char **)realloc(dv->d.str, size * sizeof(char *));
			if(stemp) {
				dv->d.str = stemp;
				for(i=oldsize; i<size; i++)
		  		dv->d.str[i] = NULL;
	  		}
			else {
				for(i=0; i<oldsize; i++)
		  		if(dv->d.str[i]) {
						free(dv->d.str[i]);
		    		dv->d.str[i] = NULL;
		  			}
				setError(mInstance,ERR_OUTOFMEMORY);
				return NULL;
	  		}
			break;
    case INTID:
      itemp = (int16_t *)realloc(dv->d.ival, size * sizeof(int16_t));
      if(itemp)
        dv->d.ival = itemp;
			else {
				setError(mInstance,ERR_OUTOFMEMORY);
				return NULL;
	  		}
			break;
		default:
			basicAssert(0);
			break;
		}

  for(i=0; i<MAXDIMS; i++)
		dv->dim[i] = dimensions[i];
  dv->ndims = ndims;

  return dv;
	}


/*
  get the address of a dimensioned array element.
  works for both string and real arrays.
  Params: dv - the array's entry in variable list
          ... - integers telling which array element to get
  Returns: the address of that element, 0 on fail
*/ 
void *getDimVar(MINIBASIC *mInstance, DIMVAR *dv, ...) {
  va_list vargs;
  DIM_SIZE index[MAXDIMS];
  int i;
  void *answer = NULL;

  va_start(vargs, dv);
  for(i=0; i<dv->ndims; i++) {
		index[i] = va_arg(vargs, int);
		}
  va_end(vargs);

  for(i=0; i<dv->ndims; i++) {
    if(index[i] > dv->dim[i] || index[i] < 0) {
			setError(mInstance,ERR_BADSUBSCRIPT);
			return NULL;
  		}
		}
//  for(i=0; i<dv->ndims; i++)  OPTION BASE diciamo, lui partiva da 1, io NO!
//    index[i]--;

  switch(dv->type) {
    case FLTID:
      switch(dv->ndims) {
        case 1:
          answer = &dv->d.dval[ index[0] ]; 
          break;
        case 2:
          answer = &dv->d.dval[ index[1] * dv->dim[0] 
            + index[0] ];
          break;
        case 3:
          answer = &dv->d.dval[ index[2] * (dv->dim[0] * dv->dim[1]) 
          + index[1] * dv->dim[0] 
          + index[0] ];
          break;
        case 4:
          answer = &dv->d.dval[ index[3] * (dv->dim[0] + dv->dim[1] + dv->dim[2]) 
            + index[2] * (dv->dim[0] * dv->dim[1]) 
            + index[1] * dv->dim[0] 
            + index[0] ];
          // MANCAva BREAK???
          break;
        case 5:
          answer = &dv->d.dval[ index[4] * (dv->dim[0] + dv->dim[1] + dv->dim[2] + dv->dim[3])
            + index[3] * (dv->dim[0] + dv->dim[1] + dv->dim[2])
            + index[2] * (dv->dim[0] + dv->dim[1])
            + index[1] * dv->dim[0]
            + index[0] ];
          break;
        }
      break;
    case STRID:
      switch(dv->ndims)	{
        case 1:
          answer = &dv->d.str[ index[0] ]; 
          break;
        case 2:
          answer = &dv->d.str[ index[1] * dv->dim[0] 
            + index[0] ];
          break;
        case 3:
          answer = &dv->d.str[ index[2] * (dv->dim[0] * dv->dim[1]) 
          + index[1] * dv->dim[0] 
          + index[0] ];
          break;
        case 4:
          answer = &dv->d.str[ index[3] * (dv->dim[0] + dv->dim[1] + dv->dim[2]) 
            + index[2] * (dv->dim[0] * dv->dim[1]) 
            + index[1] * dv->dim[0] 
            + index[0] ];
          // MANCAva BREAK???
          break;
        case 5:
          answer = &dv->d.str[ index[4] * (dv->dim[0] + dv->dim[1] + dv->dim[2] + dv->dim[3])
            + index[3] * (dv->dim[0] + dv->dim[1] + dv->dim[2])
            + index[2] * (dv->dim[0] + dv->dim[1])
            + index[1] * dv->dim[0]
            + index[0] ];
          break;
        }
      break;
    case INTID:
      switch(dv->ndims) {
        case 1:
          answer = &dv->d.ival[ index[0] ]; 
          break;
        case 2:
          answer = &dv->d.ival[ index[1] * dv->dim[0] 
            + index[0] ];
          break;
        case 3:
          answer = &dv->d.ival[ index[2] * (dv->dim[0] * dv->dim[1]) 
          + index[1] * dv->dim[0] 
          + index[0] ];
          break;
        case 4:
          answer = &dv->d.ival[ index[3] * (dv->dim[0] + dv->dim[1] + dv->dim[2]) 
            + index[2] * (dv->dim[0] * dv->dim[1]) 
            + index[1] * dv->dim[0] 
            + index[0] ];
          // MANCAva BREAK???
          break;
        case 5:
          answer = &dv->d.ival[ index[4] * (dv->dim[0] + dv->dim[1] + dv->dim[2] + dv->dim[3])
            + index[3] * (dv->dim[0] + dv->dim[1] + dv->dim[2])
            + index[2] * (dv->dim[0] + dv->dim[1])
            + index[1] * dv->dim[0]
            + index[0] ];
          break;
        }
      break;
		}

  return answer;
	}


/*
  ------------------
*/
VARIABLE *addNewVar(MINIBASIC *mInstance,const char *id, char type) {
  VARIABLE *vars;

  vars = (VARIABLE *)realloc(mInstance->variables, (mInstance->nvariables + 1) * sizeof(VARIABLE));
  if(vars) {
		mInstance->variables = vars;
    strncpy(mInstance->variables[mInstance->nvariables].id, id, IDLENGTH-1);
		mInstance->variables[mInstance->nvariables].d.dval = 0;		// clears all
		mInstance->variables[mInstance->nvariables].type = type;
		mInstance->nvariables++;
		return &mInstance->variables[mInstance->nvariables-1];
	  }
  else {
		setError(mInstance,ERR_OUTOFMEMORY);
		return NULL;
		}
	}

/*
  add a real variable to our variable list
  Params: id - id of variable to add.
  Returns: pointer to new entry in table
*/
VARIABLE *addFloat(MINIBASIC *mInstance,const char *id) {
  VARIABLE *v = addNewVar(mInstance,id,FLTID);

  return v; 
	}

/*
  add a string variable to table.
  Params: id - id of variable to get (including trailing $)
  Retruns: pointer to new entry in table, 0 on fail.       
*/
VARIABLE *addString(MINIBASIC *mInstance,const char *id) {
  VARIABLE *v = addNewVar(mInstance,id,STRID);

  return v; 
	}

/*
  add a integer variable to our variable list
  Params: id - id of variable to add.
  Returns: pointer to new entry in table
*/
VARIABLE *addInt(MINIBASIC *mInstance,const char *id) {
  VARIABLE *v = addNewVar(mInstance,id,INTID);

  return v;
	}



/*
  add a new array to our symbol table.
  Params: id - id of array (include leading ()
  Returns: pointer to new entry, 0 on fail.
*/
DIMVAR *adddimvar(MINIBASIC *mInstance,const char *id) {
  DIMVAR *vars;

  vars = (DIMVAR *)realloc(mInstance->dimVariables, (mInstance->ndimVariables + 1) * sizeof(DIMVAR));
  if(vars) {
    mInstance->dimVariables = vars;
		strcpy(mInstance->dimVariables[mInstance->ndimVariables].id, id);
		mInstance->dimVariables[mInstance->ndimVariables].d.dval = 0;
		mInstance->dimVariables[mInstance->ndimVariables].ndims = 0;
		mInstance->dimVariables[mInstance->ndimVariables].type = strchr(id, '$') ? STRID : (strchr(id, '%') ? INTID : FLTID);
		mInstance->ndimVariables++;
		return &mInstance->dimVariables[mInstance->ndimVariables-1];
	  }
  else
		setError(mInstance,ERR_OUTOFMEMORY);
 
  return NULL;
	}


/*
  high level string parsing function.
  Returns: a malloc'ed pointer, or 0 on error condition.
  caller must free!
*/
char *stringExpr(MINIBASIC *mInstance) {
  char *left;
  char *right;
  char *temp;

  switch(mInstance->token) {
    case DIMSTRID:
			left = strdup(stringDimVar(mInstance));
			break;
    case STRID:
      left = strdup(stringVar(mInstance));
		  break;
		case QUOTE:
			left = stringLiteral(mInstance);
			break;
		case CHRSTRING:
			left = chrString(mInstance);
			break;
		case STRSTRING:
			left = strString(mInstance);
			break;
		case LEFTSTRING:
			left = leftString(mInstance);
			break;
		case RIGHTSTRING:
			left = rightString(mInstance);
			break;
		case MIDSTRING:
			left = midString(mInstance);
			break;
		case INKEYSTRING:
			left = inkeyString(mInstance);
			break;
		case GETKEYSTRING:
			left = getkeyString(mInstance);
			break;
		case MIDISTRING:
			left = midiString(mInstance);
			break;
		case MODEMSTRING:
			left = modemString(mInstance);
			break;
#ifdef USA_BREAKTHROUGH
		case MENUKEYSTRING:
			left = menukeyString(mInstance);
			break;
#endif
		case ERRORSTRING:
			left = errorString(mInstance);
			break;
    case VERSTRING:
			left = verString(mInstance);
	  	break;
		case STRINGSTRING:
			left = stringString(mInstance);
			break;
		case HEXSTRING:
			left = hexString(mInstance);
			break;
		case TRIMSTRING:
			left = trimString(mInstance);
			break;
		case TIMESTRING:
			left = timeString(mInstance);
			break;
    case DIRSTRING:   // v. anche doDir
      left = dirString(mInstance);
      break;
    case CURDIRSTRING: 
      left = curdirString(mInstance);
      break;
		case CURDRIVESTRING:
      left = curdriveString(mInstance);
		  break;
    case IPADDRESSSTRING:
      left = ipaddressString(mInstance);
      break;
    case ACCESSPOINTSSTRING: 
      left = accesspointsString(mInstance);
      break;
		default:
			if(!isString(mInstance->token))
				setError(mInstance,ERR_TYPEMISMATCH);
			else
				setError(mInstance,ERR_SYNTAX);
			return strdup(EmptyString);
			break;
	  }

  if(!left) {
    setError(mInstance,ERR_OUTOFMEMORY);
    return 0;
		}

  switch(mInstance->token) {
    case PLUS:
			match(mInstance,PLUS);
			right = stringExpr(mInstance);
			if(right) {
				temp = myStrConcat(left, right);
				free(right);
				if(temp) {
		  		free(left);
					left = temp;
					}
				else
		  		setError(mInstance,ERR_OUTOFMEMORY);
	  		}
			else
				setError(mInstance,ERR_OUTOFMEMORY);
			break;
		default:
			return left;
			break;
	  }

  return left;
	}



/*
  parse the CHR$ token
*/
char *chrString(MINIBASIC *mInstance) {
  char x;
  char buff[2];
  char *answer;

  match(mInstance,CHRSTRING);
  match(mInstance,OPAREN);
  x = integer(mInstance,expr(mInstance));
  match(mInstance,CPAREN);

  buff[0] = (char) x;
  buff[1] = 0;
  answer = strdup(buff);

  if(!answer)
		setError(mInstance,ERR_OUTOFMEMORY);

  return answer;
	}


/*
  parse the HEX$ token
*/
char *hexString(MINIBASIC *mInstance) {
  int x;
  char *answer;

  match(mInstance,HEXSTRING);
  match(mInstance,OPAREN);
  x = expr(mInstance);
  match(mInstance,CPAREN);

  answer = strdup(myitohex((int)x));
  if(!answer)
		setError(mInstance,ERR_OUTOFMEMORY);

  return answer;
	}


/*
  parse the token
*/
char *trimString(MINIBASIC *mInstance) {
  char *str,*str2,*str3;
  char *answer;

  match(mInstance,TRIMSTRING);
  match(mInstance,OPAREN);
  str = stringExpr(mInstance);
  if(!str)
		return 0;
  match(mInstance,CPAREN);

	str3=str;
  while(isspace(*str))
		str++;
	str2=str;
  while(*str)
		str++;
	str--;
  while(isspace(*str))
		str--;
  *(str+1) = 0;
  answer = strdup(str2);
  free(str3);
  if(!answer)
		setError(mInstance,ERR_OUTOFMEMORY);
  return answer;
	}

char *timeString(MINIBASIC *mInstance) {
  DWORD ti;
  char *answer,buf[32];
  PIC32_DATE date;
  PIC32_TIME time;
  signed char i;

  match(mInstance,TIMESTRING);
  if(mInstance->token == OPAREN) {
    match(mInstance,OPAREN);
    ti = expr(mInstance);
    match(mInstance,CPAREN);
    }
  else {
    ti=now;
    }

  SetTimeFromNow(ti,&date,&time);
  i=findTimezone();
  sprintf(buf,"%02u/%02u/%04u %02u:%02u:%02u %c%d",
          date.mday,date.mon,date.year,
          time.hour,time.min,time.sec,
          i>=0 ? '+' : '-',abs(i));
          
  answer = strdup(buf);
  if(!answer)
		setError(mInstance,ERR_OUTOFMEMORY);
  return answer;
	}


/*
  handle INKEY$ token, based on inkey() (numeric) QUESTA NON ASPETTA
*/
char *inkeyString(MINIBASIC *mInstance) {
  char buf[2];
	int i;
  char *answer;

  // verificare con BREAKTHROUGH...
  match(mInstance,INKEYSTRING);
//  match(mInstance,OPAREN);
//  match(mInstance,CPAREN);

	i=mInstance->incomingChar[0];
	if(i>0) {
		buf[0]=i;
		buf[1]=0;
		}
	else
		buf[0]=0;
  answer = strdup(buf);
  if(!answer)
		setError(mInstance,ERR_OUTOFMEMORY);

  return answer;
	}

/*
  handle GETKEY$ token, based on inkey() (numeric) QUESTA ASPETTA!
*/
char *getkeyString(MINIBASIC *mInstance) {
  char buf[2];
	int i;
  char *answer;

  match(mInstance,GETKEYSTRING);
//  match(mInstance,OPAREN);
//  match(mInstance,CPAREN);

  // verificare con BREAKTHROUGH...
  buf[0]=0;
	for(;;) {
    i=mInstance->incomingChar[0];
    if(i>0) {
      buf[0]=i;
      buf[1]=0;
      break;
      }
    handle_events();
#if defined(USA_USB_HOST) || defined(USA_USB_SLAVE_CDC)
    SYS_Tasks();
#ifdef USA_USB_HOST_MSD
    SYS_FS_Tasks();   // serve, per rapidità...
#endif
#else
    APP_Tasks();
#endif
#ifdef USA_WIFI
    m2m_wifi_handle_events(NULL);
#endif
#ifdef USA_ETHERNET
    StackTask();
    StackApplications();
#endif
    mLED_1 ^= 1;     //  
    mInstance->incomingChar[0]=keypress.key; mInstance->incomingChar[1]=keypress.modifier;
    KBClear();
    if(hitCtrlC(TRUE)) {
      setError(mInstance,ERR_STOP);
      break;
      }
    }
  answer = strdup(buf);
  if(!answer)
		setError(mInstance,ERR_OUTOFMEMORY);

  return answer;
	}

char *midiString(MINIBASIC *mInstance) {
  char buf[2];
	int i;
  char *answer;

  match(mInstance,MIDISTRING);
//  match(mInstance,OPAREN);
//  match(mInstance,CPAREN);

  buf[0] = ReadMidi(); //byteRecMidi
  buf[1]=0;
    
  answer = strdup(buf);
  if(!answer)
		setError(mInstance,ERR_OUTOFMEMORY);

  return answer;
	}

static const char *modemStrings[10]= {
  "ERROR",
  "OK",
  "IDLE",
  "DISCONNECTED",
  "DIALING",
  "CONNECTING",
  "TRAINING",
  "NEGOTIATING",
  "CONNECTED"
  };
char *modemString(MINIBASIC *mInstance) {
  char buf[16];
	int i;
  char *answer;

  match(mInstance,MODEMSTRING);
  if(mInstance->token != OPAREN)
    goto leggo_char;
  match(mInstance,OPAREN);
  i = expr(mInstance);
  match(mInstance,CPAREN);

  switch(i) {
    case 0:     // leggo char
leggo_char:
      buf[0] = ReadModem();
      buf[1]=0;
      break;
    case 1:     // leggo status/error
// v. anche modemInited      
      i = ReadModemStatus(); //
      if(i & 128) {
        i &= 1;
        strcpy(buf,modemStrings[i]);
        }
      else
        itoa(buf,i & 0xf,10);
      break;
    case 2:     // leggo connectState
// v. anche modemInited, cmq se non è init'ed qua dovrebbe dare IDLE che è ok
      i = ReadModemStatus(); //
      if(i & 128) {
        i >>= 4;
        i &= 7;
        i += 2;
        strcpy(buf,modemStrings[i]);
        }
      else
        itoa(buf,(i >> 4) & 0x7,10);
      break;
    }
    
  answer = strdup(buf);
  if(!answer)
		setError(mInstance,ERR_OUTOFMEMORY);

  return answer;
	}

#ifdef USA_BREAKTHROUGH
char *menukeyString(MINIBASIC *mInstance) {
  char buf[2];
	int i;
  char *answer;

  match(mInstance,MENUKEYSTRING);
  match(mInstance,OPAREN);
  match(mInstance,CPAREN);

//	i=menucommand();
  //fare
		buf[0]=0;
    
  answer = strdup(buf);
  if(!answer)
		setError(mInstance,ERR_OUTOFMEMORY);

  return answer;
	}
#endif

/*
  handle ER$ token, based on inkey() (numeric) ??
*/
char *errorString(MINIBASIC *mInstance) {
  char *answer;
	BYTE x;

  match(mInstance,ERRORSTRING);
  match(mInstance,OPAREN);
  x = integer(mInstance,expr(mInstance));
  match(mInstance,CPAREN);

	if(x >= sizeof(error_msgs)/sizeof(char *)) {
		setError(mInstance, ERR_BADVALUE);
		}
	
  answer = strdup((STRINGFARPTR)error_msgs[x]);
  if(!answer)
		setError(mInstance,ERR_OUTOFMEMORY);

  return answer;
	}

char *verString(MINIBASIC *mInstance) {
  char *answer;
  char buff[6];

  match(mInstance,VERSTRING);
  buff[0]=MINIBASIC_COPYRIGHT_STRING[23];
  buff[1]=MINIBASIC_COPYRIGHT_STRING[23+1];
  buff[2]=MINIBASIC_COPYRIGHT_STRING[23+2];
  buff[3]=MINIBASIC_COPYRIGHT_STRING[23+3];
  buff[4]=MINIBASIC_COPYRIGHT_STRING[23+4];
  buff[5] = 0;
  answer = strdup(buff);

  if(!answer)
    setError(mInstance,ERR_OUTOFMEMORY);

  return answer;
	}


/*
  parse the STR$ token
*/
char *strString(MINIBASIC *mInstance) {
  NUM_TYPE x;
  char buff[20];
  char *answer;

  match(mInstance,STRSTRING);
  match(mInstance,OPAREN);
  x = expr(mInstance);
  match(mInstance,CPAREN);

  //sprintf(buff, "%g", x);
	{
	long n;
	n=(long) fabs(((x - (long) x ) * 1000000));
//	sprintf(buff,(STRINGFARPTR)"%d.%06lu", (int) x, (int) fabs(((x - (int) x ) * 1000000))); 
	if(n) {		// un trucchetto per stampare interi o float
		unsigned char i;

		sprintf(buff,(STRINGFARPTR)"%ld.%06lu", (long) x, (long) n); 
		for(i=strlen(buff)-1; i; i--) {
			if(buff[i] == '0')
				buff[i]=0;
			else
				break;
			}
		}
	else
		sprintf(buff,(STRINGFARPTR)"%ld", (long) x); 
  answer = strdup(buff);
	}
  if(!answer)
		setError(mInstance,ERR_OUTOFMEMORY);
  return answer;
	}


/*
  parse the LEFT$ token
*/
char *leftString(MINIBASIC *mInstance) {
  char *str;
  int x;
  char *answer;

  match(mInstance,LEFTSTRING);
  match(mInstance,OPAREN);
  str = stringExpr(mInstance);
  if(!str)
		return 0;
  match(mInstance,COMMA);
  x = integer(mInstance,expr(mInstance));
  match(mInstance,CPAREN);

  if(x > (int) strlen(str))
		return str;
  if(x < 0) {
    setError(mInstance,ERR_ILLEGALOFFSET);
    return str;
  	}
  str[x] = 0;
  answer = strdup(str);
  free(str);
  if(!answer)
		setError(mInstance,ERR_OUTOFMEMORY);
  return answer;
	}


/*
  parse the RIGHT$ token
*/
char *rightString(MINIBASIC *mInstance) {
  int x;
  char *str;
  char *answer;

  match(mInstance,RIGHTSTRING);
  match(mInstance,OPAREN);
  str = stringExpr(mInstance);
  if(!str)
		return 0;
  match(mInstance,COMMA);
  x = integer(mInstance,expr(mInstance));
  match(mInstance,CPAREN);

  if( x > (int) strlen(str))
		return str;

  if(x < 0) {
    setError(mInstance,ERR_ILLEGALOFFSET);
		return str;
  	}
  
  answer = strdup( &str[strlen(str) - x] );
  free(str);
  if(!answer)
		setError(mInstance,ERR_OUTOFMEMORY);
  return answer;
	}


/*
  parse the MID$ token
*/
char *midString(MINIBASIC *mInstance) {
  char *str;
  int x;
  int len;
  char *answer;
  char *temp;

  match(mInstance,MIDSTRING);
  match(mInstance,OPAREN);
  str = stringExpr(mInstance);
  match(mInstance,COMMA);
  x = integer(mInstance,expr(mInstance));
	if(mInstance->token==COMMA) {
		match(mInstance,COMMA);
		len = integer(mInstance,expr(mInstance));
		}
	else
		len=-1;
  match(mInstance,CPAREN);

  if(!str)
		return 0;

  if(len == -1)
		len = strlen(str) - x + 1;

  if( x > (int) strlen(str) || len < 1) {
		free(str);
		answer = strdup(EmptyString);
		if(!answer)
	  	setError(mInstance,ERR_OUTOFMEMORY);
    return answer;
  	}
  
  if(x < 1) {
    setError(mInstance,ERR_ILLEGALOFFSET);
		return str;
  	}

  temp = &str[x-1];

  answer = (char *)malloc(len + 1);
  if(!answer) {
    setError(mInstance,ERR_OUTOFMEMORY);
		return str;
  	}
  strncpy(answer, temp, len);
  answer[len] = 0;
  free(str);

  return answer;
	}


/*
  parse the string$ token
*/
char *stringString(MINIBASIC *mInstance) {
  int x;
  char *str;
  char *answer;
  int len;
  int N;
  int i;

  match(mInstance,STRINGSTRING);
  match(mInstance,OPAREN);
  x = integer(mInstance,expr(mInstance));
  match(mInstance,COMMA);
  str = stringExpr(mInstance);
  match(mInstance,CPAREN);

  if(!str)
		return 0;

  N = x;

  if(N < 1) {
    free(str);
		answer = strdup(EmptyString);
		if(!answer)
	  	setError(mInstance,ERR_OUTOFMEMORY);
		return answer;
	  }

  len = strlen(str);
  answer = (char *)malloc( N * len + 1 );
  if(!answer) {
    free(str);
		setError(mInstance,ERR_OUTOFMEMORY);
		return 0;
  	}
  for(i=0; i < N; i++) {
    strcpy(answer + len * i, str);
  	}
  free(str);

  return answer;
	}


/*
  read a dimensioned string variable from input.
  Returns: pointer to string (not malloced) 
*/
char *stringDimVar(MINIBASIC *mInstance) {
  char id[IDLENGTH];
  IDENT_LEN len;
  DIMVAR *dimvar;
  char **answer;
  DIM_SIZE index[MAXDIMS];

  getId(mInstance,mInstance->string, id, &len);
  match(mInstance,DIMSTRID);
  dimvar = findDimVar(mInstance,id);

  if(dimvar) {
    switch(dimvar->ndims) {
	  	case 1:
	    	index[0] = integer(mInstance,expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0]);
				break;
      case 2:
				index[0] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[1] = integer(mInstance,expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0], index[1]);
				break;
		  case 3:
				index[0] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[1] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[2] = integer(mInstance,expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0], index[1], index[2]);
				break;
		  case 4:
				index[0] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[1] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[2] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[3] = integer(mInstance,expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0], index[1], index[2], index[3]);
				break;
		  case 5:
				index[0] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[1] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[2] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[3] = integer(mInstance,expr(mInstance));
				match(mInstance,COMMA);
				index[4] = integer(mInstance,expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0], index[1], index[2], index[3], index[4]);
				break;
			}

		match(mInstance,CPAREN);
  	}
  else
		setError(mInstance,ERR_NOSUCHVARIABLE);

  if(mInstance->errorFlag == ERR_CLEAR)
		if(*answer)
	     return *answer;
	 
  return (char *)EmptyString;
	}



/*
  parse a string variable.
  Returns: pointer to string (not malloced) 
*/
char *stringVar(MINIBASIC *mInstance) {
  char id[IDLENGTH];
  IDENT_LEN len;
  VARIABLE *var;

  getId(mInstance,mInstance->string, id, &len);
  match(mInstance,STRID);
  var = findVariable(mInstance,id);
  if(var) {
    if(var->d.sval)
	  	return var->d.sval;
		else
			return (char *)EmptyString;
  	}
  setError(mInstance,ERR_NOSUCHVARIABLE);
  return (char *)EmptyString;
	}


/* returns directory as a CSV list of files + free space*/
char *dirString(MINIBASIC *mInstance) {
  SearchRec rec;
  FS_DISK_PROPERTIES disk_properties;
#if defined(USA_USB_HOST_MSD)
  SYS_FS_HANDLE myFileHandle;
  SYS_FS_FSTAT stat;
  uint32_t totalSectors,freeSectors,sectorSize;
#endif
  int i,totfiles; 
  char *str,*filter,*result;
  char buf[32];
  char *answer;
  char mydrive;

  match(mInstance,DIRSTRING);
  match(mInstance,OPAREN);
  // fare opzionale!
  str = stringExpr(mInstance);
  match(mInstance,CPAREN);

  WORD totFiles;

  if(str)
    mydrive=getDrive(str,NULL,&filter);
  else
    mydrive=currDrive;
  if(!*filter)
    filter="*.*";
      
  switch(mydrive) {
    case 'A':
    case 'C':
#ifdef USA_RAM_DISK 
    case DEVICE_RAMDISK:
#endif
      switch(mydrive) {
        case 'A':
          if(!SDcardOK)
            goto fine;
          i=FindFirst(filter, ATTR_MASK ^ ATTR_VOLUME, &rec);
          break;
        case 'C':
          if(!HDOK)
            goto fine;
          i=IDEFindFirst(filter, ATTR_MASK ^ ATTR_VOLUME, &rec);
          break;
#ifdef USA_RAM_DISK 
        case DEVICE_RAMDISK:
          if(!RAMdiscArea)
            goto fine;
          i=RAMFindFirst(filter, ATTR_MASK ^ ATTR_VOLUME, &rec);
          break;
#endif
        }

      if(!i) {
        totfiles=0;
        result=malloc(50 +20 /*freespace*/);
        if(!result) {
          setError(mInstance,ERR_OUTOFMEMORY);
          goto fine;
          }
        *result=0;

        for(;;) {
          strcat(result,rec.filename); 
          strcat(result,"\t");
          if(rec.attributes & ATTR_DIRECTORY) {
            strcat(result,"DIR\t");
            }
          else {
            sprintf(buf,"%u\t",rec.filesize);
            strcat(result,buf);
            sprintf(buf,"%02u/%02u/%04u %02u:%02u:%02u\t",
              (rec.timestamp >> 16) & 31,
              (rec.timestamp >> (5+16)) & 15,
              (rec.timestamp >> (9+16)) + 1980,
              (rec.timestamp >> 11) & 31,
              (rec.timestamp >> 5) & 63,
              rec.timestamp & 63);
            strcat(result,buf);
            }

          totfiles++;
          result=realloc(result,50*(totfiles+1)+20);
          if(!result) {
            setError(mInstance,ERR_OUTOFMEMORY);
            goto fine;
            }

          switch(mydrive) {
            case 'A':
              if(FindNext(&rec))
                goto fine_dir;
              break;
            case 'C':
              if(IDEFindNext(&rec))
                goto fine_dir;
              break;
#ifdef USA_RAM_DISK 
            case DEVICE_RAMDISK:
              if(RAMFindNext(&rec))
                goto fine_dir;
              break;
#endif
            }
          } 
        }
      else {
        free(result);
// GESTIRE come di là        if(FSerror() != CE_FILE_NOT_FOUND) {
        setError(mInstance,ERR_FILE);
        goto fine;
        }

fine_dir:
      disk_properties.new_request=1;
      do {
        switch(mydrive) {
          case 'A':
            FSGetDiskProperties(&disk_properties);
            break;
          case 'C':
            IDEFSGetDiskProperties(&disk_properties);
            break;
#ifdef USA_RAM_DISK 
          case DEVICE_RAMDISK:
            RAMFSGetDiskProperties(&disk_properties);
            break;
#endif
          }
        ClrWdt();
        } while(disk_properties.properties_status == FS_GET_PROPERTIES_STILL_WORKING);
      if(disk_properties.properties_status == FS_GET_PROPERTIES_NO_ERRORS) {
        sprintf(buf,"%lu\r",disk_properties.results.free_clusters*disk_properties.results.sectors_per_cluster*disk_properties.results.sector_size/1024); 
        strcat(result,buf);
        }

      answer= strdup(result);
      if(!answer)
        setError(mInstance,ERR_OUTOFMEMORY);
      free(str);
      return answer;
      break;
#if defined(USA_USB_HOST_MSD)
    case 'E':
      if(!USBmemOK)
        goto fine;
      if((myFileHandle=SYS_FS_DirOpen("/")) != SYS_FS_HANDLE_INVALID) {
        totfiles=0;
        result=malloc(50 +20 /*freespace*/);
        if(!result) {
          setError(mInstance,ERR_OUTOFMEMORY);
          goto fine;
          }
        *result=0;
        
        while(SYS_FS_DirSearch(myFileHandle,filter,SYS_FS_ATTR_MASK ^ SYS_FS_ATTR_VOL,&stat) == SYS_FS_RES_SUCCESS) {
          strcat(result,rec.filename); 
          strcat(result,"\t");
          if(stat.fattrib & ATTR_DIRECTORY) {
            strcat(result,"DIR\t");
            }
          else {
            sprintf(buf,"%u\t",stat.fsize);
            strcat(result,buf);
            sprintf(buf,"%02u/%02u/%04u %02u:%02u:%02u\t",
              stat.fdate & 31,
              (stat.fdate >> (5)) & 15,
              (stat.fdate >> (9)) + 1980,
              (stat.ftime >> 11) & 31,
              (stat.ftime >> 5) & 63,
              stat.ftime & 63);
            }
          }
        SYS_FS_DirClose(myFileHandle);
        }
      else {
        setError(mInstance,ERR_FILE);
        goto fine;
        }

      // stampare...totdirs e totsize
      disk_properties.new_request=1;
      if(SYS_FS_DriveSectorGet(NULL,&totalSectors,&freeSectors,&sectorSize)==SYS_FS_RES_SUCCESS) {
        sprintf(buf,"%lu\r",freeSectors* MEDIA_SECTOR_SIZE /* boh dov'è??*//1024); 
        strcat(result,buf);
        }
      break;
#endif
    }

fine:
  free(str);
  return NULL;
  }

char *curdirString(MINIBASIC *mInstance) {
  char *answer;
  char buf[64];

  match(mInstance,CURDIRSTRING);

  switch(currDrive) {
    case 'A':
      if(SDcardOK)
        answer= strdup(FSgetcwd(NULL,0));
      else {
fine:
    		setError(mInstance,ERR_FILE);
        return NULL;
        }
      break;
    case 'C':
      if(HDOK)
        answer= strdup(IDEFSgetcwd(NULL,0));
      else
        goto fine;
      break;
#if defined(USA_USB_HOST_MSD)
    case 'E':
      if(USBmemOK && SYS_FS_CurrentWorkingDirectoryGet(buf,63) == SYS_FS_RES_SUCCESS)
        answer= strdup(buf);
      else
        goto fine;
      break;
#endif
#ifdef USA_RAM_DISK 
  case DEVICE_RAMDISK:
      if(RAMdiscArea)
        answer= strdup(RAMFSgetcwd(NULL,0));
      else
        goto fine;
      break;
#endif
    default:
  		setError(mInstance,ERR_BADVALUE);
      return NULL;
      break;
    }
  if(!answer)
		setError(mInstance,ERR_OUTOFMEMORY);
  return answer;
  }

char *curdriveString(MINIBASIC *mInstance) {
  char *answer,buf[3];

  match(mInstance,CURDRIVESTRING);

  buf[0]=currDrive;
  buf[1]=':';
  buf[2]=0;
  answer= strdup(buf);
  if(!answer)
		setError(mInstance,ERR_OUTOFMEMORY);
  return answer;
  }

char *ipaddressString(MINIBASIC *mInstance) {
  char buf[32],*answer;
  int x=0;

  match(mInstance,IPADDRESSSTRING);
  match(mInstance,OPAREN);
  x = integer(mInstance,expr(mInstance));
  match(mInstance,CPAREN);

#if defined(USA_WIFI) && defined(USA_ETHERNET)
  if(x==0)
    sprintf(buf,"%u.%u.%u.%u",myIp.addr[0],myIp.addr[1],myIp.addr[2],myIp.addr[3]);
  else
    sprintf(buf,"%u.%u.%u.%u",BiosArea.MyIPAddr.v[0],BiosArea.MyIPAddr.v[1],BiosArea.MyIPAddr.v[2],BiosArea.MyIPAddr.v[3]);
#elif defined(USA_WIFI)
  sprintf(buf,"%u.%u.%u.%u",myIp.addr[0],myIp.addr[1],myIp.addr[2],myIp.addr[3]);
#elif defined(USA_ETHERNET)
  sprintf(buf,"%u.%u.%u.%u",BiosArea.MyIPAddr.v[0],BiosArea.MyIPAddr.v[1],BiosArea.MyIPAddr.v[2],BiosArea.MyIPAddr.v[3]);
#else
  sprintf(buf,"%u.%u.%u.%u",0,0,0,0);
#endif
  
  answer= strdup(buf);
  if(!answer)
		setError(mInstance,ERR_OUTOFMEMORY);
  return answer;
  }

char *accesspointsString(MINIBASIC *mInstance) {
  char *answer;
  int n;
  WORD tOut=0;

  match(mInstance,ACCESSPOINTSSTRING);

  if(mInstance->token==INTID) {
  	n=integer(mInstance,expr(mInstance));
    }
  else
    n=M2M_WIFI_CH_ALL;
  
#ifdef USA_WIFI
  *(unsigned long*)internetBuffer=0;
  m2m_wifi_request_scan(n);    // serve callback...
	while(tOut<5000) {
    m2m_wifi_handle_events(NULL);
    tOut++;
    __delay_ms(1);
		}
  // opp. provare m2m_wifi_req_scan_result(uint8 index);
  // e v. anche m2m_wifi_get_num_ap_found(void)


  answer= strdup(internetBuffer);
  *internetBuffer=0;
  if(!answer)
		setError(mInstance,ERR_OUTOFMEMORY);
#else
  answer=(char*)EmptyString;
#endif
  return answer;
  }


/*
  parse a string literal
  Returns: malloc'ed string literal
  Notes: newlines aren't allowed in literals, but blind concatenation across newlines is. 
*/
char *stringLiteral(MINIBASIC *mInstance) {
  int len = 1;
  char *answer = 0;
  char *temp;
  char *substr;
  const char *end;

  while(mInstance->token == QUOTE) {
    mInstance->string=(char *)skipSpaces(mInstance->string);

    end = mystrend((const char *)mInstance->string, '"');
    if(end) {
      len = end - mInstance->string;
      substr = (char *)malloc(len);
	  	if(!substr) {
	    	setError(mInstance,ERR_OUTOFMEMORY);
	    	return answer;
	  		}
	  	mystrgrablit(substr, mInstance->string);
	  	if(answer) {
				temp = myStrConcat(answer, substr);
	    	free(substr);
				free(answer);
				answer = temp;
				if(!answer) {
	      	setError(mInstance,ERR_OUTOFMEMORY);
		  		return answer;
					}
	  		}
	  	else
	    	answer = substr;
		  mInstance->string = (char *)end;
			}
		else {
		  setError(mInstance,ERR_SYNTAX);
		  return answer;
			}

		match(mInstance,QUOTE);
  	}

  return answer;
	}


/*
  cast a double to an integer, triggering errors if out of range
*/
int integer(MINIBASIC *mInstance,double x) {

  if(isString(mInstance->token))
		setError(mInstance, ERR_TYPEMISMATCH);
  if(x < SHRT_MIN || x > USHRT_MAX)   // boh lascio passare cmq 32768..65535, che poi diventa negativo ma è più comodo!
		setError(mInstance, ERR_BADVALUE);
  
//  if( x != floor(x))
//		setError(mInstance, ERR_NOTINT);   bah ma fottiti bigotto del cazzo :D
  
  return (int)x;
	}



/*
  check that we have a token of the passed type 
    (if not set the errorFlag)
  Move parser on to next token. Sets token and string.
*/
void match(MINIBASIC *mInstance,TOKEN_NUM tok) {

  if(mInstance->token != tok) {
		setError(mInstance,ERR_SYNTAX);
		return;
	  }

  mInstance->string=(char *)skipSpaces(mInstance->string);

  mInstance->string += tokenLen(mInstance,mInstance->string, mInstance->token);
  mInstance->token = getToken(mInstance->string);
  if(mInstance->token == B_ERROR)
		setError(mInstance,ERR_SYNTAX);

	}


/*
  set the errorFlag.
  Params: errorcode - the error.
  Notes: ignores error cascades
*/
/*BSTATIC*/ void setError(MINIBASIC *mInstance,enum INTERPRETER_ERRORS errorcode) {

  if(mInstance->errorFlag==ERR_CLEAR || !errorcode)
		mInstance->errorFlag = errorcode;
	}


/*
  get the next line number
  Params: str - pointer to parse string
  Returns: line no of next line, 0 if end
  Notes: goes to newline, then finds
         first line starting with a digit.
*/
BSTATIC LINE_NUMBER_TYPE getNextLine(MINIBASIC *mInstance,const char *str) {
  
  while(*str) {
    while(*str && *str != '\n')
	  	str++;
    if(*str == 0)
	  	return 0;
    str++;
    if(isdigit(*str))
	  	return atoi(str);
  	}
  return 0;
	}

BSTATIC LINE_NUMBER_STMT_POS_TYPE getNextStatement(MINIBASIC *mInstance,const char *str) {
	LINE_NUMBER_STMT_POS_TYPE answer;
  int i;
  
  i=mInstance->curline;
  answer.d=STMT_CONTINUE;
  
  while(*str && *str != ':' && *str != '\n')    // il ' viene correttamente scartato, come anche sopra! ok
    str++;
  switch(*str) {
    case 0:
      break;
    case ':':
      str++;
      break;
    case '\n':
      i++;
      str++;
      break;
    }
  str=skipSpaces(str);
  if(isdigit(*str)) {    // se siamo a riga nuova...
    while(isdigit(*str))
      str++;
    str=skipSpaces(str);
    }
  answer.line=i;
  answer.pos=str-mInstance->lines[i].str;
	return answer;
	}



/*
  get a token from the string
  Params: str - string to read token from
  Notes: ignores white space between tokens
*/
TOKEN_NUM getToken(const char *str) {
	unsigned char i,j;
  
  str=skipSpaces(str);

  if(isdigit(*str))
    return VALUE;
 
  switch(*str) {
    case 0:
	  	return EOS;
    case '\n':
	  	return EOL;
		case '/': 
		  return DIV;
		case '*':
		  return MULT;
		case '\\':
		  return MOD;
		case '(':
		  return OPAREN;
		case ')':
		  return CPAREN;
		case '+':
		  return PLUS;
		case '-':
		  return MINUS;
		case '!':
		  return SHRIEK;
		case '&':
		  return AMPERSAND;
		case ',':
		  return COMMA;
		case ';':
		  return SEMICOLON;
		case ':':
		  return COLON;
		case '#':
		  return DIESIS;
		case '"':
		  return QUOTE;
		case '=':
		  return EQUALS;
		case '<':
		  return LESS;
		case '>':
		  return GREATER;
		case '~':
		  return BITNOT;
		case '?':
		  return PRINT2;
		case '\'':
		  return REM2;
		default:

		  if(tolower(*str)=='e' && !isalnum(str[1]))  // questo impedisce e% come var...
				return ETOK;
		  if(tolower(*str)=='p' && tolower(str[1])=='i' && !isalnum(str[2]))
				return PITOK;
//		  if(isupper(*str)) {
//#warning tolto UPPERCASE basic! 2022 fuck bigots ;)
				for(i=0; i<sizeof(tl)/sizeof(TOKEN_LIST); i++) {
// bit of wasted code: ? and ' are searched here too, but they are not found (not strings) and handled above...
					j=tl[i].length;
					if(!strnicmp((char *)str, (char *)tl[i].tokenname, j) &&
					  !isalnum(str[j]))
						return i+PRINT;
					}
//			  }			  /* end isupper() */
	
		  if(isalpha(*str)) {
				while(isalnum(*str))
				  str++;
				switch(*str) {
				  case '%':
						return str[1] == '(' ? DIMINTID : INTID;
				  case '$':
						return str[1] == '(' ? DIMSTRID : STRID;
				  case '(':
						return DIMFLTID;
				  default:
						return FLTID;
					}
			  }
		
			return B_ERROR;
			break;
	  } 		// switch
	}


/*
  get the length of a token.
  Params: str - pointer to the string containing the token
          token - the type of the token read
  Returns: length of the token, or 0 for EOL to prevent it being read past.
*/
uint8_t tokenLen(MINIBASIC *mInstance,const char *str, TOKEN_NUM token) {
  IDENT_LEN len = 0;
  char buff[20];

  switch(token) {
    case EOS:
	  	return 0;
    case EOL:
	  	return 1;
    case VALUE:
	  	getValue(str, &len);
	  	return len;
		case DIMSTRID:
		case DIMFLTID:
		case DIMINTID:
		case STRID:
			// fallthrough
//		  getId(mInstance,str, buff, &len);
//	  	return len;
		case INTID:
			// fallthrough
//	  	getId(mInstance,str, buff, &len);
//	  	return len;
		case FLTID:
	  	getId(mInstance,str, buff, &len);
	  	return len;
    case PITOK:
	  	return 2;
    case ETOK:
	  	return 1;

    case DIV:
    case MULT:
		case OPAREN:
    case CPAREN:
    case PLUS:
    case MINUS:
    case MOD:
//    case NOT:
    case SHRIEK:
    case COMMA:
		case QUOTE:
		case EQUALS:
		case LESS:
		case GREATER:
		case SEMICOLON:
		case COLON:
		case DIESIS:
		case BITNOT:
			return 1;
    case AMPERSAND:
			return 1;

    case B_ERROR:
	  	return 0;

		default:
			if(token>=PRINT && token<(PRINT+sizeof(tl)/sizeof(TOKEN_LIST)))
				return tl[token-PRINT].length;
			else
		  	basicAssert(0);
		  return 0;
  	}
	}


/*
  test if a token represents a string expression
  Params: token - token to test
  Returns: 1 if a string, else 0
*/
signed char isString(TOKEN_NUM token) {
//	unsigned char a=CHRSTRING;

  if(token == STRID || token == QUOTE || token == DIMSTRID 
		|| (token >= FIRST_CHAR_FUNCTION && token <= LAST_CHAR_FUNCTION)
/*	  || token == CHRSTRING || token == STRSTRING 
	  || token == LEFTSTRING || token == RIGHTSTRING 
	  || token == MIDSTRING || token == STRINGSTRING || token == HEXSTRING
		|| token == TRIMSTRING || token == INKEYSTRING || token == ERRORSTRING */ )
		return 1;
  return 0;
	}

signed char isNumber(TOKEN_NUM token) {   // per ora non usata ma .. v. doPrint

  if(token == INTID || token == FLTID || token == DIMINTID || token == DIMFLTID || token == VALUE 
		|| (token >= FIRST_NUM_FUNCTION && token <= LAST_NUM_FUNCTION))
		return 1;
  return 0;
	}


/*
  get a numerical value from the parse string
  Params: str - the string to search
          len - return pointer for no chars read
  Retuns: the value of the string.
*/
NUM_TYPE getValue(const char *str, IDENT_LEN *len) {
  NUM_TYPE answer;
  char *end;		// no CONST
	
  answer = strtod(str, &end);

  basicAssert(end != str);
  *len = end - str;
  return answer;
	}


/*
  getId - get an id from the parse string:
  Params: str - string to search
          out - id output [32 chars max ]
		  len - return pointer for id length
  Notes: triggers an error if id > 31 chars
         the id includes the $ and ( qualifiers.
*/
void getId(MINIBASIC *mInstance,const char *str, char *out, IDENT_LEN *len) {
  IDENT_LEN nread = 0;

  str=skipSpaces(str);
  basicAssert(isalpha(*str));
  while(isalnum(*str)) {
		if(nread < IDLENGTH-1)
			out[nread++] = *str++;
		else {
			setError(mInstance,ERR_IDTOOLONG);
			break;
			}
		}
  if(*str == '$' || *str == '%') {
		if(nread < IDLENGTH-1)
		  out[nread++] = *str++;
		else
		 setError(mInstance,ERR_IDTOOLONG);
	  }
  if(*str == '(') {
		if(nread < IDLENGTH-1)
		  out[nread++] = *str++;
		else
		  setError(mInstance,ERR_IDTOOLONG);
	  }
  out[nread] = 0;
  *len = nread;
	}



/*
  grab a literal from the parse string.
  Params: dest - destination string
          src - source string
  Notes: strings are in quotes, double quotes the escape
*/
static void mystrgrablit(char *dest, const char * src) {
  
	basicAssert(*src == 0x22 /* '\"' */);
  src++;
  
  while(*src) {
		if(*src == '"')	{
	  	if(src[1] == '"') {
				*dest++ = *src;
	    	src++;
	    	src++;
	  		}
	  	else
				break;
			}
		else
    	*dest++ = *src++;
  	}

  *dest = 0;
	}



/*
  find where a source string literal ends
  Params: src - string to check (must point to quote)
          quote - character to use for quotation
  Returns: pointer to quote which ends string
  Notes: quotes escape quotes
*/
static const char *mystrend(const char *str, char quote) {
  
	basicAssert(*str == quote);
  str++;

  while(*str) {
    while(*str != quote) {
	  	if(*str == '\n' || *str == 0)
				return 0;
	  	str++;
			}
    if(str[1] == quote)
	  	str += 2;
		else
	  	break;
  	}

  return (char *) (*str ? str : 0);
	}


/*
  Count the instances of ch in str
  Params: str - string to check
          ch - character to count
  Returns: number of times ch occurs in str. 
*/
static unsigned short int myStrCount(const char *str, char ch) {
  unsigned short int answer=0;

  while(*str) {
    if(*str++ == ch)
		  answer++;
  	}

  return answer;
	}


/*
  duplicate a string:
  Params: str - string to duplicate
  Returns: malloced duplicate.
*/
char *strdup(const char *str) {
  char *answer;

  answer = (char *)malloc(strlen(str) + 1);
  if(answer)
    strcpy(answer,str);

  return answer;
	}

char *strupr(char *str) {   // frocissimi!
  char *answer=str;
  
  while(*str) {
    if(isalpha(*str))
      *str &= ~0x20;
    str++;
    }
  
  return answer;
  }



/*
  concatenate two strings
  Params: str - firsts string
          cat - second string
  Returns: malloced string.
*/
static char *myStrConcat(const char *str, const char *cat) {
  int len;
  char *answer;

  len = strlen(str) + strlen(cat);
  answer = (char *)malloc(len + 1);
  if(answer) {
    strcpy(answer, str);
    strcat(answer, cat);
  	}
  return answer;
	}



/*
  compute x!  
*/
double factorial(double x) {
  double answer = 1.0;
  double t;

  if(x > 1000.0)
    x = 1000.0;

  for(t=1; t<=x; t+=1.0)
		answer *= t;

  return answer;
	}




/******************** PIC32 ?? Specific Functions **********************/

int8_t stricmp(const char *string1, const char *string2) {

  while(tolower(*string1) == tolower(*string2)) {
    if(!*string1)
      return 0;
    string1++;
    string2++;
    }
  return tolower(*(unsigned const char *)string1) - tolower(*(unsigned const char *)string2);
	}

int8_t strnicmp(const char *string1, const char *string2, size_t count) {

  if(!count)
    return 0;

  do {
    if(tolower((unsigned char)*string1) != tolower((unsigned char)*string2++))
      return (int)tolower((unsigned char)*string1) -	(int)tolower((unsigned char)*--string2);
    if(!*string1++)
      break;
    } while(--count);
  
	return 0;
	}




//void assert(char s) {}		// already in libs...


char *myitohex(unsigned int n) {
	char *p = &myitoabuf[sizeof(myitoabuf)-1];
	
  *p = '\0';
	do {
		*(--p) = "0123456789ABCDEF"[n & 0x0F];
		} while ((n >>= 4) > 0);
	return p;
	}

char *myitob(unsigned int n) {
	char *p = &myitoabuf[sizeof(myitoabuf)-1];
	
  *p = '\0';
	do {
		*(--p) = "01"[n & 1];
		} while ((n >>= 1) > 0);
	return p;
	}

BYTE myhextob(char c) {
  
  if(isdigit(c))
    return c-'0';
  else if(toupper(c) >='A' && toupper(c) <='F')
    return toupper(c)-'A'+10;
  }

unsigned int myhextoi(const char *p) {
  unsigned int n=0;
  BYTE c;
	
  while(c=*p++) {
    n <<= 4;
    n |= myhextob(c);
    }
	return n;
	}

unsigned int mybtoi(const char *p) {
  unsigned int n=0;
  BYTE c;
	
  while(c=*p++) {
    n <<= 1;
    n |= (c == '1' ? 1 : 0);
    }
	return n;
	}


static const char *skipSpaces(const char *str) {

	while(/* isspace*/ *str == ' ' || *str == 9)
		str++;
	return str;
	}


// ----------------------------------------------------
int myTextOut(MINIBASIC *mInstance,const char *s) {
  
#ifndef USING_SIMULATOR
#ifdef USA_BREAKTHROUGH
  HDC myDC,*hDC;
  hDC=GetDC(mInstance->hWnd,&myDC);
  SetTextColor(hDC,Color24To565(mInstance->Color));
  SetBkColor(hDC,Color24To565(mInstance->ColorBK));

  TextOut(hDC,mInstance->Cursor.x*hDC->font.size*6,
          mInstance->Cursor.y*hDC->font.size*8,s);
  ReleaseDC(mInstance->hWnd,hDC);
#else
  SetColors(Color24To565(mInstance->Color),Color24To565(mInstance->ColorBK));
  SetXYText(mInstance->Cursor.x,mInstance->Cursor.y);
  
  while(*s) {
    switch(*s) {
      case 8:   // 
        if(mInstance->Cursor.x>0)
          mInstance->Cursor.x--;
        break;
      case 10:
      case 13:
printLF:
        mInstance->Cursor.x=0;
        mInstance->Cursor.y++;
        if(mInstance->Cursor.y >= ScreenText.cy) {
// ??    while(isCtrlS());
          mInstance->Cursor.y--;
          }
        break;
      case 9:   // TAB dovrebbe essere gestito sopra da PRINT
        break;
      default:
        putchar(*s);
        mInstance->Cursor.x++;
        if(mInstance->Cursor.x >= ScreenText.cx) {
          goto printLF;
          }
        break;
      }
    s++;
    }
#endif
#endif
  
  return 1;
  }

int myCR(MINIBASIC *mInstance) {
  
#ifndef USING_SIMULATOR
  mInstance->Cursor.x=0;  mInstance->Cursor.y++;
  if(mInstance->Cursor.y >= ScreenText.cy) {
    while(isCtrlS());
    mInstance->Cursor.y--;
    putchar('\n');    // forzo scroll!
    }
  SetXYText(mInstance->Cursor.x,mInstance->Cursor.y);
#endif
  return 1;
  }

#if 0
int inkey(void) {
  BYTE keypress[2];
//  extern struct KEYPRESS keypress;

  ReadPMPs(SOUTH_BRIDGE,BIOS_KEYBOARD_READ,keypress,2);
	//qua faccio così... verificare...
  //e i keypressModif
  return keypress[0] /* | keypress[1]*/;
  
//v.      inputKey();
//    i=inputKey();
//    ch=tolower(LOBYTE(i));

  }
#endif
