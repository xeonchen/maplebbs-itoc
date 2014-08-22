/* ----------------------------------------------------- */
/* pip_royal.c     ( NTHU CS MapleBBS Ver 3.10 )         */
/* ----------------------------------------------------- */
/* target : 小雞 royal                                   */
/* create :   /  /                                       */
/* update : 01/08/14                                     */
/* author : dsyan.bbs@forever.twbbs.org                  */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/* ----------------------------------------------------- */


#include "bbs.h"

#ifdef HAVE_GAME

#include "pip.h"


/* royalset:  num name needmode needvalue addtoman maxtoman words1 words2 */

struct royalset royallist[] = 
{
  "T", "拜訪對象",   0,   0,   0,   0, NULL, NULL,
  "A", "皇城騎兵連", 1,  10,  15, 100, "您真好，來陪我聊天..",		"守衛星空的安全是很辛苦的..",
  "B", "００７特務", 1, 100,  25, 200, "真是禮貌的小雞..我喜歡..",	"特務就是秘密保護站長安全的人..",
  "C", "鎮國大將軍", 1, 200,  30, 250, "當年那個戰役很精彩喔..",	"您真是高貴優雅的小雞..",
  "D", "參謀總務長", 1, 300,  35, 300, "我幫站長管理這個國家唷..",	"您的聲音很好聽耶..我很喜歡喔..:)",
  "E", "管理副站長", 1, 400,  35, 300, "您很有教養唷！很高興認識您..",	"優雅的您，請讓我幫您祈福..",
  "F", "系統站長",   1, 500,  40, 350, "您好可愛喔..我喜歡您唷..",	"對啦..以後要多多來和我玩喔..",
  "G", "程式站長",   1, 550,  40, 350, "告訴您唷，跟您講話很快樂喔..",	"來，坐我膝蓋\上，聽我講故事..",
  "H", SYSOPNICK,    1, 600,  50, 400, "一站之長責任重大呀..:)..",	"謝謝您聽我講話..以後要多來喔..",
  "I", "瘋狂灌水群", 2,  60,  20, 150, "不錯唷..蠻機靈的喔..很可愛..",	"來，我們一起來灌水吧..",
  "J", "青年帥武官", 0,   0,   0,   0, "您好，我是武官，剛從邊境回來",	"希望下次還能見到您..:)",
  NULL, NULL,        0,   0,   0,   0, NULL, NULL
};


static int
pip_go_palace_screen(p)
  struct royalset *p;
{
  char inbuf1[128], inbuf2[20];
  char *needmode[3] = {"      ", "禮儀表現＞", "談吐技巧＞"};
  int n, a, b, choice, change;
  int save[11];

  /* 秀出所有可以拜訪的人 */
  clear();
  show_palace_pic(0);
  move(13, 0);
  outs("    \033[1;31m┌──────┤\033[37;41m 來到總司令部了   請選擇您欲拜訪的對象\033[0;1;31m├──────┐\033[m\n");
  outs("    \033[1;31m│                                                                  │\033[m\n");

  for (n = 0; n < 5; n++)
  {
    a = 2 * n + 1;
    b = 2 * n + 2;
    sprintf(inbuf1, "%-10s%3d", needmode[p[a].needmode], p[a].needvalue);

    if (n == 4)	/* 王子 */
      sprintf(inbuf2, "%-10s", needmode[p[b].needmode]);
    else
      sprintf(inbuf2, "%-10s%3d", needmode[p[b].needmode], p[b].needvalue);

    if ((d.seeroyalJ == 1 && n == 4) || (n != 4))
    {
      prints("    \033[1;31m│\033[36m(\033[37m%s\033[36m)\033[33m%-10s \033[37m%-14s    \033[36m(\033[37m%s\033[36m)\033[33m%-10s \033[37m%-14s      \033[31m│\033[m\n",
	p[a].num, p[a].name, inbuf1, p[b].num, p[b].name, inbuf2);
    }
    else
    {
      prints("    \033[1;31m│\033[36m(\033[37m%s\033[36m)\033[33m%-10s \033[37m%-14s                                        \033[31m│\033[0m\n", 
  	p[a].num, p[a].name, inbuf1);
    }
  }
  outs("    \033[1;31m│                                                                  │\033[m\n");
  outs("    \033[1;31m└─────────────────────────────────┘\033[m");

  while (1)
  {
    sprintf(inbuf1, COLOR1 " [生命力] %6d/%6d  [疲勞度] %6d                                      \033[m", d.hp, d.maxhp, d.tired);
    out_cmd(inbuf1, COLOR1 " 參見選單 " COLOR2 " [字母]選擇欲拜訪的人物 [Q]離開總司令部                             \033[m");

    choice = vkey();
    if (choice == 'q' || choice == KEY_LEFT)
    {
      vmsg("離開" BBSNAME "總司令部.....");
      return 0;
    }

    /* 將各人物已經給與的數值先儲存起來*/
    save[1] = d.royalA;		/* from守衛 */
    save[2] = d.royalB;		/* from近衛 */
    save[3] = d.royalC;		/* from將軍 */
    save[4] = d.royalD;		/* from大臣 */
    save[5] = d.royalE;		/* from祭司 */
    save[6] = d.royalF;		/* from寵妃 */
    save[7] = d.royalG;		/* from王妃 */
    save[8] = d.royalH;		/* from國王 */
    save[9] = d.royalI;		/* from小丑 */
    save[10] = d.royalJ;	/* from王子 */

    choice -= 'a' - 1;

    if ((choice >= 1 && choice <= 10 && d.seeroyalJ == 1) || (choice >= 1 && choice <= 9 && d.seeroyalJ == 0))
    {
      d.social += rand() % 3 + 3;
      d.hp -= rand() % 5 + 6;
      d.tired += rand() % 5 + 8;

      if ((p[choice].needmode == 0) || (p[choice].needmode == 1 && d.manners >= p[choice].needvalue) || 
        (p[choice].needmode == 2 && d.speech >= p[choice].needvalue))
      {
	if (choice >= 1 && choice <= 9 && save[choice] >= p[choice].maxtoman)
	{
	  vmsg(rand() % 2 ? "能和這麼偉大的您講話真是榮幸ㄚ..." : "很高興您來拜訪我，但我不能給您什麼了..");
	}
	else
	{
	  if (choice >= 1 && choice <= 8)	/* 晉見官員，增加待人接物 */
	  {
	    switch (choice)
	    {
	    case 1:
	      change = d.character / 5;
	      break;
	    case 2:
	      change = d.character / 8;
	      break;
	    case 3:
	      change = d.charm / 5;
	      break;
	    case 4:
	      change = d.wisdom / 10;
	      break;
	    case 5:
	      change = d.belief / 10;
	      break;
	    case 6:
	      change = d.speech / 10;
	      break;
	    case 7:
	      change = d.social / 10;
	      break;
	    case 8:
	      change = d.hexp / 10;
	      break;
	    }

	    if (change > p[choice].addtoman)		/* 如果大於每次的增加最大量 */
	      change = p[choice].addtoman;
	    else if ((change + save[choice]) >= p[choice].maxtoman)	/* 如果加上原先的之後大於所能給的所有值時 */
	      change = p[choice].maxtoman - save[choice];

	    save[choice] += change;
	    d.toman += change;
	  }
	  else if (choice == 9)			/* 找小丑 */
	  {
	    save[9] = 0;
	    d.social -= 13 + rand() % 4;
	    d.affect += 13 + rand() % 4;
	  }
	  else if (choice == 10 && d.seeroyalJ == 1)	/* 拜訪王子 */
	  {
	    save[10] += 15 + rand() % 4;
	    d.seeroyalJ = 0;
	  }

	  vmsg(rand() % 2 ? p[choice].words1 : p[choice].words2);
	}
      }
      else
      {
	vmsg(rand() % 2 ? "我不和這樣的雞談話...." : "沒教養的雞，再去學學禮儀吧....");
      }
    }

    d.royalA = save[1];
    d.royalB = save[2];
    d.royalC = save[3];
    d.royalD = save[4];
    d.royalE = save[5];
    d.royalF = save[6];
    d.royalG = save[7];
    d.royalH = save[8];
    d.royalI = save[9];
    d.royalJ = save[10];
  }
}


int
pip_go_palace()			/* 參見 */
{
  pip_go_palace_screen(royallist);
  return 0;
}
#endif	/* HAVE_GAME */

