/*-------------------------------------------------------*/
/* dragon.c	( YZU WindTopBBS Ver 3.00 )		 */
/*-------------------------------------------------------*/
/* target : 接龍遊戲					 */
/* create : 01/01/12					 */
/* update : 03/07/23					 */
/* author : verit.bbs@bbs.yzu.edu.tw			 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME

static int cards[52];
static int seven[7];		/* 每堆牌墩各還有幾張牌 */
static int points[7];


/*-------------------------------------------------------*/
/* 畫出牌						 */
/*-------------------------------------------------------*/


static void
draw_card(x, y, card)
  int x, y;
  int card;	/* >=0:印出整張牌及此張牌的號碼  -1:只印牌外殼 */
{
  char flower[4][3] = {"Ｃ", "Ｄ", "Ｈ", "Ｓ"};
  char number[13][3] = {"Ａ", "２", "３", "４", "５", "６", "７", "８", "９", "Ｔ", "Ｊ", "Ｑ", "Ｋ"};

  move(x, y);
  outs("╭───╮");

  if (card < 0)
    return;

  move(x + 1, y);
  prints("│%s　　│", number[card % 13]);
  move(x + 2, y);
  prints("│%s　　│", flower[card % 4]);
  move(x + 3, y);
  outs("│　　　│");
  move(x + 4, y);
  outs("│　　　│");
  move(x + 5, y);
  outs("╰───╯");
}


/*-------------------------------------------------------*/
/* 遊戲說明						 */
/*-------------------------------------------------------*/


static void
draw_explain()
{
  vs_bar("接龍遊戲");

  move(4, 10);
  outs("【遊戲說明】");
  move(6, 15);
  outs("(1) 在螢幕上方為牌墩，可以利用 ←、→ 切換。");
  move(8, 15);
  outs("(2) 在螢幕左下方為持牌 , 可以利用 c 切換。");
  move(10, 15);
  outs("(3) 當牌墩的牌是持牌的下一張或上一張，即可利用 Enter 吃牌。");
  move(11, 15);
  outs("   (只看點數，不看花色)");
  move(13, 15);
  outs("(4) 當牌墩的牌都吃完，及遊戲獲勝。");
  move(15, 15);
  outs("(5) 當持牌切換完且尚未吃完牌墩的牌，即遊戲失敗。");
  vmsg(NULL);
}


/*-------------------------------------------------------*/
/* 畫出遊戲牌的配置					 */
/*-------------------------------------------------------*/


static void
draw_screen()
{
  int i, j;
  vs_bar("接龍遊戲");

  for (i = 0; i < 7; i++)
  {
    for (j = 0; j <= i; j++)
      draw_card(4 + j, 5 + i * 10, (i == j) ? cards[i] : -1);
  }
}


/*-------------------------------------------------------*/
/* 畫出游標						 */
/*-------------------------------------------------------*/


static void
draw_cursor(location, mode)
  int location;
  int mode;		/* 1:上色  0:清除 */
{
  int x, y;

  x = 8 + seven[location];
  y = 9 + location * 10;
  move(x, y);
  outs(mode ? "●" : "　");
  if (mode)
    move(x, y + 1);		/* 避免自動偵測全形 */
}


/*-------------------------------------------------------*/
/* 清除螢幕上的牌					 */
/*-------------------------------------------------------*/


static void 
clear_card(location)
  int location;
{
  move(9 + seven[location], 5 + location * 10);
  outs("          ");
}


/*-------------------------------------------------------*/
/* 遊戲參數初始化					 */
/*-------------------------------------------------------*/


static int
init_dragon()
{
  int i, j, num;

  for (i = 0; i < 52; i++)	/* 牌先一張一張排好，準備洗牌 */
    cards[i] = i;

  for (i = 0; i < 51; i++)
  {
    j = rnd(52 - i) + i;

    /* cards[j] 和 cards[i] 交換 */
    num = cards[i];
    cards[i] = cards[j];
    cards[j] = num;
  }

  for (i = 0; i < 7; i++)
  {
    seven[i] = i;
    points[i] = cards[i];
  }

  return 0;
}


/*-------------------------------------------------------*/
/* 判斷遊戲是否結束					 */
/*-------------------------------------------------------*/


static int	/* 1:成功 */
gameover()
{
  int i;

  for (i = 0; i < 7; i++)
  {
    if (seven[i] != -1)
      return 0;
  }
  return 1;
}


/*-------------------------------------------------------*/
/* 遊戲主程式						 */
/*-------------------------------------------------------*/


int			/* >=0:成功 -1:失敗 -2:離開 */
play_dragon()
{
  int i;
  int location = 0;	/* 目前游標的位置 */
  int now = 7;		/* 目前用到 cards[] 第幾張牌 */
  int have_card = 22;	/* 22 次換牌機會 */
  int point;		/* 目前手上的這張牌 */

  clear();

  draw_screen();
  draw_cursor(location, 1);
  point = cards[now];
  draw_card(14, 5, cards[now++]);
  move(19, 40);
  prints("您還有 %2d 次機會可以換牌", have_card);
  move(b_lines, 0);
  outs("★ 操作說明：(←)左移 (→)右移 (Enter)吃牌 (c)換牌 (q)離開");

  for (;;)
  {
    switch (vkey())
    {
    case 'c':
      if (have_card <= 0)
	return -1;
      have_card--;
      move(19, 47);
      prints("%2d", have_card);
      point = cards[now];
      draw_card(14, 5, cards[now++]);
      break;

    case KEY_RIGHT:
      draw_cursor(location, 0);
      do
      {
	location = (location + 1) % 7;
      } while (seven[location] == -1);
      draw_cursor(location, 1);
      break;

    case KEY_LEFT:
      draw_cursor(location, 0);
      do
      {
	location = (location == 0) ? 6 : location - 1;
      } while (seven[location] == -1);
      draw_cursor(location, 1);
      break;

    case '\n':
    case ' ':
      if (points[location] % 13 - point % 13 == 1 ||
	points[location] % 13 - point % 13 == -1 ||
	points[location] % 13 - point % 13 == 12 ||
	points[location] % 13 - point % 13 == -12)
      {
	point = points[location];
	draw_card(14, 5, point);
	clear_card(location);
	draw_cursor(location, 0);
	seven[location]--;
	if (seven[location] >= 0)
	{
	  points[location] = cards[now];
	  draw_card(4 + seven[location], 5 + location * 10, cards[now++]);
	  draw_cursor(location, 1);
	}
	else
	{
	  for (i = 0; i < 5; i++)
	  {
	    move(4 + i, 5 + location * 10);
	    outs("          ");
	  }
	  if (gameover() == 1)
	    return have_card;
	  do
	  {
	    location = (location + 1) % 7;
	  } while (seven[location] == -1);
	  draw_cursor(location, 1);
	}
      }
      break;

    case 'q':
      return -2;
    }
  }
}


int
main_dragon()
{
  draw_explain();

  while (1)
  {
    init_dragon();

    switch (play_dragon())
    {
    case -1:
      vmsg("挑戰失敗！");
      break;

    case -2:
      vmsg(MSG_QUITGAME);
      return 0;

    default:
      vmsg("恭喜您過關啦！");
    }

    if (vans("是否要繼續玩(Y/N)？[N] ") != 'y')
      break;
  }

  return 0;
}
#endif	/* HAVE_GAME */
