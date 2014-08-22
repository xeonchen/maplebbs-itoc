/*-------------------------------------------------------*/
/* gp.c		( NTHU CS MapleBBS Ver 3.10 )            */
/*-------------------------------------------------------*/
/* target : 金撲克梭哈遊戲                               */
/* create : 98/10/24                                     */
/* update : 01/04/21                                     */
/* author : dsyan.bbs@forever.twbbs.org                  */
/* recast : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#if 0
             -=== 金撲克梭哈遊戲 ===-

        1. 玩法類似梭哈，跟電腦比大，可加倍！
        2. 可以將獎金當下一次的賭注。

        大小：
        同花順＞鐵枝＞葫蘆＞同花＞順子＞三條＞兔胚＞單胚＞單張

        特殊加分：
        同花順  １５倍
        四  張  １０倍
        葫　蘆　　５倍

#endif


#include "bbs.h"


#ifdef HAVE_GAME

#define MAX_CHEAT	2	/* 電腦作弊多換牌次數 (0:不作弊，最多可作弊 6 次) */

static char mycard[5];		/* 我的 5 張牌 */
static char cpucard[5];		/* 電腦 5 張牌 */


static void
out_song()
{
  static int count = 0;

  /* 周華健˙朋友 */
  uschar *msg[7] = 
  {
    "這些年  一個人  風也過  雨也走",
    "有過淚\  有過錯  還記得堅持什麼",
    "真愛過  才會懂  會寂寞  會回首",
    "終有夢  終有你  在心中",
    "朋友一生一起走  那些日子不再有",
    "一句話  一輩子  一生情  一杯酒",
    "朋友不曾孤單過  一聲朋友你會懂"
  };
  move(b_lines - 2, 0);
  prints("\033[1;3%dm%s\033[m  籌碼還有 %d 元", time(0) % 7, msg[count], cuser.money);
  clrtoeol();
  if (++count == 7)
    count = 0;
}


static void
show_card(isDealer, c, x)
  int isDealer;		/* 1:電腦  2:玩家 */
  char c;		/* 牌張 */
  int x;		/* 第幾張牌 */
{
  int beginL;
  char *suit[4] = {"Ｃ", "Ｄ", "Ｈ", "Ｓ"};
  char *num[13] = {"Ｋ", "Ａ", "２", "３", "４", "５", "６", "７", "８", "９", "Ｔ", "Ｊ", "Ｑ"};

  beginL = (isDealer) ? 2 : 12;
  move(beginL, x * 4);
  outs("╭───╮");
  move(beginL + 1, x * 4);
  prints("│%2s    │", num[c % 13]);
  move(beginL + 2, x * 4);
  prints("│%2s    │", suit[c / 13]);
  move(beginL + 3, x * 4);
  outs("│      │");
  move(beginL + 4, x * 4);
  outs("│      │");
  move(beginL + 5, x * 4);
  outs("│      │");
  move(beginL + 6, x * 4);
  outs("╰───╯");
}


/* 同花順、鐵枝、葫、同花、順、三條、兔胚、胚、一隻 */
static void
show_style(my, cpu)
  int my, cpu;
{
  char *style[9] = {"同花順", "四張", "葫蘆", "同花", "順子", "三條", "兔胚", "單胚", "一張"};

  move(5, 26);
  prints("\033[41;37;1m%s\033[m", style[cpu - 1]);
  move(15, 26);
  prints("\033[41;37;1m%s\033[m", style[my - 1]);
}


static int
card_cmp(a, b)
  char *a, *b;
{
  /* 00~12: C, KA23456789TJQ
     13~25: D, KA23456789TJQ
     26~38: H, KA23456789TJQ
     39~51: S, KA23456789TJQ */

  char c = (*a) % 13;
  char d = (*b) % 13;

  if (c == 0)
    c = 13;
  else if (c == 1)
    c = 14;
  if (d == 0)
    d = 13;
  else if (d == 1)
    d = 14;

  /* 先比點數，再比花色 */
  if (c == d)
    return *a - *b;
  return c - d;
}


/* a 是點數 .. b 是花色 */
static void
tran(a, b, c)
  char *a, *b, *c;
{
  int i;
  for (i = 0; i < 5; i++)
  {
    a[i] = c[i] % 13;
    if (!a[i])
      a[i] = 13;
  }

  for (i = 0; i < 5; i++)
    b[i] = c[i] / 13;
}


static void
check(p, q, r, cc)
  char *p, *q, *r, *cc;
{
  char i;

  for (i = 0; i < 13; i++)
    p[i] = 0;
  for (i = 0; i < 5; i++)
    q[i] = 0;
  for (i = 0; i < 4; i++)
    r[i] = 0;

  for (i = 0; i < 5; i++)
    p[cc[i] % 13]++;

  for (i = 0; i < 13; i++)
    q[p[i]]++;

  for (i = 0; i < 5; i++)
    r[cc[i] / 13]++;
}


/* 同花順、鐵枝、葫、同花、順、三條、兔胚、胚、一隻 */
static int
complex(cc, x, y)
  char *cc, *x, *y;
{
  char p[13], q[5], r[4];
  char a[5], b[5], c[5], d[5];
  int i, j, k;

  tran(a, b, cc);
  check(p, q, r, cc);

  /* 同花順 */
  if ((a[0] == a[1] - 1 && a[1] == a[2] - 1 && a[2] == a[3] - 1 && a[3] == a[4] - 1) &&
    (b[0] == b[1] && b[1] == b[2] && b[2] == b[3] && b[3] == b[4]))
  {
    *x = a[4];
    *y = b[4];
    return 1;
  }

  if (a[4] == 1 && a[0] == 2 && a[1] == 3 && a[2] == 4 && a[3] == 5 &&
    (b[0] == b[1] && b[1] == b[2] && b[2] == b[3] && b[3] == b[4]))
  {
    *x = a[3];
    *y = b[4];
    return 1;
  }

  if (a[4] == 1 && a[0] == 10 && a[1] == 11 && a[2] == 12 && a[3] == 13 &&
    (b[0] == b[1] && b[1] == b[2] && b[2] == b[3] && b[3] == b[4]))
  {
    *x = 1;
    *y = b[4];
    return 1;
  }

  /* 鐵枝 */
  if (q[4] == 1)
  {
    for (i = 0; i < 13; i++)
    {
      if (p[i] == 4)
	*x = i ? i : 13;
    }
    return 2;
  }

  /* 葫蘆 */
  if (q[3] == 1 && q[2] == 1)
  {
    for (i = 0; i < 13; i++)
    {
      if (p[i] == 3)
	*x = i ? i : 13;
    }
    return 3;
  }

  /* 同花 */
  for (i = 0; i < 4; i++)
  {
    if (r[i] == 5)
    {
      *x = i;
      return 4;
    }
  }

  /* 順子 */
  memcpy(c, a, 5);
  memcpy(d, b, 5);
  for (i = 0; i < 4; i++)
  {
    for (j = i; j < 5; j++)
    {
      if (c[i] > c[j])
      {
	k = c[i];
	c[i] = c[j];
	c[j] = k;
	k = d[i];
	d[i] = d[j];
	d[j] = k;
      }
    }
  }

  if (10 == c[1] && c[1] == c[2] - 1 && c[2] == c[3] - 1 && c[3] == c[4] - 1 && c[0] == 1)
  {
    *x = 1;
    *y = d[0];
    return 5;
  }

  if (c[0] == c[1] - 1 && c[1] == c[2] - 1 && c[2] == c[3] - 1 && c[3] == c[4] - 1)
  {
    *x = c[4];
    *y = d[4];
    return 5;
  }

  /* 三條 */
  if (q[3] == 1)
  {
    for (i = 0; i < 13; i++)
    {
      if (p[i] == 3)
      {
	*x = i ? i : 13;
	return 6;
      }
    }
  }

  /* 兔胚 */
  if (q[2] == 2)
  {
    for (*x = 0, i = 0; i < 13; i++)
    {
      if (p[i] == 2)
      {
	if ((i > 1 ? i : i + 13) > (*x == 1 ? 14 : *x))
	{
	  *x = i ? i : 13;
	  *y = 0;
	  for (j = 0; j < 5; j++)
	  {
	    if (a[j] == i && b[j] > *y)
	      *y = b[j];
	  }
	}
      }
    }
    return 7;
  }

  /* 單胚 */
  if (q[2] == 1)
  {
    for (i = 0; i < 13; i++)
    {
      if (p[i] == 2)
      {
	*x = i ? i : 13;
	*y = 0;
	for (j = 0; j < 5; j++)
	  if (a[j] == i && b[j] > *y)
	    *y = b[j];
	return 8;
      }
    }
  }

  /* 一張 */
  *x = 0;
  *y = 0;
  for (i = 0; i < 5; i++)
  {
    if ((a[i] = a[i] ? a[i] : 13 > *x || a[i] == 1) && *x != 1)
    {
      *x = a[i];
      *y = b[i];
    }
  }
  return 9;
}


static int	/* <0:玩家贏牌 <-1000:玩家特殊贏牌 >0:電腦贏牌 */
gp_win(my, cpu)
  int *my, *cpu;	/* 傳回玩家和電腦的牌組 */
{
  int ret;
  char myX, myY, cpuX, cpuY;

  *my = complex(mycard, &myX, &myY);
  *cpu = complex(cpucard, &cpuX, &cpuY);

  if (*my != *cpu)		/* 如果牌型不同，直接比較牌型大小 */
    ret = *my - *cpu;
  else if (myX == 1 && cpuX != 1)
    ret = -1;
  else if (myX != 1 && cpuX == 1)
    ret = 1;
  else if (myX != cpuX)
    ret = cpuX - myX;
  else if (myY != cpuY)
    ret = cpuY - myY;
  else
    ret = -1;

  if (ret < 0)		/* 如果玩家贏牌 */
  {
    switch (*my)
    {
    case 1:		/* 同花順 */
      ret = -1001;
      break;
    case 2:		/* 鐵枝 */
      ret = -1002;
      break;
    case 3:		/* 葫蘆 */
      ret = -1003;
      break;
    }
  }

  return ret;
}


static char
get_newcard(mode)
  int mode;		/* 0:重新洗牌  1:發牌 */
{
  static char card[20 + 5 * MAX_CHEAT];	/* 最多只會用到 20+5*MAX_CHEAT 張牌 */
  static int now;			/* 發出第 now 張牌 */
  char num;
  int i;

  if (!mode)	/* 重新洗牌 */
  {
    now = 0;
    return -1;
  }

rand_num:		/* random 出一張和之前都不同的牌 */
  num = rnd(52);
  for (i = 0; i < now; i++)
  {
    if (num == card[i])	/* 這張牌以前 random 過了 */
      goto rand_num;
  }

  card[now] = num;
  now++;

  return num;
}


static int
cpu_doing()
{
  int my, cpu;
  int i, j, k;
  char hold[5];
  char p[13], q[5], r[4];
  char a[5], b[5];

  for (i = 0; i < 5; i++)
  {
    cpucard[i] = get_newcard(1);
    hold[i] = 0;
  }
  qsort(cpucard, 5, sizeof(char), card_cmp);
  for (i = 0; i < 5; i++)
    show_card(1, cpucard[i], i);

  tran(a, b, cpucard);
  check(p, q, r, cpucard);

  /* 若有特殊牌型，則保留 */
  k = 0;	/* 1:有特殊牌型 */
  for (j = 0; j < 13; j++)
  {
    if (p[j] > 1)
    {
      for (i = 0; i < 5; i++)
      {
	if (j == cpucard[i] % 13)
	{
	  hold[i] = 1;
	  k = 1;
	}
      }
    }
  }

  for (i = 0; i < 5; i++)
  {
    /* 如果沒有特殊牌型，那麼保留 A、K，否則全部不保留 */
    if (!k && (a[i] == 13 || a[i] == 1))
      hold[i] = 1;

    move(6, i * 4 + 2);
    outs(hold[i] ? "保" : "  ");
    move(7, i * 4 + 2);
    outs(hold[i] ? "留" : "  ");
  }

  vmsg("電腦換牌前..");

  for (j = 0; j < 1 + MAX_CHEAT; j++)	/* 換牌一次、作弊 MAX_CHEAT 次 */
  {
    /* 電腦換牌 */
    for (i = 0; i < 5; i++)
    {
      if (!hold[i])
	cpucard[i] = get_newcard(1);
    }
    qsort(cpucard, 5, sizeof(char), card_cmp);

    if ((k = gp_win(&my, &cpu)) > 0)	/* 若電腦贏，離開作弊迴圈 */
      break;
  }

  for (i = 0; i < 5; i++)
    show_card(1, cpucard[i], i);

  show_style(my, cpu);

  return k;
}


int
main_gp()
{
  int money;		/* 壓注金額 */
  int cont;		/* 繼續壓注的次數 */
  int doub;		/* 是否賭倍 */
  char hold[5];		/* 欲保留的牌 */

  char buf[60];
  int i, x, xx;

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  cont = 0;		/* 壓注次數歸零 */

  while (1)
  {
    vs_bar("金撲克梭哈");
    out_song();

    if (!cont)		/* 第一次壓注 */
    {
      vget(b_lines - 3, 0, "請問要下注多少呢？(1 ~ 50000) ", buf, 6, DOECHO);
      money = atoi(buf);
      if (money < 1 || money > 50000 || money > cuser.money)
        break;		/* 離開賭場 */
      cuser.money -= money;
      move(b_lines - 4, 0);
      prints(COLOR1 " (←)(→)改變選牌  (d)Double  (SPCAE)改變換牌  (Enter)確定                    \033[m");
    }
    else		/* 繼續上一盤贏的押金，就不可以再 double 了 */
    {
      move(b_lines - 4, 0);
      prints(COLOR1 " (←)(→)改變選牌  (SPCAE)改變換牌  (Enter)確定                               \033[m");
    }

    out_song();

    get_newcard(0);	/* 洗牌 */

    doub = 0;
    for (i = 0; i < 5; i++)
    {
      mycard[i] = get_newcard(1);
      hold[i] = 1;
    }
    qsort(mycard, 5, sizeof(char), card_cmp);

    for (i = 0; i < 5; i++)
      show_card(0, mycard[i], i);

    x = xx = 0;
    do
    {
      for (i = 0; i < 5; i++)
      {
	move(16, i * 4 + 2);
	outs(hold[i] < 0 ? "保" : "  ");
	move(17, i * 4 + 2);
	outs(hold[i] < 0 ? "留" : "  ");
      }
      move(11, xx * 4 + 2);
      outs("  ");
      move(11, x * 4 + 2);
      outs("↓");
      move(11, x * 4 + 3);	/* 避免全形偵測 */
      xx = x;

      switch (i = vkey())
      {
      case KEY_LEFT:
	x = x ? x - 1 : 4;
	break;

      case KEY_RIGHT:
	x = (x == 4) ? 0 : x + 1;
	break;

      case ' ':
	hold[x] *= -1;
	break;

      case 'd':
	if (!cont && !doub && cuser.money >= money)
	{
	  doub = 1;
	  cuser.money -= money;
	  money *= 2;
          move(b_lines - 4, 0);
	  prints(COLOR1 " (←)(→)改變選牌  (SPCAE)改變換牌  (Enter)確定                               \033[m");
	  out_song();
	}
	break;
      }
    } while (i != '\n');

    for (i = 0; i < 5; i++)
    {
      if (hold[i] == 1)
	mycard[i] = get_newcard(1);
    }
    qsort(mycard, 5, sizeof(char), card_cmp);
    for (i = 0; i < 5; i++)
      show_card(0, mycard[i], i);
    move(11, x * 4 + 2);
    outs("  ");

    i = cpu_doing();

    if (i < 0)		/* 玩家贏牌 */
    {
      switch (i)
      {      
      /* 特殊牌型有特別的賠率 */
      case -1001:
        money *= 16;
	break;
      case -1002:
	money *= 11;
	break;
      case -1003:
        money *= 6;
	break;
      default:
	money <<= 1;
	break;
      }
      sprintf(buf, "哇！好棒喔！得到 %d 元咧 :)", money);
      vmsg(buf);

      if (vans("您要把獎金繼續壓注嗎(Y/N)？[N] ") == 'y')
      {
        cont++;
      }
      else
      {
        cont = 0;
        addmoney(money);	/* 一般牌型多贏一倍，特殊牌型多 15/10/5 倍 */
      }
    }
    else			/* 輸牌 */
    {
      vmsg("輸了..:~~~");
      cont = 0;
    }
  }
  return 0;
}
#endif	/* HAVE_GAME */
