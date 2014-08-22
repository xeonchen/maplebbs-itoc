/*-------------------------------------------------------*/
/* pushbox.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : 倉庫番					 */
/* create : 98/11/11					 */
/* update : 02/09/05					 */
/* author : period.bbs@smth.org				 */
/*          cityhunter.bbs@smth.org			 */
/* modify : zhch.bbs@bbs.nju.edu.cn			 */
/*          hightman.bbs@bbs.hightman.net		 */
/* recast : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME

#define MAX_MAP_WIDTH		20
#define MAX_MAP_HEIGHT		16
#define MAX_MOVE_TIMES		1000	/* 最多移動幾步 */

#define move2(x, y)		move(x+6, y*2)
#define move3(x, y)		move(x+6, y*2+1)	/* 避免全形偵測 */

static int cx, cy;		/* 目前所在位置 */
static int stage;		/* 第幾關 */
static int NUM_TABLE;		/* 總共有幾關 */
static int total_line;		/* 本關有幾列 */
static usint map[MAX_MAP_HEIGHT][MAX_MAP_WIDTH];


static char *
map_char(n)
  usint n;
{
  if (n & 8)
    return "■";
  if (n & 4)
    return "□";
  if (n & 2)
    return "⊙";
  if (n & 1)
    return "﹒";
  return "  ";
}


static usint
map_item(item)
  char *item;
{
  if (!str_ncmp(item, map_char(1), 2))
    return 1;
  if (!str_ncmp(item, map_char(2), 2))
    return 2;
  if (!str_ncmp(item, map_char(4), 2))
    return 4;
  if (!str_ncmp(item, map_char(8), 2))
    return 8;
  return 0;
}


static int	/* 0:失敗  !=0:共有幾列 */
map_init(fp)
  FILE *fp;
{
  int i, j;
  char buf[80], level[8];

  sprintf(level, "--%03d", stage);
  while (fgets(buf, 10, fp))
  {
    if (!str_ncmp(buf, level, 5))	/* 找到正確的關卡 */
      break;
  }

  memset(map, 0, sizeof(map));

  i = 0;
  while (fgets(buf, 80, fp))
  {
    /* 載入地圖 */
    if (buf[0] == '-')
      break;

    for (j = 0; j < MAX_MAP_WIDTH; j++)
      map[i][j] = map_item(buf + j * 2);

    if (++i >= MAX_MAP_HEIGHT)
      break;

    memset(buf, 0, sizeof(buf));
  }

  return i;
}


static void
map_line(x)	/* 印出第 x 列 */
  int x;
{
  int j;
  usint n;

  move2(x, 0);
  clrtoeol();
  for (j = 0; j < MAX_MAP_WIDTH; j++)
  {
    n = map[x][j];
    if (n & 5)		/* 加強顏色使清楚 */
      prints("\033[1;32m%s\033[m", map_char(n));
    else
      outs(map_char(n));
  }
}


static void
map_show()
{
  int i;

  for (i = 0; i < total_line; i++)
    map_line(i);

  move3(cx, cy);
}


static void
map_move(x0, y0, x1, y1)	/* (x0, y0) -> (x1, y1) */
  int x0, y0, x1, y1;
{
  usint m;

  m = map[x0][y0];

  map[x1][y1] = (m & 6) | (map[x1][y1] & 1);	/* 目的格加入 '⊙' 及 '□' */
  map[x0][y0] = m & 1;				/* 所在格清空 '⊙' 及 '□' */

  map_line(x0);
  if (x1 != x0)
    map_line(x1);
}


static int	/* 1:成功 */
check_win()
{
  int i, j;
  for (i = 0; i < total_line; i++)
  {
    for (j = 0; j < MAX_MAP_WIDTH; j++)
    {
      if ((map[i][j] & 1) && !(map[i][j] & 4))	/* 還有 '﹒' 上沒有 '□' */
	return 0;
    }
  }
  return 1;
}


static int
find_cxy()		/* 找初始位置 */
{
  int i, j;
  for (i = 0; i < total_line; i++)
  {
    for (j = 0; j < MAX_MAP_WIDTH; j++)
    {
      if (map[i][j] & 2)
      {
	cx = i;
	cy = j;
	return 1;
      }
    }
  }
  return 0;
}


static int
select_stage()
{
  int count;
  char buf[80], ans[4];
  FILE *fp;

  if (!(fp = fopen("etc/game/pushbox.map", "r")))
    return 0;

  if (stage < 0)	/* 第一次進入遊戲 */
  {
    fgets(buf, 4, fp);
    NUM_TABLE = atoi(buf);	/* etc/game/pushbox.map 第一行記錄關卡數 */
    sprintf(buf, "請選擇編號 [1-%d]，[0] 隨機出題，或按 [Q] 離開：", NUM_TABLE);
    if (vget(b_lines, 0, buf, ans, 4, DOECHO) == 'q')
    {
      fclose(fp);
      return 0;
    }
    stage = atoi(ans);
    if (stage <= 0 || stage > NUM_TABLE)	/* 隨機出題 */
      stage = 1 + time(0) % NUM_TABLE;
  }

  count = map_init(fp);
  fclose(fp);

  return count;
}


int
main_pushbox()
{
  int dx, dy;		/* 下一步所行位秒 */
  int valid;
  usint n;

  stage = -1;

start_game:

  if (!(total_line = select_stage()))
    return XEASY;

  vs_bar("倉庫番");
  move(2, 0);
  prints("第 \033[1;32m%03d\033[m 關：把所有的 '□' 都推到 '﹒' 上面去(會變成\033[1;32m綠色\033[m)就過關了\n", stage);
  outs("按鍵說明：(↑↓←→)移動 (s)重玩 (q)離開 (^L)螢幕重繪");

  if (!find_cxy())
  {
    vmsg("這張地圖似乎不對勁！");
    return 0;
  }

  map_show();

  while (1)
  {
    dx = dy = 0;

    switch (vkey())
    {
    case KEY_UP:
      dx = -1;
      break;

    case KEY_DOWN:
      dx = 1;
      break;

    case KEY_LEFT:
      dy = -1;
      break;

    case KEY_RIGHT:
      dy = 1;
      break;

    case Ctrl('L'):
      map_show();
      break;

    case 'q':
      goto game_over;

    case 's':
      goto start_game;
    }

    if (!dx && !dy)
      continue;

    /* 開始移動 */
    valid = 0;
    n = map[cx + dx][cy + dy];	/* 目的格 */

    if (n <= 1)		/* 目的格是空的，直接移入即可 */
    {
      map_move(cx, cy, cx + dx, cy + dy);
      valid = 1;
    }
    else if (n & 4)	/* 目的格有 '□'，推去下下格 */
    {
      if (map[cx + dx * 2][cy + dy * 2] <= 1)	/* 下下格是空的 */
      {
	map_move(cx + dx, cy + dy, cx + dx * 2, cy + dy * 2);
	map_move(cx, cy, cx + dx, cy + dy);
	valid = 1;
      }
    }

    if (valid)	/* 有效移動 */
    {
      if (check_win())
	break;

      cx += dx;
      cy += dy;
      move3(cx, cy);
    }
  }

  vmsg("祝賀您！成功\過關");

  if (++stage > NUM_TABLE)
    stage = 1;

  goto start_game;

game_over:
  return 0;
}
#endif	/* HAVE_GAME */
