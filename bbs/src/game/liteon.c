/*-------------------------------------------------------*/
/* liteon.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : 開燈遊戲					 */
/* create : 02/05/23					 */
/* update :   /  /                                       */
/* author : Gein.bbs@csdc.twbbs.org			 */
/* recast : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "bbs.h"


#ifdef HAVE_GAME

#define MAX_LEVEL	(b_lines - 3)

enum
{
  TL_XPOS = 2,
  TL_YPOS = 5,

  /* 用 bitwise operators */
  TILE_BLANK = 0,	/* 暗區 */
  TILE_LIGHT = 1	/* 亮區 */
};


static int cx, cy;	/* 目前所在游標 */
static int level;	/* 等級，同時也是 board 的邊長 */
static int onturn;	/* 有幾個燈打開了 */
static int candle;	/* 點了幾次蠟燭 */
static int tl_board[T_LINES - 4][T_LINES - 4];


static void 
tl_setb()			/* set board all 0 */
{
  int i, j;

  move(1, 0);
  clrtobot();

  for (i = 0; i < level; i++)
  {
    move(i + TL_XPOS, TL_YPOS);
    for (j = 0; j < level; j++)
    {
      tl_board[i][j] = TILE_BLANK;
      outs("○");
    }
  }

  cx = cy = onturn = candle = 0;
  move(TL_XPOS, TL_YPOS + 1);	/* move back to (0, 0) */
}


static void
tl_draw(x, y)			/* set/reset and draw a tile */
  int x, y;
{
  tl_board[x][y] ^= TILE_LIGHT;
  move(x + TL_XPOS, y * 2 + TL_YPOS);
  if (tl_board[x][y] == TILE_BLANK)	/* on-turn -> off-turn */
  {
    onturn--;
    outs("○");
  }
  else					/* off-turn -> on-turn */
  {
    onturn++;
    outs("●");
  }
}


static void 
tl_turn()			/* turn light and light arround it */
{
  tl_draw(cx, cy);

  if (cx > 0)
    tl_draw(cx - 1, cy);

  if (cx < level - 1)
    tl_draw(cx + 1, cy);

  if (cy > 0)
    tl_draw(cx, cy - 1);

  if (cy < level - 1)
    tl_draw(cx, cy + 1);
}


static void 
tl_candle()			/* cheat: use candle */
{
  /* itoc.註解: 因為大家都破不了這遊戲，所以提供一下作弊用的點蠟燭 */
  tl_draw(cx, cy);
  candle++;
}

static int			/* 1:win 0:lose */
tl_play()			/* play turn_light */
{
  tl_setb();

  while (onturn != level * level)
  {
    switch (vkey())
    {
    case KEY_LEFT:
      cy--;
      if (cy < 0)
	cy = level - 1;
      break;

    case KEY_RIGHT:
      cy++;
      if (cy == level)
	cy = 0;
      break;

    case KEY_UP:
      cx--;
      if (cx < 0)
	cx = level - 1;
      break;

    case KEY_DOWN:
      cx++;
      if (cx == level)
	cx = 0;
      break;

    case 'c':
      tl_candle();
      break;

    case ' ':
    case '\n':
      tl_turn();
      break;

    case 'r':
      tl_setb();
      break;

    case 'q':
      return 0;
    }
    move(cx + TL_XPOS, cy * 2 + TL_YPOS + 1);	/* move back to current (x, y) */
  }
  return 1;
}


int
main_liteon()
{
  char ans[5], buf[80];

  sprintf(buf, "請選擇等級(1～%d)，或按 [Q] 離開：", MAX_LEVEL);
  level = vget(b_lines, 0, buf, ans, 3, DOECHO);
  if (level == 'q' || level == 'Q')
  {
    return XEASY;
  }
  else
  {
    level = atoi(ans);
    if (level < 1 || level > MAX_LEVEL)
      return XEASY;
  }

  vs_bar("開燈遊戲");
  move(4, 13);
  outs("前情提要：");
  move(5, 15);
  outs("有一天，小建回到家發現燈都被關了。");
  move(6, 15);
  outs("可是他家的燈有一個特性，那就是：");
  move(7, 15);
  outs("當一盞燈被按下開關以後，他周圍的燈");
  move(8, 15);
  outs("原本亮的，就會變暗，原本暗的，就會變亮。 -____-#");
  move(9, 15);
  outs("現在就請聰明的您幫他把所有燈打開吧！");

  move(11, 13);
  outs("按鍵說明：");
  move(12, 15);
  outs("↑↓←→     移動方向");
  move(13, 15);
  outs("Enter/Space  切換開關");
  move(14, 15);
  outs("c            點燃蠟燭 [密技]");
  move(15, 15);
  outs("r            重新來過");
  move(16, 15);
  outs("q            離開遊戲");

  vmsg(NULL);

  if (tl_play())		/* if win */
  {
    sprintf(buf, "恭喜您成功\了  (用了 %d 根蠟燭)", candle);
    vmsg(buf);
  }

  return 0;
}
#endif				/* HAVE_GAME */
