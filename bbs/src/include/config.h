/*-------------------------------------------------------*/
/* config.h	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : site-configurable settings		 	 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/


/*--------------------------------------------------------------------*/
/* MapleBridge Bulletin Board System  Version: 2.36                   */
/* Copyright (C) 1994-1995            Jeng-Hermes Lin, Hung-Pin Chen, */
/*                                    SoC, Xshadow                    */
/*--------------------------------------------------------------------*/
/* MapleBridge Bulletin Board System  Version: 3.10                   */
/* Copyright (C) 1997-1998            Jeng-Hermes Lin                 */
/*--------------------------------------------------------------------*/


#ifndef	_CONFIG_H_
#define	_CONFIG_H_


/* ----------------------------------------------------- */
/* 定義 BBS 站名位址					 */
/* ------------------------------------------------------*/

#define SCHOOLNAME	"台南一中"		/* 組織名稱 */
#define BBSNAME		"與南共舞"		/* 中文站名 */
#define BBSNAME2	"WolfBBS"		/* 英文站名 */
#define SYSOPNICK	"狼人長老"		/* sysop 的暱稱 */
#define TAG_VALID       "["BBSNAME2"]To"	/* 身分認證函 token */

#define MYIPADDR	"210.70.137.5"		/* IP address */
#define MYHOSTNAME	"bbs.tnfsh.tn.edu.tw"	/* 網路地址 FQDN */

#define HOST_ALIASES	{MYHOSTNAME, MYIPADDR, \
			 "wolf.twbbs.org", "wolf.twbbs.org.tw", \
			 NULL}

#define MYCHARSET	"big5"			/* BBS 所使用的字集 */

#define BBSHOME		"/home/bbs"		/* BBS 的家 */
#define BAKPATH		"/home/bbs/bak"		/* 備份檔的路徑 */

#define BBSUID		9999
#define BBSGID		99			/* Linux 請設為 999 */


/* ----------------------------------------------------- */
/* 組態規劃						 */
/* ----------------------------------------------------- */

  /* ------------------------------------------------- */
  /* 組態規劃˙系統追蹤                                */
  /* ------------------------------------------------- */

#define	HAVE_SEM		/* 使用 semaphore */

#ifndef CYGWIN
#define	HAVE_RLIMIT		/* 採用 resource limit，Cygwin 不能用 */
#endif

#undef	MODE_STAT               /* 觀察及統計 user 的生態，以做為經營方針 */

#undef	SYSOP_CHECK_MAIL	/* itoc.001029: 站長可以讀取使用者信箱 */

#undef	SYSOP_SU		/* itoc.001102: 以 sysop 登入可以變更使用者身分 */

#define	HAVE_MULTI_BYTE		/* hightman.060504: 支援雙字節漢字處理 */

  /* ------------------------------------------------- */
  /* 組態規劃˙註冊認證                                */
  /* ------------------------------------------------- */

#define LOGINASNEW		/* 採用上站申請帳號制度 */

#ifdef LOGINASNEW
#undef	HAVE_GUARANTOR		/* itoc.000319: 採用保證人制度 */
#endif

#undef	HAVE_LOGIN_DENIED	/* itoc.000319: 擋掉某些來源的連結，參照 etc/bbs.acl */

#undef	NEWUSER_LIMIT		/* 新手上路的三天限制 */

#define	JUSTIFY_PERIODICAL	/* 定期身分認證 */

#define	EMAIL_JUSTIFY		/* 發出 Internet Email 身分認證信函 */

#ifdef EMAIL_JUSTIFY
#define	HAVE_POP3_CHECK		/* itoc.000315: POP3 系統認證 */
#define	HAVE_REGKEY_CHECK	/* itoc.010112: 認證碼驗證 */
#endif

#define	HAVE_REGISTER_FORM	/* 註冊單認證 */

  /* ------------------------------------------------- */
  /* 組態規劃˙結交朋友                                */
  /* ------------------------------------------------- */

#define	HAVE_MODERATED_BOARD	/* 提供好友秘密板 */	/* 秘密看板的閱讀權限要是 PERM_SYSOP 才會被當成秘密看板 */
							/* 好友看板的閱讀權限要是 PERM_BOARD 才會被當成好友看板 */
#define	CHECK_ONLINE		/* itoc.010306: 文章列表中可以顯示使用者是否在站上 */

#define	HAVE_BADPAL		/* itoc.010302: 提供壞人的功能 */

#define	HAVE_LIST		/* itoc.010923: 群組名單 */

#define HAVE_ALOHA              /* itoc.001202: 上站通知 */

#undef	LOGIN_NOTIFY            /* 系統協尋網友 */

#if (defined(HAVE_ALOHA) || defined(LOGIN_NOTIFY))
#define	HAVE_NOALOHA		/* itoc.010716: 上站不通知/協尋 */
#endif

#define	LOG_BMW                 /* lkchu.981201: 水球記錄處理 */

#ifdef LOG_BMW
#define	RETAIN_BMW		/* itoc.021102: 水球存證 */
#endif

#define	LOG_TALK		/* lkchu.981201: 聊天記錄處理 */

#define	HAVE_NOBROAD		/* itoc.010716: 拒收廣播 */

#define	BMW_COUNT		/* itoc.010312: 計算中了幾個水球 */

#define	BMW_DISPLAY		/* itoc.010313: 顯示之前的水球 */

#define	HAVE_CHANGE_NICK	/* 使用者名單 ^N 永久更改暱稱 */

#define	HAVE_CHANGE_FROM        /* 使用者名單 ^F 暫時更改故鄉 */

#define	HAVE_CHANGE_ID		/* 使用者名單 ^D 暫時更改 ID */

#define	HAVE_WHERE		/* itoc.001102: 使用者名單故鄉辨識，參照 etc/host fqdn */

#ifdef HAVE_WHERE
#define GUEST_WHERE		/* itoc.010208: guest 亂數取故鄉 */
#endif

#define	GUEST_NICK		/* itoc.000319: guest 亂數取暱稱 */

#define	DETAIL_IDLETIME		/* itoc.020316: 時常更新閒置時間 */

#define	TIME_KICKER		/* itoc.030514: 是否自動簽退 idle 過久的使用者 */

#define	HAVE_BRDMATE		/* itoc.020602: 板伴 (閱讀同一看板) */

#define	HAVE_SUPERCLOAK		/* itoc.020602: 紫隱，站長超級隱形 */

  /* ------------------------------------------------- */
  /* 組態規劃˙看板信箱                                */
  /* ------------------------------------------------- */
      
#define	HAVE_ANONYMOUS		/* 提供 anonymous 板 */

#ifdef HAVE_ANONYMOUS
#define	HAVE_UNANONYMOUS_BOARD	/* itoc.020602: 反匿名板，必須有開 BN_UNANONYMOUS */
#endif

#define	SHOW_USER_IN_TEXT       /* 在文件中 Ctrl+Q 可顯示 User 的名字 */

#undef	ANTI_PHONETIC		/* itoc.030503: 禁用注音文 */

#define	ENHANCED_VISIT		/* itoc.010407: 已讀/未讀檢查是該看板的最後一篇決定 */

#define	COLOR_HEADER            /* lkchu.981201: 變換彩色標頭 */

#define	CURSOR_BAR		/* itoc.010113: 選單光棒，若開啟選單光棒，選單就不能有顏色控制碼 */

#define	HAVE_DECLARE		/* 使 title 中有 [] 更明顯，且日期上色 */

#define	HAVE_POPUPMENU          /* 蹦出式選單 */

#ifdef HAVE_POPUPMENU
#define	POPUP_ANSWER		/* 蹦出式選單 -- 詢問選項 */
#define	POPUP_MESSAGE		/* 蹦出式選單 -- 訊息視窗 */
#endif

#define	AUTHOR_EXTRACTION	/* 尋找同一作者文章 */

#define	HAVE_TERMINATOR		/* 終結文章大法 --- 拂楓落葉斬 */

#define	HAVE_XYNEWS		/* itoc.010822: 新聞閱讀模式 */

#define	HAVE_SCORE		/* itoc.020114: 文章評分功能 */

#define	HAVE_DETECT_CROSSPOST	/* cross-post 自動偵測 */

#define	AUTO_JUMPBRD		/* itoc.010910: 看板列表自動跳去下一個未讀看板 */

#undef	AUTO_JUMPPOST		/* itoc.010910: 文章列表自動跳去最後一篇未讀 */

#define	ENHANCED_BSHM_UPDATE	/* itoc.021101: 看板列表刪除/標記文章不列入未讀的燈 */

#define	SLIDE_SHOW		/* itoc.030411: 自動播放文章 */

#define	EVERY_Z			/* ctrl-z Everywhere (除了寫文章) */

#define	EVERY_BIFF		/* Thor.980805: 郵差到處來按鈴 */

#undef	HAVE_SIGNED_MAIL	/* Thor.990409: 外送信件加簽名 */

#define	INPUT_TOOLS		/* itoc.000319: 符號輸入工具 */

#define	HAVE_FORCE_BOARD	/* itoc.000319: 強迫 user login 時候讀取某公告看板 */
				/* itoc.010726: 該公告看板要不可 zap，否則 zap 後不會記錄 brh */

#define	MY_FAVORITE		/* itoc.001202: 提供我的最愛看板 */

#define	HAVE_COSIGN		/* itoc.010108: 提供看板連署 */

#ifdef HAVE_COSIGN
#undef	SYSOP_START_COSIGN	/* itoc.030613: 新板連署要先經站長審核才能開始 */
#endif

#define	HAVE_REFUSEMARK		/* itoc.010602: 提供看板文章加密 */

#define	HAVE_LABELMARK		/* itoc.020307: 提供看板文章加待砍標記 */

#undef	POST_PREFIX		/* itoc.020113: 發表文章時標題可選擇種類 */

#define	MULTI_MAIL		/* 群組寄信功能 */

#define	HAVE_MAIL_ZIP		/* itoc.010228: 提供把信件/精華區壓縮轉寄的功能 */

#undef	OVERDUE_MAILDEL		/* itoc.020217: 清除過期信件 */

  /* ------------------------------------------------- */
  /* 組態規劃˙外掛程式                                */
  /* ------------------------------------------------- */

#define	HAVE_EXTERNAL		/* Xyz 選單 */

#ifdef HAVE_EXTERNAL
#  define HAVE_SONG		/* itoc.010205: 提供點歌功能 */
#  define HAVE_NETTOOL		/* itoc.010209: 提供網路服務工具 */
#  define HAVE_GAME		/* itoc.010208: 提供遊戲 */
#  define HAVE_BUY		/* itoc.010716: 提供購買權限 */
#  define HAVE_TIP		/* itoc.010301: 提供每日小秘訣 */
#  define HAVE_CLASSTABLE	/* itoc.010907: 提供功課表 */
#  define HAVE_CREDIT		/* itoc.020125: 提供記帳本 */
#  define HAVE_LOVELETTER	/* itoc.020602: 提供情書產生器 */
#  define HAVE_CALENDAR		/* itoc.020831: 提供萬年曆 */
#endif

#ifdef HAVE_SONG
#  define HAVE_SONG_CAMERA	/* itoc.010207: 提供點歌到動態看板功能 */
#  define LOG_SONG_USIES	/* itoc.010928: 點歌記錄 */
#endif


/* ----------------------------------------------------- */
/* 組態設定                                              */
/* ------------------------------------------------------*/

#ifdef HAVE_SIGNED_MAIL
#define PRIVATE_KEY_PERIOD 0	/* 平均幾天換一次key，0 表示不換key，自動產生 */
#endif

#ifndef	SHOW_USER_IN_TEXT
#define outx	outs
#endif


/* ----------------------------------------------------- */
/* 系統參數˙隨 BBS 站規模成長而擴增		         */
/* ----------------------------------------------------- */

#define MAXBOARD	1024		/* 最大開板個數 */

#define MAXACTIVE	512		/* 最多同時上站人數 */

/* ----------------------------------------------------- */
/* 系統參數˙其他常用參數                                */
/* ----------------------------------------------------- */

/* bbsd.c 上站登入 */

#define LOGINATTEMPTS	3		/* 最大進站密碼打錯次數 */
#define MULTI_MAX	2		/* 一般使用者最大 multi-login 個數 */

/* bbsd.c user.c admutil.c 定期身分認證，記得同步修改 gem/@/@re-reg */

#ifdef JUSTIFY_PERIODICAL
#define VALID_PERIOD		(86400 * 365)	/* 身分認證有效期(秒) */
#define INVALID_NOTICE_PERIOD	(86400 * 10)	/* 身分認證失效前多久時間提醒使用者(秒) */
#endif

/* talk.c 朋友名單/水球列表 */

/* itoc.010825: 注意 BMW_LOCAL_MAX >= BMW_PER_USER */

#define PAL_MAX		200		/* 朋友名單上限(人) */
#define BMW_EXPIRE	60		/* 水球處理時間(秒) */
#define BMW_PER_USER	5		/* 水球處理時間內，允許丟幾個水球 */
#define BMW_MAX		128		/* UCACHE 中 pool 上限 */
#define BMW_LOCAL_MAX	8		/* Ctrl+R 上下所能瀏覽之前水球個數 */

/* aloha.c 上站通知 */

#ifdef HAVE_ALOHA
#define ALOHA_MAX	64		/* 上站通知上限(人) */
#endif

/* menu.c 留言板 */

#define NOTE_MAX	100		/* 留言板保留篇數 */
#define NOTE_DUE	48		/* 留言板保留時間(天) */

/* mail.c 用來檢查信箱大小，超過則提示，記得同步修改 etc/justified gem/@/@mailover */

#define	MAX_BBSMAIL	500		/* PERM_MBOX 收信上限(封) */
#define	MAX_VALIDMAIL	300		/* 認證 user 收信上限(封) */
#define	MAX_NOVALIDMAIL	100		/* 未認證 user 收信上限(封) */

/* bquota.c mail.c 用來清過期檔案/信件的時間，記得同步修改 etc/justified */

#ifdef OVERDUE_MAILDEL
#define MARK_DUE        180		/* 標記保存之信件保留時間(天) */
#define MAIL_DUE        60		/* 一般信件保留時間(天) */
#define FILE_DUE        30		/* 其他檔案保留時間(天) */
#endif

/* newbrd.c 看板連署 */

#ifdef HAVE_COSIGN
#define NBRD_NUM_BRD	12		/* 開板需要連署人數 */
#define NBRD_DAY_BRD	3		/* 開板可連署天數 */
#endif

/* post.c 偵測 cross-post */

#ifdef  HAVE_DETECT_CROSSPOST
#define	MAX_CROSS_POST		3	/* cross post 最大數量(篇) */
#define CROSSPOST_DENY_DAY	30	/* cross post 停權時間(天) */
#endif

/* visio.c bguard.c 自動踢人 */

#ifdef TIME_KICKER
#define IDLE_TIMEOUT	30		/* visio.c bguard.c 發呆過久自動簽退(分) */
#define IDLE_WARNOUT	3		/* visio.c 發呆過久提醒(分) -- 自動簽退前三分鐘前 */
#endif

/* more.c edit.c 翻頁 */

#define PAGE_SCROLL	(b_lines - 1)	/* 按 PgUp/PgDn 要捲動幾列 */

#define MAXLASTCMD	8		/* line input buffer */
#define	BLK_SIZ		4096		/* disk I/O block size */
#define MAXSIGLINES	6		/* edit.c 簽名檔引入最大行數 */
#define MAXQUERYLINES	17		/* talk.c xchatd.c 顯示 Query/Plan 訊息最大行數 */
#define	MAX_CHOICES	32		/* vote.c 投票時最多有 32 種選擇 */
#define	TAG_MAX		256		/* xover.c TagList 標籤數目之上限 */
#define LINE_HEADER	3		/* more.c bhttpd.c 檔頭有三列 */

/* bbsd.c mail.c 整理週期 */

#define	CHECK_PERIOD	(86400 * 20)	/* 整理信箱/朋友名單的週期(秒) */

/* camera.c 動態看板 */

#define	MOVIE_MAX	180		/* 動畫張數 */
#define	MOVIE_SIZE	(108 * 1024)	/* 動畫 cache size */

/* expire.c 看板自動砍過期文章 */

#define BRD_EXPIRE_DAYS	365		/* 預設清除超過 365 天的文章 */
#define BRD_EXPIRE_MAXP	5000		/* 預設清除超過 5000 篇的文章 */
#define BRD_EXPIRE_MINP	500		/* 預設低於 500 篇的看板不砍文章 */

/* reaper.c 自動砍太久沒上站的帳號 */

/* 保留帳號期限 -- 學期中 */
#define REAPER_DAY_NEWUSR	7	/* 登入不超過三次的使用者保留 7 天 */
#define REAPER_DAY_FORFUN	120	/* 未完成身分認證的使用者保留 120 天 */
#define REAPER_DAY_OCCUPY	120	/* 已完成身分認證的使用者保留 120 天 */
/* 保留帳號期限 -- 暑假 */
#define REAPER_VAC_NEWUSR	7	/* 登入不超過三次的使用者保留 7 天 */
#define REAPER_VAC_FORFUN	180	/* 未完成身分認證的使用者保留 180 天 */
#define REAPER_VAC_OCCUPY	180	/* 已完成身分認證的使用者保留 180 天 */


/* ----------------------------------------------------- */
/* chat.c & xchatd.c 中採用的 protocol			 */
/* ------------------------------------------------------*/

#define CHAT_SECURE			/* 安全的聊天室 */

#define EXIT_LOGOUT     0
#define EXIT_LOSTCONN   -1
#define EXIT_CLIERROR   -2
#define EXIT_TIMEDOUT   -3
#define EXIT_KICK       -4

#define CHAT_LOGIN_OK       "OK"
#define CHAT_LOGIN_EXISTS   "EX"
#define CHAT_LOGIN_INVALID  "IN"
#define CHAT_LOGIN_BOGUS    "BG"


/* ----------------------------------------------------- */
/* BBS 服務所用的 port					 */
/* ------------------------------------------------------*/

/* itoc.030512: 如果要在同一台機器上架二個 bbs 站，那麼要：

   1) 在 OS 開一個新的 user (例如叫 bbs2)，其 uid 要和另一個 bbs 不同
   2) 改 BBSHOME、BAKPATH、BBSUID、BBSGID 為 bbs2 的路徑及UID、GID
   3) 改以下的 *_PORT 及 *_KEY 和另一個 bbs 不同 (例如都加上 10000)
*/

#define MAX_BBSDPORT	1		/* bbsd 要開幾個 port，隨 BBSD_PORT 而變 */
#define BBSD_PORT	{23 /*,3456*/}	/* bbsd   所用的 port (bbsd.c) */
#define BMTA_PORT	25		/* SMTP   所用的 port (bmtad.c) */
#define GEMD_PORT	70		/* Gopher 所用的 port (gemd.c) */
#define FINGER_PORT	79		/* Finger 所用的 port (bguard.c) */
#define BHTTP_PORT	80		/* HTTP   所用的 port (bhttpd.c) */
#define POP3_PORT	110		/* POP3   所用的 port (bpop3d.c) */
#define BNNTP_PORT	119		/* NNTP   所用的 port (bnntp.c) */
#define CHAT_PORT	3838		/* 聊天室 所用的 port (chat.c xchatd.c) */
#define INNBBS_PORT	7777		/* 轉信   所用的 port (channel.c) */


/* ----------------------------------------------------- */
/* SHM 及 SEM 所用的 key				 */
/* ----------------------------------------------------- */

#define BRDSHM_KEY	2997	/* 看板 */
#define UTMPSHM_KEY	1998	/* 使用者 */
#define FILMSHM_KEY	2999	/* 動態看板 */
#define PIPSHM_KEY	4998	/* 電子雞對戰 */

#define	BSEM_KEY	2000	/* semaphore key */
#define	BSEM_FLG	0600	/* semaphore mode */
#define BSEM_ENTER      -1	/* enter semaphore */
#define BSEM_LEAVE      1	/* leave semaphore */
#define BSEM_RESET	0	/* reset semaphore */

#endif				/* _CONFIG_H_ */
