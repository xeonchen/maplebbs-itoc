/*-------------------------------------------------------*/
/* mine.c         ( NTHU CS MapleBBS Ver 3.10 )          */
/*-------------------------------------------------------*/
/* target : 踩地雷遊戲                                   */
/* create : 01/02/15                                     */
/* update : 01/03/01                                     */
/* author : piaip.bbs@sob.twbbs.org                      */
/* recast : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME

#define _CHINESE_	/* 中文字 symbol */

enum
{
  /* MINE_XPOS + MAP_MAX_X 要小於 b_lines - 2 = 21    *
   * MINE_YPOS + MAP_MAX_Y * 2 要小於 STRLEN - 1 = 79 *
   * MINE_YPOS 要足夠使 out_prompt() 放入             */

  MINE_XPOS = 0,
  MINE_YPOS = 17,
  MAP_MAX_X = 20,		/* ↓ x 方向 */
  MAP_MAX_Y = 30,		/* → y 方向 */

  /* These are flags for bitwise operators */
  TILE_BLANK = 0,		/* 沒有地雷 */
  TILE_MINE = 1,		/* 有地雷 */
  TILE_TAGGED = 0x10,		/* 被標記 */
  TILE_EXPAND = 0x20		/* 已被展開 */
};


static char MineMap[MAP_MAX_X + 2][MAP_MAX_Y + 2];	/* 地圖上每格的屬性 */
static char MineNei[MAP_MAX_X + 2][MAP_MAX_Y + 2];	/* 地圖上每格鄰居有多少地雷 */

static int MAP_X, MAP_Y;	/* 棋盤大小 */
static int cx, cy;		/* current (x, y) */
static int TotalMines;		/* 已標記的地雷數 */
static int TaggedMines;		/* 已標記的地雷數 */
static time_t InitTime;		/* 開始玩的時間 */
static int LoseGame;		/* 1: 輸了     0: 還在玩 */
static int EndGame;		/* 1: 離開遊戲 0: 還在玩 */


#ifdef _CHINESE_
static char symTag[3] = "※";	/* 標記地雷/此處有地雷且有被標示 */
static char symMine[3] = "⊙";	/* /此處有地雷但沒被標示 */
static char symWrong[3] = "Ｘ";	/* /此處沒地雷但有被標示 */
static char symBlank[3] = "■";	/* 未被展開/此處沒地雷且沒被展開 */
static char *strMines[9] = {"　", "１", "２", "３", "４", "５", "６", "７", "８"};	/* 旁邊有幾顆地雷 */
#else
static char symTag[3] = " M";
static char symMine[3] = " m";
static char symWrong[3] = " X";
static char symBlank[3] = " o";
static char *strMines[9] = {" _", " 1", " 2", " 3", " 4", " 5", " 6", " 7", " 8"};
#endif


static inline int
count_neighbor(x, y, bitmask)
  int x, y, bitmask;
{
  return (((MineMap[x - 1][y - 1] & bitmask) + (MineMap[x - 1][y] & bitmask) +
    (MineMap[x - 1][y + 1] & bitmask) + (MineMap[x][y - 1] & bitmask) +
    (MineMap[x][y] & bitmask) + (MineMap[x][y + 1] & bitmask) +
    (MineMap[x + 1][y - 1] & bitmask) + (MineMap[x + 1][y] & bitmask) +
    (MineMap[x + 1][y + 1] & bitmask)) / bitmask);
}


static void
init_map()
{
  int x, y, i;

  /* 設定有效棋盤 */

  for (x = 0; x < MAP_X + 2; x++)
  {
    for (y = 0; y < MAP_Y + 2; y++)
    {
      MineMap[x][y] = TILE_BLANK;
      if (x == 0 || y == 0 || x == MAP_X + 1 || y == MAP_Y + 1)
	MineMap[x][y] |= TILE_EXPAND;
    }
  }

  /* 設定地雷所在 */

  for (i = 0; i < TotalMines;)
  {
    x = rnd(MAP_X) + 1;
    y = rnd(MAP_Y) + 1;

    if (MineMap[x][y] == TILE_BLANK)
    {
      MineMap[x][y] = TILE_MINE;
      i++;
    }
  }

  /* 算出所有格的鄰居有多少地雷 */

  for (x = 1; x <= MAP_X; x++)
  {
    for (y = 1; y <= MAP_Y; y++)
    {
      MineNei[x][y] = count_neighbor(x, y, TILE_MINE);
    }
  }

  /* 現在開始計時，因為 out_map() 的 out_info() 要用到 */
  InitTime = time(0);
}


static void
out_prompt()
{
  /* outs() 裡面的敘述不得超過 MINE_YPOS，否則會錯亂 */
  move(3, 0);
  outs("按鍵說明：");
  move(5, 0);
  outs("移動     方向鍵");
  move(7, 0);
  outs("翻開     空白鍵");
  move(9, 0);
  outs("標記地雷   ｆ");
  move(11, 0);
  outs("掃雷       ｄ");
  move(13, 0);
  outs("離開       ｑ");
}


static inline void
out_song()
{
  uschar *msg[8] = 
  {
    "非澹泊無以明志，非寧靜無以致遠",			/* 諸葛亮誡子書 */
    "一粥一飯，當思得來不易；半絲半縷\，恆念物力維艱",	/* 朱子致家格言 */
    "宜未雨而綢繆，毋臨渴而掘井",			/* 朱子致家格言 */
    "無念爾祖，聿脩厥德",				/* 大雅 */
    "大學之道，在明明德，在親民，在止於至善",		/* 大學 */
    "天命之謂性，率性之誦道，脩道之謂教",		/* 中庸 */
    "不患人之不知己，患不知人也",			/* 論語˙學而 */
    "朝聞道，夕死可矣"					/* 論語˙里仁 */
  };
  move(b_lines - 2, 0);
  prints("\033[1;3%dm%s\033[m", time(0) % 7, msg[time(0) % 8]);
  clrtoeol();
}


static void
out_info()
{
  move(b_lines - 1, 0);
  prints("所花時間: %.0lf 秒，剩下 %d 個地雷未標記。",
    difftime(time(0), InitTime), TotalMines - TaggedMines);
  clrtoeol();

  out_song();
}


static inline void
out_map()
{
  int x, y;

  vs_bar("勁爆踩地雷");

  for (x = 1; x <= MAP_X; x++)
  {
    move(MINE_XPOS + x, MINE_YPOS + 2);
    for (y = 1; y <= MAP_Y; y++)
      outs(symBlank);
  }

  out_prompt();
  out_info();
  move(MINE_XPOS + cx, MINE_YPOS + cy * 2 + 1);	/* move to (0, 0) */
}


static void
draw_map()			/* 畫出完整答案 */
{
  int x, y;

  vs_bar("勁爆踩地雷");

  for (x = 1; x <= MAP_X; x++)
  {
    move(MINE_XPOS + x, MINE_YPOS + 2);
    for (y = 1; y <= MAP_Y; y++)
    {
      if (MineMap[x][y] & TILE_TAGGED)
      {
	if (!(MineMap[x][y] & TILE_MINE))
	  outs(symWrong);
	else
	  outs(symTag);
      }
      else if (MineMap[x][y] & TILE_EXPAND)
	outs(strMines[MineNei[x][y]]);
      else if (MineMap[x][y] & TILE_MINE)
	outs(symMine);
      else
	outs(symBlank);
    }
  }
}


static void
expand_map(x, y)
  int x, y;
{
  if (MineMap[x][y] & TILE_TAGGED || MineMap[x][y] & TILE_EXPAND)
    return;

  if (MineMap[x][y] & TILE_MINE && !(MineMap[x][y] & TILE_TAGGED))
  {
    LoseGame = 1;
    return;
  }

  MineMap[x][y] |= TILE_EXPAND;
  move(MINE_XPOS + x, MINE_YPOS + y * 2);
  outs(strMines[MineNei[x][y]]);

  if (MineNei[x][y] == 0)
  {
    if ((MineMap[x - 1][y] & TILE_EXPAND) == 0)
      expand_map(x - 1, y);
    if ((MineMap[x][y - 1] & TILE_EXPAND) == 0)
      expand_map(x, y - 1);
    if ((MineMap[x + 1][y] & TILE_EXPAND) == 0)
      expand_map(x + 1, y);
    if ((MineMap[x][y + 1] & TILE_EXPAND) == 0)
      expand_map(x, y + 1);
    if ((MineMap[x - 1][y - 1] & TILE_EXPAND) == 0)
      expand_map(x - 1, y - 1);
    if ((MineMap[x + 1][y - 1] & TILE_EXPAND) == 0)
      expand_map(x + 1, y - 1);
    if ((MineMap[x - 1][y + 1] & TILE_EXPAND) == 0)
      expand_map(x - 1, y + 1);
    if ((MineMap[x + 1][y + 1] & TILE_EXPAND) == 0)
      expand_map(x + 1, y + 1);
  }
}


static inline void
trace_map(x, y)
  int x, y;
{
  if (MineMap[x][y] & TILE_EXPAND &&
    MineNei[x][y] == count_neighbor(x, y, TILE_TAGGED))
  {
    expand_map(x - 1, y);
    expand_map(x, y - 1);
    expand_map(x + 1, y);
    expand_map(x, y + 1);
    expand_map(x - 1, y - 1);
    expand_map(x + 1, y - 1);
    expand_map(x - 1, y + 1);
    expand_map(x + 1, y + 1);
  }
}


static inline void
play_mine()
{
  int ch;

  LoseGame = 0;
  EndGame = 0;

  while (!LoseGame && (ch = vkey()))
  {
    switch (ch)
    {
    case KEY_ESC:
    case 'q':
      EndGame = 1;
      return;

      /* 做任何動作都要把游標回復到原來的位置 */

    case KEY_UP:
      if (cx > 1)
      {
	cx--;
	move(MINE_XPOS + cx, MINE_YPOS + cy * 2 + 1);
      }
      break;

    case KEY_DOWN:
      if (cx < MAP_X)
      {
	cx++;
	move(MINE_XPOS + cx, MINE_YPOS + cy * 2 + 1);
      }
      break;

    case KEY_LEFT:
      if (cy > 1)
      {
	cy--;
	move(MINE_XPOS + cx, MINE_YPOS + cy * 2 + 1);
      }
      break;

    case KEY_RIGHT:
      if (cy < MAP_Y)
      {
	cy++;
	move(MINE_XPOS + cx, MINE_YPOS + cy * 2 + 1);
      }
      break;

    case 'd':
    case '\n':
      trace_map(cx, cy);
      out_info();
      move(MINE_XPOS + cx, MINE_YPOS + cy * 2 + 1);
      break;

    case ' ':
      expand_map(cx, cy);
      out_info();
      move(MINE_XPOS + cx, MINE_YPOS + cy * 2 + 1);
      break;

    case 'f':
      if (MineMap[cx][cy] & TILE_EXPAND)
      {
	if (MineMap[cx][cy] & TILE_TAGGED)	/* 本來被標記, 取消 */
	{
	  TaggedMines--;
	  MineMap[cx][cy] ^= TILE_EXPAND;
	  MineMap[cx][cy] ^= TILE_TAGGED;
	  move(MINE_XPOS + cx, MINE_YPOS + cy * 2);
	  outs(symBlank);
	}
      }
      else		/* 本來沒標記, 上標記 */
      {
	TaggedMines++;
	MineMap[cx][cy] ^= TILE_EXPAND;
	MineMap[cx][cy] ^= TILE_TAGGED;
	move(MINE_XPOS + cx, MINE_YPOS + cy * 2);
	outs(symTag);
	if (TaggedMines == TotalMines)
	  return;
      }
      out_info();
      move(MINE_XPOS + cx, MINE_YPOS + cy * 2 + 1);
      break;

    default:
      break;
    }
  }
}


static inline int
win()
{
  int x, y;

  for (x = 1; x <= MAP_X; x++)
  {
    for (y = 1; y <= MAP_Y; y++)
    {
      if (((MineMap[x][y] & TILE_TAGGED) && !(MineMap[x][y] & TILE_MINE)) ||
        (!(MineMap[x][y] & TILE_TAGGED) && (MineMap[x][y] & TILE_MINE)))
      {
        return 0;		/* 標錯地方 */
      }
    }
  }
  return 1;
}


int
main_mine()
{
  int level;
  char ans[4];

  level = vans("請選擇 [1-5] 等級，[0] 自定，或按 [Q] 離開：");
  if (level == 'q')
  {
    return XEASY;
  }
  else if (level < '1' || level > '5')	/* 自定棋盤不得大於 60 * 20 */
  {
    vget(b_lines, 0, "請輸入地圖的長：", ans, 3, DOECHO);
    MAP_Y = atoi(ans) > MAP_MAX_Y ? MAP_MAX_Y : atoi(ans);

    vget(b_lines, 0, "請輸入地圖的寬：", ans, 3, DOECHO);
    MAP_X = atoi(ans) > MAP_MAX_X ? MAP_MAX_X : atoi(ans);

    vget(b_lines, 0, "請輸入地雷數：", ans, 3, DOECHO);
    level = atoi(ans);
    TotalMines = MAP_Y * MAP_X / 3;
    if (TotalMines > level)
      TotalMines = level;
    /* 限制地雷數不得超過 MAP_Y * MAP_X / 3，以免 init_map() 亂數取太久 */

    if (MAP_Y < 1 || MAP_X < 1 || TotalMines < 1)
      return 0;
    level = 0;
  }
  else
  {
    level -= '0';
    MAP_Y = 5 * level;		/* 不得超過 MAP_MAX_Y */
    MAP_X = (level < 4) ? 5 * level : MAP_MAX_X;	/* 不得超過 MAP_MAX_X */
    TotalMines = MAP_Y * MAP_X / 10;
  }

  TaggedMines = 0;
  cx = MAP_X / 2 + 1;
  cy = MAP_Y / 2 + 1;

  init_map();
  out_map();
  play_mine();

  if (!EndGame)
  {
    if (LoseGame)
    {
      draw_map();
      vmsg("碰！踩到地雷了！");
    }
    else	/* 標記數 == 地雷數 */
    {
      if (win())	/* itoc.010711: 要檢查是否破關，以免隨便亂標記，當標記數=地雷數就說過關了 */
      {
	char buf[STRLEN];
	sprintf(buf, "您花了 %.0lf 秒 破第 %d 關 好崇拜 ^O^", difftime(time(0), InitTime), level);
	vmsg(buf);
	addmoney(level * 75);
      }
      else
      {
        draw_map();
        vmsg("您標錯地雷了喔 =.=");
      }
    }
  }
  else
  {
    vmsg(MSG_QUITGAME);
  }

  return 0;
}
#endif	/* HAVE_GAME */
