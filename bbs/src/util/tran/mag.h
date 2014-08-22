/*-------------------------------------------------------*/
/* util/mag.h						 */
/*-------------------------------------------------------*/
/* target : Magic 至 Maple 3.02 使用者轉換		 */
/* create : 02/01/03					 */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#if 0

  1. 設定 FN_PASSWD、FN_BOARD、OLD_MAILPATH、OLD_BOARDPATH、OLD_MANPATH
  2. 修改所有的 old struct

  3. 把 舊站的 mail 移到 OLD_MAILPATH
  4. 把 舊站的 boards/brdname 移到 OLD_BOARDPATH/brdname
  5. 把 舊站的 0Announce/groups/ooxx.faq/brdname 移到 OLD_MANPATH/brdname

  6. 必須在 brd 轉完才可以轉換 gem
  7. 建議轉換順序為 usr -> brd -> gem

#endif

      
#include "bbs.h"


#define FN_PASSWDS      "/home/oldbbs/.PASSWDS"
#define FN_BOARDS       "/home/oldbbs/.BOARDS"
#define OLD_MAILPATH    "/home/oldbbs/mail"
#define OLD_BOARDPATH   "/home/oldbbs/boards"
#define OLD_MANPATH     "/home/oldbbs/groups"


/* ----------------------------------------------------- */
/* old .PASSWDS struct : 512 bytes                       */
/* ----------------------------------------------------- */


typedef struct
{
  char userid[14];
  time_t firstlogin;
  char termtype[16];
  unsigned int numlogins;
  unsigned int numposts;
  char flags[2];
  char passwd[14];
  char username[40];
  char ident[40];
  char lasthost[40];
  char realemail[40];
  unsigned userlevel;
  time_t lastlogin;
  time_t stay;
  char realname[40];
  char address[80];
  char email[80];
  int signature;
  unsigned int userdefine;
  int editor;
  unsigned int showfile;
  int magic;
  int addmagic;
  uschar bmonth;
  uschar bday;
  uschar byear;
  uschar sex;
  int money;
  int bank;
  int lent;

  int card;
  uschar mind;
  int unused1;
  usint unsign_0000;
  usint unsign_ffff;
}	userec;


/* ----------------------------------------------------- */
/* old DIR of board struct : 256 bytes			 */
/* ----------------------------------------------------- */


typedef struct		/* the DIR files */
{
  char filename[80];
  char owner[80];
  char title[80];
  unsigned level;
  unsigned char accessed[12];
}	fileheader;


/* ----------------------------------------------------- */
/* old BOARDS struct : 256 bytes			 */
/* ----------------------------------------------------- */


typedef struct		/* the BOARDS files */
{
  char filename[80];
  char owner[60];
  char BM[19];
  char flag;
  char title[80];
  unsigned level;
  unsigned char accessed[12];
}	boardheader;
