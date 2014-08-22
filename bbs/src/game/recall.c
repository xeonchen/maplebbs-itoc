/*-------------------------------------------------------*/
/* recall.c     ( NTHU CS MapleBBS Ver 3.10 )            */
/*-------------------------------------------------------*/
/* target : Memory Game routines                         */
/* create : 01/07/19                                     */
/* update :   /  /                                       */
/* author : einstein@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/

#include "bbs.h"

#ifdef HAVE_GAME

enum
{
  MG_XPOS = 4,
  MG_YPOS = 4,

  /* MAX_X * MAX_Y 必須是偶數 */
  MAX_X = 10,
  MAX_Y = 10,
};


static int cx, cy;
static int board[MAX_X][MAX_Y], isopen[MAX_X][MAX_Y];
static char card[52][3] = {"Ａ", "Ｂ", "Ｃ", "Ｄ", "Ｅ", "Ｆ", "Ｇ", "Ｈ", "Ｉ", "Ｊ", 
			   "Ｋ", "Ｌ", "Ｍ", "Ｎ", "Ｏ", "Ｐ", "Ｑ", "Ｒ", "Ｓ", "Ｔ", 
			   "Ｕ", "Ｖ", "Ｗ", "Ｘ", "Ｙ", "Ｚ", "ａ", "ｂ", "ｃ", "ｄ", 
			   "ｅ", "ｆ", "ｇ", "ｈ", "ｉ", "ｊ", "ｋ", "ｌ", "ｍ", "ｎ", 			   
			   "ｏ", "ｐ", "ｑ", "ｒ", "ｓ", "ｔ", "ｕ", "ｖ", "ｗ", "ｘ", 
			   "ｙ", "ｚ"};



static inline void
init_board()
{
  int i, j, rx, ry, temp, count = 0;

  for (i = 0; i < MAX_X; i++)
  {
    for (j = 0; j < MAX_Y; j++)
    {
      board[i][j] = (count++) / 2;
      isopen[i][j] = 0;
    }
  }

  for (i = 0; i < MAX_X; i++)
  {
    for (j = 0; j < MAX_Y; j++)
    {
      rx = rnd(MAX_X);
      ry = rnd(MAX_Y);
      temp = board[i][j];
      board[i][j] = board[rx][ry];
      board[rx][ry] = temp;
    }
  }

  cx = 0;
  cy = 0;
}


static void
show_board()
{
  int i, j;

  vs_bar("記憶遊戲");

  for (i = 0; i < MAX_X; i++)
  {
    for (j = 0; j < MAX_Y; j++)
    {
      move(MG_XPOS + i, MG_YPOS + j * 2);
      if (isopen[i][j])
      {
	outs(card[board[i][j]]);
      }
      else
      {
	outs("■");
      }
    }
  }

  move(3, 40);
  outs("↑↓←→         方向鍵");
  move(5, 40);
  outs("[Space][Enter]   翻開");
  move(7, 40);
  outs("Q/q              離開");

  move(MG_XPOS + cx, MG_YPOS + cy * 2 + 1);
}


static inline int
valid_pos(x, y)
  int x, y;
{
  if (x < 0 || x >= MAX_X || y < 0 || y >= MAX_Y)
  {
    return 0;
  }
  return 1;
}


static void
get_pos(x, y)
  int *x, *y;
{
  char ch;
  while (1)
  {
    ch = vkey();
    if (ch == KEY_UP && valid_pos(cx - 1, cy))
    {
      cx -= 1;
      move(MG_XPOS + cx, MG_YPOS + cy * 2 + 1);
    }
    else if (ch == KEY_DOWN && valid_pos(cx + 1, cy))
    {
      cx += 1;
      move(MG_XPOS + cx, MG_YPOS + cy * 2 + 1);
    }
    else if (ch == KEY_LEFT && valid_pos(cx, cy - 1))
    {
      cy -= 1;
      move(MG_XPOS + cx, MG_YPOS + cy * 2 + 1);
    }
    else if (ch == KEY_RIGHT && valid_pos(cx, cy + 1))
    {
      cy += 1;
      move(MG_XPOS + cx, MG_YPOS + cy * 2 + 1);
    }
    else if (ch == 'q' || ch == 'Q')
    {
      vmsg(MSG_QUITGAME);
      *x = -1;
      break;
    }
    else if (ch == '\n' || ch == ' ')
    {
      *x = cx;
      *y = cy;
      break;
    }
  }
}


int
main_recall()
{
  int fx, fy, sx, sy, count = 0;

  init_board();
  show_board();

  while (1)
  {

    while (1)			/* 第一次 */
    {
      get_pos(&fx, &fy);
      if (fx < 0)
      {
	goto abort_game;
      }
      if (isopen[fx][fy])
      {
	continue;
      }
      move(MG_XPOS + fx, MG_YPOS + 2 * fy);
      outs(card[board[fx][fy]]);
      move(MG_XPOS + fx, MG_YPOS + 2 * fy + 1);
      isopen[fx][fy] = 1;
      break;
    }

    while (1)			/* 第二次 */
    {
      get_pos(&sx, &sy);
      if (sx < 0)
      {
	goto abort_game;
      }
      if (isopen[sx][sy])
      {
	continue;
      }
      move(MG_XPOS + sx, MG_YPOS + 2 * sy);
      outs(card[board[sx][sy]]);
      move(MG_XPOS + sx, MG_YPOS + 2 * sy + 1);
      isopen[sx][sy] = 1;
      if (board[fx][fy] == board[sx][sy])
      {
	count += 2;
      }
      else
      {
	vmsg("看清楚了沒？");
	move(b_lines, 0);
	clrtoeol();
	move(MG_XPOS + fx, MG_YPOS + 2 * fy);
	outs("■");
	isopen[fx][fy] = 0;
	move(MG_XPOS + sx, MG_YPOS + 2 * sy);
	outs("■");
	move(MG_XPOS + sx, MG_YPOS + 2 * sy + 1);
	isopen[sx][sy] = 0;
      }
      break;
    }

    if (count == MAX_X * MAX_Y)
    {
      vmsg("恭喜您成功\了");
      break;
    }

  }
abort_game:
  return 0;
}
#endif	/* HAVE_GAME */
