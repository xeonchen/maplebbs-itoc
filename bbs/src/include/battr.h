/*-------------------------------------------------------*/
/* battr.h	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : Board Attribution				 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/


#ifndef	_BATTR_H_
#define	_BATTR_H_


/* ----------------------------------------------------- */
/* Board Attribution : flags in BRD.battr		 */
/* ----------------------------------------------------- */


#define BRD_NOZAP	0x01	/* 不可 zap */
#define BRD_NOTRAN	0x02	/* 不轉信 */
#define BRD_NOCOUNT	0x04	/* 不計文章發表篇數 */
#define BRD_NOSTAT	0x08	/* 不納入熱門話題統計 */
#define BRD_NOVOTE	0x10	/* 不公佈投票結果於 [record] 板 */
#define BRD_ANONYMOUS	0x20	/* 匿名看板 */
#define BRD_NOSCORE	0x40	/* 不評分看板 */


/* ----------------------------------------------------- */
/* 各種旗標的中文意義					 */
/* ----------------------------------------------------- */


#define NUMBATTRS	7

#define STR_BATTR	"zTcsvA%"			/* itoc: 新增旗標的時候別忘了改這裡啊 */


#ifdef _ADMIN_C_
static char *battr_tbl[NUMBATTRS] =
{
  "不可 Zap",			/* BRD_NOZAP */
  "不轉信出去",			/* BRD_NOTRAN */
  "不記錄篇數",			/* BRD_NOCOUNT */
  "不做熱門話題統計",		/* BRD_NOSTAT */
  "不公開投票結果",		/* BRD_NOVOTE */
  "匿名看板",			/* BRD_ANONYMOUS */
  "不評分看板",			/* BRD_NOSCORE */
};

#endif

#endif				/* _BATTR_H_ */
