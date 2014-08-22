/*-------------------------------------------------------*/
/* modes.h	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : user operating mode & status		 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/


#ifndef _MODES_H_
#define _MODES_H_


/* ----------------------------------------------------- */
/* user 操作狀態與模式					 */
/* ----------------------------------------------------- */

/* itoc.010329.註解: 把 0 保留，拿來做特殊用途 */

#define M_0MENU		1	/*  main MENU */
#define M_AMENU		2	/* admin MENU */
#define M_MMENU		3	/*  mail MENU */
#define M_TMENU		4	/*  talk MENU */
#define M_UMENU		5	/*  user MENU */
#define M_XMENU		6	/*  tool MENU */
				/* M_XMENU 是 menu 的最後一個 */
				/* M_XMENU 之前有動態看板 */

#define M_LOGIN		7	/* login */

#define M_GEM		8	/* announce */
#define M_BOARD		9	/* board list */
#define M_MF		10	/* my favorite */
#define M_READA		11	/* read article */
#define M_RMAIL		12	/* read mail */

#define M_PAL		13	/* set pal/aloha list */
#define M_LUSERS	14	/* user list */
#define M_VOTE		15
#define M_BMW           16
#define M_SONG		17
#define M_COSIGN	18

#define M_SYSTEM	19	/* → M_SYSTEM(含) 與 M_CHAT(含) 間不接受 talk request */

#define M_XFILES        20	/* admin set system files */
#define M_UFILES        21	/* user set user files  */

#define M_BMW_REPLY     22
#define M_GAME		23
#define M_POST		24
#define M_SMAIL		25
#define M_TRQST		26

#define M_TALK		27	/* ← M_TALK(含) 與 M_IDLE(含) 間接 mateid */
#define M_CHAT		28	/* → M_BBTP(含) 與 M_CHAT(含) 間不接受 talk request */
#define M_PAGE		29
#define M_QUERY		30
#define M_IDLE		31	/* ← M_TALK(含) 與 M_IDLE(含) 間接 mateid */

#define M_XMODE		32
#define M_MAX           M_XMODE


#ifdef	_MODES_C_
static char *ModeTypeTable[] =
{
  "保留",

  "主選單",			/* M_0MENU */
  "系統維護",			/* M_AMENU */
  "郵件選單",			/* M_MMENU */
  "交談選單",			/* M_TMENU */
  "使用者選單",			/* M_UMENU */
  "Xyz 選單",			/* M_XMENU */

  "上站途中",			/* M_LOGIN */

  "公佈欄",			/* M_GEM */
  "看板列表",			/* M_BOARD */
  "我的最愛",			/* M_MF */
  "閱\讀文章",			/* M_READA */
  "讀信",			/* M_RMAIL */

  "結交朋友",			/* M_PAL */
  "使用者名單",			/* M_LUSERS */
  "投票中",			/* M_VOTE */
  "察看水球",                   /* M_BMW */
  "點歌",			/* M_SONG */
  "看板連署",			/* M_COSIGN */

  "系統管理",			/* M_SYSTEM */

  "編系統檔案",                 /* M_XFILES */
  "編個人檔案",                 /* M_UFILES */

  "水球準備中",			/* M_BMW_REPLY */
  "玩遊戲",			/* M_GAME */
  "發表文章",			/* M_POST */
  "寫信",			/* M_SMAIL */
  "待機",			/* M_TRQST */

  "交談",			/* M_TALK */	/* 接 mateid 的動態中文字不可太長 */
  "聊天",			/* M_CHAT */
  "呼叫",			/* M_PAGE */
  "查詢",			/* M_QUERY */
  "發呆",			/* M_IDLE */

  "其他"			/* M_XMODE */
};
#endif				/* _MODES_C_ */


/* ----------------------------------------------------- */
/* menu.c 中的模式					 */
/* ----------------------------------------------------- */


#define XEASY	0x333		/* Return value to un-redraw screen */


/* ----------------------------------------------------- */
/* pal.c 中的模式					 */
/* ----------------------------------------------------- */


#define PALTYPE_PAL	0		/* 朋友名單 */
#define PALTYPE_LIST	1		/* 群組名單 */
#define PALTYPE_BPAL	2		/* 板友名單 */
#define PALTYPE_VOTE	3		/* 限制投票名單 */


/* ----------------------------------------------------- */
/* visio.c 中的模式					 */
/* ----------------------------------------------------- */


/* Flags to getdata input function */

#define NOECHO		0x0000		/* 不顯示，用於密碼取得 */
#define DOECHO		0x0100		/* 一般顯示 */
#define LCECHO		0x0200		/* low case echo，換成小寫 */
#define GCARRY		0x0400		/* 會顯示上一次/目前的值 */

#define GET_LIST	0x1000		/* 取得 Link List */
#define GET_USER	0x2000		/* 取得 user id */
#define GET_BRD		0x4000		/* 取得 board id */


/* ----------------------------------------------------- */
/* read.c 中的模式					 */
/* ----------------------------------------------------- */

/* for tag */

#define	TAG_NIN		0		/* 不屬於 TagList */
#define	TAG_TOGGLE	1		/* 切換 Taglist */
#define	TAG_INSERT	2		/* 加入 TagList */


/* for bbstate : bbs operating state */

#define	STAT_POST	0x0010000	/* 是否可以在 currboard 發表文章 */
#define STAT_BOARD	0x0020000	/* 是否可以在 currboard 刪除、mark文章 (板主、看板總管) */
#define STAT_BM		0x0040000	/* 是否為 currboard 的板主 */
#define	STAT_LOCK	0x0100000	/* 是否為鎖定螢幕 */
#define	STAT_STARTED	0x8000000	/* 是否已經進入系統 */


/* for user's board permission level & state record */

#define BRD_L_BIT	0x0001		/* 可列，list */
#define BRD_R_BIT	0x0002		/* 可讀，read */
#define BRD_W_BIT	0x0004		/* 可寫，write */
#define BRD_X_BIT	0x0008		/* 可管，execute，板主、看板總管 */
#define BRD_M_BIT	0x0010		/* 可理，manage，板主 */

#define BRD_V_BIT	0x0020		/* 已經逛過了，visit ==> 看過「進板畫面」 */
#define BRD_H_BIT	0x0040		/* .BRH 中有閱讀記錄 (history) */
#define BRD_Z_BIT	0x0080		/* .BRH zap 掉了 */


/* for user's gem permission level record */

#define GEM_W_BIT	0x0001		/* 可寫，write，板主，看板總管 */
#define GEM_X_BIT	0x0002		/* 可管，execute，站長 */
#define GEM_M_BIT	0x0004		/* 可理，manage，板主 */


/* for curredit */

#define EDIT_MAIL	0x0001		/* 目前是 mail/board ? */
#define EDIT_LIST	0x0002		/* 是否為 mail list ? */
#define EDIT_BOTH	0x0004		/* both reply to author/board ? */
#define EDIT_OUTGO	0x0008		/* 待轉信出去 */
#define EDIT_ANONYMOUS	0x0010		/* 匿名模式 */
#define EDIT_RESTRICT	0x0020		/* 加密存檔 */


/* ----------------------------------------------------- */
/* xover.c 中的模式					 */
/* ----------------------------------------------------- */


#define XO_DL		0x80000000

#define	XO_MODE		0x10000000


#define XO_NONE		(XO_MODE + 0)
#define XO_INIT		(XO_MODE + 1)
#define XO_LOAD		(XO_MODE + 2)
#define XO_HEAD		(XO_MODE + 3)
#define XO_NECK		(XO_MODE + 4)
#define XO_BODY		(XO_MODE + 5)
#define XO_FOOT		(XO_MODE + 6)	/* itoc.註解: 清除 b_lines */
#define XO_LAST		(XO_MODE + 7)
#define	XO_QUIT		(XO_MODE + 8)


#define	XO_RSIZ		256		/* max record length */
#define XO_TALL		(b_lines - 3)	/* page size = b_lines - 3 (扣去 head/neck/foot 共三行) */


#define	XO_MOVE		0x20000000	/* cursor movement */
#define	XO_WRAP		0x08000000	/* cursor wrap in movement */
#define	XO_TAIL		(XO_MOVE - 999)	/* init cursor to tail */


#define	XO_ZONE		0x40000000	/* 進入某一個 zone */
#define	XZ_BACK		0x100


#define	XZ_CLASS	(XO_ZONE + 0)	/* 看板列表 */
#define	XZ_ULIST	(XO_ZONE + 1)	/* 線上使用者名單 */
#define	XZ_PAL		(XO_ZONE + 2)	/* 朋友名單 */
#define XZ_ALOHA	(XO_ZONE + 3)	/* 上站通知名單 */ 
#define	XZ_VOTE		(XO_ZONE + 4)	/* 投票 */
#define XZ_BMW		(XO_ZONE + 5)	/* 水球 */
#define	XZ_MF		(XO_ZONE + 6)	/* 我的最愛 */
#define XZ_COSIGN	(XO_ZONE + 7)	/* 連署 */
#define	XZ_SONG		(XO_ZONE + 8)	/* 點歌 */
#define XZ_NEWS		(XO_ZONE + 9)	/* 新聞閱讀模式 */

/* 以下的有 thread 主題式閱讀的功能 */
/* 以下的有 tag 功能 */

#define XZ_XPOST        (XO_ZONE + 10)	/* 搜尋文章模式 */
#define	XZ_MBOX		(XO_ZONE + 11)	/* 信箱 */
#define	XZ_POST		(XO_ZONE + 12)	/* 看板 */
#define XZ_GEM		(XO_ZONE + 13)	/* 精華區 */

#endif				/* _MODES_H_ */
