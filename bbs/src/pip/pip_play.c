/*-------------------------------------------------------*/
/* pip_play.c         ( NTHU CS MapleBBS Ver 3.10 )      */
/*-------------------------------------------------------*/
/* target : 玩樂選單                                     */
/* create :   /  /                                       */
/* update : 01/08/15                                     */
/* author : dsyan.bbs@forever.twbbs.org                  */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME

#include "pip.h"


/*-------------------------------------------------------*/
/* 玩樂選單:散步 旅遊 運動 約會 猜拳			 */
/*-------------------------------------------------------*/


int
pip_play_stroll()		/* 散步 */
{
  /* 預設改變值，若有偶發事件，另外加成於下 */
  count_tired(3, 3, 1, 100, 0);	/* 增加疲勞 */
  d.happy += rand() % 3 + 3;
  d.satisfy += rand() % 2 + 1;
  d.shit += rand() % 3 + 2;
  d.hp -= rand() % 3 + 2;

  switch (rand() % 10)
  {
  case 0:
    d.happy += 6;
    d.satisfy += 6;
    show_play_pic(1);
    vmsg("遇到朋友囉  真好.... ^_^");
    break;

  case 1:
    d.happy += 4;
    d.satisfy += 8;
    show_play_pic(2);
    vmsg(d.sex == 1 ? "看到漂亮的女生囉  真好.... ^_^" : "看到英俊的男生囉  真好.... ^_^");
    break;

  case 2:
    d.money += 100;
    d.happy += 4;
    show_play_pic(3);
    vmsg("撿到了100元了..耶耶耶....");
    break;

  case 3:
    d.happy -= 10;
    d.satisfy -= 3;
    show_play_pic(4);
    if (d.money > 50)
    {
      d.money -= 50;
      vmsg("掉了50元了..嗚嗚嗚....");
    }
    else
    {
      d.money = 0;
      vmsg("錢掉光光了..嗚嗚嗚....");
    }
    break;

  case 4:
    d.happy += 3;
    show_play_pic(5);
    if (d.money > 50)
    {
      d.money -= 50;
      vmsg("用了50元了..不可以罵我喔....");
    }
    else
    {
      d.money = 0;
      vmsg("錢被我偷用光光了..:p");
    }
    break;

  case 5:
    d.toy++;
    show_play_pic(6);
    vmsg("好棒喔，撿到玩具了說.....");
    break;

  case 6:
    d.cookie++;
    show_play_pic(7);
    vmsg("好棒喔，撿到餅乾了說.....");
    break;

  case 7:
    d.satisfy -= 5;
    d.shit += 5;
    show_play_pic(9);
    vmsg("真是倒楣  可以去買愛國獎券");
    break;

  default:
    show_play_pic(8);
    vmsg("沒有特別的事發生啦.....");
    break;
  }

  if (d.happy > 100)
    d.happy = 100;
  if (d.satisfy > 100)
    d.satisfy = 100;

  return 0;
}


int
pip_play_sport()		/* 運動 */
{
  count_tired(3, 8, 1, 100, 1);
  d.speed += 2 + rand() % 3;
  d.weight -= rand() % 3 + 2;
  d.shit += rand() % 5 + 10;
  d.hp -= rand() % 2 + 8;
  d.satisfy += rand() % 2 + 3;
  if (d.satisfy > 100)
    d.satisfy = 100;

  show_play_pic(10);
  vmsg("運動好處多多啦...");

  return 0;
}


int
pip_play_date()			/* 約會 */
{
  if (d.money < 150)
  {
    vmsg("錢不夠多啦！約會總得花點錢錢");
  }
  else
  {
    count_tired(3, 6, 1, 100, 1);
    d.money -= 150;
    d.shit += rand() % 3 + 5;
    d.hp -= rand() % 4 + 8;
    d.character += rand() % 3 + 1;
    d.happy += rand() % 5 + 12;
    d.satisfy += rand() % 5 + 7;

    if (d.happy > 100)
      d.happy = 100;
    if (d.satisfy > 100)
      d.satisfy = 100;

    show_play_pic(11);
    vmsg("約會去  呼呼");
  }
  return 0;
}


int
pip_play_outing()		/* 郊遊 */
{
  if (d.money < 250)
  {
    vmsg("錢不夠多啦！旅遊總得花點錢錢");
  }
  else
  {
    count_tired(10, 45, 0, 100, 0);
    d.money -= 250;
    d.weight += rand() % 2 + 1;
    d.hp -= rand() % 7 + 15;
    d.character += rand() % 5 + 5;
    d.happy += rand() % 10 + 12;
    d.satisfy += rand() % 10 + 10;

    if (d.happy > 100)
      d.happy = 100;
    if (d.satisfy > 100)
      d.satisfy = 100;

    switch (rand() % 4)
    {
    case 0:
      d.art += rand() % 2;
      show_play_pic(12);
      vmsg(rand() % 2 ? "心中有一股淡淡的感覺  好舒服喔...." : "雲水 閑情 心情好多了.....");
      break;

    case 1:
      d.art += rand() % 3;
      show_play_pic(13);
      vmsg(rand() % 2 ? "有山有水有落日  形成一幅美麗的畫.." : "看著看著  全身疲憊都不見囉..");
      break;

    case 2:
      d.love += rand() % 3;
      show_play_pic(14);
      vmsg(rand() % 2 ? "看  太陽快沒入水中囉..." : "真是一幅美景");
      break;

    case 3:
      d.hp += d.maxhp;
      show_play_pic(15);
      vmsg(rand() % 2 ? "讓我們瘋狂在夜裡的海灘吧....呼呼.." : "涼爽的海風迎面襲來  最喜歡這種感覺了....");
    }

    /* 隨機遇到天使 */
    if (rand() % 301 == 0)
      pip_meet_angel();
  }

  return 0;
}


int
pip_play_kite()			/* 風箏 */
{
  count_tired(4, 4, 1, 100, 0);
  d.weight += (rand() % 2 + 2);
  d.shit += rand() % 5 + 6;
  d.hp -= rand() % 2 + 7;
  d.affect += rand() % 4 + 6;
  d.happy += rand() % 5 + 10;
  d.satisfy += rand() % 3 + 12;

  if (d.happy > 100)
    d.happy = 100;
  if (d.satisfy > 100)
    d.satisfy = 100;

  show_play_pic(16);
  vmsg("放風箏真好玩啦...");
  return 0;
}


int
pip_play_KTV()			/* KTV */
{
  if (d.money < 250)
  {
    vmsg("錢不夠多啦！唱歌總得花點錢錢");
  }
  else
  {
    count_tired(10, 10, 1, 100, 0);
    d.money -= 250;
    d.shit += rand() % 5 + 6;
    d.hp += rand() % 2 + 6;
    d.art += rand() % 4 + 3;
    d.happy += rand() % 3 + 20;
    d.satisfy += rand() % 2 + 20;

    if (d.happy > 100)
      d.happy = 100;
    if (d.satisfy > 100)
      d.satisfy = 100;

    show_play_pic(17);
    vmsg("二隻老虎..二隻老虎..跑得快..跑得快..");
  }
  return 0;
}


static void
guess_pip_lose()
{
  d.winn++;
  d.shit += rand() % 3 + 2;
  d.hp -= rand() % 2 + 3;
  d.satisfy--;
  d.happy -= 2;
  outs("小雞輸了....~>_<~");
  show_guess_pic(2);
}


static void
guess_pip_tie()
{
  d.tiee++;
  count_tired(2, 2, 1, 100, 1);
  d.shit += rand() % 3 + 2;
  d.hp -= rand() % 2 + 3;
  d.satisfy++;
  d.happy++;
  outs("平手........-_-");
  show_guess_pic(3);
}


static void
guess_pip_win()
{
  d.losee++;
  count_tired(2, 2, 1, 100, 1);
  d.shit += rand() % 3 + 2;
  d.hp -= rand() % 2 + 3;
  d.satisfy += rand() % 3 + 2;
  d.happy += rand() % 3 + 5;
  outs("小雞贏囉....*^_^*");
  show_guess_pic(1);
}


int
pip_play_guess()		/* 猜拳程式 */
{
  int mankey;		/* 我出的手 */
  int pipkey;		/* 小雞出的手 */
  char msg[3][5] = {"剪刀", "石頭", "布  "};

  out_cmd("", COLOR1 " 猜拳 " COLOR2 " [1]我出剪刀 [2]我出石頭 [3]我出布啦 [Q]跳出                            \033[m");

  /* itoc.010814: 可以一直猜拳 */
  while (1)
  {
    /* 我先出 */
    mankey = vkey() - '1';
    if (mankey < 0 || mankey > 2)
      return 0;

    /* 小雞再出 */
    pipkey = rand() % 3;

    /* 在 b_lines - 2 秀全部的勝負訊息 */
    move(b_lines - 2, 0);
    prints("您：%s   小雞：%s    ", msg[mankey], msg[pipkey]);

    /* 判定勝負 */
    if (mankey == pipkey)	/* 平手 */
      guess_pip_tie();
    else if (pipkey == mankey + 1 || pipkey == mankey - 2)	/* 小雞勝 */
      guess_pip_win();
    else			/* 小雞敗 */
      guess_pip_lose();
  }
}
#endif		/* HAVE_GAME */
