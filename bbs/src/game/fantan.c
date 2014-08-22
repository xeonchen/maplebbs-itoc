/*-------------------------------------------------------*/
/* fantan.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : 接龍遊戲					 */
/* create : 98/08/04					 */
/* update : 01/03/01					 */
/* author : dsyan.bbs@Forever.twbbs.org			 */
/* recast : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME

enum
{
  SIDE_UP = 1,				/* 在上面 */
  SIDE_DOWN = 0				/* 在下面 */
};


static inline int
cal_kind(card)				/* 算花色 */
  int card;
{
  /* card:  1 - 13 第○種花色 ☆
	   14 - 26 第一種花色 ★
	   27 - 39 第二種花色 ○
	   40 - 52 第三種花色 ● */

  return (card - 1) / 13;
}


static inline int 
cal_num(card)				/* 算點數 */
  int card;
{
  card %= 13;
  return card ? card : 13;
}


static void
move_cur(a, b, c, d)			/* 移動箭號 */
  int a, b;		/* 原位置 */
  int c, d;		/* 新位置 */
{
  move(a, b);
  outs("  ");
  move(c, d);
  outs("→");
  move(c, d + 1);	/* 避免自動偵測全形 */
}


static inline int
ycol(y)					/* 算座標 */
  int y;
{
  return y * 11 - 8;
}


static void
draw(x, y, card)			/* 畫牌 */
  int x, y;
  int card;
{
  char kind[4][3] = {"☆", "★", "○", "●"};			/* 花色 */
  char num[13][3] = {"Ａ", "２", "３", "４", "５", "６", "７",	/* 點數 */
		     "８", "９", "Ｔ", "Ｊ", "Ｑ", "Ｋ"};

  move(x, y);

  if (card > 0)
    prints("├%s%s┤", kind[cal_kind(card)], num[cal_num(card) - 1]);
  else if (!card)
    outs("├──┤");
  else
    outs("        ");
}


static inline void
out_prompt()			/* 提示字眼 */
{
  move(b_lines - 1, 0);
  outs(COLOR2 " (q)離開 (r)重玩 (←→↑↓)移動 (↑)翻轉 (Enter)堆疊 (Space)指定/移動/翻牌    \033[m");
}


static char
get_newcard(mode)
  int mode;			/* 0:重新洗牌  1:發牌 */
{
  static char card[52];		/* 最多只會用到 52 張牌 */
  static int now;		/* 發出第 now 張牌 */
  int i, num;
  char tmp;

  if (!mode)   /* 重新洗牌 */
  {
    for (i = 0; i < 52; i++)
      card[i] = i + 1;

    for (i = 0; i < 51; i++)
    {
      num = rnd(52 - i) + i;

      /* card[num] 和 card[i] 交換 */
      tmp = card[i];
      card[i] = card[num];
      card[num] = tmp;
    }

    now = 0;
    return -1;
  }

  tmp = card[now];
  now++;
  return tmp;
}


int
main_fantan()
{
  char max[9];				/* 每堆的牌數，第八堆是左上角的牌 */
  char rmax[8];				/* 每堆未尚被翻開的牌數 */

  char left_stack[25];			/* 左上角的 24 張牌 */
  char up_stack[5];			/* 右上角的 4 張牌 (只要記錄最大牌即可) */
  char down_stack[8][21];		/* 下面七個堆疊的所有牌 */

  int level;				/* 一次翻幾張牌 */
  int side;				/* 游標在上面還是下面 */

  int cx, cy;				/* 目前所在 (x, y) 座標 */
  int xx, yy;				/* 過去所在 (x, y) 座標 */
  int star_c, star_x, star_y;		/* 打 '*' 處的 牌、座標 */
  int left;				/* 左上角堆疊翻到第幾張 */

  int i, j;

  time_t init_time;			/* 遊戲開始的時間 */

  level = vans("請選擇一次翻 [1~3] 一～三 張牌，或按 [Q] 離開：");
  if (level > '0' && level < '4')
    level -= '0';
  else
    return XEASY;

game_start:
  vs_bar("接龍");
  out_prompt();

  side = SIDE_DOWN;
  star_c = 0;
  star_x = 2;
  star_y = 79;

  for (i = 0; i <= 4; i++)			/* 上面的四個堆疊歸零 */
    up_stack[i] = 0;

  get_newcard(0);	/* 洗牌 */

  for (i = 1; i <= 7; i++)
  {
    max[i] = i;					/* 第 i 堆剛開始有 i 張牌 */
    rmax[i] = i - 1;				/* 第 i 堆剛開始有 i-1 張未打開 */
    for (j = 1; j <= i; j++)
    {
      down_stack[i][j] = get_newcard(1);	/* 配置下面的牌 */
      draw(j + 2, ycol(i), i != j ? 0 : down_stack[i][j]);	/* 每堆打開最後一張牌 */
    }
  }

  max[8] = 24;					/* 左上角剛開始有 24 張牌 */
  for (i = 1; i <= 24; i++)
    left_stack[i] = get_newcard(1);		/* 配置左上角的牌 */
  draw(1, 1, 0);

  left = 0;
  cx = 1;
  cy = 1;
  xx = 1;
  yy = 1;

  init_time = time(0);		/* 開始記錄時間 */

  for (;;)
  {
    if (side == SIDE_DOWN)
    {
      move_cur(xx + 2, ycol(yy) - 2, cx + 2, ycol(cy) - 2);
      xx = cx;
      yy = cy;

      switch (vkey())
      {
      case 'q':
	vmsg(MSG_QUITGAME);
	return 0;

      case 'r':
	goto game_start;

      case KEY_LEFT:
	cy--;
	if (!cy)
	  cy = 7;
	if (cx > max[cy] + 1)
	  cx = max[cy] + 1;
	break;

      case KEY_RIGHT:
	cy++;
	if (cy == 8)
	  cy = 1;
	if (cx > max[cy] + 1)
	  cx = max[cy] + 1;
	break;

      case KEY_DOWN:
	cx++;
	if (cx == max[cy] + 2)
	  cx--;
	break;

      case KEY_UP:
	cx--;
	if (!cx)					/* 跑到上面去了 */
	{
	  side = SIDE_UP;
	  move_cur(xx + 2, ycol(yy) - 2, 1, 9);
	}
	break;

      case '\n':				/* 拿牌到右上角 */
	j = down_stack[cy][cx];
	if ((cal_num(j) == up_stack[cal_kind(j)] + 1) && cx == max[cy] && cx > rmax[cy])
	{
	  up_stack[cal_kind(j)]++;
	  max[cy]--;
	  draw(1, cal_kind(j) * 10 + 40, j);
	  draw(cx + 2, ycol(cy), -1);
	  if (star_c == j)			/* 如果有記號就消掉 */
	  {
	    move(star_x, star_y);
	    outc(' ');
	  }
	}
	/* 破關條件: 右上角四個都是 13 */
	if (up_stack[0] & up_stack[1] & up_stack[2] & up_stack[3] == 13)
	{
	  char buf[80];
	  sprintf(buf, "您花了 %.0lf 秒 破第 %d 關 好崇拜 ^O^", 
	    difftime(time(0), init_time), level);
	  vmsg(buf);
	  addmoney(level * 100);
	  return 0;	  
	}
	break;

      case ' ':
	if (cx == max[cy] && cx == rmax[cy])	/* 翻新牌 */
	{
	  rmax[cy]--;
	  draw(cx + 2, ycol(cy), down_stack[cy][cx]);
	  break;
	}
	else if (cx > rmax[cy] && cx <= max[cy])	/* 剪下 */
	{
	  move(star_x, star_y);
	  outc(' ');
	  star_c = down_stack[cy][cx];
	  star_x = cx + 2;
	  star_y = cy * 11;
	  move(star_x, star_y);
	  outc('*');
	  break;
	}
	else if (cx != max[cy] + 1)
	  break;				/* 貼上 */

	if ((max[cy] && (cal_num(down_stack[cy][max[cy]]) == cal_num(star_c) + 1) && 
	  (cal_kind(down_stack[cy][max[cy]]) + cal_kind(star_c)) % 2) ||
	  (max[cy] == 0 && cal_num(star_c) == 13))
	{
	  if (star_x == 1)		/* 從上面貼下來的 */
	  {
	    max[cy]++;
	    max[8]--;
	    star_x = 2;
	    left--;
	    for (i = left + 1; i <= max[8]; i++)
	      left_stack[i] = left_stack[i + 1];
	    down_stack[cy][max[cy]] = star_c;
	    draw(max[cy] + 2, ycol(cy), star_c);
	    move(1, 19);
	    outc(' ');
	    draw(1, 11, left ? left_stack[left] : -1);
	  }
	  else if (star_x > 2)		/* 在下面貼來貼去的 */
	  {
	    int tmp;;
	    j = star_y / 11;
	    tmp = max[j];	    
	    for (i = star_x - 2; i <= tmp; i++)
	    {
	      max[cy]++;
	      max[j]--;
	      down_stack[cy][max[cy]] = down_stack[j][i];
	      draw(max[cy] + 2, ycol(cy), down_stack[cy][max[cy]]);
	      draw(i + 2, ycol(j), -1);
	    }
	    move(star_x, star_y);
	    outc(' ');
	    star_x = 2;
	  }
	}
	break;
      }

    }
    else /* side == SIDE_UP */	/* 在上面 */
    {
      draw(1, 11, left ? left_stack[left] : -1);

      switch (vkey())
      {
      case 'q':
	vmsg(MSG_QUITGAME);
	return 0;

      case 'r':
	goto game_start;

      case '\n':				/* 拿牌到右上角 */
	j = left_stack[left];
	if (cal_num(j) == up_stack[cal_kind(j)] + 1)
	{
	  up_stack[cal_kind(j)]++;
	  max[8]--;
	  left--;
	  draw(1, cal_kind(j) * 10 + 40, j);

	  for (i = left + 1; i <= max[8]; i++)
	    left_stack[i] = left_stack[i + 1];

	  draw(1, 11, left ? left_stack[left] : -1);

	  if (star_x == 1)	/* 如果有記號就清掉 */
	  {
	    star_x = 2;
	    move(1, 19);
	    outc(' ');
	  }
	  /* 破關條件: 右上角四個都是 13 */
	  if (up_stack[0] & up_stack[1] & up_stack[2] & up_stack[3] == 13)
	  {
	    char buf[80];
	    sprintf(buf, "您花了 %.0lf 秒 破第 %d 關 好崇拜 ^O^", 
	      difftime(time(0), init_time), level);
	    vmsg(buf);
	  }
	}
	break;

      case KEY_DOWN:
	side = SIDE_DOWN;
	cx = 1;
	move_cur(1, 9, cx + 2, ycol(cy) - 2);
	break;

      case KEY_UP:
	if (left == max[8])
	  left = 0;
	else
	  left += level;	/* 一次發 level 張牌 */
	if (left > max[8])
	  left = max[8];

	if (star_x == 1)
	{
	  star_x = 2;
	  move(1, 19);
	  outc(' ');
	}

	draw(1, 1, left == max[8] ? -1 : 0);
	break;

      case ' ':
	if (left > 0)
	{
	  move(star_x, star_y);
	  outc(' ');
	  star_c = left_stack[left];
	  star_x = 1;
	  star_y = 19;
	  move(1, 19);
	  outc('*');
	}
	break;
      }
    }
  }

  return 0;
}
#endif	/* HAVE_GAME */
