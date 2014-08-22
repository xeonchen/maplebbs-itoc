/*-------------------------------------------------------*/
/* bj.c          ( NTHU CS MapleBBS Ver 3.10 )           */
/*-------------------------------------------------------*/
/* target : 黑傑克二十一點遊戲                           */
/* create :   /  /                                       */
/* update : 01/04/23                                     */
/* author : unknown                                      */
/* recast : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"


#ifdef HAVE_GAME


enum
{				/* 倍率 */

  SEVEN = 5,			/* 777 */
  SUPERAJ = 5,			/* spade A+J */
  AJ = 4,			/* A+J */
  FIVE = 3,			/* 過五關 */
  JACK = 3,			/* 為前兩張就 21 點 */
  WIN = 2			/* 贏 */
};


static char *flower[4] = {"Ｓ", "Ｈ", "Ｄ", "Ｃ"};
static char *poker[53] = 
{
  "Ａ", "Ａ", "Ａ", "Ａ", "２", "２", "２", "２", "３", "３", "３", "３",
  "４", "４", "４", "４", "５", "５", "５", "５", "６", "６", "６", "６",
  "７", "７", "７", "７", "８", "８", "８", "８", "９", "９", "９", "９",
  "Ｔ", "Ｔ", "Ｔ", "Ｔ", "Ｊ", "Ｊ", "Ｊ", "Ｊ", "Ｑ", "Ｑ", "Ｑ", "Ｑ",
  "Ｋ", "Ｋ", "Ｋ", "Ｋ", "  "
};


static void
out_song()
{
  static int count = 0;

  /* 許茹芸˙愛只剩一秒 */
  uschar *msg[9] = 
  {
    "緣份快滅了  眼前是海角  還往前跳  手戴上手銬",
    "不想讓擁抱　再放掉  你一掙扎  我就痛到  呼吸不了",
    "愛曾那麼好  不能到老  你何苦　連假戲真作你都做不到",
    "眼看  幸福只剩一秒  你還落淚\求饒",
    "要求我放掉　不知該說什麼才好",
    "自己深愛的人  竟像小孩　無理取鬧",
    "生命　如果只剩一秒  我只想要投靠",
    "死在你懷抱  夢裡自尋天荒地老　隔世把心葬掉",
    "你的歉意　再傷不了　我的好"
  };
  move(b_lines - 2, 0);
  prints("\033[1;3%dm%s\033[m  籌碼還有 %d 元", time(0) % 7, msg[count], cuser.money);
  clrtoeol();
  if (++count == 9)
    count = 0;
}


static void
print_card(int card, int x, int y)
{

  move(x, y);
  outs("╭───╮");
  move(x + 1, y);
  prints("│%s    │", poker[card]);
  move(x + 2, y);
  prints("│%s    │", card != 52 ? flower[card % 4] : "  ");
  move(x + 3, y);
  outs("│      │");
  move(x + 4, y);
  outs("│      │");
  move(x + 5, y);
  outs("│      │");
  move(x + 6, y);
  outs("╰───╯");
}


static char
get_newcard(mode)
  int mode;			/* 0:重新洗牌  1:發牌 */
{
  static char card[10];	/* 最多只會用到 10 張牌 */
  static int now;	/* 發出第 now 張牌 */
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


int
main_bj()
{
  int money;			/* 押金 */

  /* 電腦是莊家 */
  char host_card[12];		/* 電腦的牌張 */
  char guest_card[12];		/* 玩家的牌張 */

  int host_count;		/* 電腦拿了幾張牌 */
  int guest_count;		/* 玩家拿了幾張牌 */

  int host_A;			/* 電腦拿 A 的張數 */
  int guest_A;			/* 玩家拿 A 的張數 */
  int host_point;		/* 電腦拿的總點數 */
  int guest_point;		/* 玩家拿的總點數 */

  int doub;			/* 是否已經加倍押金 */
  int ch;			/* 按鍵 */

  int card;
  char buf[60];

  int num[52] =		/* 各張牌的點數 */
  {
    11, 11, 11, 11, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6,
    7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10
  };

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  while (1)
  {
    vs_bar("黑傑克大戰");
    out_song();

    vget(2, 0, "請問要下注多少呢？(1 ~ 50000) ", buf, 6, DOECHO);
    money = atoi(buf);
    if (money < 1 || money > 50000 || money > cuser.money)
      break;			/* 離開賭場 */

    cuser.money -= money;
    doub = 0;

    move(3, 0);
    outs("(按 y 續牌, n 不續牌, d double)");

    out_song();

    get_newcard(0);	/* 洗牌 */

    guest_card[0] = get_newcard(1);
    guest_card[1] = get_newcard(1);
    host_card[0] = get_newcard(1);
    host_card[1] = get_newcard(1);

    host_count = 2;		/* 此時，雙方各有二張牌 */
    guest_count = 2;
    host_A = 0;			/* 此時，雙方都沒有 Ace */
    guest_A = 0;

    /* 檢查 A 的張數，來決定是否有 Black Jack 或是把 A 當 11 點 */

    if (host_card[0] < 4)
      host_A++;
    if (host_card[1] < 4)
      host_A++;
    if (guest_card[0] < 4)
      guest_A++;
    if (guest_card[1] < 4)
      guest_A++;

    print_card(52, 5, 0);		/* 印出電腦的第一張牌(但顯示空白) */
    print_card(host_card[1], 5, 4);	/* 印出電腦的第二張牌 */
    print_card(guest_card[0], 14, 0);	/* 印出玩家的第一張牌 */
    print_card(guest_card[1], 14, 4);	/* 印出玩家的第二張牌 */

    host_point = num[host_card[1]];	/* 電腦的第一張牌暫時不算點(以免暴露答案) */
    guest_point = num[guest_card[0]] + num[guest_card[1]];

    move(12, 0);
    prints("\033[1;32m點數：\033[33m%2d\033[m", host_point);

    for (;;)
    {
      /* 檢查牌型 */
check_condition:

      /* itoc.011025: 玩家每取一張牌就要重繪一次點數 */
      move(13, 0);
      prints("\033[1;35m點數：\033[36m%2d\033[m", guest_point);

      if (guest_count == 3 &&	/* 要檢查 guest_count 以免第三張牌是上次牌 */
	(guest_card[0] >= 24 && guest_card[0] <= 27) &&
	(guest_card[1] >= 24 && guest_card[1] <= 27) &&
	(guest_card[2] >= 24 && guest_card[2] <= 27))
      {
	money *= SEVEN;
	sprintf(buf, "\033[1;41;33m     ７７７     得獎金 %d 銀兩   \033[m", money);
	goto next_game;
      }

      else if ((guest_card[0] == 40 && guest_card[1] == 0) ||
	(guest_card[0] == 0 && guest_card[1] == 40))
      {
        money *= SUPERAJ;
	sprintf(buf, "\033[1;41;33m正統 BLACK JACK 得獎金 %d 銀兩   \033[m", money);
	goto next_game;
      }

      else if (((guest_card[0] <= 3 && guest_card[0] >= 0) && 
        (guest_card[1] <= 43 && guest_card[1] >= 40)) ||
	((guest_card[1] <= 3 && guest_card[1] >= 0) && 
	(guest_card[0] <= 43 && guest_card[0] >= 40)))
      {
	money *= AJ;
	sprintf(buf, "\033[1;41;33m  BLACK JACK    得獎金 %d 銀兩   \033[m", money);
	goto next_game;
      }

      else if (guest_point == 21 && guest_count == 2)	/* 前兩張就 21 點 */
      {
	money *= JACK;
	sprintf(buf, "\033[1;41;33m     JACK       得獎金 %d 銀兩   \033[m", money);
	goto next_game;
      }

      else if (guest_count == 5 && guest_point <= 21)
      {
	money *= FIVE;
	sprintf(buf, "\033[1;41;33m    過五關      得獎金 %d 銀兩   \033[m", money);
	goto next_game;
      }

      else if (guest_point > 21 && guest_A)
      {
	guest_point -= 10;
	guest_A--;
        move(13, 0);
        prints("\033[1;35m點數：\033[36m%2d\033[m", guest_point);
	goto check_condition;
      }

      /* 一般牌型 */

      else
      {
	/* 爆了 */

	if (guest_point > 21)
	{
	  money = 0;
	  strcpy(buf, "\033[1;41;33m     爆了       錢被吃光了   \033[m");
	  goto next_game;
	}

	/* 沒爆 */

	do
	{
	  ch = igetch();

	  if (ch == 'd' && !doub && guest_count == 2)	/* 只能在第二張牌能時 double */
	  {
	    doub = 1;	/* 已 double，不能再 double */

	    if (cuser.money >= money)
	    {
	      cuser.money -= money;
	      money *= 2;
	    }
	    else
	    {
	      money += cuser.money;	/* 錢不夠二倍，就全梭了 */
	      cuser.money = 0;
	    }
	    out_song();
	  }
	} while (ch != 'y' && ch != 'n');

	if (ch == 'y' && guest_point != 21)	/* 玩家續牌 */
	{
	  card = get_newcard(1);
	  guest_card[guest_count] = card;
	  if (card < 4)
	    guest_A++;

          guest_point += num[card];
          print_card(card, 14, 4 * guest_count);
          guest_count++;
	  move(13, 0);
	  prints("\033[1;35m點數：\033[36m%2d\033[m", guest_point);
	  goto check_condition;
	}
	else					/* 玩家不續牌 */
	{
	  /* 先顯示莊家的第一張牌 */
          move(6, 2);
          outs(poker[host_card[0]]);
          move(7, 2);
          outs(flower[host_card[0] % 4]);
          host_point += num[host_card[0]];	/* 把剛開始沒加的第一張牌點加回來 */

	  /* 輪到電腦(莊家)取牌 */

	  if (host_A == 2)	/* 特例: 電腦二張底牌恰好都是 A 時會判斷錯誤 */
	  {
	    /* 把一張 A 從 11 點換成 1 點 */
	    host_point -= 10;
	    host_A--;
	  }

	  while (host_point < guest_point)	/* 莊家如果點比較少，就強迫取牌 */
	  {
	    card = get_newcard(1);
	    host_card[host_count] = card;
	    if (card < 4)
	      host_A++;

	    host_point += num[card];
	    print_card(card, 5, 4 * host_count);
	    host_count++;

	    if (host_point > 21)
	    {
	      if (host_A)
	      {
	        host_point -= 10;
	        host_A--;
	        continue;	/* 繼續取下一張牌 */
	      }

	      move(12, 0);
	      prints("\033[1;32m點數：\033[33m%2d\033[m", host_point);

	      /* 電腦(莊家)爆了 */

	      money *= WIN;
	      sprintf(buf, "\033[1;41;33m    WINNER      得獎金 %d 銀兩   \033[m", money);
	      goto next_game;
	    }
	  }

	  /* 電腦(莊家)取牌結束，且沒有爆掉，電腦獲勝 */

	  move(12, 0);
	  prints("\033[1;32m點數：\033[33m%2d\033[m", host_point);

	  money = 0;
	  strcpy(buf, "\033[1;41;33m     輸了       錢被吃光了   \033[m");
	  goto next_game;
	}

      }				/* if 檢查牌型結束 */

    }				/* for 迴圈結束，結束這回合 */

next_game:
    move(18, 3);
    outs(buf);
    addmoney(money);
    vmsg(NULL);
  }				/* while 迴圈結束，離開遊戲 */

  return 0;
}
#endif				/* HAVE_GAME */
