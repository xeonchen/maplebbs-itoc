/*-------------------------------------------------------*/
/* util/snap.h						 */
/*-------------------------------------------------------*/
/* target : Maple 轉換					 */
/* create : 98/12/15					 */
/* update : 02/04/29					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#if 0

  0. 保留原來 brd/ gem/ usr/ .USR，其餘換成新版的

  1. 利用 snap2brd 轉換 .BRD

  2. 利用 snap2usr 轉換 .ACCT

  3. 將新版的 gem/@/ 下的這些檔案複製過來
     @apply @e-mail @goodbye @index @justify @newuser @opening.0
     @opening.1 @opening.2 @post @re-reg @tryout @welcome

  4. 上 BBS 站，在 (A)nnounce 裡面，建以下二個卷宗的所有資料
     {話題} 熱門討論
     {排行} 統計資料

#endif


#include "bbs.h"

#define	MAK_DIRS	/* 建目錄 MF/ 及 gem/ */


/* ----------------------------------------------------- */
/* (舊的) 使用者帳號 .ACCT struct			 */
/* ----------------------------------------------------- */


typedef struct			/* 要和舊版程式 struct 一樣 */
{
  int userno;			/* unique positive code */
  char userid[13];
  char passwd[14];
  uschar signature;
  char realname[20];
  char username[24];
  usint userlevel;
  int numlogins;
  int numposts;
  usint ufo;
  time_t firstlogin;
  time_t lastlogin;
  time_t staytime;		/* 總共停留時間 */
  time_t tcheck;		/* time to check mbox/pal */
  char lasthost[32];
  int numemail;			/* 寄發 Inetrnet E-mail 次數 */
  time_t tvalid;		/* 通過認證、更改 mail address 的時間 */
  char email[60];
  char address[60];
  char justify[60];		/* FROM of replied justify mail */
  char vmail[60];		/* 通過認證之 email */
  char ident[140 - 20];
  time_t vtime;			/* validate time */
}	userec;


/* ----------------------------------------------------- */
/* (舊的) 使用者習慣 ufo				 */
/* ----------------------------------------------------- */

/* old UFO */			/* 要和舊版程式 struct 一樣 */

#define HABIT_COLOR       BFLAG(0)        /* true if the ANSI color mode open */
#define HABIT_MOVIE       BFLAG(1)        /* true if show movie */
#define HABIT_BRDNEW      BFLAG(2)        /* 新文章模式 */
#define HABIT_BNOTE       BFLAG(3)        /* 顯示進板畫面 */
#define HABIT_VEDIT       BFLAG(4)        /* 簡化編輯器 */
#define HABIT_PAGER       BFLAG(5)        /* 關閉呼叫器 */
#define HABIT_QUIET       BFLAG(6)        /* 結廬在人境，而無車馬喧 */
#define HABIT_PAL         BFLAG(7)        /* true if show pals only */
#define HABIT_ALOHA       BFLAG(8)        /* 上站時主動通知好友 */
#define HABIT_MOTD        BFLAG(9)        /* 簡化進站畫面 */
#define HABIT_CLOAK       BFLAG(19)       /* true if cloak was ON */
#define HABIT_ACL         BFLAG(20)       /* true if ACL was ON */
#define HABIT_MPAGER      BFLAG(10)       /* lkchu.990428: 電子郵件傳呼 */
#define HABIT_NWLOG       BFLAG(11)       /* lkchu.990510: 不存對話紀錄 */
#define HABIT_NTLOG       BFLAG(12)       /* lkchu.990510: 不存聊天紀錄 */


/* ----------------------------------------------------- */
/* (舊的) BOARDS struct					 */
/* ----------------------------------------------------- */

typedef struct
{
  char brdname[13];		/* board ID */
  char title[49];
  char BM[37];			/* BMs' uid, token '/' */

  uschar bvote;			/* 共有幾項投票舉行中 */

  time_t bstamp;		/* 建立看板的時間, unique */
  usint readlevel;		/* 閱讀文章的權限 */
  usint postlevel;		/* 發表文章的權限 */
  usint battr;			/* 看板屬性 */
  time_t btime;			/* .DIR 的 st_mtime */
  int bpost;			/* 共有幾篇 post */
  time_t blast;			/* 最後一篇 post 的時間 */
}	boardheader;
