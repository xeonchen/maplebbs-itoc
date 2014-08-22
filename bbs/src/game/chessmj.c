/*-------------------------------------------------------*/
/* chessmj.c      ( NTHU CS MapleBBS Ver 3.10 )          */
/*-------------------------------------------------------*/
/* target : 象棋麻將遊戲                                 */
/* create : 98/07/29                                     */
/* update : 05/06/30                                     */
/* author : weiren@mail.eki.com.tw                       */
/* recast : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/* modify : yiting.bbs@bbs.cs.tku.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"


#ifdef HAVE_GAME


static int host_card[5];	/* 電腦(莊家) 的牌 */
static int guest_card[5];	/* 玩家的牌 */
static int throw[50];		/* 被丟棄的牌，多留一些位置放被吃的牌 */

static int flag;
static int tflag;
static int selftouch;		/* 自摸 */

static int cnum[32] = 
{
  1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
  7, 7, 7, 7, 7, 8, 9, 9, 10, 10,
  11, 11, 12, 12, 13, 13,
  14, 14, 14, 14, 14
};

static int group[14] = 
{
  1, 1, 1, 2, 2, 2, 3,
  4, 4, 4, 5, 5, 5, 6
};

static char *chess[15] = 
{
  "\033[1;30m  \033[m",
  "\033[1;31m帥\033[m", "\033[1;31m仕\033[m", "\033[1;31m相\033[m", "\033[1;31m硨\033[m", "\033[1;31m傌\033[m", "\033[1;31m炮\033[m", "\033[1;31m兵\033[m",
  "\033[1;37m將\033[m", "\033[1;37m士\033[m", "\033[1;37m象\033[m", "\033[1;37m車\033[m", "\033[1;37m馬\033[m", "\033[1;37m包\033[m", "\033[1;37m卒\033[m"
};


static void
out_song(money)
  int money;
{
  move(b_lines - 2, 0);
  prints("\033[1;37;44m現有籌碼: %-20d", cuser.money);
  if(money)
    prints("押注金額: %-38d\033[m", money);
  else
    outs("                                                \033[m");
}


static inline void
print_sign(host)
  int host;
{
  int y = (tflag-1) / 2 * 4; 
  move((host * 3 + 8), y);
  outs("╭●╮");
}


static inline void
print_chess(x, y)
  int x, y;
{
  move(x, y);
  outs("╭─╮");
  move(x + 2, y);
  outs("╰─╯");
}


static inline void
clear_chess(x, y)
  int x, y;
{
  move(x, y);
  outs("      ");
  move(x + 2, y);
  outs("      ");
}


static void
print_all(cards, x)
  int cards[5], x;
{
  int i;

  move(x + 1, 0);
  clrtoeol();

  for (i = 0; i < 4; i++)
    prints("│%s│", chess[cards[i]]);

  if (cards[4] == 0)
    clear_chess(x, 24);
  else
  {
    prints("│%s│", chess[cards[4]]);
    print_chess(x, 24);
  }
}


static inline void
print_guest()
{
  print_all(guest_card, 15);
}


static inline void
print_host()
{
  print_all(host_card, 4);
}


static void
print_throw(host)
  int host;
{
  int x = 11 - host * 3;

  move(x + 1, 0);
  outs("│");

  for(; host < tflag; host += 2)
    prints("%s│", chess[throw[host] & 0x0f]);
  
  print_chess(x, (host-2) / 2 * 4);
}


static inline void
sortchess()
{
  int i, j, x;

  for (i = 0; i < 4; i++)
  {
    for (j = 0; j < (3 - i); j++)
    {
      if (guest_card[j] > guest_card[j + 1])
      {
	x = guest_card[j];
	guest_card[j] = guest_card[j + 1];
	guest_card[j + 1] = x;
      }
      if (host_card[j] > host_card[j + 1])
      {
	x = host_card[j];
	host_card[j] = host_card[j + 1];
	host_card[j + 1] = x;
      }
    }
  }
}


static int
testpair(a, b)
  int a, b;
{
  if (a == b)
    return 1;
  if (a == 1 && b == 8)
    return 1;
  if (a == 8 && b == 1)
    return 1;
  return 0;
}


static int
testthree(a, b, c)
  int a, b, c;
{
  int tmp;
  if (a > b)
  {
    tmp = a;
    a = b;
    b = tmp;
  }
  if (b > c)
  {
    tmp = b;
    b = c;
    c = tmp;
  }
  if (a > b)
  {
    tmp = a;
    a = b;
    b = tmp;
  }
  if (a == 1 && b == 2 && c == 3)
    return 1;			/* 帥仕相 */
  if (a == 4 && b == 5 && c == 6)
    return 1;			/* 硨傌炮 */
  if (a == 8 && b == 9 && c == 10)
    return 1;			/* 將士象 */
  if (a == 11 && b == 12 && c == 13)
    return 1;			/* 車馬包 */
  if (a == 7 && b == 7 && c == 7)
    return 1;			/* 兵兵兵 */
  if (a == 14 && b == 14 && c == 14)
    return 1;			/* 卒卒卒 */
  return 0;
}


static int
testall(set)
  int set[5];
{
  int i, j, k, m, p[3];

  for (i = 0; i < 4; i++)
  {
    for (j = i + 1; j < 5; j++)
    {
      m = 0;
      for (k = 0; k < 5; k++)
      {
	if (k != i && k != j)
	{
	  p[m] = set[k];
	  m++;
	}
      }
      if (testpair(set[i], set[j]) != 0 && testthree(p[0], p[1], p[2]) != 0)
	return 1;
    }
  }
  return 0;
}


static int
testlisten(set)
  int set[4];
{
  int i, j, k, p[2] = {0}, m = 0, mm = 0;

  j = 0;
  for (i = 0; i < 4; i++)
  {
    if (group[set[i]] != 3)
      j++;
  }
  if (j == 0)
    return 1;			/* 四支兵 */

  j = 0;
  for (i = 0; i < 4; i++)
  {
    if (group[set[i]] != 6)
      j++;
  }
  if (j == 0)
    return 1;			/* 四支卒 */

  if (testthree(set[1], set[2], set[3]) != 0)
    return 1;
  if (testthree(set[0], set[2], set[3]) != 0)
    return 1;
  if (testthree(set[0], set[1], set[3]) != 0)
    return 1;
  if (testthree(set[0], set[1], set[2]) != 0)
    return 1;			/* 三支成形則聽 */

  for (i = 0; i < 3; i++)
  {
    for (j = i + 1; j < 4; j++)
    {
      if (testpair(set[i], set[j]))
      {				/* 兩支有胚看另兩支有沒有聽 */
	m = 0;
	for (k = 0; k < 4; k++)
	{
	  if (k != i && k != j)
	  {
	    p[m] = set[k];
	    m++;
	  }
	  if (group[set[i]] == 3 || group[set[i]] == 6)
	    mm = 1;		/* 有胚的是兵或卒 */
	}
      }
    }
  }
  if (m != 0)
  {
    if ((group[p[0]] == group[p[1]]) && (p[0] != p[1]))
      return 1;			/* 兩支是 pair 另兩支有聽 */
    if ((group[p[0]] == group[p[1]] == 3) || (group[p[0]] == group[p[1]] == 6))
      return 1;
    if (testpair(p[0], p[1]) && mm == 1)
      return 1;
  }
  return 0;
}

static int
diecard(a)			/* 傳進一張牌, 看是否絕張 */
  int a;
{
  int i, k = 0;
  for (i = 0; i < tflag; i++)
  {
    if (throw[i] == a)
      k++;
    if (throw[i] == 1 && a == 8)
      return 1;
    if (throw[i] == 8 && a == 1)
      return 1;
  }
  if ((a == 7 || a == 14) && k == 4)
    return 1;			/* 兵卒絕張 */
  if (k == 1 && (a != 7 && a != 14))
    return 1;
  return 0;
}


static inline int
any_throw()
{
  int i, j, k = 0, set[5] = {0}, tmp[4] = {0};
  int point[5] = {0};	/* point[5] 為評分系統, 看丟哪張牌比較好, 分數高的優先丟 */

  /* 測試將手上五支拿掉一支 */
  for (i = 0; i < 5; i++)
  {
    k = 0;
    for (j = 0; j < 5; j++)
    {
      if (i != j)
      {
	tmp[k] = host_card[j];
	k++;
      }
    }
    if (testlisten(tmp))	/* 若剩餘的四支已聽牌, 丟多的那張 */
    {
      point[i] += 10;		/* 有聽就加 10 分 */
      if (diecard(host_card[i]))
	point[i] += 5;		/* 絕張更該丟 */
      for (k = 0; k < 4; k++)
      {
	if (((host_card[i] == tmp[k])
	    || (tmp[k] == 1 && host_card[i] == 8)
	    || (tmp[k] == 8 && host_card[i] == 1))
	  && host_card[i] != 7 && host_card[i] != 14)
	  point[i] += 10;
      }
      /* 車馬包包, 包該丟 */
    }
  }
  k = 0;
  for (i = 0; i < 5; i++)	/* 算有幾支兵 */
  {
    if (host_card[i] == 7)
      k++;
  }
  if (k == 3)			/* 有三支兵: 剩下二支不是兵的各加 5 分 */
  {
    for (i = 0; i < 5; i++)
      if (host_card[i] != 7)
	point[i] += 5;
  }
  else if (k == 4)		/* 有四支兵的話 */
  {
    if (diecard(7))		/* 但最後一支兵已絕張: 丟兵 */
    {
      for (i = 0; i < 5; i++)
	if (host_card[i] == 7)
	  point[i] += 999;
    }
    else			/* 最後一支兵尚未絕張: 丟不是兵的那支 */
    {
      for (i = 0; i < 5; i++)
	if (host_card[i] != 7)
	  point[i] += 999;
    }
  }
  k = 0;
  for (i = 0; i < 5; i++)	/* 算有幾支卒 */
  {
    if (host_card[i] == 14)
      k++;
  }
  if (k == 3)			/* 有三支卒: 剩下二支不是卒的各加 5 分 */
  {
    for (i = 0; i < 5; i++)
      if (host_card[i] != 14)
	point[i] += 5;
  }
  else if (k == 4)		/* 有四支卒的話 */
  {
    if (diecard(14))		/* 但最後一支卒已絕張: 丟卒 */
    {
      for (i = 0; i < 5; i++)
	if (host_card[i] == 14)
	  point[i] += 999;
    }
    else			/* 最後一支卒尚未絕張: 丟不是卒的那支 */
    {
      for (i = 0; i < 5; i++)
	if (host_card[i] != 14)
	  point[i] += 999;
    }
  }

  for (i = 0; i < 5; i++)
  {
    if (host_card[i] == 7)
      point[i] -= 1;
    if (host_card[i] == 14)
      point[i] -= 1;		/* 兵卒盡量不丟 */
  }

  for (i = 0; i < 4; i++)
  {
    for (j = i + 1; j < 5; j++)
    {
      if (group[host_card[i]] == group[host_card[j]])
      {
	point[i] -= 2;
	point[j] -= 2;		/* 差一支成三的不丟 */
      }
      if (testpair(host_card[i], host_card[j]))
      {
	point[i] -= 2;
	point[j] -= 2;		/* 有胚的不丟 */
      }
    }
  }

#if 1	/* 耍賤, 如果丟了會被胡就死都不丟, 耍賤機率 1/2 */
  for (i = 0; i < 4; i++)
    set[i] = guest_card[i];
  for (i = 0; i < 5; i++)
  {
    set[4] = host_card[i];
    if (testall(set) && rnd(2))
      point[i] = -999;
  }
#endif

  /* 找出分數最高的 */
  j = 0;
  k = point[0];
  for (i = 1; i < 5; i++)
  {
    if (point[i] > k)
    {
      k = point[i];
      j = i;
    }
  }
  return j;
}


static int
count_tai(set)
  int set[5];		/* 贏的五張牌 */
{
  char *name[10] = 
  {
    "將帥對", "將士象", "帥仕相",
    "五兵合縱", "五卒連橫", "三兵入列", "三卒入列",
    "海底", "天胡", "自摸"
  };

  int tai[10] = 	/* 台數對應上面的敘述 */
  {
    2, 1, 1,
    5, 5, 2, 2,
    3, 5, 1
  };

  int yes[10] = {0};
  int i, j, k, sum;

  if (selftouch)
    yes[9] = 1;			/* 自摸 */

  if (flag == 32)
    yes[7] = 1;			/* 海底 */
  else if (tflag <= 1)
    yes[8] = 1;			/* 天胡 */

  for (i = 0, j = 0, k = 0; i < 5; i++)
  {
    /* 算 帥/將 的支數 */
    if (set[i] == 1)
      j++;
    if (set[i] == 8)
      k++;
  }
  if (j)
  {
    if (k)
      yes[0] = 1;		/* 有帥又有將就是 將帥對 */
    else
      yes[2] = 1;		/* 有帥沒有將就是 帥仕相 */
  }
  else if (k)
  {
    yes[1] = 1;			/* 有將沒有帥就是 將士象 */
  }

  for (i = 0, j = 0; i < 5; i++)
  {
    /* 算 兵 的支數 */
    if (set[i] == 7)
      j++;
  }
  if (j == 5)
    yes[3] = 1;			/* 五兵合縱 */
  else if (j == 3)
    yes[5] = 1;			/* 三兵入列 */

  for (i = 0, j = 0; i < 5; i++)
  {
    /* 算 卒 的支數 */
    if (set[i] == 14)
      j++;
  }
  if (j == 5)
    yes[4] = 1;			/* 五卒連橫 */
  else if (j == 3)
    yes[6] = 1;			/* 三卒入列 */

  /* 算台數 */
  sum = 0;
  for (i = 0; i < 10; i++)
  {
    if (yes[i])
      sum += tai[i];
  }

  /* 列印出獎項 */
  move(b_lines - 5, 0);
  outs("┌───────────────────────────────────┐\n");
  for (i = 0; i < 10; i++)
  {
    if (yes[i])		/* 最多四種 */
      prints("  %8s [%d 台] ", name[i], tai[i]);
  }
  move(b_lines - 2, 0);
  clrtoeol();		/* 清除 out_song() */
  prints("        底 [2 台]       合計 [%d 台]\n", sum += 2);
  outs("└───────────────────────────────────┘");

  return sum;
}


int
main_chessmj()
{
  int money;			/* 押金 */
  int mo;			/* 1:已摸牌 0:未摸牌 */
  int picky;			/* 1:撿別人的牌 0:自己摸的 */
  int pickup;
  int listen;			/* 1:聽牌 0:沒有聽牌 */
  int chesslist[32];		/* 32 張牌組 */

  int i, j, k, m;
  int jp, x, xx, ch, z;

  char ans[10], msg[40];
  int tmp[4];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  while (1)
  {
    vs_bar("象棋麻將");

    out_song(0);

    vget(2, 0, "請問要下注多少呢？(1 ~ 50000) ", ans, 6, DOECHO);
    money = atoi(ans);
    if (money < 1 || money > 50000 || money > cuser.money)
      break;			/* 離開賭場 */
    cuser.money -= money;	/* 扣一份賭金，玩家如果中途離開將拿不回賭金 */

    out_song(money);
    move(2, 0);
    clrtoeol();		/* 清掉「請問要下注多少」 */
    outs("(按 ←→選牌, ↑丟牌, 按 ENTER 胡牌)");

    for (i = 0; i < 32; i++)		/* 牌先一張一張排好，準備洗牌 */
      chesslist[i] = cnum[i];

    for (i = 0; i < 31; i++)
    {
      j = rnd(32 - i) + i;

      /* chesslist[j] 和 chesslist[i] 交換 */
      m = chesslist[i];
      chesslist[i] = chesslist[j];
      chesslist[j] = m;
    }

    selftouch = 0;			/* 歸零 */
    mo = 0;
    pickup = 0;
    picky = 0;
    listen = 0;
    flag = 0;
    tflag = 0;

    for (i = 0; i < 4; i++)		/* 發前四張牌 */
    {
      host_card[i] = chesslist[flag];
      flag++;
      guest_card[i] = chesslist[flag];
      flag++;
    }
    guest_card[4] = 0;

    sortchess();			/* 排序 */

    move(4, 0);
    outs("╭─╮╭─╮╭─╮╭─╮");
    move(5, 0);
    outs("│  ││  ││  ││  │");
    move(6, 0);
    outs("╰─╯╰─╯╰─╯╰─╯");
    move(15, 0);
    outs("╭─╮╭─╮╭─╮╭─╮");

	print_guest();

    move(17, 0);
    outs("╰─╯╰─╯╰─╯╰─╯");  /* 印出前四張牌 */

    for (;;)
    {
      jp = 5;
      x = 0;
      z = 1;
      move(18, 26);
      do
      {
	if (!mo)
	{
	  move(14, 24);
	  outs("按空白鍵摸牌(或 ↓ 撿牌)");
	}
	else
	{
	  move(14, 0);
	  clrtoeol();
	}
	move(18, 2 + (jp - 1) * 6);
	outs("▲");
	move(18, 3 + (jp - 1) * 6);		/* 避免全形偵測 */

	ch = vkey();

	if (!mo && ch != KEY_DOWN && ch != '\n')
	{
	  ch = 'p';		/* 四張牌則強制摸牌 */
	}

	switch (ch)
	{
	case KEY_RIGHT:
	  move(18, 2 + (jp - 1) * 6);
	  outs("  ");
	  jp += 1;
	  if (jp > 5)
	    jp = 5;
	  move(18, 2 + (jp - 1) * 6);
	  outs("▲");
	  move(18, 3 + (jp - 1) * 6);	/* 避免全形偵測 */
	  break;

	case KEY_LEFT:
	  move(18, 2 + (jp - 1) * 6);
	  outs("  ");
	  jp -= 1;
	  if (jp < 1)
	    jp = 1;
	  move(18, 2 + (jp - 1) * 6);
	  outs("▲");
	  move(18, 3 + (jp - 1) * 6);	/* 避免全形偵測 */
	  break;

	case KEY_UP:		/* 出牌 */
	  move(18, 2 + (jp - 1) * 6);
	  outs("  ");
	  throw[tflag] = guest_card[jp - 1];
	  tflag++;
	  z = 0;
	  mo = 0;
	  guest_card[jp - 1] = guest_card[4];
	  guest_card[4] = 0;
	  sortchess();
	  print_guest();
	  print_throw(0);
	  picky = 0;
	  break;

	case 'p':		/* 摸牌 */
	  if (!mo)
	  {
        if (flag == 32)
        {
          strcpy(msg, "流局");
          goto next_game;
        }
	    move(18, 2 + (jp - 1) * 6);
	    outs("  ");
	    guest_card[4] = chesslist[flag];
	    flag++;
	    print_guest();
	    mo = 1;
	  }
	  break;

	case KEY_DOWN:
	  if (tflag > 0 && !mo)
	  {
	    guest_card[4] = throw[tflag - 1];
	    throw[tflag - 1] |= 0x80;
	    print_sign(0);
	    print_guest();
	    mo = 1;
	    picky = 1;
	  }
	  break;

	case 'q':
	  return 0;
	  goto abort_game;
	  break;

	case '\n':
	  if (testall(guest_card) && mo && !picky)
	  {
	    selftouch = 1;
	    addmoney(money *= count_tai(guest_card));
	    sprintf(msg, "哇咧自摸啦！贏了%d元", money);
	    goto next_game;
	  }
	  else if (picky && testall(guest_card))
	  {
	    addmoney(money *= count_tai(guest_card));
	    sprintf(msg, "看我的厲害，胡啦！贏了%d元", money);
	    goto next_game;
	  }

	  if (tflag > 0 && !mo)
	  {
	    i = guest_card[4];
	    guest_card[4] = throw[tflag - 1];
	    if (testall(guest_card) == 1)
	    {
	      print_sign(0);
	      print_guest();
	      addmoney(money *= count_tai(guest_card));
	      sprintf(msg, "胡！贏了%d元", money);
	      goto next_game;
	    }
	    guest_card[4] = i;
	  }
	  break;

	default:
	  break;
	}
      } while (z == 1);

      host_card[4] = throw[tflag - 1];
      if (testall(host_card))
      {
	print_sign(1);	/* 印撿牌符號 */
	sprintf(msg, "電腦胡啦！輸了%d元", money *= count_tai(host_card));
	cuser.money -= money;	/* 電腦台數越多就賠越多，且押金沒收 */
	if (cuser.money < 0)
	  cuser.money = 0;
	goto next_game;
      }

      if (flag == 32)
      {
        host_card[4] = 0;
        strcpy(msg, "流局");
        goto next_game;
      }

      host_card[4] = chesslist[flag];
      if (testall(host_card))
      {
	selftouch = 1;
	sprintf(msg, "電腦自摸！輸了%d元", money *= count_tai(host_card));
	cuser.money -= money;	/* 電腦台數越多就賠越多，且押金沒收 */
	if (cuser.money < 0)
	  cuser.money = 0;
	goto next_game;
      }

      for (i = 0; i < 4; i++)
	tmp[i] = host_card[i];

      if (!testlisten(tmp))
      {				/* 沒聽的話 */
	for (i = 0; i < 4; i++)
	{
	  k = 0;
	  for (j = 0; j < 4; j++)
	  {
	    if (i != j)
	    {
	      tmp[k] = host_card[j];
	      k++;
	    }
	  }
	  tmp[3] = throw[tflag - 1];	/* 把撿起那張跟手上的牌比對 */
	  if (testlisten(tmp))
	  {			/* 撿牌有聽的話 */
	    listen = 1;
	    host_card[4] = throw[tflag - 1];
	    throw[tflag - 1] |= 0x80;
	    print_sign(1);	/* 印撿牌符號 */
	    xx = i;		/* 紀錄下要丟的那張牌 */
	    pickup = 1;
	    break;		/* 跳出 i loop */
	  }
	}
      }

      for (i = 0; i < 4; i++)
	tmp[i] = host_card[i];
      if (testlisten(tmp) && !pickup)
      {				/* 有聽且剛剛沒撿 */
	m = 0;
	for (i = 0; i < 4; i++)
	  if (tmp[i] == 7)
	    m++;
	if (m == 2 && throw[tflag - 1] == 7)
	  pickup = 1;
	if (m == 3 && throw[tflag - 1] == 7)
	{
	  pickup = 1;
	  for (i = 0; i < tflag - 1; i++)
	    if (throw[i] == 7)
	      pickup = 0;
	}
	m = 0;
	for (i = 0; i < 4; i++)
	  if (tmp[i] == 14)
	    m++;
	if (m == 2 && throw[tflag - 1] == 14)
	  pickup = 1;
	if (m == 3 && throw[tflag - 1] == 14)
	{
	  pickup = 1;
	  for (i = 0; i < tflag - 1; i++)
	    if (throw[i] == 14)
	      pickup = 0;
	}
	if (pickup)
	{
	  host_card[4] = throw[tflag - 1];
	  throw[tflag - 1] |= 0x80;
	  print_sign(1);	/* 印撿牌符號 */
	}
      }


      if (!pickup)
      {
	host_card[4] = chesslist[flag];
	flag++;
      }
      /* 剛剛沒撿牌現在就摸牌 */

      if (!pickup)
      {
	for (i = 0; i < 4; i++)
	{
	  k = 0;
	  for (j = 0; j < 4; j++)
	  {
	    if (i != j)
	    {
	      tmp[k] = host_card[j];
	      k++;
	    }
	  }
	  tmp[3] = host_card[4];
	  if (testlisten(tmp))
	  {			/* 摸牌有聽的話 */
	    listen = 1;
	    xx = i;		/* 紀錄下要丟的那張牌 */
	    break;		/* 跳出 i loop */
	  }
	}
      }

      for (i = 0; i < 4; i++)
	tmp[i] = host_card[i];

      xx = any_throw();

      throw[tflag] = host_card[xx];
      tflag++;
      host_card[xx] = host_card[4];	/* 丟出沒聽那張 */
      print_throw(1);

      host_card[4] = 0;
      pickup = 0;
      listen = 0;

    }		/* for 迴圈結束 */

next_game:
    print_host();
    move(b_lines, 0);
    clrtoeol();
    prints("\033[1;37;44m ◆ %-55s \033[1;33;46m [請按任意鍵繼續] \033[m", msg);
    vkey();

  }		/* while 迴圈結束 */

abort_game:
  return 0;
}
#endif				/* HAVE_GAME */
