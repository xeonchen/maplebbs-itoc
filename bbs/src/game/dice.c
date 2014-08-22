/*-------------------------------------------------------*/
/* dice.c         ( NTHU CS MapleBBS Ver 3.10 )          */
/*-------------------------------------------------------*/
/* target : 擲骰子遊戲                                   */
/* create : 01/02/15                                     */
/* update : 01/04/20                                     */
/* author : wsyfish                                      */
/* recast : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME


static char *pic[6][3] = 
{
  "        ",
  "   ●   ",		/* 1 */
  "        ",

  "   ●   ",
  "        ",		/* 2 */
  "   ●   ",

  "●      ",
  "   ●   ",		/* 3 */
  "      ●",

  "●    ●",
  "        ",		/* 4 */
  "●    ●",

  "●    ●",
  "   ●   ",		/* 5 */
  "●    ●",

  "●    ●",
  "●    ●",		/* 6 */
  "●    ●"
};


static void
out_song()
{
  static int count = 0;

  /* 費玉清˙開一扇心窗 */
  uschar *msg[11] = 
  {
    "開一扇心窗  乘著夢的翅膀飛翔",
    "轉瞬間就能到達",
    "開一扇心窗  不要在黑暗中徬徨",
    "揮別了痛苦心酸",
    "讓生活從此過得簡單  閉上眼睛就可以想像",
    "遨遊無邊無際浩瀚海洋  看見藍天不再迷惘",
    "要多姿多彩自由奔放",
    "陽光  輕輕地開一扇心窗",
    "匆匆地帶我們走出黑暗  整個世界都燦爛輝煌",
    "陽光  輕輕地開一扇心窗",
    "柔柔地閃耀著美麗夢想  所有歡樂願與你分享"
  };
  move(b_lines - 2, 0);
  prints("\033[1;3%dm%s\033[m  籌碼還有 %d 元", time(0) % 7, msg[count], cuser.money);
  clrtoeol();
  if (++count == 11)
    count = 0;
}


int
main_dice()
{
  int money;		/* 押金 */
  int i;		/* 亂數 */
  char choice;		/* 記錄選項 */
  char dice[3];		/* 三個骰子的值 */
  char total;		/* 三個骰子的和 */
  char buf[60];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  vs_bar("ㄒㄧ ㄅㄚ ㄌㄚ 下注");
  outs("\n\n\n"
    "┌────────────────────────────────────┐\n"
    "│  2倍   1. 大      2. 小                                                │\n"
    "│ 14倍   3. 三點    4. 四點    5. 五點    6. 六點    7. 七點             │\n"
    "│  8倍   8. 八點    9. 九點   10. 十點   11. 十一點 12. 十二點 13. 十三點│\n"
    "│ 14倍  14. 十四點 15. 十五點 16. 十六點 17. 十七點 18. 十八點           │\n"
    "│216倍  19. 一一一 20. 二二二 21. 三三三 22. 四四四 23. 五五五 24. 六六六│\n"
    "└────────────────────────────────────┘\n");

#if 0	/* 擲骰子每 216 次各總數出現的次數機率 */
┌──┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┐
│總數│3 │4 │5 │6 │7 │8 │9 │10│11│12│13│14│15│16│17│18│
├──┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│次數│1 │3 │6 │10│15│21│25│27│27│25│21│15│10│6 │3 │1 │ / 216 次
└──┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┘
#endif

  out_song();

  while (1)
  {
    vget(2, 0, "請問要下注多少呢？(1 ~ 50000) ", buf, 6, DOECHO);
    money = atoi(buf);
    if (money < 1 || money > 50000 || money > cuser.money)
      break;				/* 離開賭場 */

    vget(12, 0, "要押哪一項呢？(請輸入號碼) ", buf, 3, DOECHO);
    choice = atoi(buf);
    if (choice < 1 || choice > 24)
      break;				/* 離開賭場 */

    outs("\n按任一鍵擲出骰子 \033[5m....\033[m\n");
    igetch();

    /* 決定三個骰子點數 */
    total = 0;
    for (i = 0; i < 3; i++)
    {
      dice[i] = rnd(6) + 1;
      total += dice[i];
    }

    /* 處理結果 */
    if ((choice == 1 && total > 10) || (choice == 2 && total <= 10))	/* 處理大小 */
    {
      sprintf(buf, "中了！得到２倍獎金 %d 元", money * 2);
      addmoney(money);
    }
    else if (choice <= 18 && total == choice)				/* 處理總和 */
    {
      if (choice >= 8 && choice <= 13)
      {
	sprintf(buf, "中了！得到８倍獎金 %d 元", money * 8);
	addmoney(money * 7);
      }
      else
      {
	sprintf(buf, "中了！得到１４倍獎金 %d 元", money * 14);
	addmoney(money * 13);
      }
    }
    else if ((choice - 18) == dice[0] && (dice[0] == dice[1]) && (dice[1] == dice[2]))/* 處理三個一樣 */
    {
      sprintf(buf, "中了！得到２１６倍獎金 %d 元", money * 216);
      addmoney(money * 215);
    }
    else								/* 處理沒中 */
    {
      strcpy(buf, "很可惜沒有押中！");
      cuser.money -= money;
    }

    /* 印出骰子結果 */
    outs("╭────╮╭────╮╭────╮\n");
    for (i = 0; i < 3; i++)
    {
      prints("│%s││%s││%s│\n", pic[dice[0] - 1][i], 
        pic[dice[1] - 1][i], pic[dice[2] - 1][i]);
    }
    outs("╰────╯╰────╯╰────╯\n\n");

    out_song();
    vmsg(buf);
  }

  return 0;
}

#endif	/* HAVE_GAME */
