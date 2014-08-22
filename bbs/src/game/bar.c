/*-------------------------------------------------------*/
/* bar.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : BAR 台遊戲                                   */
/* create :   /  /                                       */
/* update : 01/04/27                                     */
/* author : unknown                                      */
/* recast : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME


#if 0
  ╭─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─╮
  │♂│Χ│◇│㊣│☆│77│♀│♂│△│◇│Χ│Ω│◇│77│☆│♂│♀│◇│
  ├─┼─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┼─┤
  │Χ│                                                              │㊣│
  ├─┤                                                              ├─┤
  │△│                                                              │△│
  ├─┤                                                              ├─┤
  │Ω│                                                              │Ω│
  ├─┤                                                              ├─┤
  │Χ│                                                              │♀│
  ├─┤                                                              ├─┤
  │77│                                                              │☆│
  ├─┤                      ▓大           □小                     ├─┤
  │◇│                                                              │Χ│
  ├─┼─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┼─┤
  │㊣│♀│77│Χ│◇│△│♂│☆│♀│Χ│㊣│Ω│◇│77│Χ│♂│△│77│
  ╰─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─╯
#endif

static char *itemlist[10] =  {"  ", "Ω", "♀", "♂", "㊣", "△", "☆", "77", "◇", "Χ"};	/* 項目名 */

static int bar[49] = 	/* 上圖 48 格中(由左上"♂"開始順時針繞)所對應的 itemlist[] */
{
  0, 3, 9, 8, 4, 6, 7, 2, 3, 5,
  8, 9, 1, 8, 7, 6, 3, 2, 8, 4,
  5, 1, 2, 6, 9, 7, 5, 3, 9, 7,
  8, 1, 4, 9, 2, 6, 3, 5, 8, 9,
  7, 2, 4, 8, 7, 9, 1, 5, 9
};				/* 板面位址 */

static int money[10];		/* 押金 */


static int
total_money()			/* 是否有下注 */
{
  if (money[1] || money[2] || money[3] || money[4] || money[5] ||
    money[6] || money[7] || money[8] || money[9])
  {
    return 1;
  }
  return 0;
}


static void
run(step, last, freq)
  int step;		/* 位置 */
  int last;		/* 1: 最後一步  0: 中間步 */
  int freq;		/* frequency: Hz */
{
  int x1, y1, x2, y2;

  if (step == 1)
  {
    x1 = 6;
    y1 = 4;
    x2 = 4;
    y2 = 4;
  }

  else if (step > 1 && step < 19)
  {
    x1 = 4;
    y1 = 4 * step - 4;
    x2 = 4;
    y2 = 4 * step;
  }

  else if (step > 18 && step < 26)
  {
    x1 = 2 * step - 34;
    y1 = 72;
    x2 = 2 * step - 32;
    y2 = 72;
  }

  else if (step > 25 && step < 43)
  {
    x1 = 18;
    y1 = 176 - 4 * step;
    x2 = 18;
    y2 = 172 - 4 * step;
  }

  else if (step > 42 && step < 49)
  {
    x1 = 104 - 2 * step;
    y1 = 4;
    x2 = 102 - 2 * step;
    y2 = 4;
  }

  move(x1, y1);
  if (step == 1)
    outs(itemlist[9]);
  else
    outs(itemlist[bar[step - 1]]);

  move(x2, y2);
  if (last)
    outs(itemlist[bar[step]]);
  else
    outs("▓");

  refresh();
  usleep(1000000 / freq);	/* 等待 */
}


static inline int
get_item()		/* 亂數選取要中的項目 */
{

#if 0
編號：   1     2     3     4     5     6     7     8     9
項目： "Ω", "♀", "♂", "㊣", "△", "☆", "77", "◇", "Χ"
機率：  13    16    21    26    32    43    64   127   308    /650
賠率：  50    40    30    25    20    15    10     5     2
#endif

  int randnum = rnd(650);	/* 賠率 * 機率 = 期望值 (每押 1 元所回收的金額) */

  if (randnum < 308)		/* 2 * 308 / 650 = 0.948 */
    return 9;
  if (randnum < 435)		/* 5 * 127 / 650 = 0.978 */
    return 8;
  if (randnum < 499)		/* 10 * 64 / 650 = 0.985 */
    return 7;
  if (randnum < 542)		/* 15 * 43 / 650 = 0.992 */
    return 6;
  if (randnum < 574)		/* 20 * 32 / 650 = 0.985 */
    return 5;
  if (randnum < 600)		/* 25 * 26 / 650 = 1.000 */
    return 4;
  if (randnum < 621)		/* 30 * 21 / 650 = 0.969 */
    return 3;
  if (randnum < 637)		/* 40 * 16 / 650 = 0.985 */
    return 2;

  return 1;			/* 50 * 13 / 650 = 1.000 */
}


static inline int	/* 傳回 map 上的 1~48 其中一個數字 */
get_dst(item)		/* 傳入中的項目選取最後停留的位置 */
  int item;
{
  int dst, randnum;

  randnum = rnd(48) + 1;	/* 從中間往前後選取，避免每次都從 1 選會都中右上角的部分 */

  for (dst = randnum; dst <= 48; dst++)
  {
    if (bar[dst] == item)
      return dst;
  }

  for (dst = randnum; dst >= 1; dst--)
  {
    if (bar[dst] == item)
      return dst;
  }

  return 0;
}


static inline void
print_total()
{
  outs("\n\n");
  outs("  ╭─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─╮\n");
  outs("  │♂│Χ│◇│㊣│☆│77│♀│♂│△│◇│Χ│Ω│Χ│77│☆│♂│♀│◇│\n");
  outs("  ├─┼─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┼─┤\n");
  outs("  │Χ│                                                              │㊣│\n");
  outs("  ├─┤                                                              ├─┤\n");
  outs("  │△│                                                              │△│\n");
  outs("  ├─┤                                                              ├─┤\n");
  outs("  │Ω│                                                              │Ω│\n");
  outs("  ├─┤                                                              ├─┤\n");
  outs("  │Χ│                                                              │♀│\n");
  outs("  ├─┤                                                              ├─┤\n");
  outs("  │77│                                                              │☆│\n");
  outs("  ├─┤                      ▓大           □小                     ├─┤\n");
  outs("  │◇│                                                              │Χ│\n");
  outs("  ├─┼─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┼─┤\n");
  outs("  │㊣│♀│77│Χ│◇│△│♂│☆│♀│Χ│㊣│Ω│◇│77│Χ│♂│△│77│\n");
  outs("  ╰─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─╯\n");
  outs("    \033[1;31m│Ω│  \033[32m│♀│  \033[33m│♂│  \033[34m│㊣│  \033[35m│△│  \033[36m│☆│  \033[37m│77│  \033[0;36m│◇│  \033[33m│Χ│ \033[m\n");
  outs("    \033[1;31m│50│  \033[32m│40│  \033[33m│30│  \033[34m│25│  \033[35m│20│  \033[36m│15│  \033[37m│10│  \033[0;36m│ 5│  \033[33m│ 2│ \033[m");
}


int
main_bar()
{
  int price[10] = {0, 50, 40, 30, 25, 20, 15, 10, 5, 2};	/* 倍率 */

  int item;			/* 項目編號 */
  int ogn, dst;			/* OriGiN: 起點  DeStinaTion: 終點 */  
  int ch, i, j;
  char buf[80];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  vs_bar("BAR 台");
  print_total();		/* 印板面 */
  ogn = 1;			/* 起點在第一格 */

  while (1)
  {
    for (i = 1; i < 10; i++)	/* money 歸零 */
      money[i] = 0;

    for (;;)
    {
      /* 決定各項賭注 */

      ch = vget(2, 0, "您要押哪項(1-9)？[S]開始 [Q]離開：", buf, 3, DOECHO);
      if (!ch || ch == 's')
      {
	if (total_money())
	  break;
	addmoney(money[1] + money[2] + money[3] + money[4] + money[5] +	money[6] + money[7] + money[8] + money[9]);	/* 還錢 */
	goto abort_game;
      }
      else if (ch < '1' || ch > '9')
      {
	addmoney(money[1] + money[2] + money[3] + money[4] + money[5] +	money[6] + money[7] + money[8] + money[9]);	/* 還錢 */
	goto abort_game;
      }

      if (!(vget(2, 0, "要押多少賭金？", buf, 6, DOECHO)))
      {
	if (total_money())
	  break;
	addmoney(money[1] + money[2] + money[3] + money[4] + money[5] +	money[6] + money[7] + money[8] + money[9]);	/* 還錢 */
	goto abort_game;
      }

      j = atoi(buf);
      if (j < 1 || j > cuser.money)
      {
	addmoney(money[1] + money[2] + money[3] + money[4] + money[5] +	money[6] + money[7] + money[8] + money[9]);	/* 還錢 */
	goto abort_game;
      }

      cuser.money -= j;
      money[ch - '0'] += j;

      move(b_lines - 1, 0);
      clrtoeol();
      outs("\033[1m   ");
      for (i = 1; i < 10; i++)
	prints("\033[3%dm%6d  ", i, money[i]);  
      prints("\n\033[m         籌碼還有 %d 元     ", cuser.money);
    }

    /* 開始跑了 */

    item = get_item();		/* 亂數選取中的項目 */
    dst = get_dst(item);	/* 由所中的項目來決定最後停留的位置 */

    for (i = ogn; i <= 48; i++)	/* 第一圈 */
    {
      run(i, 0, 10 + i / 10);	/* 起始加速，每秒 10 - 14 次，越跑越快 */
    }

    for (j = 0; j < 2; j++)	/* 中間跑二圈就好了 */
    {
      for (i = 1; i < 49; i++)	/* 中間的圈 */
      {
	run(i, 0, 15);		/* 中段高速，維持每秒 15 次 */
      }
    }

    for (i = 1; i <= dst; i++)	/* 最後一圈 */
    {
      run(i, 0, 14 - i / 10);	/* 最後減速，每秒 14 - 10 次 */
    }

    for (j = 0; j < 2; j++)	/* 在最後停留的位置閃二次 */
    {
      run(dst, 0, 2);
      run(dst, 1, 2);
    }

    move(2, 0);
    clrtoeol();
    prints("中獎的是 \033[37m%s\033[m，", itemlist[item]);

    if (!money[item])
    {
      outs("摃龜 +_+");
    }
    else
    {
      money[0] = money[item] * price[item];
      prints("恭喜您押中了，獲得獎金 \033[32m%d\033[m", money[0]);

      for (;;)		/* 可以一直比大小比到爽 */
      {
	sprintf(buf, "目前獎金: %d 您還要比大小嗎(Y/N)？[N] ", money[0]);

	if (vans(buf) != 'y')	/* 不比大小 */
	{
	  move(2, 0);
	  clrtoeol();
	  prints("得獎金 %d", money[0]);
	  break;
	}
	else			/* 比大小 */
	{
	  sprintf(buf, "您要押什麼？ [1]大 (2)小 ");
	  ch = vans(buf) - '1';		/* ch = 0:大  1:小 */

	  for (i = 16; i <= 20; i++)
	  {
	    move(15, 30);
	    outs("▓大           □小");
	    refresh();
	    usleep(6000 * (i ^ 2));

	    move(15, 30);
	    outs("□大           ▓小");
	    usleep(6000 * (i ^ 2));
	    refresh();
	  }

	  price[0] = rnd(2);
	  move(15, 30);
	  if (price[0])
	    outs("▓大           □小");
	  else
	    outs("□大           ▓小");

	  if (price[0] == ch)
	  {
	    money[0] *= 2;
	    move(2, 0);
	    clrtoeol();
	    outs("啊！押中了！獎金變成二倍！");
	  }
	  else
	  {
	    money[0] = 0;
	    move(2, 0);
	    clrtoeol();
	    outs("答錯了！零分！");
	    break;	/* 比大小結束 */
	  }
	}
      }
      addmoney(money[0]);
    }		/* 比大小結束 */

    vmsg("繼續下一盤大賽");
    move(b_lines, 0);
    clrtoeol();			/* 清除請按任意鍵繼續 */

    ogn = dst;			/* 下次起點是上次的終點 */
  }

abort_game:
  return 0;
}
#endif	/* HAVE_GAME */
