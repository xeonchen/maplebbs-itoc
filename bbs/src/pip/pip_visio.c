/*-------------------------------------------------------*/
/* pipvisio.c	( NTHU CS MapleBBS Ver 3.10 )      	 */
/*-------------------------------------------------------*/
/* target : 養小雞遊戲                                   */
/* create :   /  /                                       */
/* update : 01/08/03                                     */
/* author : dsyan.bbs@forever.twbbs.org                  */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/* picture: tiball.bbs@bbs.nhctc.edu.tw                  */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME

#include "pip.h"


#if 0	/* itoc.010803.註解:螢幕配置 */

最前面      一些資料顯示

中間        圖

b_lines - 2 詢問列
b_lines - 1 指令列
b_lines     指令列

#endif



/*-------------------------------------------------------*/
/* 指令取得                                              */
/*-------------------------------------------------------*/


int			/* itoc.010802: 取代 vans("Y/N？[Y] ") 這類問題用的函式 */
ians(x, y, msg)
  int x, y;		/* itoc.010803: 一般拿 (b_lines - 2, 0) 來當詢問列 */
  char *msg;
{
  move(x, 0);		/* 清掉整列 */
  clrtoeol();
  move(x, y);
  outs(msg);
  outs("\033[47m  \033[m");
  move(x, y + strlen(msg));	/* 移標移動框框內 */
  return vkey();		/* 只按鍵，不需要按 ENTER */

#if 0
  if (ch >= 'A' && ch <= 'Z')
    ch |= 0x20;		/* 換小寫 */
#endif
}


/*-------------------------------------------------------*/
/* 螢幕控制						 */
/*-------------------------------------------------------*/


void
clrfromto(from, to)	/* 清除 from~to 列，最後游標停留在 (from, 0) */
  int from, to;
{
  while (to >= from)
  {
    move(to, 0);
    clrtoeol();
    to--;
  }
}


static int			/* 1: 檔案在  0: 檔案不在 */
show_file(fpath, ln, lines)	/* 從第 ln 列開始印檔案 lines 列 */
  char *fpath;
  int ln, lines;
{
  FILE *fp;
  char buf[ANSILINELEN];
  int i;

  if ((fp = fopen(fpath, "r")))
  {
    clrfromto(ln, ln + lines);
    i = lines;
    while (fgets(buf, ANSILINELEN, fp) && i--)
      outs(buf);
    fclose(fp);
    return 1;
  }

  /* itoc.010802: 回報檔案遺失 */
  sprintf(buf, "%s 遺失，請告知站長", fpath);
  zmsg(buf);
  return 0;
}


/*-------------------------------------------------------*/
/* 小雞圖形區                  				 */
/*-------------------------------------------------------*/


/* itoc.010801: 請依照各 show_xxx_pic() 中 show_file(buf, ln, lines) 的 lines 來控制圖案列數 */
/* itoc.010801: 正常狀況下都是 10 列 (剛好也是動態看板的列數)。若 ln=0 且 lines=b_lines+1 表示整個螢幕全清 */


/* -------------------- */
/* 一般的圖，在 7~16 列 */
/* -------------------- */

int
show_basic_pic(i)
  int i;
{
  char buf[64];
  sprintf(buf, PIP_PICHOME "basic/pic%d", i);
  return show_file(buf, 7, 10);
}


int
show_feed_pic(i)		/* 吃東西 */
  int i;
{
  char buf[64];
  sprintf(buf, PIP_PICHOME "feed/pic%d", i);
  return show_file(buf, 7, 10);
}


int
show_usual_pic(i)		/* 平常狀態 */
  int i;
{
  char buf[64];
  sprintf(buf, PIP_PICHOME "usual/pic%d", i);
  return show_file(buf, 7, 10);
}


int
show_special_pic(i)		/* 特殊選單 */
  int i;
{
  char buf[64];
  sprintf(buf, PIP_PICHOME "special/pic%d", i);
  return show_file(buf, 7, 10);
}

int
show_practice_pic(i)		/* 修行用的圖 */
  int i;
{
  char buf[64];
  sprintf(buf, PIP_PICHOME "practice/pic%d", i);
  return show_file(buf, 7, 10);
}


int
show_job_pic(i)			/* 打工的show圖 */
  int i;
{
  char buf[64];
  sprintf(buf, PIP_PICHOME "job/pic%d", i);
  return show_file(buf, 7, 10);
}


int
show_play_pic(i)		/* 休閒的圖 */
  int i;
{
  char buf[64];
  sprintf(buf, PIP_PICHOME "play/pic%d", i);
  return show_file(buf, 7, 10);
}


int
show_guess_pic(i)		/* 猜拳用 */
  int i;
{
  char buf[64];
  sprintf(buf, PIP_PICHOME "guess/pic%d", i);
  return show_file(buf, 7, 10);
}


int
show_badman_pic(i)		/* 怪物 */
  int i;
{
  char buf[64];

/* itoc.010731:  picabc   abc 是編號
   百位數 a 是分類，十位數個位數 bc 是該類圖的編號 */

  sprintf(buf, PIP_PICHOME "badman/pic%03d", i);
  return show_file(buf, 7, 10);
}


int
show_fight_pic(i)		/* 打架 */
  int i;
{
  char buf[64];
  sprintf(buf, PIP_PICHOME "fight/pic%d", i);
  return show_file(buf, 7, 10);
}


int
show_resultshow_pic(i)		/* 收穫季 */
  int i;
{
  char buf[64];
  sprintf(buf, PIP_PICHOME "resultshow/pic%d", i);
  return show_file(buf, 7, 10);
}


int
show_quest_pic(i)		/* 任務 */
  int i;
{
  char buf[64];
  sprintf(buf, PIP_PICHOME "quest/pic%d", i);
  return show_file(buf, 7, 10);
}


/* -------------------------- */
/* 一些特殊的圖，秀在 1~10 列 */
/* -------------------------- */

int
show_weapon_pic(i)		/* 武器用 */
  int i;
{
  char buf[64];
  sprintf(buf, PIP_PICHOME "weapon/pic%d", i);
  return show_file(buf, 1, 10);
}


int
show_palace_pic(i)		/* 參見王臣用 */
  int i;
{
  char buf[64];
  sprintf(buf, PIP_PICHOME "palace/pic%d", i);
  return show_file(buf, 1, 10);
}


/* ------------------------------------- */
/* 一些特殊的圖，秀在 4~19(b_lines-4) 列 */
/* ------------------------------------- */

int
show_system_pic(i)		/* 系統 */
  int i;
{
  char buf[64];
  sprintf(buf, PIP_PICHOME "system/pic%d", i);
  return show_file(buf, 4, 16);
}


int
show_ending_pic(i)		/* 結束 */
  int i;
{
  char buf[64];
  sprintf(buf, PIP_PICHOME "ending/pic%d", i);
  return show_file(buf, 4, 16);
}


/* -------------------------- */
/* 一些特殊的圖，秀在整個螢幕 */
/* -------------------------- */

int
show_die_pic(i)			/* 死亡 */
  int i;
{
  char buf[64];
  sprintf(buf, PIP_PICHOME "die/pic%d", i);
  return show_file(buf, 0, b_lines + 1);
}
#endif	/* HAVE_GAME */
