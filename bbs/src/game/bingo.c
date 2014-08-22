/*-------------------------------------------------------*/
/* bingo.c        ( NTHU CS MapleBBS Ver 3.10 )          */
/*-------------------------------------------------------*/
/* target : 賓果遊戲                                     */
/* create :   /  /                                       */
/* update : 01/04/21                                     */
/* author : unknown                                      */
/* recast : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME


static void
out_song()
{
  static int count = 0;

  /* 張學友 & 高慧君˙你最珍貴 */
  uschar *msg[9] = 
  {
    "明年這個時間 約在這個地點",
    "記得帶著玫瑰 打上領帶繫上思念",
    "動情時刻最美 真情的給不累",
    "太多的愛怕醉 沒人疼愛再美的人 也會憔悴",
    "我會送你紅色玫瑰 你別拿一生眼淚\相對",
    "未來的日子有你才美 夢才會真一點",
    "我學著 在你愛裡沉醉 你守護著我穿過黑夜",
    "我不撤退  我願意",
    "這條情路 相守相隨 你最珍貴"
  };
  move(b_lines - 2, 0);
  prints("\033[1;3%dm%s\033[m  籌碼還有 %d 元", time(0) % 7, msg[count], cuser.money);
  clrtoeol();
  if (++count == 9)
    count = 0;
}


int
main_bingo()
{
  /* 25個 {x坐標, y坐標, 值} */
  int place[5][5][3] = 
  {
    {{3, 2, 0}, {3, 6, 0}, {3, 10, 0}, {3, 14, 0}, {3, 18, 0}},
    {{5, 2, 0}, {5, 6, 0}, {5, 10, 0}, {5, 14, 0}, {5, 18, 0}},
    {{7, 2, 0}, {7, 6, 0}, {7, 10, 0}, {7, 14, 0}, {7, 18, 0}},
    {{9, 2, 0}, {9, 6, 0}, {9, 10, 0}, {9, 14, 0}, {9, 18, 0}},
    {{11, 2, 0}, {11, 6, 0}, {11, 10, 0}, {11, 14, 0}, {11, 18, 0}}
  };

  /* 賠率 : 猜 5 次就成功賠 20 倍，猜 6 次就成功賠 15 倍 ... 至少要猜 5 次才可能成功 */
  int rate[13 + 1] = {0, 1, 2, 3, 5, 7, 10, 15, 20, 0, 0, 0, 0, 0};
  
  char used[25];	/* 25 個數字是否用過 */
  int money;		/* 押金 */
  int account;		/* 還有幾次可猜 */
  int success;		/* 連成一直線了嗎 */

  int row, col, i;
  char buf[60];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  while (1)
  {
    vs_bar("賓果大戰");
    out_song();

    vget(2, 0, "請問要下注多少呢？(1 ~ 50000) ", buf, 6, DOECHO);
    money = atoi(buf);
    if (money < 1 || money > 50000 || money > cuser.money)
      break;				/* 離開賭場 */
    cuser.money -= money;

    /* initialize */

    for (i = 0; i < 25; i++)	/* 25 個數字都還沒被選 */
      used[i] = 0;

    for (row = 0; row < 5; row++)	/* 棋盤的 25 格先歸零 */
    {
      for (col = 0; col < 5; col++)
	place[row][col][2] = 0;
    }

    /* 畫棋盤 */

    move(4, 0);
    for (i = 0; i < 5; i++)
    {
      outs("   \033[1;44m                      \033[m\n"
        "   \033[44m  \033[42m  \033[44m  \033[42m  \033[44m  \033[42m  "
        "\033[44m  \033[42m  \033[44m  \033[42m  \033[44m  \033[m\n");
    }
    outs("   \033[44m                      \033[m\n");

    /* game start */

    for (account = 13, success = 0; account && !success; account--)
    {
      move(b_lines - 7, 0);
      outs("\033[1;37;44m尚未開出的號碼\033[m\n");
      for (i = 1; i <= 25; i++)
      {
	if (!used[i - 1])
	  prints(" %2d", i);
      }

      prints("\n尚有\033[1;33;41m %2d \033[m次機會可猜 下次猜中可得\033[1;37;44m %d \033[m倍\n",
        account, rate[account]);

      do
      {
        vget(b_lines - 4, 0, "請輸入您的號碼：", buf, 3, DOECHO);
        i = atoi(buf);
      } while (i <= 0 || i > 25 || used[i - 1]);

      /* 並不是一開始就決定棋盤上 25 個位置的值，而是 user 每猜一個數字
         再去決定要把這個數字放在棋盤上的哪個位置 */
      do
      {
        row = rnd(5);
        col = rnd(5);
      } while (place[row][col][2]);

      place[row][col][2] = i;
      used[i - 1] = 1;

      /* 把新的點那整列畫上棋盤 */

      move(5 + row * 2, 0);
      clrtoeol();
      outs("   \033[1m");
      for (i = 0; i < 5; i++)
      {      
	outs("\033[44m  ");
	if (place[row][i][2])
	  prints("\033[40m%2d", place[row][i][2]);
	else
	  outs("\033[42m  ");
      }
      outs("\033[44m  \033[m\n");

      if (account >= 13 - 5)	/* 至少要猜 5 次後才需要檢查是否連成一直線 */
        continue;
      
      /* 檢查是否連成一直線 */
      /* 若在二條對角線上，要特別檢查對角線，否則只要檢查該行列即可 */

      if ((row == col && place[0][0][2] && place[1][1][2] && place[2][2][2] && place[3][3][2] && place[4][4][2]) ||
        (row + col == 4 && place[0][4][2] && place[1][3][2] && place[2][2][2] && place[3][1][2] && place[4][0][2]) ||
        (place[row][0][2] && place[row][1][2] && place[row][2][2] && place[row][3][2] && place[row][4][2]) ||
        (place[0][col][2] && place[1][col][2] && place[2][col][2] && place[3][col][2] && place[4][col][2]))
      {
	success = 1;
	break;
      }

    }	/* for 迴圈結束，可能是猜完 13 次或是成功了 */

    if (success)
    {
      money *= rate[account];
      sprintf(buf, "恭喜您...贏了 %d 元 ", money);
      addmoney(money);
    }
    else
    {      
      strcpy(buf, "運氣不佳...再來一盤吧！");
    }
    vmsg(buf);

  }	/* while 迴圈結束，離開遊戲 */

  return 0;
}
#endif	/* HAVE_GAME */
