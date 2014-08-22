/*-------------------------------------------------------*/
/* innbbsconf.h	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : innbbsd configurable settings		 */
/* create : 95/04/27					 */
/* modify :   /  /  					 */
/* author : skhuang@csie.nctu.edu.tw			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#ifndef _INNBBSCONF_H_
#define _INNBBSCONF_H_

#include "bbs.h"

#include <sys/wait.h>


/* ----------------------------------------------------- */
/* 一般組態						 */
/* ----------------------------------------------------- */

#define VERSION		"0.8-MapleBBS"		/* 版本宣告 */

#define MYBBSID		BBSNAME2		/* Path: 用英文站名 */

#define LOGFILE		"innd/innbbs.log"	/* 記錄檔路徑 */


/* ----------------------------------------------------- */
/* innbbsd 的設定					 */
/* ----------------------------------------------------- */

  /* --------------------------------------------------- */
  /* channel 的設定					 */
  /* --------------------------------------------------- */

#define MAXCLIENT 	20		/* Maximum number of connections accepted by innbbsd */

#define ChannelSize	4096
#define ReadSize	4096

  /* --------------------------------------------------- */
  /* rec_article 的設定					 */
  /* --------------------------------------------------- */

#define	_NoCeM_				/* No See Them 擋信機制 */

#undef	_KEEP_CANCEL_			/* 保留 cancel 的文章於 deleted 板 */


/* ----------------------------------------------------- */
/* bbslink 的設定					 */
/* ----------------------------------------------------- */

#define MAX_ARTS	100		/* 每個 newsgroup 一次最多收幾封信 */

#define BBSLINK_EXPIRE	3600		/* bbslink 若執行過久，就直接 kill 掉(秒) */


/* ----------------------------------------------------- */
/* History 轉信記錄維護					 */
/* ----------------------------------------------------- */

#define EXPIREDAYS	5		/* 轉信記錄保留天數 */
#define HIS_MAINT_HOUR	4		/* time to maintain history database */
#define HIS_MAINT_MIN	30		/* time to maintain history database */

#endif	/* _INNBBSCONF_H_ */
