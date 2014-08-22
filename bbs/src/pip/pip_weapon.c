/* ----------------------------------------------------- */
/* pip_weapon.c     ( NTHU CS MapleBBS Ver 3.10 )        */
/* ----------------------------------------------------- */
/* target : 小雞 weapon structure                        */
/* create :   /  /                                       */
/* update : 01/08/15                                     */
/* author : dsyan.bbs@forever.twbbs.org                  */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/* ----------------------------------------------------- */


#include "bbs.h"

#ifdef HAVE_GAME

#include "pip.h"


/* ------------------------------------------------------- */
/* 武器購買函式                                            */
/* ------------------------------------------------------- */


/* name[11] quality cost */
static weapon p[9];		/* 記錄武器 */


/* itoc.021031: 為了增加遊戲的多樣性，寫一支武器產生器 */
static void
weapon_generate(type)
  int type;			/* 哪一部分裝備 */
{
  int i, num;

  char adje[14][5] = {"損壞", "再生", "二手", "絕版", "塑膠", "牛皮", "鋼鐵", "黃金", "特級", "屠龍", "忘情", "屠龍", "飛天", "傳奇"};
  char prep[13][3] = {"破",   "爛",   "鳥",   "之",   "狂",   "烈",   "炫",   "聖",   "魔",   "寶",   "光",   "神",   ""};
  char noun[5][9][5] =
  {
    /* 頭部武器 */    "帽",   "頭盔", "頭罩", "頭巾", "頭飾", "耳機", "眼鏡", "髮箍", "項鍊", 
    /* 手部武器 */    "劍",   "刀",   "杖",   "棒",   "槍",   "矛",   "弓",   "鎚",   "扳手", 
    /* 盾牌武器 */    "錶",   "盾",   "戒指", "手套", "手環", "臂章", "盾牌", "課本", "講義", 
    /* 身體武器 */    "盔甲", "冑甲", "皮甲", "披風", "套裝", "洋裝", "衣服", "Ｔ恤", "毛衣", 
    /* 腳部武器 */    "鞋",   "靴",   "屐",   "履",   "雲",   "輪",   "襪",   "毯",   "踏"
  };

  for (i = 0; i < 9; i++)
  {
    /* 依能力及手頭的錢來決定武器的好壞 */

    if (d.money < 12)
    {
      p[i].quality = 1;
      p[i].cost = d.money;
    }
    else
    {
      num = d.money / 1000 + 1;
      if (num > 300)
        num = 300;
      num = rand() % num + 1;
      p[i].quality = num;
      p[i].cost = 3 * num * num;
    }

    num = rand();	/* 用同一亂數來決定 adj+prep+noun，所以 mod 的數不要一樣 */
    /* 依哪一部分裝備來決定武器名稱，注意字串長度 */
    sprintf(p[i].name, "%s%s%s", adje[num % 14], prep[num % 13], noun[type][num % 9]);
  }
}


void
pip_weapon_wear(type, variance)	/* 裝備武器，計算能力的改變 */
  int type;			/* 哪一部分裝備 */
  int variance;			/* 新舊武器的品質差異 */
{
  /* 依裝備部位不同來改變指數 */
  if (type == 0)	/* 頭部武器 */
  {
    d.speed += variance;
    d.immune += variance;
  }
  else if (type == 1)	/* 手部武器 */
  {
    d.attack += variance;
    d.immune += variance;
  }
  else if (type == 2)	/* 盾牌武器 */
  {
    d.attack += variance;
    d.resist += variance;
  }
  else if (type == 3)	/* 身體武器 */
  {
    d.resist += variance;
    d.immune += variance;
  }
  else if (type == 4)	/* 腳部武器 */
  {
    d.attack += variance;
    d.speed += variance;
  }
}


static int
pip_weapon_doing_menu(quality, type, name)	/* 武器購買畫面 */
  int quality;			/* 傳入目前配戴 */
  int type;			/* 哪一部分裝備 */
  char *name;
{
  char menutitle[5][11] = {"頭部裝備區", "手部裝備區", "盾牌裝備區", "身體裝備區", "腳部裝備區"};
  char buf[80];
  int n;

  /* 亂數產生武器 */
  weapon_generate(type);

  /* 印出武器列表 */
  vs_head(menutitle[type], str_site);
  show_weapon_pic(0);
  move(11, 0);
  outs("  \033[1;37;41m [NO]  [武器名稱]  [品質]  [售價] \033[m\n");

  /* 印出武器單項 */    
  for (n = 0; n < 9; n++)
    prints("   %d     %-10s  %6d  %6d\n", n, p[n].name, p[n].quality, p[n].cost);

  /* 選單處理 */
  while (1)
  {
    out_cmd("", COLOR1 " 採買 " COLOR2 " (軍火販子) [B]購買武器 [E]強化武器 [D]拋棄武器 [Q]跳出                 \033[m");

    switch (vkey())
    {
    case 'b':
      sprintf(buf, "您有 %d 元，想要購買啥呢？[Q] ", d.money);
      n = ians(b_lines - 2, 1, buf) - '0';

      if (n >= 0 && n < 9)
      {
	sprintf(buf, "確定要購買價值 %d 元 的%s嗎(Y/N)？[N] ", p[n].cost, p[n].name);
	if (ians(b_lines - 2, 1, buf) == 'y')
	{
	  /* 換武器 */
	  d.money -= p[n].cost;
	  strcpy(name, p[n].name);
	  pip_weapon_wear(type, p[n].quality - quality);
	  quality = p[n].quality;

	  sprintf(buf, "小雞已經裝配上%s了", name);
	  vmsg(buf);
	}
	else
	{
	  vmsg("放棄購買");
	}
      }
      break;

    case 'e':
      n = quality * 100;
      if (quality && d.money >= n)
      {
        sprintf(buf, "確定要花 %d 元來提升%s的潛能嗎(Y/N)？[N] ", n, name);
        if (ians(b_lines - 2, 1, buf) == 'y')
        {
          /* 品質越好的武器強化收費越高 */
          d.money -= n;
          quality++;
          pip_weapon_wear(type, 1);
        }
      }
      break;

    case 'd':
      sprintf(buf, "確定要拋棄%s嗎(Y/N)？[N] ", name);
      if (ians(b_lines - 2, 1, buf) == 'y')
      {
        pip_weapon_wear(type, -quality);
        name[0] = '\0';
        quality = 0;
      }
      break;

    case 'q':
    case KEY_LEFT:
      return quality;
    }

    /* itoc.010816: 消掉 ians() 留下的殘骸 */
    move (b_lines - 2, 0);
    clrtoeol();
  }
}


/*-------------------------------------------------------*/
/* 武器商店選單: 各部位                                  */
/*-------------------------------------------------------*/


int
pip_store_weapon_head()		/* 頭部武器 */
{
  d.weaponhead = pip_weapon_doing_menu(d.weaponhead, 0, d.equiphead);
  return 0;
}


int
pip_store_weapon_hand()		/* 手部武器 */
{
  d.weaponhand = pip_weapon_doing_menu(d.weaponhand, 1, d.equiphand);
  return 0;
}


int
pip_store_weapon_shield()	/* 盾牌武器 */
{
  d.weaponshield = pip_weapon_doing_menu(d.weaponshield, 2, d.equipshield);
  return 0;
}


int
pip_store_weapon_body()		/* 身體武器 */
{
  d.weaponbody = pip_weapon_doing_menu(d.weaponbody, 3, d.equipbody);
  return 0;
}


int
pip_store_weapon_foot()		/* 腳部武器 */
{
  d.weaponfoot = pip_weapon_doing_menu(d.weaponfoot, 4, d.equipfoot);
  return 0;
}
#endif	/* HAVE_GAME */
