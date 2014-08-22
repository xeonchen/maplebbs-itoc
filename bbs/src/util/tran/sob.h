/*-------------------------------------------------------*/
/* util/sob.h 	                                         */
/*-------------------------------------------------------*/
/* target : SOB 至 Maple 3.02 轉換			 */
/* create : 02/10/26					 */
/* author : ernie@micro8.ee.nthu.edu.tw                  */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#if 0

  1. 設定 OLD_BBSHOME、FN_PASSWD、FN_BOARD
  2. 修改所有的 old struct

  3. 必須在 brd 轉完才可以轉換 gem
  4. 必須在 usr 及 brd 都轉完才可以轉換 pal
  5. 建議轉換順序為 usr -> brd -> gem ->pal

#endif


#include "bbs.h"


#define OLD_BBSHOME     "/home/oldbbs"		/* SOB */
#define FN_PASSWD	"/home/oldbbs/.PASSWDS"	/* SOB */
#define FN_BOARD        "/home/oldbbs/.BOARDS"	/* SOB */


#undef	HAVE_PERSONAL_GEM			/* SOB 是沒有個人精華區的 */


/* ----------------------------------------------------- */
/* old .PASSWDS struct : 256 bytes                       */
/* ----------------------------------------------------- */

struct userec
{
  char userid[13];
  char realname[20];
  char username[24];
  char passwd[14];
  uschar uflag;
  usint userlevel;
  unsigned short numlogins;
  unsigned short numposts;
  time_t firstlogin;
  time_t lastlogin;
  char lasthost[16];
  char remoteuser[8];
  char email[50];
  char address[50];
  char justify[39];
  uschar month;
  uschar day;
  uschar year;
  uschar sex;
  uschar state;
};
typedef struct userec userec;


/* ----------------------------------------------------- */
/* old DIR of board struct : 128 bytes                   */
/* ----------------------------------------------------- */

struct fileheader
{
  char filename[33];		/* M.9876543210.A */
  char savemode;		/* file save mode */
  char owner[14];		/* uid[.] */
  char date[6];			/* [02/02] or space(5) */
  char title[73];
  uschar filemode;		/* must be last field @ boards.c */
};
typedef struct fileheader fileheader;


/* ----------------------------------------------------- */
/* old BOARDS struct : 128 bytes                         */
/* ----------------------------------------------------- */

struct boardheader
{
  char brdname[13];		/* bid */
  char title[49];
  char BM[39];			/* BMs' uid, token '/' */
  char pad[11];
  time_t bupdate;		/* note update time */
  char pad2[3];
  uschar bvote;			/* Vote flags */
  time_t vtime;			/* Vote close time */
  usint level;
};
typedef struct boardheader boardheader;
