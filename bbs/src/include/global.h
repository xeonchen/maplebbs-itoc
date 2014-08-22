/*-------------------------------------------------------*/
/* global.h	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : global definitions & variables		 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/


#ifndef	_GLOBAL_H_
#define	_GLOBAL_H_


#ifdef	_MAIN_C_
# define VAR
# define INI(x)		= x
#else
# define VAR		extern
# define INI(x)
#endif


/* ----------------------------------------------------- */
/* GLOBAL DEFINITION					 */
/* ----------------------------------------------------- */


/* itoc.010821: 命名原則: 個人擁有者(如個人/看板/精華區所獨自擁有的)謂之 FN_XXXX
   系統擁有者，若是不變動的文件，謂之 FN_ETC_XXXX；若是變動檔案，謂之 FN_RUN_XXXX */


/* ----------------------------------------------------- */
/* 個人目錄檔名設定                                      */
/* ----------------------------------------------------- */


#define	FN_ACCT		".ACCT"		/* User account */
#define FN_BRH		".BRH"		/* board reading history */
#define FN_CZH		".CZH"		/* class zap history */

#define	FN_PLANS	"plans"		/* 計畫檔 */
#define	FN_SIGN		"sign"		/* 簽名檔.? */

#define FN_LOG		"log"		/* 上站來源記錄 */
#define FN_JUSTIFY	"justify"	/* 歷次認證資料 */
#define FN_EMAIL	"email"		/* 認證完整回信記錄 */
#define FN_ACL		"acl"		/* 上站地點設定 */

#define FN_BMW		"bmw"		/* 水球記錄 Binary Message Write */
#define FN_AMW		"amw"		/* 水球記錄 ASCII Message Write */
#define	FN_PAL		"friend"	/* 朋友名單 (儲存在自己目錄，記錄有哪些人) */

#ifdef HAVE_LIST
#define FN_LIST		"list"		/* 特殊名單.? */
#endif

#ifdef LOGIN_NOTIFY
#define FN_BENZ		"benz"		/* 系統協尋上站通知 */
#endif

#ifdef  HAVE_ALOHA
#define FN_ALOHA	"aloha"		/* 上站通知 (儲存在自己目錄，記錄有哪些人) */
#define FN_FRIENZ	"frienz"	/* 上站通知 (儲存在對方目錄，對方上站時觸發) */
#endif

#define FN_PAYCHECK	"paycheck"	/* 支票 */

#ifdef LOG_TALK
#define FN_TALK_LOG	"talk.log"	/* 聊天記錄檔 */
#endif

#ifdef MY_FAVORITE
#define FN_MF		"@MyFavorite"	/* 我的最愛 */
#endif

#ifdef HAVE_CLASSTABLE
#define FN_CLASSTBL	"classtable"	/* 功課表 */
#define FN_CLASSTBL_LOG	"classtable.log"/* 功課表 */
#endif

#ifdef HAVE_CREDIT
#define FN_CREDIT	"credit"	/* 記帳本 */
#endif

#ifdef HAVE_CALENDAR
#define FN_TODO		"todo"		/* 行事曆 */
#endif


/* ----------------------------------------------------- */
/* 看板/精華區/信箱檔名設定                              */
/* ----------------------------------------------------- */


#define FN_DIR		".DIR"		/* index */
#define FN_VCH		".VCH"		/* vote control header */
#define FN_NOTE		"note"		/* 進板畫面 */


/* ----------------------------------------------------- */
/* 系統檔名設定                                          */
/* ----------------------------------------------------- */

  /* --------------------------------------------------- */
  /* 根目錄下系統檔案                                    */
  /* --------------------------------------------------- */


#define FN_BRD		".BRD"		/* board list */
#define FN_SCHEMA	".USR"		/* userid schema */


  /* --------------------------------------------------- */
  /* run/ 目錄下系統檔案                                 */
  /* --------------------------------------------------- */


#define	FN_RUN_USIES	"run/usies"	/* BBS log */
#define FN_RUN_NOTE_ALL	"run/note.all"	/* 留言板 */
#define FN_RUN_PAL	"run/pal.log"	/* 朋友超過上限記錄 */

#define FN_RUN_ADMIN	"run/admin.log"	/* 站長行為記錄 */

#ifdef LOG_SONG_USIES
#define FN_RUN_SONGUSIES "run/song_usies" /* 點歌記錄 */
#endif

#ifdef HAVE_REGISTER_FORM
#define FN_RUN_RFORM	"run/rform"	/* 註冊表單 */
#define FN_RUN_RFORM_LOG "run/rform.log" /* 註冊表單審核記錄檔 */
#endif

#ifdef MODE_STAT
#define FN_RUN_MODE_LOG	"run/mode.log"	/* 使用者動態統計 - record per hour */
#define FN_RUN_MODE_CUR	"run/mode.cur"
#define FN_RUN_MODE_TMP	"run/mode.tmp"
#endif

#ifdef HAVE_ANONYMOUS
#define FN_RUN_ANONYMOUS "run/anonymous" /* 匿名發表文章記錄 */
#endif

#ifdef HAVE_BUY
#define FN_RUN_BANK_LOG	"run/bank.log"	/* 匯錢記錄 */
#endif

#ifdef HAVE_SIGNED_MAIL
#define FN_RUN_PRIVATE	"run/prikey"	/* 電子簽章 */
#endif

#define FN_RUN_EMAILREG	"run/emailreg"	/* 記錄用來認證的信箱 */
#define FN_RUN_MAIL_LOG	"run/mail.log"	/* 寄信的記錄 */

#define FN_RUN_POST	"run/post"	/* 文章篇數統計 */
#define	FN_RUN_POST_LOG	"run/post.log"	/* 文章篇數統計 */

/* reaper 所產生的 log */
#define FN_RUN_LAZYBM	"run/lazybm"	/* 偷懶板主統計 */
#define FN_RUN_MANAGER	"run/manager"	/* 特殊權限使用者列表 */
#define FN_RUN_REAPER	"run/reaper"	/* 長期未上站被清除的使用者列表 */
#define FN_RUN_EMAILADDR "run/emailaddr" /* 同一 email 認證多次列表 */

#define BMTA_LOGFILE	"run/bmta.log"	/* 收外部信的記錄 */


  /* --------------------------------------------------- */
  /* etc/ 目錄下系統檔案				 */
  /* --------------------------------------------------- */


#define FN_ETC_EXPIRE	"etc/expire.conf" 	/* 看板文章篇數上限設定 */

#define FN_ETC_VALID	"etc/valid"		/* 身分認證信函 (Email 認證時，寄去給站外信箱) */
#define FN_ETC_JUSTIFIED "etc/justified"	/* 認證通過通知 (認證通過時，寄到站內信箱) */
#define FN_ETC_REREG	"etc/re-reg"		/* 重新認證通知 (認證過期時，寄到站內信箱) */

#define FN_ETC_CROSSPOST "etc/cross-post"	/* 跨貼停權通知 (Cross-Post 時，寄到站內信箱) */

#define FN_ETC_BADID	"etc/badid"		/* 不雅名單 (拒絕註冊 ID) */
#define FN_ETC_SYSOP	"etc/sysop"		/* 站務名單 */

#define FN_ETC_FEAST	"etc/feast"		/* 節日 */

#define FN_ETC_HOST	"etc/host"		/* 故鄉 IP */
#define FN_ETC_FQDN	"etc/fqdn"		/* 故鄉 FQDN */

#define FN_ETC_TIP	"etc/tip"		/* 每日小秘訣 */

#define FN_ETC_LOVELETTER "etc/loveletter"	/* 情書產生器文庫 */


  /* --------------------------------------------------- */
  /* etc/ 目錄下 access crontrol list (ACL)		 */
  /* --------------------------------------------------- */

#define TRUST_ACLFILE	"etc/trust.acl"		/* 認證白名單 */
#define UNTRUST_ACLFILE	"etc/untrust.acl"	/* 認證黑名單 */

#define MAIL_ACLFILE	"etc/mail.acl"		/* 收信白名單 */
#define UNMAIL_ACLFILE	"etc/unmail.acl"	/* 收信黑名單 */

#define BBS_ACLFILE	"etc/bbs.acl"		/* 拒絕 telnet 連線名單 */


/* ----------------------------------------------------- */
/* 各個板的檔名設定                                      */
/* ----------------------------------------------------- */


#if 0	/* itoc.000512: 系統看板，預設小寫，大寫無妨 */
看板        中   文   敘   述       閱讀權限      發表權限
sysop       站務 報告站長‧給我報報 0             0
0announce   站務 奉天承運‧站長詔曰 0             PERM_ALLADMIN
test        站務 測試專區‧新手試爆 0             PERM_BASIC
note        站務 動態看板‧珠機話語 0             PERM_POST
newboard    站務 開闢專欄‧連署重地 0             PERM_POST
ktv         站務 點歌記錄‧真情對話 0             PERM_SYSOP
record      站務 酸甜苦辣‧系統記錄 0             PERM_SYSOP
deleted     站務 文章拯救‧資源回收 PERM_BM       PERM_SYSOP
bm          站務 專業討論‧板主交誼 PERM_BM       0
admin       站務 惡搞天地‧站長自摸 PERM_ALLADMIN 0
log         站務 系統保險‧安全記錄 PERM_ALLADMIN PERM_SYSOP
junk        站務 文章清理‧垃圾掩埋 PERM_ALLBOARD PERM_SYSOP
UnAnonymous 站務 黑函滿天‧匿名現身 PERM_ALLBOARD PERM_SYSOP

其中限制讀取的看板在 Class 分類中要設定 資料保密
#endif

/* 以下是一定要有的系統看板，但是一個板可以重覆使用多次，例如投稿歌本的和動態看板共用 note 板 */

#define BN_CAMERA	"note"		/* 動態看板放在這板的精華區 */
#define BN_ANNOUNCE	"0announce"	/* 公告看板，強迫閱讀 */
#define BN_JUNK		"junk"		/* 自己刪除的文章放在此 */
#define BN_DELETED	"deleted"	/* 板主刪除的文章放在此 */
#define BN_SECURITY	"log"		/* 系統安全記錄 */
#define BN_RECORD	"record"	/* 系統的一般記錄 */
#define BN_UNANONYMOUS	"UnAnonymous"	/* 匿名板的文章會複製一份在這裡 */
#define BN_KTV		"ktv"		/* 點歌記錄放在這板，歌本放在這板的精華區 */
#define BN_REQUEST	BN_CAMERA	/* 歌本投稿處 */

#define BN_NULL		"尚未選定"	/* 進站後還沒進入任何看板，所顯示的看板名稱 */


/* ----------------------------------------------------- */
/* 鍵盤設定                                              */
/* ----------------------------------------------------- */


#define KEY_BKSP	8	/* 和 Ctrl('H') 相同 */
#define KEY_TAB		9	/* 和 Ctrl('I') 相同 */
#define KEY_ENTER	10	/* 和 Ctrl('J') 相同 */
#define KEY_ESC		27
#define KEY_UP		-1
#define KEY_DOWN	-2
#define KEY_RIGHT	-3
#define KEY_LEFT	-4
#define KEY_HOME	-21
#define KEY_INS		-22
#define KEY_DEL		-23
#define KEY_END		-24
#define KEY_PGUP	-25
#define KEY_PGDN	-26


#define I_TIMEOUT	-31
#define I_OTHERDATA	-32


#define Ctrl(c)		(c & '\037')
#define Esc(c)		(c)		/* itoc.030824: 不 TRAP_ESC */
#define isprint2(c)	(c >= ' ')


#if 0	/* itoc.020108: 按鍵對應表 */

  int HEX = vkey('KEY');
  這表是由 vkey() 輸入鍵盤，傳出的整數值對應表。


/* 值是負的 */

KEY_INS   ffffffea	KEY_DEL   ffffffe9
KEY_HOME  ffffffeb	KEY_END   ffffffe8
KEY_PGUP  ffffffe7	KEY_PGDN  ffffffe6

KEY_UP    ffffffff	KEY_DOWN  fffffffe
KEY_RIGHT fffffffd	KEY_LEFT  fffffffc

/* 值是正的 */

┌──┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┐
│ HEX│00│01│02│03│04│05│06│07│08│09│0a│0b│0c│0d│0e│0f│
├──┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│ KEY│  │^A│^B│^C│^D│^E│^F│^G│^H│^I│^J│^K│^L│^M│^N│^O│
├──┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│ HEX│10│11│12│13│14│15│16│17│18│19│1a│1b│1c│1d│1e│1f│
├──┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│ KEY│^P│^Q│^R│^S│^T│^U│^V│^W│^X│^Y│^Z│Es│  │  │  │  │
├──┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│ HEX│20│21│22│23│24│25│26│27│28│29│2a│2b│2c│2d│2e│2f│	/* 0x22 是雙引號，避免 compile 錯誤 */
├──┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│ KEY│  │ !│XX│ #│ $│ %│ &│XX│ (│ )│ *│  │ ,│ -│ .│ /│	/* 0x27 是單引號，避免 compile 錯誤 */
├──┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│ HEX│30│31│32│33│34│35│36│37│38│39│3a│3b│3c│3d│3e│3f│
├──┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│ KEY│ 0│ 1│ 2│ 3│ 4│ 5│ 6│ 7│ 8│ 9│ :│ ;│ <│  │ >│ ?│
├──┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│ HEX│40│41│42│43│44│45│46│47│48│49│4a│4b│4c│4d│4e│4f│
├──┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│ KEY│ @│ A│ B│ C│ D│ E│ F│ G│ H│ I│ J│ K│ L│ M│ N│ O│
├──┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│ HEX│50│51│52│53│54│55│56│57│58│59│5a│5b│5c│5d│5e│5f│
├──┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│ KEY│ P│ Q│ R│ S│ T│ U│ V│ W│ X│ Y│ Z│ [│ \│ ]│ ^│ _│
├──┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│ HEX│60│61│62│63│64│65│66│67│68│69│6a│6b│6c│6d│6e│6f│
├──┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│ KEY│ `│ a│ b│ c│ d│ e│ f│ g│ h│ i│ j│ k│ l│ m│ n│ o│
├──┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│ HEX│70│71│72│73│74│75│76│77│78│79│7a│7b│7c│7d│7e│7f│
├──┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│ KEY│ p│ q│ r│ s│ t│ u│ v│ w│ x│ y│ z│ {│ |│ }│  │  │
└──┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┘

/* 值重覆 */

KEY_BKSP == Ctrl('H') == 0x08
'\t' == Ctrl('I') == 0x09
'\n' == Ctrl('J') == 0x0a
'\r' == Ctrl('M') == 0x0d

#endif


/* ----------------------------------------------------- */
/* 訊息字串：獨立出來，以利支援各種語言			 */
/* ----------------------------------------------------- */


#define QUOTE_CHAR1	'>'
#define QUOTE_CHAR2	':'

#define	STR_SPACE	" \t\n\r"

#define	STR_AUTHOR1	"作者:"
#define	STR_AUTHOR2	"發信人:"
#define	STR_POST1	"看板:"
#define	STR_POST2	"站內:"
#define	STR_REPLY	"Re: "
#define	STR_FORWARD	"Fw: "

#define STR_LINE	"\n\
> -------------------------------------------------------------------------- <\n\n"

#define LEN_AUTHOR1	(sizeof(STR_AUTHOR1) - 1)
#define LEN_AUTHOR2	(sizeof(STR_AUTHOR2) - 1)

#define STR_SYSOP	"sysop"
#define STR_GUEST	"guest"
#define STR_NEW		"new"

#define STR_ANONYMOUS	"神秘路人甲"		/* 要短於 IDLEN 個字 */

#define MSG_SEPERATOR	"\
───────────────────────────────────────"

#define MSG_CANCEL	"取消"
#define MSG_USR_LEFT	"對方已經離去"
#define MSG_XY_NONE	"空無一物"

#define MSG_USERPERM	"權限等級："
#define MSG_READPERM	"閱\讀權限："
#define MSG_POSTPERM	"發表權限："
#define MSG_BRDATTR	"看板屬性："
#define MSG_USERUFO	"習慣旗標：             ■ / □                                 ■ / □"

#define MSG_XYPOST1	"標題關鍵字："
#define MSG_XYPOST2	"作者關鍵字："

#define MSG_DEL_OK	"刪除完畢"
#define MSG_DEL_CANCEL	"取消刪除"
#define MSG_DEL_ERROR	"刪除錯誤"
#define MSG_DEL_NY	"請確定刪除(Y/N)？[N] "

#define MSG_SURE_NY	"請您確定(Y/N)？[N] "
#define MSG_SURE_YN	"請您確定(Y/N)？[Y] "

#define MSG_MULTIREPLY	"您確定要群組回信(Y/N)？[N] "

#define MSG_BID		"請輸入看板名稱："
#define MSG_UID		"請輸入代號："
#define MSG_PASSWD	"請輸入密碼："

#define ERR_BID		"錯誤的看板名稱"
#define ERR_UID		"錯誤的使用者代號"
#define ERR_PASSWD	"密碼輸入錯誤"
#define ERR_EMAIL	"不合格的 E-mail address"

#define MSG_SENT_OK	"信已寄出"

#define MSG_LIST_OVER	"您的名單太多，請善加整理"

#define MSG_COINLOCK	"您不能以分身進行這樣的服務"
#define MSG_REG_VALID	"您已經通過身分認證，請重新上站"

#define	MSG_LL		"\033[32m[群組名單]\033[m\n"
#define MSG_DATA_CLOAK	"<資料保密>\n"

#define MSG_CHKDATA	"★ 資料整理稽核中，請稍候 \033[5m...\033[m"
#define MSG_QUITGAME	"不玩了啊？下次再來哦！ ^_^"

#define MSG_CHAT_ULIST	\
"\033[7m 使用者代號    目前狀態  │ 使用者代號    目前狀態  │ 使用者代號    目前狀態 \033[m"


/* ----------------------------------------------------- */
/* GLOBAL VARIABLE					 */
/* ----------------------------------------------------- */


VAR int bbsmode;		/* bbs operating mode, see modes.h */
VAR usint bbstate;		/* bbs operatine state */

VAR ACCT cuser;			/* current user structure */
VAR UTMP *cutmp;		/* current user temporary */

VAR time_t ap_start;		/* 進站時刻 */
VAR int total_user;		/* 使用者自以為的站上人數 */

VAR int b_lines;		/* bottom line */
VAR int b_cols;			/* bottom columns */
VAR int d_cols;			/* difference columns from standard */

VAR char fromhost[48];		/* from FQDN */

VAR char ve_title[80];		/* edited title */
VAR char quote_file[80];
VAR char quote_user[80];
VAR char quote_nick[80];

VAR char hunt[32];		/* hunt keyword */

VAR int curredit;		/* current edit mode */
VAR time_t currchrono;		/* current file timestamp */
VAR char currtitle[80];		/* currently selected article title */

VAR int currbno;		/* currently selected board bno */
VAR usint currbattr;		/* currently selected board battr */
VAR char currboard[BNLEN + 1];	/* currently selected board brdname */
VAR char currBM[BMLEN + 7];	/* currently selected board BM */	/* BMLEN + 1 + strlen("板主：") */

/* filename */

VAR char *fn_acct	INI(FN_ACCT);
VAR char *fn_dir	INI(FN_DIR);
VAR char *fn_bmw	INI(FN_BMW);
VAR char *fn_amw	INI(FN_AMW);
VAR char *fn_pal	INI(FN_PAL);
VAR char *fn_plans	INI(FN_PLANS);
VAR char *fn_note	INI(FN_NOTE);


/* message */

VAR char *msg_seperator	INI(MSG_SEPERATOR);

VAR char *msg_cancel	INI(MSG_CANCEL);

VAR char *msg_sure_ny	INI(MSG_SURE_NY);

VAR char *msg_uid	INI(MSG_UID);

VAR char *msg_del_ny	INI(MSG_DEL_NY);

VAR char *err_bid	INI(ERR_BID);
VAR char *err_uid	INI(ERR_UID);
VAR char *err_email	INI(ERR_EMAIL);

VAR char *msg_sent_ok	INI(MSG_SENT_OK);

VAR char *msg_list_over	INI(MSG_LIST_OVER);

VAR char *msg_reg_valid	INI(MSG_REG_VALID);
VAR char *msg_coinlock	INI(MSG_COINLOCK);

VAR char *str_sysop	INI(STR_SYSOP);
VAR char *str_author1	INI(STR_AUTHOR1);
VAR char *str_author2	INI(STR_AUTHOR2);
VAR char *str_post1	INI(STR_POST1);
VAR char *str_post2	INI(STR_POST2);
VAR char *str_host	INI(MYHOSTNAME);
VAR char *str_site	INI(BBSNAME);
VAR char *str_line	INI(STR_LINE);

VAR char *str_ransi	INI("\033[m");

#undef	VAR
#undef	INI

#endif				/* _GLOBAL_H_ */
