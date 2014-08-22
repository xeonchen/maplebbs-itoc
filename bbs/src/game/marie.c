/*-------------------------------------------------------*/
/* marie.c         ( NTHU CS MapleBBS Ver 3.10 )         */
/*-------------------------------------------------------*/
/* target : 小瑪莉樂園遊戲                               */
/* create :   /  /                                       */
/* update : 01/04/26                                     */
/* author : unknown                                      */
/* recast : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME

#define MAX_MARIEBET	50000	/* 最多押到 50000 元 */


static inline int
get_item()		/* 亂數選取要中的項目 */
{

#if 0
編號：   0     1     2     3     4     5     6     7     8     9
機率： 200     2     5    10     1    20    40   100   500   122   /1000
賠率：   5   500   200   100  1000    50    25    10     2     0
#endif

  int randnum = rnd(1000);	/* 賠率 * 機率 = 期望值 (每押 1 元所回收的金額) */

  if (randnum < 500)		/* 2 * 0.500 = 1 */
    return 8;
  if (randnum < 700)		/* 5 * 0.200 = 1 */
    return 0;
  if (randnum < 800)		/* 10 * 0.100 = 1 */
    return 7;
  if (randnum < 840)		/* 25 * 0.040 = 1 */
    return 6;
  if (randnum < 860)		/* 50 * 0.020 = 1 */
    return 5;
  if (randnum < 870)		/* 100 * 0.010 = 1 */
    return 3;
  if (randnum < 875)		/* 200 * 0.005 = 1 */
    return 2;
  if (randnum < 877)		/* 500 * 0.002 = 1 */
    return 1;
  if (randnum < 878)		/* 1000 * 0.001 = 1 */
    return 4;

  return 9;			/* 銘謝惠顧機率是 0.122 */
}


int
main_marie()
{
  int c_flag[7] = {1, 5, 10, 50, 100, 500, 1000};		/* 倍率 */
  int price[10] = {5, 500, 200, 100, 1000, 50, 25, 10, 2, 0};	/* 賭率 */
  int x[9] = {0};						/* 各項押金 */

  int w;		/* 倍率的種類 */
  int flag;		/* flag = c_flag[w] */
  int item;		/* 中獎的項目 */
  int xtotal;		/* 總押金 */
  int i, ch;
  FILE *fp;
  char buf[STRLEN];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (!(fp = fopen("etc/game/marie", "r")))
    return XEASY;

  vs_bar("小瑪莉樂園");
  move(1, 0);
  while (fgets(buf, STRLEN, fp))	/* 印出賭場內部擺設 */
    outs(buf);
  fclose(fp);

  w = 0;			/* 第一次進入選擇一倍，第二次以後則跟上次玩一樣 */
  flag = c_flag[w];
  item = 0;
  xtotal = 0;

  while (1)
  {
    move(9, 44);
    prints("\033[1m您身上還有籌碼 %8d 元\033[m", cuser.money);
    move(10, 44);
    prints("\033[1m目前押注的倍率是 \033[46m%6d 倍\033[m", flag);

    move(b_lines - 3, 0);
    for (i = 0; i < 9; i++)
      prints("  %5d", x[i]);

    ch = igetch();		/* 不需用到 vkey() */
    switch (ch)
    {
    case 'w':			/* 切換倍率 */
      w = (w + 1) % 7;
      flag = c_flag[w];		/* 切換倍率時才需要重設 flag */
      break;

    case 'a':			/* 全壓 */
      i = 9 * flag;
      if ((xtotal + i <= MAX_MARIEBET) && (cuser.money >= i))
      {
        cuser.money -= i;
        xtotal += i;
	for (i = 0; i <= 8; i++)
	  x[i] += flag;
      }
      break;

    case '1':			/* 分別下注 */
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      if ((xtotal + flag <= MAX_MARIEBET) && (cuser.money >= flag))
      {
        cuser.money -= flag;
        xtotal += flag;
	x[ch - '1'] += flag;
      }
      break;

    case 's':			/* 開始遊戲 */
    case '\n':
      if (x[0] || x[1] || x[2] || x[3] || x[4] || x[5] || x[6] || x[7] || x[8])
      {		/* 有下注才能開始玩 */
	move(15, 5 + 7 * item);
	outs("  ");   		/* 清除上次中的項目 */
	item = get_item();
	if (item != 9)		/* item=9 是銘謝惠顧 */
	{
	  move(15, 5 + 7 * item);
	  outs("●");		/* 繪上這次中的項目 */

	  if (x[item])
	  {
	    i = x[item] * price[item];
	    addmoney(i);
	    sprintf(buf, "您可得 %d 元", i);
	    vmsg(buf);
	  }
	  else
	  {
	    vmsg("很抱歉，您沒有押中");
	  }
	}
	else
	{
	  vmsg("很抱歉，銘謝惠顧");
	}

	move(b_lines, 0);
	clrtoeol();		/* 清除 vmsg() */

	/* 各項賭金歸零 */
	xtotal = 0;
	for (i = 0; i < 9; i++)
 	  x[i] = 0;
      }
      else
      {
        goto abort_game;
      }
      break;

    case 'q':			/* 離開 */
      goto abort_game;

    }	/* switch 結束 */

  }	/* while 迴圈結束 */

abort_game:
  return 0;
}
#endif	/* HAVE_GAME */
