/*-------------------------------------------------------*/
/* util/cola.h						 */
/*-------------------------------------------------------*/
/* target : Cola 至 Maple 3.02 轉換			 */
/* create : 03/02/11                                     */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#if 0

  0. 請先於 Cola 把所有看板英文名字改在 12 字以內。

  1. 必須在 brd 轉完才可以轉換 gem
  2. 建議轉換順序為 usr -> brd -> gem -> post

  3. 設定 COLABBS_HOME、COLABBS_BOARDS、COLABBS_MAN、FN_BOARD

#endif


#include "bbs.h"


#define	COLABBS_HOME	"/tmp/home"		/* 舊的 Cola BBS 的使用者目錄 */
#define COLABBS_BOARDS	"/tmp/boards"		/* 舊的 Cola BBS 的看板目錄 */
#define COLABBS_MAN	"/tmp/man"		/* 舊的 Cola BBS 的精華區目錄 */
#define FN_BOARD	"/tmp/.boards"		/* 舊的 Cola BBS 的 .boards */


/* ----------------------------------------------------- */
/* old .PASSWDS struct : 512 bytes                       */
/* ----------------------------------------------------- */

/* itoc.030211: 只轉換 userid 和 passwd */
typedef struct
{
  char userid[13];
  char blank1;
  char passwd[14];
  char username[40];
  char realname[80];
  char blank2[364];
} userec;


/* ----------------------------------------------------- */
/* old DIR of board struct : 128 bytes                   */
/* ----------------------------------------------------- */

typedef struct
{
  char filename[34];		/* M.9876543210.A */
  char blank1[46];
  char owner[14];		/* userid[.] */
  char blank2[57];
  char date[6];			/* [08/27] or space(5) */
  char title[74];
  char blank3[25];
} fileheader;


/* ----------------------------------------------------- */
/* old BOARDS struct : 512 bytes                         */
/* ----------------------------------------------------- */

/* itoc.030211: 只轉換 brdname 和 title */
typedef struct
{
  char brdname[13];
  char blank1[147];
  char title[96];
} boardheader;
