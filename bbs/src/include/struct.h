/*-------------------------------------------------------*/
/* struct.h	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : all definitions about data structure	 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/


#ifndef _STRUCT_H_
#define _STRUCT_H_


/* screen control */

#define STRLEN		80	/* Length of most string data */
#define ANSILINELEN	500	/* Maximum Screen width in chars，不能超過 1023 */

#define SCR_WIDTH	78	/* edit/talk/camera screen width */

#define TAB_STOP	4	/* 按 TAB 換成幾格空白 (要是 2 的次方) */
#define TAB_WIDTH	(TAB_STOP - 1)

#define T_LINES		50	/* maximum total lines */
#define T_COLS		120	/* maximum total columns，要比 ANSILINELEN 小 */

/* board structure */

#define BNLEN		12	/* Length of board name */
#define BCLEN		4	/* Length of board class */
#define BTLEN		43	/* Length of board title */
#define BMLEN		36	/* Length of board managers */

/* file structure */

#define TTLEN		72	/* Length of title */
#define FNLEN		28	/* Length of filename  */

/* user structure */
/* DES 編碼法，接受 8 個字元長度的字串，產生的鍵值為 13 位元組 */

#define IDLEN		12	/* Length of user id */
#define PASSLEN		13	/* Length of encrypted passwd field */
#define PSWDLEN		8	/* Length of passwd (未加密的長度) */
#define RNLEN		19	/* Length of real name */
#define UNLEN		23	/* Length of user name */


typedef char const *STRING;
typedef unsigned char uschar;	/* length = 1 */
typedef unsigned int usint;	/* length = 4 */
typedef struct UTMP UTMP;


/* ----------------------------------------------------- */
/* 使用者帳號 .ACCT struct : 512 bytes			 */
/* ----------------------------------------------------- */


typedef struct
{
  int userno;			/* unique positive code */

  char userid[IDLEN + 1];	/* ID */
  char passwd[PASSLEN + 1];	/* 密碼 */
  char realname[RNLEN + 1];	/* 真實姓名 */
  char username[UNLEN + 1];	/* 暱稱 */

  usint userlevel;		/* 權限 */
  usint ufo;			/* habit */
  uschar signature;		/* 預設簽名檔 */

  char year;			/* 生日(民國年) */
  char month;			/* 生日(月) */
  char day;			/* 生日(日) */
  char sex;			/* 性別 0:中性 奇數:男性 偶數:女性 */
  int money;			/* 銀幣 */
  int gold;			/* 金幣 */

  int numlogins;		/* 上站次數 */
  int numposts;			/* 發表次數 */
  int numemails;		/* 寄發 Inetrnet E-mail 次數 */

  time_t firstlogin;		/* 第一次上站時間 */
  time_t lastlogin;		/* 上一次上站時間 */
  time_t tcheck;		/* 上次 check 信箱/朋友名單的時間 */
  time_t tvalid;		/* 若停權，停權期滿的時間；
                                   若未停權且通過認證，通過認證的時間；
                                   若未停權且未通過認證，認證函的 time-seed */

  char lasthost[30];		/* 上次登入來源 */
  char email[60];		/* 目前登記的電子信箱 */
}      ACCT;


typedef struct			/* 16 bytes */
{
  time_t uptime;
  char userid[IDLEN];
}      SCHEMA;


#ifdef HAVE_REGISTER_FORM

/* itoc.041025: RFROM 不應出現和 ACCT 一樣但值可能不同的欄位
   RFORM 和 ACCT 唯二相同的欄位是 userno、userid */

typedef struct	/* 註冊表單 (Register From) 256 bytes */
{
  int userno;
  time_t rtime;
  char userid[IDLEN + 1];
  char agent[IDLEN + 1];
  char nouse[20];
  char career[50];
  char address[60];
  char phone[20];
  char reply[72];
}      RFORM;

#endif


#include "hdr.h"


/* ----------------------------------------------------- */
/* control of board vote : 256 bytes			 */
/* ----------------------------------------------------- */


/* itoc.041101.註解: VCH 和 HDR 的 xname 位置、長度要匹配 */

typedef struct VoteControlHeader
{
  time_t chrono;		/* 投票開辦時間 */	/* Thor: 為 key 而且 match HDR chrono */
  time_t bstamp;		/* 看板辨識代碼 */	/* Thor: 為 key */
  time_t vclose;		/* 投票結束時間 */

  char xname[32];		/* 主檔名 */		/* Thor: match HDR 的 xname */
  char date[9];			/* 開始日期 */		/* Thor: match HDR 的 date */
  char cdate[9];		/* 結束日期 */		/* Thor: 只供顯示，不做比較 */  
  char owner[IDLEN + 1];	/* 舉辦人 */
  char title[TTLEN + 1];	/* 投票主題 */

  char vgamble;			/* 是否為賭盤        '$':賭盤  ' ':一般投票 */
  char vsort;			/* 開票結果是否排序  's':排序  ' ':不排序 */
  char vpercent;		/* 是否顯示百分比例  '%':百分  ' ':一般 */
  char vprivate;		/* 是否為私人投票    ')':私人  ' ':公開 */

  int maxblt;			/* 每人可投幾票 */
  int price;			/* 每張賭票的售價 */

  int limitlogins;		/* 限制要登入超過幾次以上的使用者才能投票 */
  int limitposts;		/* 限制要發文超過幾次以上的使用者才能投票 */

  char nouse[88];
}      VCH;


typedef char vitem_t[32];	/* 投票選項 */


typedef struct
{
  char userid[IDLEN + 1];
  char numvotes;		/* 投幾張 */
  usint choice;
}      VLOG;


/* filepath : brd/<board>/.VCH, brd/<board>/@/... */


/* ----------------------------------------------------- */
/* Mail-Queue struct : 256 bytes			 */
/* ----------------------------------------------------- */


typedef struct
{
  time_t mailtime;		/* 寄信時間 */
  char method;
  char sender[IDLEN + 1];
  char username[UNLEN + 1];
  char subject[TTLEN + 1];
  char rcpt[60];
  char filepath[77];
  char *niamod;			/* reverse domain */
}      MailQueue;


#define MQ_JUSTIFY	0x01	/* 身分認證信函 */
#define MQ_ATTACH	0x02	/* 有 attachment 的信函 */


/* ----------------------------------------------------- */
/* PAL : friend struct : 64 bytes			 */
/* ----------------------------------------------------- */


typedef struct
{
  char userid[IDLEN + 1];
  char ftype;
  char ship[46];
  int userno;
}      PAL;

#define	PAL_BAD		0x02	/* 好友 vs 壞人 */


/* ----------------------------------------------------- */
/* structure for call-in message : 100 bytes		 */
/* ----------------------------------------------------- */


typedef struct
{
  time_t btime;
  UTMP *caller;			/* who call-in me ? */
  int sender;			/* calling userno */
  int recver;			/* called userno */
  char userid[IDLEN + 1 + 2];	/* itoc.010529: 保留 2 byte 給廣播符號 > */
  char msg[69];			/* 水球 */
}      BMW;			/* bbs message write */


#ifdef LOGIN_NOTIFY

/* ----------------------------------------------------- */
/* BENZ : 系統協尋名單 : 20 bytes			 */
/* ----------------------------------------------------- */


typedef struct
{
  int userno;
  char userid[IDLEN + 1];
}       BENZ;

#endif	/* LOGIN_NOTIFY */


#ifdef HAVE_ALOHA

/* ----------------------------------------------------- */
/* ALOHA : 上站通知名單 : 20 bytes			 */
/* ----------------------------------------------------- */


typedef struct
{
  char userid[IDLEN + 1];
  int userno;
}      ALOHA;


/* ----------------------------------------------------- */
/* FRIENZ : 上站通知名單 : 100 bytes			 */
/* ----------------------------------------------------- */


/* itoc.041011.註解: 根本沒必要用這麼大 */

typedef struct
{
  int nouse1;
  int nouse2;
  int nouse3;
  int userno;
  char userid[IDLEN + 1];
  char nouse4[71];
}      FRIENZ;

#endif	/* HAVE_ALOHA */


/* ----------------------------------------------------- */
/* PAYCHECK : 支票 : 32 bytes				 */
/* ----------------------------------------------------- */


typedef struct
{
  time_t tissue;		/* 發支票時間 */
  int money;
  int gold;
  char reason[20];		/* "[動作] brdname/userid"，假設 BNLEN、IDLEN 不超過 12 */
}      PAYCHECK;


/* ----------------------------------------------------- */
/* Structure used in UTMP file : 128 bytes		 */
/* ----------------------------------------------------- */


struct UTMP
{
  pid_t pid;			/* process ID */
  int userno;			/* user number */

  int mode;			/* bbsmode */
  usint userlevel;		/* the save as ACCT.userlevel */
  usint ufo;			/* the same as ACCT.ufo */
  usint status;			/* status */

  time_t idle_time;		/* active time for last event */
  u_long in_addr;		/* Internet address */
  int sockport;			/* socket port for talk */
  UTMP *talker;			/* who talk-to me ? */

  BMW *mslot[BMW_PER_USER];

  char userid[IDLEN + 1];	/* user's ID */
  char mateid[IDLEN + 1];	/* partner's ID */
  char username[UNLEN + 1];	/* user's nickname */
  char from[34];		/* remote host */
#ifdef HAVE_BRDMATE
  char reading[BNLEN + 1];	/* reading board */
#endif

  int pal_max;			/* 有幾個朋友 */
  int pal_spool[PAL_MAX];	/* 所有朋友的 userno */

#ifdef BMW_COUNT
  int bmw_count;		/* 記錄中了幾個水球 */
#endif
};


/* ----------------------------------------------------- */
/* BOARDS struct : 128 bytes				 */
/* ----------------------------------------------------- */


typedef struct BoardHeader
{
  char brdname[BNLEN + 1];	/* board name */
  char class[BCLEN + 1];
  char title[BTLEN + 1];
  char BM[BMLEN + 1];		/* BMs' uid, token '/' */

  char bvote;			/* 0:無投票 -1:有賭盤(可能有投票) 1:有投票 */

  time_t bstamp;		/* 建立看板的時間, unique */
  usint readlevel;		/* 閱讀文章的權限 */
  usint postlevel;		/* 發表文章的權限 */
  usint battr;			/* 看板屬性 */
  time_t btime;			/* -1:bpost/blast 需要更新 */
  int bpost;			/* 共有幾篇 post */
  time_t blast;			/* 最後一篇 post 的時間 */
}           BRD;


typedef struct
{
  int pal_max;			/* 有幾個板友 */
  int pal_spool[PAL_MAX];	/* 所有板友的 userno */
}	BPAL;


/* ----------------------------------------------------- */
/* Class image						 */
/* ----------------------------------------------------- */


#define CLASS_INIFILE		"Class"

/* itoc.010413: 把 class.img 分成二個 */
#define CLASS_IMGFILE_NAME	"run/classname.img"
#define CLASS_IMGFILE_TITLE	"run/classtitle.img"


#define CH_MAX		100	/* 最大分類數目 */
#define	CH_END		-1
#define	CH_TTLEN	64


typedef	struct
{
  int count;
  char title[CH_TTLEN];
  short chno[0];
}	ClassHeader;


#ifdef MY_FAVORITE

/* ----------------------------------------------------- */
/* favor.c 中運用的資料結構                              */
/* ----------------------------------------------------- */


typedef struct MF
{
  time_t chrono;		/* 建立時間 */
  int mftype;			/* type */
  char xname[BNLEN + 1];	/* 板名或檔名 */
  char class[BCLEN + 1];	/* 分類 */
  char title[BTLEN + 1];	/* 主題 */
}	MF;

#define	MF_MARK		0x01	/* 被 mark 起來的 */
#define	MF_BOARD    	0x02	/* 看板捷徑 */
#define	MF_FOLDER   	0x04	/* 卷宗 */
#define	MF_GEM      	0x08	/* 精華區捷徑 */
#define MF_LINE		0x10	/* 分隔線 */
#define MF_CLASS	0x20	/* 分類群組 */

#endif  /* MY_FAVORITE */


#ifdef HAVE_COSIGN

/* ----------------------------------------------------- */
/* newbrd.c 中運用的資料結構                             */
/* ----------------------------------------------------- */


typedef struct NewBoardHeader
{
  char brdname[BNLEN + 1];
  char class[BCLEN + 1];
  char title[BTLEN + 1];
  time_t btime;
  time_t etime;
  char xname[32];
  char owner[IDLEN + 1];
  char date[9];
  usint mode;
  int total;
}	NBRD;


#define NBRD_FINISH	0x00001	/* 已結案 */
#define NBRD_END	0x00002	/* 連署結束 */
#define NBRD_START	0x00004	/* 連署開始 */
#define NBRD_ANONYMOUS	0x00100	/* 匿名 */
#define NBRD_NEWBOARD	0x10000	/* 新板連署 */
#define NBRD_OTHER	0x20000	/* 其他連署 */

#endif	/* HAVE_COSIGN */


#ifdef LOG_SONG_USIES

/* ----------------------------------------------------- */
/* SONG log 中運用的資料結構                             */
/* ----------------------------------------------------- */

typedef struct SONGDATA
{
  time_t chrono;
  int count;		/* 被點次數 */
  char title[80];
}	SONGDATA;

#endif	/* LOG_SONG_USIES */


/* ----------------------------------------------------- */
/* cache.c 中運用的資料結構				 */
/* ----------------------------------------------------- */


typedef struct
{
  int shot[MOVIE_MAX];	/* Thor.980805: 合理範圍為 0..MOVIE_MAX - 1 */
  char film[MOVIE_SIZE];
  char today[16];
} FCACHE;


#define	FILM_SIZ	4000	/* max size for each film */


#define FILM_OPENING0	0	/* 開頭畫面(０) */
#define FILM_OPENING1	1	/* 開頭畫面(１) */
#define FILM_OPENING2	2	/* 開頭畫面(２) */
#define FILM_GOODBYE	3	/* 再見畫面 */
#define FILM_NOTIFY	4	/* 尚未通過認證通知 */
#define FILM_MQUOTA	5	/* 信件超過保存期限通知 */
#define FILM_MAILOVER	6	/* 信件封數過多通知 */
#define FILM_MGEMOVER	7	/* 個人精華區過多通知 */
#define FILM_BIRTHDAY	8	/* 生日當天的上站畫面 */
#define FILM_APPLY	9	/* 註冊提示畫面 */
#define FILM_JUSTIFY	10	/* 身份認證的方法 */
#define FILM_REREG	11	/* 重新認證說明 */
#define FILM_EMAIL	12	/* 郵件信箱認證說明 */
#define FILM_NEWUSER	13	/* 新手上路須知 */
#define FILM_TRYOUT	14	/* 密碼錯誤 */
#define FILM_POST	15	/* 文章發表綱領 */
#define FILM_MOVIE	16	/* 動態看板 FILM_MOVIE 要放在最後面 */


typedef struct
{
  UTMP uslot[MAXACTIVE];	/* UTMP slots */
  usint count;			/* number of active session */
  usint offset;			/* offset for last active UTMP */

  double sysload[3];
  int avgload;

  BMW *mbase;			/* sequential pointer for BMW */
  BMW mpool[BMW_MAX];
} UCACHE;


typedef struct
{
  BRD bcache[MAXBOARD];
  BPAL pcache[MAXBOARD];
  int mantime[MAXBOARD];	/* 各板目前正有多少人在閱讀 */
  int number;			/* 全部看板的數目 */
  int numberOld;		/* 剛開站時看板的數目 */
  int min_chn;			/* 記錄總共有幾個分類 */
  time_t uptime;
} BCACHE;


/* ----------------------------------------------------- */
/* visio.c 中運用的資料結構				 */
/* ----------------------------------------------------- */


/* Screen Line buffer modes */


#define SL_MODIFIED	(1)	/* if line has been modifed, screen output */
#define SL_STANDOUT	(2)	/* if this line contains standout code */
#define SL_ANSICODE	(4)	/* if this line contains ANSI code */


typedef struct screenline
{
  int oldlen;			/* previous line length */
  int len;			/* current length of line */
  int width;			/* padding length of ANSI codes */
  int smod;			/* start of modified data */
  int emod;			/* end of modified data */
  int sso;			/* start of standout data */
  int eso;			/* end of standout data */
  uschar mode;			/* status of line, as far as update */	/* 由於 SL_* 的 mode 不超過八個，故用 uschar 即可 */
  uschar data[ANSILINELEN];
}          screenline;


typedef struct LinkList
{
  struct LinkList *next;
  char data[0];
}        LinkList;


/* ----------------------------------------------------- */
/* xover.c 中運用的資料結構				 */
/* ----------------------------------------------------- */


typedef struct OverView
{
  int pos;			/* current position */
  int top;			/* top */
  int max;			/* max */
  int key;			/* key */
  char *xyz;			/* staff */
  struct OverView *nxt;		/* next */
  char dir[0];			/* data path */
}        XO;


typedef struct
{
  int key;
  int (*func) ();
}      KeyFunc;


typedef struct
{
  XO *xo;
  KeyFunc *cb;
  int mode;
  char *feeter;
} XZ;


typedef struct
{
  time_t chrono;
  int recno;
}      TagItem;


/* ----------------------------------------------------- */
/* poststat.c 中運用的資料結構				 */
/* ----------------------------------------------------- */


typedef struct
{
  char author[IDLEN + 1];
  char board[BNLEN + 1];
  char title[66];
  time_t date;		/* last post's date */
  int number;		/* post number */
} POSTLOG;


#ifdef MODE_STAT

/* ----------------------------------------------------- */
/* modestat.c 中運用的資料結構				 */
/* ----------------------------------------------------- */

typedef struct
{
  time_t logtime;
  time_t used_time[M_MAX + 1];	/* itoc.010901: depend on mode.h */
} UMODELOG;


typedef struct
{
  time_t logtime;
  time_t used_time[M_MAX + 1];	/* itoc.010901: depend on mode.h */
  int count[30];
  int usercount;
} MODELOG;

#endif	/* MODE_STAT */


/* ----------------------------------------------------- */
/* innbbsd 中運用的資料結構				 */
/* ----------------------------------------------------- */


typedef struct
{
  time_t chrono;	/* >=0:stamp -1:cancel */
  char board[BNLEN + 1];

  /* 以下欄位的大小與 HDR 相同 */
  char xname[32];
  char owner[80];
  char nick[49];
  char title[TTLEN + 1];
} bntp_t;


#define INN_USEIHAVE	0x0001
#define INN_USEPOST	0x0002
#define INN_FEEDED	0x0010

typedef struct
{
  char name[13];	/* 該站的 short-name */
  char host[83];	/* 該站的 host */
  int port;		/* 該站的 port */
  usint xmode;		/* 該站的 xmode */
  char blank[20];	/* 保留 */
  int feedfd;		/* bbslink.c 使用 */
} nodelist_t;


#define INN_NOINCOME	0x0001
#define INN_ERROR	0x0004

typedef struct
{
  char path[13];	/* 該群組所對轉的站台 */
  char newsgroup[83];	/* 該群組的名稱 */
  char board[BNLEN + 1];/* 該群組所對應的看板 */
  char charset[11];	/* 該群組的字集 */
  usint xmode;		/* 該群組的 xmode */
  int high;		/* 目前抓到該群組的哪一篇 */
} newsfeeds_t;


typedef struct
{
  char issuer[128];	/* NCM message 的發信人 */
  char type[64];	/* NCM message 的種類名稱 */
  int perm;		/* 允許此 NCM message 刪信 (1:開 0:關) */
  char blank[60];	/* 保留 */
} ncmperm_t;


#define INN_SPAMADDR	0x0001
#define INN_SPAMNICK	0x0002
#define INN_SPAMSUBJECT	0x0010
#define INN_SPAMPATH	0x0020
#define INN_SPAMMSGID	0x0040
#define INN_SPAMBODY	0x0100
#define INN_SPAMSITE	0x0200
#define INN_SPAMPOSTHOST 0x0400

typedef struct
{
  char detail[80];	/* 該規則的 內容 */
  usint xmode;		/* 該規則的 xmode */
  char board[BNLEN + 1];/* 該規則適用的看板 */
  char path[13];	/* 該規則適用的站台 */
  char blank[18];	/* 保留 */
} spamrule_t;

#endif				/* _STRUCT_H_ */
