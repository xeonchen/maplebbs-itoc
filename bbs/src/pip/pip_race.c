/*-------------------------------------------------------*/
/* pip_race.c          ( NTHU CS MapleBBS Ver 3.10 )     */
/*-------------------------------------------------------*/
/* target : 收穫季比賽                                   */
/* create :   /  /                                       */
/* update : 03/03/31                                     */
/* author : dsyan.bbs@forever.twbbs.org                  */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME

#include "pip.h"


/*-------------------------------------------------------*/
/* 陪賽者名單	 				         */
/*-------------------------------------------------------*/


static char racename[4][9] = {"武鬥大會", "藝術大展", "皇家舞會", "烹飪大賽"};


/* name[13] attribute hp maxhp attack spirit magic armor dodge money exp pic */
/* 參考 badman_generate() 中的註解，有怪物等級的期望值 */
struct playrule racemanlist[] = 
{
  /* 相當於等級 05 的怪物 */	"茱麗葉塔", 0,   30,   30,  50,  50,  50,  50,  50,  50,  25, 001,
  /* 相當於等級 10 的怪物 */	"菲歐利娜", 0,   95,   95,  99,  99,  99,  99,  99,  99,  50, 002,
  /* 相當於等級 20 的怪物 */	"阿妮斯白", 0,  300,  300, 350,  40,  40, 200, 200, 200, 100, 003, 
  /* 相當於等級 40 的怪物 */	"帕多雷西", 0, 1200, 1200, 350, 600, 200, 600, 100, 400, 200, 004, 
  /* 相當於等級 60 的怪物 */	"卡美拉美", 0, 2700, 2700, 600, 600, 600, 600, 600, 600, 300, 005, 
  /* 相當於等級 90 的怪物 */ 	"尼古拉斯", 0, 6100, 6100, 900, 799, 999, 799, 999, 900, 450, 006, 
};

static int player[3];		/* 三位陪賽選手的號碼，強度是 player[2] > player[1] > player[0] */


/*-------------------------------------------------------*/
/* 武鬥大會	 				         */
/*-------------------------------------------------------*/


/* itoc.030331.遊戲設計: 呼叫 pip_vs_man() 這函式進入戰鬥畫面，打贏多少人來算成績 */

static int			/* 回傳: 贏了幾個人 >=3:冠軍 2:亞軍 1:季軍 <=0:最後一名 */
pip_race_eventA()
{
  int i, winorlost;
  char buf[80];

  /* 從 racemanlist 六位中挑出三個陪賽者 */
  player[0] = rand() % 2;
  player[1] = rand() % 2 + 2;
  player[2] = rand() % 2 + 4;

  winorlost = 0;
  for (i = 0; i < 3; i++)
  {
    sprintf(buf, "您的第 %d 個對手是%s", i + 1, racemanlist[player[i]].name);
    vmsg(buf);

    if (pip_vs_man(racemanlist[player[i]], 0))	/* 小雞對戰敵人 */
      winorlost++;	/* 獲勝 */
  }

  return winorlost;
}


/*-------------------------------------------------------*/
/* 藝術大展	 				         */
/*-------------------------------------------------------*/


/* itoc.030331.遊戲設計: 完全由 d.art 和 d.charm 來決定藝術大展的成績 */

static int			/* 回傳: 贏了幾個人 >=3:冠軍 2:亞軍 1:季軍 <=0:最後一名 */
pip_race_eventB()
{
  /* 從 racemanlist 六位中挑出三個陪賽者 */
  player[0] = rand() % 2 + 4;
  player[1] = rand() % 2;
  player[2] = rand() % 2 + 2;

  /* 直接看能力，沒有比賽過程 */
  return ((d.art * 2 + d.character) / 600);
}


/*-------------------------------------------------------*/
/* 皇家舞會	 				         */
/*-------------------------------------------------------*/


/* itoc.030331.遊戲設計: 完全由 d.art 和 d.charm 來決定皇家舞會的成績 */

static int			/* 回傳: 贏了幾個人 >=3:冠軍 2:亞軍 1:季軍 <=0:最後一名 */
pip_race_eventC()
{
  /* 從 racemanlist 六位中挑出三個陪賽者 */
  player[0] = rand() % 2 + 2;
  player[1] = rand() % 2 + 4;
  player[2] = rand() % 2;

  /* 直接看能力，沒有比賽過程 */
  return ((d.art * 2 + d.charm) / 600);
}


/*-------------------------------------------------------*/
/* 烹飪大賽	 				         */
/*-------------------------------------------------------*/


/* itoc.030331.遊戲設計: 原則上是由 d.cook 和 d.affect 來決定烹飪大賽的成績，
   但是如果菜色和目前的狀態吻合的話有加分效果，反之則有扣分效果 */

static int			/* 回傳: 贏了幾個人 >=3:冠軍 2:亞軍 1:季軍 <=0:最後一名 */
pip_race_eventD()
{
  int winorlost;

  /* 從 racemanlist 六位中挑出三個陪賽者 */
  player[0] = rand() % 2 + 4;
  player[1] = rand() % 2 + 2;
  player[2] = rand() % 2;

  winorlost = ians(b_lines - 1, 0, "您想煮哪種口味的菜色？0)家常 1)酸 2)甜 3)苦 4)辣 [0] ");
  if (winorlost == '1')
    winorlost = 70 - d.satisfy * 2;	/* 越不滿足煮出來的菜才越帶有醋味 (不滿足會吃醋) */
  else if (winorlost == '2')
    winorlost = d.happy * 2 - 130;	/* 越是快樂煮出來的菜才越帶有甜度 (很快樂會甜蜜) */
  else if (winorlost == '3')
    winorlost = d.shit * 2 - 130;	/* 越是骯髒煮出來的菜才越帶有苦處 (大便是苦的:p) */
  else if (winorlost == '4')
    winorlost = d.sick * 2 - 130;	/* 越是生病煮出來的菜才越帶有辣香 (生病味覺不靈) */
  else
    winorlost = 0;			/* 家常菜與狀態無關 */

  winorlost += d.cook * 2 + d.affect;

  return (winorlost / 600);
}


/*-------------------------------------------------------*/
/* 比賽結果	 				         */
/*-------------------------------------------------------*/


static void
pip_race_ending(winorlost, mode)
  int winorlost;		/* 贏了幾個人 >=3:冠軍 2:亞軍 1:季軍 <=0:最後一名 */
  int mode;			/* 參加哪一種比賽 */
{
  char *name1, *name2, *name3, *name4;
  char buf[80];

  if (winorlost <= 0)		/* 最後一名 */
  {
    name1 = racemanlist[player[2]].name;
    name2 = racemanlist[player[1]].name;
    name3 = racemanlist[player[0]].name;
    name4 = d.name;
  }
  else if (winorlost == 1)	/* 季軍獎金 2000 */
  {
    name1 = racemanlist[player[2]].name;
    name2 = racemanlist[player[1]].name;
    name3 = d.name;
    name4 = racemanlist[player[0]].name;
    d.money += 2000;
  }
  else if (winorlost == 2)	/* 亞軍獎金 5000 */
  {
    name1 = racemanlist[player[2]].name;
    name2 = d.name;
    name3 = racemanlist[player[1]].name;
    name4 = racemanlist[player[0]].name;
    d.money += 5000;
  }
  else				/* 冠軍獎金 10000 */
  {
    name1 = d.name;
    name2 = racemanlist[player[2]].name;
    name3 = racemanlist[player[1]].name;
    name4 = racemanlist[player[0]].name;
    d.money += 10000;
  }

  clear();
  move(6, 13);
  prints("\033[1;37m～～～\033[32m本屆 %s 結果揭曉\033[37m～～～\033[m", racename[mode - 1]);
  move(8, 15);
  prints("\033[1;41m 冠軍 \033[0;1m～\033[1;33m%-10s\033[36m  獎金 %d\033[m", name1, 10000);
  move(10, 15);
  prints("\033[1;41m 亞軍 \033[0;1m～\033[1;33m%-10s\033[36m  獎金 %d\033[m", name2, 5000);
  move(12, 15);
  prints("\033[1;41m 季軍 \033[0;1m～\033[1;33m%-10s\033[36m  獎金 %d\033[m", name3, 2000);
  move(14, 15);
  prints("\033[1;41m 最後 \033[0;1m～\033[1;33m%-10s\033[36m\033[0m", name4);
  sprintf(buf, "今年的%s結束囉 後年再來吧..", racename[mode - 1]);
  vmsg(buf);
}


/*-------------------------------------------------------*/
/* 收穫季比賽					         */
/*-------------------------------------------------------*/


int			/* !=0:參加的項目 0:不參加 */
pip_race_main()		/* 收穫季 */
{
  int ch;
  int winorlost;		/* 贏了幾個人 >=3:冠軍 2:亞軍 1:季軍 <=0:最後一名 */

  clear();
  move(10, 14);
  outs("\033[1;33m叮咚叮咚～ 辛苦的郵差幫我們送信來了喔...\033[m");
  vmsg("嗯  把信打開看看吧...");

  show_resultshow_pic(0);

  move(b_lines - 2, 0);
  prints("[A]%s [B]%s [C]%s [D]%s [Q]放棄：", racename[0], racename[1], racename[2], racename[3]);
  do
  {
    ch = vkey();
  } while (ch != 'q' && (ch < 'a' || ch > 'd'));

  if (ch == 'q')
  {
    vmsg("今年不參加啦.....:(");
    d.happy -= rand() % 10 + 10;
    d.satisfy -= rand() % 10 + 10;
    d.relation -= rand() % 10;
    return 0;
  }

  ch -= 'a' - 1;
  show_resultshow_pic(ch);
  vmsg("今年共有四人參賽～現在比賽開始");

  switch (ch)
  {
  case 1:					/* 武鬥大會 */
    winorlost = pip_race_eventA();
    d.hexp += rand() % 10 + 20 * winorlost;
    d.exp += rand() % 10 + d.level * winorlost;
    break;

  case 2:					/* 藝術大展 */
    winorlost = pip_race_eventB();
    d.art += rand() % 10 + 20 * winorlost;
    d.character += rand() % 10 + 20 * winorlost;
    break;

  case 3:					/* 皇家舞會 */
    winorlost = pip_race_eventC();
    d.art += rand() % 10 + 20 * winorlost;
    d.charm += rand() % 10 + 20 * winorlost;
    break;

  case 4:					/* 烹飪大賽 */
    winorlost = pip_race_eventD();
    d.cook += rand() % 10 + 20 * winorlost;
    d.family += rand() % 10 + 20 * winorlost;
    break;
  }

  pip_race_ending(winorlost, ch);

  /* 如果參加的話，恢復所有屬性 */
  d.tired = 0;
  d.hp = d.maxhp;
  d.happy += rand() % 20;
  d.satisfy += rand() % 20;
  d.relation += rand() % 10;

  return ch;
}
#endif	/* HAVE_GAME */
