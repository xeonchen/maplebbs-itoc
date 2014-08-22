/*-------------------------------------------------------*/
/* util/windtop.h					 */
/*-------------------------------------------------------*/
/* target : WindTop 至 Maple 轉換			 */
/* create : 03/06/30					 */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#if 0

  0. 保留原來 brd/ gem/ usr/ .USR，其餘換成新版的

  1. 設定 FN_BOARD

  2. 利用 windtop2brd 轉換 .BRD

  3. 利用 windtop2usr 轉換 .ACCT

  4. 利用 windtop2mf 轉換 MF

  5. 利用 windtop2pip 轉換 chicken

  6. 將新版的 gem/@/ 下的這些檔案複製過來
     @apply @e-mail @goodbye @index @justify @newuser @opening.0
     @opening.1 @opening.2 @post @re-reg @tryout @welcome
            
  7. 上 BBS 站，在 (A)nnounce 裡面，建以下二個卷宗的所有資料
     {話題} 熱門討論
     {排行} 統計資料

#endif


#include "bbs.h"


#define FN_BOARD	"/tmp/.BRD"	/* WindTop BBS 的 .BRD */


/* ----------------------------------------------------- */
/* old ACCT struct					 */
/* ----------------------------------------------------- */


typedef struct
{
  int userno;			/* unique positive code */
  char userid[13];		/* userid */
  char passwd[14];;		/* user password crypt by DES */
  uschar signature;		/* user signature number */
  char realname[20];		/* user realname */
  char username[24];		/* user nickname */
  usint userlevel;		/* user perm */
  int numlogins;		/* user login times */
  int numposts;			/* user post times */
  usint ufo;			/* user basic flags */
  time_t firstlogin;		/* user first login time */
  time_t lastlogin;		/* user last login time */
  time_t staytime;		/* user total stay time */
  time_t tcheck;		/* time to check mbox/pal */
  char lasthost[32];		/* user last login remote host */
  int numemail;			/* 寄發 Inetrnet E-mail 次數 */
  time_t tvalid;		/* 通過認證、更改 mail address 的時間 */
  char email[60];		/* user email */
  char address[60];		/* user address */
  char justify[60];		/* FROM of replied justify mail */
  char vmail[60];		/* 通過認證之 email */
  time_t deny;			/* user violatelaw time */
  int request;			/* 點歌系統 */
  usint ufo2;			/* 延伸的個人設定 */
  char ident[108];		/* user remote host ident */
  time_t vtime;			/* validate time */
}	userec;


/* ----------------------------------------------------- */
/* old BRD struct					 */
/* ----------------------------------------------------- */


typedef struct
{
  char brdname[13];		/* board ID */
  char title[43];
  char color;
  char class[5];
  char BM[37];			/* BMs' uid, token '/' */

  uschar bvote;			/* 共有幾項投票舉行中 */

  time_t bstamp;		/* 建立看板的時間, unique */
  usint readlevel;		/* 閱讀文章的權限 */
  usint postlevel;		/* 發表文章的權限 */
  usint battr;			/* 看板屬性 */
  time_t btime;			/* .DIR 的 st_mtime */
  int bpost;			/* 共有幾篇 post */
  time_t blast;			/* 最後一篇 post 的時間 */
  usint expiremax;		/* Expire Max Post */
  usint expiremin;		/* Expire Min Post */
  usint expireday;		/* Expire old Post */
  usint n_reads;		/* 看板閱讀累計 times/hour */
  usint n_posts;		/* 看板發表累計 times/hour */
  usint n_news;			/* 看板轉信累計 times/hour */
  usint n_bans;			/* 看板檔信累計 times/hour */
  char  reserve[100];		/* 保留未用 */
}	boardheader;


/* ----------------------------------------------------- */
/* old BRD battr					 */
/* ----------------------------------------------------- */


#define BATTR_NOZAP       0x0001  /* 不可 zap */
#define BATTR_NOTRAN      0x0002  /* 不轉信 */
#define BATTR_NOCOUNT     0x0004  /* 不計文章發表篇數 */
#define BATTR_NOSTAT      0x0008  /* 不納入熱門話題統計 */
#define BATTR_NOVOTE      0x0010  /* 不公佈投票結果於 [sysop] 板 */
#define BATTR_ANONYMOUS   0x0020  /* 匿名看板 */
#define BATTR_NOFORWARD   0x0040  /* lkchu.981201: 不可轉寄 */
#define BATTR_LOGEMAIL    0x0080  /* 自動附加e-mail */
#define BATTR_NOBAN       0x0100  /* 不擋信 */
#define BATTR_NOLOG       0x0200  /* 不紀錄站內違法 */
#define BATTR_NOCNTCROSS  0x0400  /* 不紀錄 cross post */
#define BATTR_NOREPLY     0x0800  /* 不能回文章 */
#define BATTR_NOLOGREAD   0x1000  /* 不紀錄看版閱讀率 */
#define BATTR_CHECKWATER  0x2000  /* 紀錄灌水次數 */
#define BATTR_CHANGETITLE 0x4000  /* 版主修改版名 */
#define BATTR_MODIFY      0x8000  /* 使用者修改文章 */
#define BATTR_PRH         0x10000 /* 推薦文章 */
#define BATTR_NOTOTAL     0x20000 /* 不統計看板使用紀錄 */


/* ----------------------------------------------------- */
/* old MF struct					 */
/* ----------------------------------------------------- */

typedef struct
{
  time_t chrono;                /* timestamp */
  int xmode;

  int xid;                      /* reserved 保留*/

  char xname[31];               /* 檔案名稱 */
  signed char pushscore;
  char owner[80];               /* 作者 (E-mail address) */
  char nick[50];                /* 暱稱 */

  char date[9];                 /* [96/12/01] */
  char title[73];               /* 主題 (TTLEN + 1) */
}   mfheader;


/* ----------------------------------------------------- */
/* old MF file						 */
/* ----------------------------------------------------- */

#define FN_OLD_FAVORITE		"favorite"
#define FN_OLDIMG_FAVORITE	"favorite.img"
