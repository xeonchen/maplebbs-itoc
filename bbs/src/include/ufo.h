/*-------------------------------------------------------*/
/* ufo.h	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : User Flag Option				 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/


#ifndef	_UFO_H_
#define	_UFO_H_


/* ----------------------------------------------------- */
/* User Flag Option : flags in ACCT.ufo			 */
/* ----------------------------------------------------- */


#define BFLAG(n)	(1 << n)	/* 32 bit-wise flag */


#define UFO_NOUSE00	BFLAG(0)	/* 沒用到 */
#define UFO_MOVIE	BFLAG(1)	/* 動態看板顯示 */
#define UFO_BRDPOST	BFLAG(2)	/* 1: 看板列表顯示篇數  0: 看板列表顯示號碼 itoc.010912: 恆為新文章模式 */
#define UFO_BRDNAME	BFLAG(3)	/* itoc.010413: 看板列表依 1:brdname 0:class+title 排序 */
#define UFO_BRDNOTE	BFLAG(4)	/* 顯示進板畫面 */
#define UFO_VEDIT	BFLAG(5)	/* 簡化編輯器 */
#define UFO_MOTD	BFLAG(6)	/* 簡化進/離站畫面 */

#define UFO_PAGER	BFLAG(7)	/* 關閉呼叫器 */
#define UFO_RCVER	BFLAG(8)	/* itoc.010716: 拒收廣播 */
#define UFO_QUIET	BFLAG(9)	/* 結廬在人境，而無車馬喧 */
#define UFO_PAL		BFLAG(10)	/* 使用者名單只顯示好友 */
#define UFO_ALOHA	BFLAG(11)	/* 接受上站通知 */
#define UFO_NOALOHA	BFLAG(12)	/* itoc.010716: 上站不通知/協尋 */

#define UFO_BMWDISPLAY	BFLAG(13)	/* itoc.010315: 水球回顧介面 */
#define UFO_NWLOG       BFLAG(14)	/* lkchu.990510: 不存對話紀錄 */
#define UFO_NTLOG       BFLAG(15)	/* lkchu.990510: 不存聊天紀錄 */

#define UFO_NOSIGN	BFLAG(16)	/* itoc.000320: 不使用簽名檔 */
#define UFO_SHOWSIGN	BFLAG(17)	/* itoc.000319: 存檔前顯示簽名檔 */

#define UFO_ZHC		BFLAG(18)	/* hightman.060504: 全型字偵測 */
#define UFO_JUMPBRD	BFLAG(19)	/* itoc.020122: 自動跳去下一個未讀看板 */
#define UFO_NOUSE20	BFLAG(20)
#define UFO_NOUSE21	BFLAG(21)
#define UFO_NOUSE22	BFLAG(22)
#define UFO_NOUSE23	BFLAG(23)

#define UFO_CLOAK	BFLAG(24)	/* 1: 進入隱形 */
#define UFO_SUPERCLOAK	BFLAG(25)	/* 1: 超級隱身 */
#define UFO_ACL		BFLAG(26)	/* 1: 使用 ACL */
#define UFO_NOUSE27	BFLAG(27)
#define UFO_NOUSE28	BFLAG(28)
#define UFO_NOUSE29	BFLAG(29)
#define UFO_NOUSE30	BFLAG(30)
#define UFO_NOUSE31	BFLAG(31)

/* 新註冊帳號、guest 的預設 ufo */

#define UFO_DEFAULT_NEW		(UFO_BRDNOTE | UFO_MOTD | UFO_BMWDISPLAY | UFO_NWLOG | UFO_NOSIGN)
#define UFO_DEFAULT_GUEST	(UFO_MOVIE | UFO_BRDNOTE | UFO_QUIET | UFO_NOALOHA | UFO_NWLOG | UFO_NTLOG | UFO_NOSIGN)


/* ----------------------------------------------------- */
/* Status : flags in UTMP.status			 */
/* ----------------------------------------------------- */


#define STATUS_BIFF	BFLAG(0)	/* 有新信件 */
#define STATUS_REJECT	BFLAG(1)	/* true if reject any body */
#define STATUS_BIRTHDAY	BFLAG(2)	/* 今天生日 */
#define STATUS_COINLOCK	BFLAG(3)	/* 錢幣鎖定 */
#define STATUS_DATALOCK	BFLAG(4)	/* 資料鎖定 */
#define STATUS_MQUOTA	BFLAG(5)	/* 信箱中有過期之信件 */
#define STATUS_MAILOVER	BFLAG(6)	/* 信箱過多信件 */
#define STATUS_MGEMOVER	BFLAG(7)	/* 個人精華區過多 */
#define STATUS_EDITHELP	BFLAG(8)	/* 在 edit 時進入 help */
#define STATUS_PALDIRTY BFLAG(9)	/* 有人在他的朋友名單新增或移除了我 */


#define	HAS_STATUS(x)	(cutmp->status&(x))


/* ----------------------------------------------------- */
/* 各種習慣的中文意義					 */
/* ----------------------------------------------------- */


/* itoc.000320: 增減項目要更改 NUMUFOS_* 大小, 也別忘了改 STR_UFO */

#define NUMUFOS		27
#define NUMUFOS_GUEST	5	/* guest 可以用前 5 個 ufo */
#define NUMUFOS_USER	20	/* 一般使用者 可以用前 20 個 ufo */

#define STR_UFO		"-mpsnemPBQFANbwtSHZJ----CHA"		/* itoc: 新增習慣的時候別忘了改這裡啊 */


#ifdef _ADMIN_C_

char *ufo_tbl[NUMUFOS] =
{
  "保留",				/* UFO_NOUSE */
  "動態看板        (開啟/關閉)",	/* UFO_MOVIE */

  "看板列表顯示    (文章/編號)",	/* UFO_BRDPOST */
  "看板列表排序    (字母/分類)",	/* UFO_BRDNAME */	/* itoc.010413: 看板依照字母/分類排序 */
  "進板畫面        (顯示/跳過)",	/* UFO_BRDNOTE */
  "文章編輯器      (簡化/完整)",	/* UFO_VEDIT */
  "進/離站畫面     (簡化/完整)",	/* UFO_MOTD */

  "呼叫器          (好友/所有)",	/* UFO_PAGER */
#ifdef HAVE_NOBROAD
  "廣播天線        (拒收/接收)",	/* UFO_RCVER */
#else
  "保留",
#endif
  "遠離塵囂        (安靜/接收)",	/* UFO_QUITE */

  "使用者名單顯示  (好友/全部)",	/* UFO_PAL */

#ifdef HAVE_ALOHA
  "接受上站通知    (通知/取消)",	/* UFO_ALOHA */
#else
  "保留",
#endif
#ifdef HAVE_NOALOHA
  "上站送通知/協尋 (不送/通知)",	/* UFO_NOALOHA */
#else
  "保留",
#endif

#ifdef BMW_DISPLAY
  "水球回顧介面    (完整/上次)",	/* UFO_BMWDISPLAY */
#else
  "保留",
#endif
  "不儲存水球紀錄  (刪除/選擇)",	/* UFO_NWLOG */
  "不儲存聊天紀錄  (刪除/選擇)",	/* UFO_NTLOG */

  "不使用簽名檔    (不用/選擇)",	/* UFO_NOSIGN */
  "顯示簽名檔      (顯示/不看)",	/* UFO_SHOWSIGN */

#ifdef HAVE_MULTI_BYTE
  "全型字偵測      (偵測/不用)",	/* UFO_ZHC */
#else
  "保留",
#endif

#ifdef AUTO_JUMPBRD
  "跳去未讀看板    (跳去/不跳)",	/* UFO_JUMPBRD */
#else
  "保留",
#endif

  "保留",
  "保留",
  "保留",
  "保留",

  "隱身術          (隱身/現身)",	/* UFO_CLOAK */
#ifdef HAVE_SUPERCLOAK
  "超級隱身術      (紫隱/現身)",	/* UFO_SUPERCLOAK */
#else
  "保留",
#endif

  "站長上站來源    (限制/任意)"		/* UFO_ACL */
};
#endif

#endif				/* _UFO_H_ */
