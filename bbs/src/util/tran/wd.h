/*-------------------------------------------------------*/
/* util/wd.h 	                                         */
/*-------------------------------------------------------*/
/* target : WD 至 Maple 3.02 轉換			 */
/* create : 02/01/03                                     */
/* author : ernie@micro8.ee.nthu.edu.tw                  */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#if 0

  1. 設定 OLD_BBSHOME、FN_PASSWD、FN_BOARD
  2. 修改所有的 old struct

  3. 必須在 brd 轉完才可以轉換 gem
  4. 必須在 usr 及 brd 都轉完才可以轉換 mf
  5. 必須在 usr 及 brd 都轉完才可以轉換 pal
  6. 必須在 usr 轉完才可以轉換 bmw pip list
  7. 建議轉換順序為 usr -> brd -> gem -> mf -> pal -> bmw -> pip -> list

#endif


#include "bbs.h"


#define OLD_BBSHOME     "/home/oldbbs"		/* WD */
#define FN_PASSWD	"/home/oldbbs/.PASSWDS"	/* WD */
#define FN_BOARD        "/home/oldbbs/.BOARDS"	/* WD */


#define	HAVE_PERSONAL_GEM			/* WD 是有個人精華區的 */


/* ----------------------------------------------------- */
/* old .PASSWDS struct : 512 bytes                       */
/* ----------------------------------------------------- */

struct userec
{
  char userid[13];                /* 使用者名稱  13 bytes */
  char realname[20];              /* 真實姓名    20 bytes */
  char username[24];              /* 暱稱        24 bytes */
  char passwd[14];                /* 密碼        14 bytes */
  uschar uflag;                   /* 使用者選項   1 byte  */
  usint userlevel;                /* 使用者權限   4 bytes */
  ushort numlogins;               /* 上站次數     2 bytes */
  ushort numposts;                /* POST次數     2 bytes */
  time_t firstlogin;              /* 註冊時間     4 bytes */
  time_t lastlogin;               /* 前次上站     4 bytes */
  char lasthost[24];              /* 上站地點    24 bytes */
  char vhost[24];                 /* 虛擬網址    24 bytes */
  char email[50];                 /* E-MAIL      50 bytes */
  char address[50];               /* 地址        50 bytes */
  char justify[39];               /* 註冊資料    39 bytes */
  uschar month;                   /* 出生月份     1 byte  */
  uschar day;                     /* 出生日期     1 byte  */
  uschar year;                    /* 出生年份     1 byte  */
  uschar sex;                     /* 性別         1 byte  */
  uschar state;                   /* 狀態??       1 byte  */
  usint habit;                    /* 喜好設定     4 bytes */
  uschar pager;                   /* 心情顏色     1 bytes */
  uschar invisible;               /* 隱身模式     1 bytes */
  usint exmailbox;                /* 信箱封數     4 bytes */
  usint exmailboxk;               /* 信箱K數      4 bytes */
  usint toquery;                  /* 好奇度       4 bytes */
  usint bequery;                  /* 人氣度       4 bytes */
  char toqid[13];	          /* 前次查誰    13 bytes */
  char beqid[13];                 /* 前次被誰查  13 bytes */
  unsigned long int totaltime;    /* 上線總時數   8 bytes */
  usint sendmsg;                  /* 發訊息次數   4 bytes */
  usint receivemsg;               /* 收訊息次數   4 bytes */
  unsigned long int goldmoney;    /* 風塵金幣     8 bytes */
  unsigned long int silvermoney;  /* 銀幣         8 bytes */
  unsigned long int exp;          /* 經驗值       8 bytes */
  time_t dtime;                   /* 存款時間     4 bytes */
  int limitmoney;                 /* 金錢下限     4 bytes */
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
/* old BOARDS struct : 512 bytes                         */
/* ----------------------------------------------------- */

struct boardheader
{
  char brdname[13];             /* 看板英文名稱    13 bytes */
  char title[49];               /* 看板中文名稱    49 bytes */
  char BM[39];                  /* 板主ID和"/"     39 bytes */
  usint brdattr;                /* 看板的屬性       4 bytes */
  time_t bupdate;               /* note update time 4 bytes */
  uschar bvote;                 /* Vote flags       1 bytes */
  time_t vtime;                 /* Vote close time  4 bytes */
  usint level;                  /* 可以看此板的權限 4 bytes */
  unsigned long int totalvisit; /* 總拜訪人數       8 bytes */
  unsigned long int totaltime;  /* 總停留時間       8 bytes */
  char lastvisit[13];           /* 最後看該板的人  13 bytes */
  time_t opentime;              /* 開板時間         4 bytes */
  time_t lastime;               /* 最後拜訪時間     4 bytes */
  char passwd[14];              /* 密碼            14 bytes */
  unsigned long int postotal;   /* 總水量 :p        8 bytes */
  usint maxpost;                /* 文章上限         4 bytes */
  usint maxtime;                /* 文章保留時間     4 bytes */
  char desc[3][80];             /* 中文描述      240 bytes */
  char pad[87];
};
typedef struct boardheader boardheader;


/* ----------------------------------------------------- */
/* old FRIEND struct : 128 bytes                         */
/* ----------------------------------------------------- */

struct FRIEND
{
  char userid[33];              /* list name/userid */
  char savemode;
  char owner[14];               /* bbcall */
  char date[6];                 /* birthday */
  char desc[73];                /* list/user description */
  uschar ftype;                 /* mode:  PAL, BAD */
};
typedef struct FRIEND FRIEND;
