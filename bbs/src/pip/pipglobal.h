/*-------------------------------------------------------*/
/* pip_global.h	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : global definitions & variables		 */
/* create : 01/07/25				 	 */
/* update : 01/08/02                                     */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#ifndef	_PIP_GLOBAL_H_
#define _PIP_GLOBAL_H_


#ifdef  _PIPMAIN_C_
# define VAR
# define INI(x)		= x
#else
# define VAR		extern
# define INI(x)
#endif


/* ----------------------------------------------------- */
/* GLOBAL DEFINITION					 */
/* ----------------------------------------------------- */


  /* --------------------------------------------------- */
  /* 遊戲名稱設定                               	 */
  /* --------------------------------------------------- */


#define PIPNAME		"寵物雞"


  /* --------------------------------------------------- */
  /* 個人目錄檔名設定                                    */
  /* --------------------------------------------------- */


#define FN_PIP		"chicken"

VAR char *fn_pip	INI(FN_PIP);


/* ----------------------------------------------------- */
/* GLOBAL VARIABLE					 */
/* ----------------------------------------------------- */


VAR struct CHICKEN d;		/* 小雞的資料 */

VAR time_t start_time;		/* 本次遊戲開始時間 */
VAR time_t last_time;		/* 上次更新時間 */

#undef VAR

#endif				/* _PIP_GLOBAL_H_ */
