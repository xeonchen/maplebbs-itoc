/*-------------------------------------------------------*/
/* perm.h	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : permission levels of user & board		 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/


#ifndef	_PERM_H_
#define	_PERM_H_


/* ----------------------------------------------------- */
/* These are the 32 basic permission bits.		 */
/* ----------------------------------------------------- */

#define	PERM_BASIC	0x00000001	/* 1-8 : 基本權限 */
#define PERM_CHAT	0x00000002
#define	PERM_PAGE	0x00000004
#define PERM_POST	0x00000008
#define	PERM_VALID 	0x00000010	/* LOGINOK */
#define PERM_MBOX	0x00000020
#define PERM_CLOAK	0x00000040
#define PERM_XEMPT 	0x00000080

#define	PERM_9		0x00000100
#define	PERM_10		0x00000200
#define	PERM_11		0x00000400
#define	PERM_12		0x00000800
#define	PERM_13		0x00001000
#define	PERM_14		0x00002000
#define	PERM_15		0x00004000
#define	PERM_16		0x00008000

#define PERM_DENYPOST	0x00010000	/* 17-24 : 禁制權限 */
#define	PERM_DENYTALK	0x00020000
#define	PERM_DENYCHAT	0x00040000
#define	PERM_DENYMAIL	0x00080000
#define	PERM_DENY5	0x00100000
#define	PERM_DENY6	0x00200000
#define	PERM_DENYLOGIN	0x00400000
#define	PERM_PURGE	0x00800000

#define PERM_BM		0x01000000	/* 25-32 : 管理權限 */
#define PERM_SEECLOAK	0x02000000
#define PERM_ADMIN3	0x04000000
#define PERM_REGISTRAR	0x08000000
#define PERM_ACCOUNTS	0x10000000
#define PERM_CHATROOM	0x20000000
#define	PERM_BOARD	0x40000000
#define PERM_SYSOP	0x80000000


/* ----------------------------------------------------- */
/* These permissions are bitwise ORs of the basic bits.	 */
/* ----------------------------------------------------- */


/* This is the default permission granted to all new accounts. */
#define PERM_DEFAULT 	PERM_BASIC

/* 有 PERM_VALID 才可以保人進來本站註冊 */
#ifdef HAVE_GUARANTOR
#define PERM_GUARANTOR	PERM_VALID
#endif

#if 0   /* itoc.說明: 關於站務權限 */

  所有程式的關於站務的權限都改成 PERM_ALLXXXX
  在此畫一個圖來描述站務權限的設定。

                  ┌ 註冊總管 PERM_REGISTRAR : 可以審註冊單。
                  │
                  ├ 帳號總管 PERM_ACCOUNTS : 可以修改權限、踢線上使用者、審註冊單。
                  │
  站長 PERM_SYSOP ┼ 聊天室總管 PERM_CHATROOM : 在聊天室是 roomop。
                  │
                  ├ 看板總管 PERM_BOARD : 可以修改看板設定、進入秘密及好友看板。
                  │
                  └ 全體站務 PERM_ALLADMIN : 以上四個總管，都有以下功能：
                     上站來源設定、隱身術、紫隱、不必定期認證、無須新手見習三天、multi-login、
                     修改站上文件、更新系統、引言可以過多、寄信給全站使用者、信箱無上限。

  站長 PERM_SYSOP 除了以上所有功能，還擁有以下功能：
  精華區建置資料、精華區看到加密目錄的標題、閱讀所有人的信件、得知所有人在看哪個板、開啟站務權限。
  
#endif

#define PERM_ALLADMIN	(PERM_REGISTRAR | PERM_BOARD | PERM_ACCOUNTS | PERM_SYSOP)	/* 站務 */
#define	PERM_ALLREG	(PERM_SYSOP | PERM_ACCOUNTS | PERM_REGISTRAR)	/* 審註冊單 */
#define	PERM_ALLACCT	(PERM_SYSOP | PERM_ACCOUNTS)			/* 帳號管理 */
#define PERM_ALLCHAT	(PERM_SYSOP | PERM_CHATROOM)			/* 聊天管理 */
#define PERM_ALLBOARD	(PERM_SYSOP | PERM_BOARD)			/* 看板管理 */

#define PERM_ALLVALID	(PERM_VALID | PERM_POST | PERM_PAGE | PERM_CHAT)	/* 認證通過後應有的完整權限 */
#define PERM_ALLDENY	(PERM_DENYPOST | PERM_DENYTALK | PERM_DENYCHAT | PERM_DENYMAIL)	/* 所有停權 */

#define PERM_LOCAL	PERM_BASIC	/* 不是 guest 就能寄信到站內其他使用者 */
#define PERM_INTERNET	PERM_VALID	/* 身分認證過關的才能寄信到 Internet */

/* #define HAS_PERM(x)	((x)?cuser.userlevel&(x):1) */
/* #define HAVE_PERM(x)	(cuser.userlevel&(x)) */
/* itoc.001217: 程式中不會有 HAS_PERM(0) 的寫法，HAVE_PERM 不會用到 */
#define HAS_PERM(x)	(cuser.userlevel&(x))


/* ----------------------------------------------------- */
/* 各種權限的中文意義					 */
/* ----------------------------------------------------- */

#define	NUMPERMS	32

#define STR_PERM	"bctpjm#x-------@PTCM--L*B#-RACBS"	/* itoc: 新增權限的時候別忘了改這裡啊 */

#ifdef _ADMIN_C_

static char *perm_tbl[NUMPERMS] = 
{
  "基本權力",			/* PERM_BASIC */
  "進入聊天室",			/* PERM_CHAT */
  "找人聊天",			/* PERM_PAGE */
  "發表文章",			/* PERM_POST */
  "身分認證",			/* PERM_VALID */
  "信件無上限",			/* PERM_MBOX */
  "隱身術",			/* PERM_CLOAK */
  "永久保留帳號",		/* PERM_XEMPT */

  "保留",			/* PERM_9 */
  "保留",			/* PERM_10 */
  "保留",			/* PERM_11 */
  "保留",			/* PERM_12 */
  "保留",			/* PERM_13 */
  "保留",			/* PERM_14 */
  "保留",			/* PERM_15 */
  "保留",			/* PERM_16 */

  "禁止發表文章",		/* PERM_DENYPOST */
  "禁止 talk",			/* PERM_DENYTALK */
  "禁止 chat",			/* PERM_DENYCHAT */
  "禁止 mail",			/* PERM_DENYMAIL */
  "保留",			/* PERM_DENY5 */
  "保留",			/* PERM_DENY6 */
  "禁止 login",			/* PERM_DENYLOGIN */
  "清除帳號",			/* PERM_PURGE */

  "板主",			/* PERM_BM */
  "看見忍者",			/* PERM_SEECLOAK */
  "保留",			/* PERM_ADMIN3 */
  "註冊總管",			/* PERM_REGISTRAR */
  "帳號總管",			/* PERM_ACCOUNTS */
  "聊天室總管",			/* PERM_CHATCLOAK */
  "看板總管",			/* PERM_BOARD */
  "站長"			/* PERM_SYSOP */
};

#endif
#endif				/* _PERM_H_ */
