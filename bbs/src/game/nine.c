/*-------------------------------------------------------*/
/* nine.c           ( NTHU CS MapleBBS Ver 3.10 )        */
/*-------------------------------------------------------*/
/* target : 天地九九遊戲                                 */
/* create : 98/11/26                                     */
/* update : 01/04/24                                     */
/* author : dsyan.bbs@Forever.twbbs.org                  */
/* recast : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"


#ifdef HAVE_GAME


#undef	NINE_DEBUG


static char AI_score[13] = {7, 6, 5, 4,10, 9, 3, 2, 1, 0,11, 8,12};
/* 電腦 AI 所在 :           K  A  2  3  4  5  6  7  8  9  T  J  Q  每點牌所對應的分數 */
/* AI_score 6 以下的是數字牌，7 以上的是特殊牌 */
/* 電腦傾向把 AI_score 小的牌丟出去 */

static char str_dir[4][3] = {"↓", "→", "↑", "←"};

/* hand[] 裡面的值所代表的牌張 0~12:CK~CQ 13~25:DK~DQ 26~38:HK~KQ 39~51:SK~SQ */
static char str_suit[4][3] = {"Ｃ", "Ｄ", "Ｈ", "Ｓ"};
static char str_num[13][3] = {"Ｋ", "Ａ", "２", "３", "４", "５", "６", "７", "８", "９", "Ｔ", "Ｊ", "Ｑ"};

static char hand[4][5];		/* 東西南北四家的牌張 */
static char now;		/* 現在桌上點數 */
static char dir;		/* 目前旋轉的方向 1:逆時針 -1:順時針 */
static char turn;		/* 目前輪到哪一家 0:自己 1-3:電腦 */
static char live;		/* 電腦還有幾家活著 0~3  0:玩家勝利 */
static int sum;			/* 桌上已經有幾張牌了 */


static void
out_song()
{
  static int count = 0;

  /* 劉若英˙很愛很愛你 */
  uschar *msg[12] = 
  {
    "想為你做件事  讓你更快樂的事",
    "好在你的心中  埋下我的名字",
    "求時間  趁著你  不注意的時候",
    "悄悄地  把這種子  釀成果實",
    "我想她的確是  更適合你的女子",
    "我太不夠溫柔優雅成熟懂事",
    "如果我  退回到  好朋友的位置",
    "你也就  不再需要  為難成這樣子",
    "很愛很愛你  所以願意  捨得讓你",
    "往更多幸福的地方飛去",
    "很愛很愛你  只有讓你  擁有愛情",
    "我才安心"    
  };
  move(b_lines - 2, 0);
  prints("\033[1;3%dm%s\033[m  籌碼還有 %d 元", time(0) % 7, msg[count], cuser.money);
  clrtoeol();
  if (++count == 12)
    count = 0;
}


static int
score_cmp(a, b)
  char *a, *b;
{
  return AI_score[(*a) % 13] - AI_score[(*b) % 13];
}


static void
show_mycard()	/* 秀出我手上的五張牌 */
{
  int i;
  char t;

  for (i = 0; i < 5; i++)
  {
    t = hand[0][i];
    move(16, 30 + i * 4);
    outs(str_num[t % 13]);
    move(17, 30 + i * 4);
    outs(str_suit[t / 13]);
  }
}


static void
show_seacard(t)	/* 秀出海底的牌 */
  char t;	/* 出了哪張牌 */
{
  char x, y;

  /* 東南西北四家的出牌置放 (x, y) 坐標 */
  /*               南  東  北  西 */
  char coorx[4] = { 8,  6,  4,  6};
  char coory[4] = {30, 38, 30, 22};

#ifdef NINE_DEBUG
  /* 秀出電腦的牌 */
  move(b_lines - 1, 5);  
  for (x = 3; x > 0; x--)
  {
    if (hand[x][0] == -1)	/* 這家已經慘遭淘汰，不必印出 */
      continue;
    qsort(hand[x], 5, sizeof(char), score_cmp);
    for (y = 0; y < 5; y++)
      outs(str_num[hand[x][y] % 13]);
    outs("  ");
  }
#endif

  x = coorx[turn];
  y = coory[turn];
  move(x, y);
  outs("╭───╮");
  move(x + 1, y);
  prints("│%s    │", str_num[t % 13]);
  move(x + 2, y);
  prints("│%s    │", str_suit[t / 13]);
  move(x + 3, y);
  outs("│      │");
  move(x + 4, y);
  outs("│      │");
  move(x + 5, y);
  outs("│      │");
  move(x + 6, y);
  outs("╰───╯");

  move(8, 50);
  prints("%s  %s", dir == 1 ? "↙" : "↗", dir == 1 ? "↖" : "↘");
  move(10, 50);
  prints("%s  %s", dir == 1 ? "↘" : "↖", dir == 1 ? "↗" : "↙");

  move(13, 46);
  prints("點數：%-2d", now);
  /* prints("點數：%c%c%c%c", (now / 10) ? 162 : 32, (now / 10) ? (now / 10 + 175) : 32, 162, now % 10 + 175); */ /* 無需換成全形 */

  move(14, 46);
  prints("張數：%d", sum);

  refresh();

  sleep(1);		/* 讓玩家看清楚出牌狀況 */
}


static void
ten_or_twenty(t)	/* 加或減 10/20 */
  char t;
{
  if (now < t)			/* 直接加 */
  {
    now += t;
  }
  else if (now > 99 - t)	/* 直接減 */
  {
    now -= t;
  }
  else				/* 詢問要加還是要減 */
  {
    int ch;

    move(b_lines - 4, 0);
    clrtoeol();
    prints("     (←)(+)加%d  (→)(-)減%d   ", t, t);

    while (1)
    {
      if (turn)		/* 電腦 */
	ch = rnd(2) + KEY_LEFT;	/* KEY_RIGHT == KEY_LEFT + 1 */
      else		/* 玩家 */
	ch = vkey();

      switch (ch)
      {
      case KEY_LEFT:
      case '+':
	now += t;
	prints("\033[32;1m加 %d\033[m", t);
	return;

      case KEY_RIGHT:
      case '-':
	now -= t;
	prints("\033[32;1m減 %d\033[m", t);
	return;
      }
    }
  }
}


static void
next_turn()
{
  while (1)
  {
    turn = (turn + 4 + dir) % 4;
    if (hand[turn][0] >= 0)		/* 如果下一家已遭淘汰，跳再下一家 */
      break;
  }
}


static char
get_newcard()
{
  /* 0~12:C)KA23456789TJQ 13~25:D))KA23456789TJQ 26~38:H))KA23456789TJQ 39~51:S)KA23456789TJQ */

  /* itoc.020929: 為求程式簡單，牌即使重覆出現也無妨，反正天地九九這遊戲
     應該是拿很多副樸克牌混起來玩 */

  return rnd(52);

  /* itoc.註解: 如果覺得天地久久難度太高，那麼可以在此亂數做調整，
     像是 now 接近 99 時，玩家的牌會變好；或是電腦取到特殊牌的機率變小 */
}


static int	/* -1:爆掉 */
lead_card(t)	/* 出牌 */
  char *t;	/* 傳入出了哪張牌，也要傳出一張新的牌替代這張牌 */
{
  int ch;
  char m;

  m = *t % 13;
  switch (m)
  {
  case 4:	/* 迴轉 */
    dir = -dir;
    break;

  case 5:	/* 指定 */
    move(b_lines - 4, 0);
    clrtoeol();
    outs("     指定那一家？");
    for (ch = 3; ch >= 0; ch--)
    {
      if (turn != ch && hand[ch][0] >= 0)
	prints("(%s) ", str_dir[ch]);
    }

    /* 指定一家尚未淘汰的 */
    while (1)
    {

#if 0		/* from global.h */
#define KEY_UP          -1
#define KEY_DOWN        -2
#define KEY_RIGHT       -3 
#define KEY_LEFT        -4
#endif

      if (turn || live == 1)	/* 如果是電腦出的牌或是只剩下一個電腦對手，就自動指定 */
	ch = rnd(4) + KEY_LEFT;
      else
	ch = vkey();

      /* 被指定的那家不能已經遭淘汰 */
      if (turn != 3 && hand[3][0] >= 0 && ch == KEY_LEFT)
	ch = 3;
      else if (turn != 2 && hand[2][0] >= 0 && (ch == KEY_UP || ch == KEY_DOWN))
	ch = 2;
      else if (turn != 1 && hand[1][0] >= 0 && ch == KEY_RIGHT)
	ch = 1;
      else if (turn != 0 && hand[0][0] >= 0)
	ch = 0;
      else
	continue;

      break;
    }

    prints("\033[32;1m(%s)\033[m", str_dir[ch]);
    break;

  case 10:	/* 加或減10 */
    ten_or_twenty(10);
    break;

  case 11:	/* Pass */
    break;

  case 12:	/* 加或減20 */
    ten_or_twenty(20);
    break;

  case 0:	/* 馬上變99 */
    now = 99;
    break;

  default:	/* 一般數字牌 */
    if (now + m > 99)
      return -1;
    else
      now += m;
    break;
  }

  show_seacard(*t);

  /* 拿一張新的牌 */
  *t = get_newcard();

  /* 輪到下一家 */
  if (m == 5)
    turn = ch;
  else
    next_turn();

  return 0;
}


static void
cpu_die()
{
  char buf[20];

  switch (turn)
  {
  case 1:
    move(9, 55);
    break;
  case 2:
    move(7, 52);
    break;
  case 3:
    move(9, 49);
    break;
  }
  outs("  ");
  live--;

  sprintf(buf, "電腦 %d 爆了", turn);
  vmsg(buf);
  sleep(1);		/* 讓玩家看清楚出牌狀況 */

  hand[turn][0] = -1;	/* 爆掉的那家的 hand[turn][0] 設為 -1 */
  next_turn();
}


static void
online_help(t)
  char t;
{
  char m;

  move(b_lines - 4, 0);
  clrtoeol();

  m = t % 13;
  switch (m)
  {
  case 0:
    outs("     九九：點數馬上變成９９");
    break;

  case 4:
    outs("     迴轉：遊戲進行方向相反");
    break;

  case 5:
    outs("     指定：自由指定下一個玩家");
    break;

  case 11:
    outs("     PASS：可 pass 一次");
    break;

  case 10:
    outs("     點數加或減 10");
    break;

  case 12:
    outs("     點數加或減 20");
    break;

  default:
    prints("     點數加 %d", m);
    break;
  }
}


int
main_nine()
{
  int money;		/* 押金 */

  int i, j;
  char m;
  char buf[STRLEN];
  FILE *fp;

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  while (1)
  {
    vs_bar("天地九九");
    out_song();

    vget(2, 0, "請問要下注多少呢？(1 ~ 50000) ", buf, 6, DOECHO);
    money = atoi(buf);
    if (money < 1 || money > 50000 || money > cuser.money)
      break;			/* 離開賭場 */

    /* 印出賭場內部擺設 */

    if (!(fp = fopen("etc/game/99", "r")))
      break;			/* 離開賭場 */

    move(2, 0);
    clrtoeol();		/* 清掉「請問要下注多少呢？」的殘餘 */

    move(1, 0);
    while (fgets(buf, STRLEN, fp))
      outs(buf);

    fclose(fp);

    cuser.money -= money;		/* 先確定能開檔再扣錢 */

    for (i = 0; i < 4; i++)
    {
      for (j = 0; j < 5; j++)
	hand[i][j] = get_newcard();
    }

    sum = 0;		/* 目前牌桌中間沒有牌 */
    now = 0;		/* 目前牌桌中間的點數是 0 */
    turn = 0;		/* 玩家開始第一步 */
    dir = 1;		/* 一開始預設 逆時針 */
    live = 3;		/* 尚有三家電腦活著 */

    /* 玩家的牌要排序，電腦的牌等輪到電腦時再排序 */
    qsort(hand[0], 5, sizeof(char), score_cmp);
    show_mycard();

    /* 遊戲開始 */
    for (;;)
    {
      move(9, 52);
      outs(str_dir[turn]);

      /* 輪到電腦出牌 */

      if (turn)
      {
	qsort(hand[turn], 5, sizeof(char), score_cmp);

	for (i = 0; i < 5; i++)
	{
	  m = hand[turn][i] % 13;
	  if (AI_score[m] >= 7)		/* 用 AI_score[] 來判斷是否為特殊牌 */
	    break;
	  if (now + m <= 99)
	    break;
	}

	if (i == 5)	/* 沒有任何一張牌能出 */
	{
	  cpu_die();
	}
	else
	{
	  /* 由於 qsort 會把 AI_score 的牌放在最左邊，就直接拋出最左邊這張牌即可 */
	  sum++;
	  lead_card(&(hand[turn][i]));
	}

	if (rnd(5) == 0)
	  out_song();

	continue;
      }

      /* 輪到玩家出牌 */

      if (!live)	/* 三家電腦都死光了 */
      {
	if (sum < 25)
	  money *= 15;
	else if (sum < 50)
	  money *= 10;
	else if (sum < 100)
	  money *= 5;
	else if (sum < 150)
	  money *= 3;
	else if (sum < 200)
	  money *= 2;
	/* 超過 200 次才贏，只還原押金 */

	addmoney(money);
	sprintf(buf, "在經過 %d 回合的廝殺，您脫穎而出，獲得獎金 %d", sum, money);
	vmsg(buf);
	break;
      }

      /* 較小的牌/特殊牌放在右邊，若最後一張不是特殊牌且加上去會超過 99，就是爆了 */
      m = hand[0][4] % 13;
      if (AI_score[m] < 7 && now + m > 99)
      {
        sprintf(buf, "嗚嗚嗚..在 %d 張牌被電腦電爆掉了.. :~", sum);
        vmsg(buf);
	break;
      }

      i = 0;		/* 在以下 while(j) 迴圈中，拿 i 來當第幾張牌 */
      while (1)		/* 直到 vkey() == '\n' 或 ' ' 時才離開迴圈 */
      {
	m = hand[0][i] % 13;
	move(18, i * 4 + 30);

	if (AI_score[m] < 7)	/* 用 AI_score[] 來判斷是否為特殊牌 */
	{
	  if (now + m > 99)
	    outs("！");		/* 警告會爆掉 */
	  else
	    outs("○");		/* 一般牌 */
	}
	else
	{
	  outs("★");		/* 特殊牌 */
	}

	move(18, i * 4 + 31);	/* 避免偵測左右鍵全形 */

	j = vkey();

	if (j == KEY_LEFT)
	{
	  move(18, i * 4 + 30);
	  outs("  ");
	  i = i ? i - 1 : 4;
	}
	else if (j == KEY_RIGHT)
	{
	  move(18, i * 4 + 30);
	  outs("  ");
	  i = (i == 4) ? 0 : i + 1;
	}
#ifdef NINE_DEBUG
	else if (j == KEY_UP)
	{
	  if (vget(b_lines - 4, 5, "把牌換成：", buf, 3, DOECHO))
	  {
	    m = atoi(buf);
	    if (m >= 0 && m < 52)
	    {
  	      hand[0][i] = m;
	      qsort(hand[0], 5, sizeof(char), score_cmp);
	      show_mycard();
	    }
	  }
	}
#endif
	else if (j == KEY_DOWN)
	{
	  online_help(hand[0][i]);
	}
	else if (j == '\n' || j == ' ')
	{
	  move(18, i * 4 + 30);
	  outs("  ");
	  sum++;
	  break;
	}
	else if (j == 'q')
	{
	  return 0;
	}
      }

      if (lead_card(&(hand[0][i])) < 0)
      {
	vmsg("嗚嗚嗚..白爛爆了!!.. :~");
	break;
      }

      qsort(hand[0], 5, sizeof(char), score_cmp);
      show_mycard();
    }
  }
  return 0;
}

#endif	/* HAVE_GAME */
