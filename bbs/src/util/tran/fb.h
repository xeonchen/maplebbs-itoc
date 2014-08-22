/*----------------------------------------------------------*/
/* util/fb/fb.h	                                            */
/*----------------------------------------------------------*/
/* target : firebird 3.0 轉 Maple 3.x                       */
/* create : 00/11/22                                        */
/* update :   /  /                                          */
/* author : hightman@263.net                                */
/*----------------------------------------------------------*/


#if 0

  1. 設定 OLD_BBSHOME、FN_PASSWD、FN_BOARD
  2. 修改所有的 old struct

  3. 必須在 brd 轉完才可以轉換 gem
  4. 必須在 usr 及 brd 都轉完才可以轉換 pal
  5. 建議轉換順序為 usr -> brd -> gem ->pal

#endif


#include "bbs.h"


#define	OLD_BBSHOME	"/home/oldbbs"		/* FB */
#define FN_PASSWD	"/home/oldbbs/.PASSWDS"	/* FB */
#define FN_BOARD	"/home/oldbbs/.BOARDS"  /* FB */

#define	NOBOARD		"1002"


/* ----------------------------------------------------- */
/* 新舊旗標/權限對應					 */
/* ----------------------------------------------------- */

struct BITS
{
  int old;
  int new;
};
typedef struct BITS BITS;


#ifdef TRANS_BITS_BRD
static BITS flag[] =
{
  {0x2, BRD_NOZAP},
  {0x8, BRD_ANONYMOUS},
  {0x4, BRD_NOTRAN}
};
#endif


#ifdef TRANS_BITS_PERM
static BITS perm[] =
{
  {0x00000001, PERM_BASIC},    /* BASIC */
  {0x00000002, PERM_CHAT},     /* CHAT */
  {0x00000004, PERM_PAGE},     /* PAGE */
  {0x00000008, PERM_POST},     /* POST */
  {0x00000010, PERM_VALID},    /* LOGIN */
  {0x00000020, PERM_DENYPOST}, /* DENYPOST */
  {0x00000040, PERM_CLOAK},    /* CLOAK */
  {0x00000080, PERM_SEECLOAK}, /* SEECLOAK */
  {0x00000100, PERM_XEMPT},    /* XEMPT */
  {0x00000400, PERM_BM},       /* BM */
  {0x00000800, PERM_ACCOUNTS}, /* ACCOUNTS */
  {0x00001000, PERM_CHATROOM}, /* CHATROOM */
  {0x00002000, PERM_BOARD},    /* BOARD */
  {0x00004000, PERM_SYSOP}     /* SYSOP */
};
#endif


/* ----------------------------------------------------- */
/* old .PASSWDS struct : 512 bytes			 */
/* ----------------------------------------------------- */

struct userec
{				/* Structure used to hold information in */
  char userid[15];
  time_t firstlogin;
  char lasthost[16];
  unsigned int numlogins;
  unsigned int numposts;
  char flags[2];
  char passwd[14];		/* 若是 MD5，要將 14 改成 35 */
  char username[40];
  char ident[40];
  char termtype[16];
  char reginfo[80 - 16];
  unsigned int userlevel;
  time_t lastlogin;
  time_t stay;
  char realname[40];
  char address[80];
  char email[80 - 12];
  unsigned int nummails;
  time_t lastjustify;
  char gender;
  unsigned char birthyear;
  unsigned char birthmonth;
  unsigned char birthday;
  int signature;
  unsigned int userdefine;
  time_t notedate;
  int noteline;
};
typedef struct userec userec;


/* ----------------------------------------------------- */
/* old DIR of board struct : 256 bytes			 */
/* ----------------------------------------------------- */

struct fileheader
{				/* This structure is used to hold data in */
  char filename[80];		/* the DIR files */
  char owner[80];
  char title[80];
  unsigned level;
  unsigned char accessed[12];	/* struct size = 256 bytes */
};
typedef struct fileheader fileheader;


/* ----------------------------------------------------- */
/* old BOARDS struct : 276 bytes			 */
/* ----------------------------------------------------- */

struct boardheader
{                               /* This structure is used to hold data i n */
  char filename[80];		/* the BOARDS files */
  char owner[80 - 60];
  char BM[80 - 1];
  char flag;
  char title[80];
  unsigned level;
  unsigned char accessed[12];
};
typedef struct boardheader boardheader;


/* ----------------------------------------------------- */
/* old FRIEND struct : 128 bytes			 */
/* ----------------------------------------------------- */

struct FRIEND
{
  char id[13];
  char exp[40];
};
typedef struct FRIEND FRIEND;
