/* ----------------------------------------------------- */
/* pip_item.c     ( NTHU CS MapleBBS Ver 3.10 )          */
/* ----------------------------------------------------- */
/* target : 小雞 item                                    */
/* create :   /  /                                       */
/* update : 01/08/14                                     */
/* author : dsyan.bbs@forever.twbbs.org                  */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */  
/* ----------------------------------------------------- */


#include "bbs.h"

#ifdef HAVE_GAME

#include "pip.h"


struct itemset pipfoodlist[] = 
{
  /*  name          msgbuy           msgfeed                        price */
  0, "物品名",     "購買須知",      "使用須知",                        0, 
  1, "好吃的食物", "體力恢復 50",   "每吃一次食物會恢復體力 50 喔",   50, 
  2, "美味的零食", "體力恢復 100",  "除了恢復體力，小雞也會更快樂",  120, 
  0, NULL, NULL, NULL, 0
};


struct itemset pipmedicinelist[] = 
{
  /*  name          msgbuy           msgfeed                        price */
  0, "物品名",     "購買須知",      "使用須知",                        0, 
  1, "補血大還丹", "體力恢復 1000", "恢復大量流失體力的良方",       1000, 
  2, "珍貴的靈芝", "法力恢復 1000", "恢復大量流失法力的良方",       1000, 
  3, "好用大補丸", "移動恢復 1000", "恢復大量流失移動的良方",       1000, 
  4, "千年人參王", "內力恢復 1000", "恢復大量流失內力的良方",       1000, 
  5, "黑玉斷續膏", "體力完全恢復",  "傳說中能將所有受傷恢復的藥材", 5000, 
  6, "天山雪蓮",   "狀態完全恢復",  "東北天山才有雪蓮子",          10000, 
  0, NULL, NULL, NULL, 0
};


struct itemset pipotherlist[] = 
{
  /*  name          msgbuy           msgfeed                        price */
  0, "物品名",     "購買須知",      "使用須知",                        0, 
  1, "百科全書",   "知識的來源",    "書本讓小雞更聰明更有氣質啦",   3000, 
  2, "樂高玩具組", "快樂滿意度",    "玩具讓小雞更快樂啦",            300, 
  3, "閣樓雜誌",   "滿足的快感",    "書中自有顏如玉啦",              500, 
  0, NULL, NULL, NULL, 0
};


/* ------------------------------------------------------- */
/* 物品購買函式                                            */
/* ------------------------------------------------------- */


int
pip_buy_item(mode, p, oldnum)
  int mode;
  int oldnum[];
  struct itemset *p;
{
  char *shopname[4] = {"店名", "便利商店", "長春藥鋪", "夜裡書局"};
  char buf[128], genbuf[20];
  int oldmoney;		/* 進商店前原有錢 */
  int total;		/* 購買/販賣個數 */
  int ch, choice;

  oldmoney = d.money;

  /* 秀出產品列表 */
  clrfromto(6, 18);
  outs("\033[1;31m  ─\033[41;37m 編號\033[0;1;31m─\033[41;37m 商      品\033[0;1;31m──\033[41;37m 效            能\033[0;1;31m──\033[41;37m 價     格\033[0;1;31m─\033[37;41m 擁有數量\033[0;1;31m─\033[m\n\n");
  for (ch = 1; ch <= oldnum[0]; ch++)
  {
    prints("    \033[1;35m[\033[37m%2d\033[35m]    \033[36m%-10s     \033[37m%-14s       \033[1;33m%-10d  \033[1;32m%-9d   \033[m\n",
      p[ch].num, p[ch].name, p[ch].msgbuy, p[ch].price, oldnum[ch]);
  }

  do
  {
    sprintf(buf, COLOR1 " 採買 " COLOR2 " (%8s) [B]買入物品 [S]賣出物品 [Q]跳出                             \033[m", shopname[mode]);
    out_cmd("", buf);

    switch (ch = vkey())
    {
    case 'b':
      sprintf(buf, "想要買入啥呢？[0]放棄買入 [1～%d]物品商號：", oldnum[0]);
      choice = ians(b_lines - 2, 0, buf) - '0';
      if (choice >= 1 && choice <= oldnum[0])
      {
	sprintf(buf, "您要買入物品 [%s] 多少個呢(1-%d)？[Q] ", p[choice].name, d.money / p[choice].price);
	vget(b_lines - 2, 0, buf, genbuf, 6, DOECHO);
	total = atoi(genbuf);

	if (total <= 0)
	{
	  vmsg("放棄買入...");
	}
	else if (d.money < total * p[choice].price)
	{
	  vmsg("您的錢沒有那麼多喔..");
	}
	else
	{
	  sprintf(buf, "確定買入總價為 %d 的物品 [%s] 數量 %d 個嗎(Y/N)？[N] ", total * p[choice].price, p[choice].name, total);
	  if (ians(b_lines - 2, 0, buf) == 'y')
	  {
	    oldnum[choice] += total;
	    d.money -= total * p[choice].price;

	    /* itoc.010816: 更新擁有數量 */
	    move(7 + choice, 0);
	    prints("    \033[1;35m[\033[37m%2d\033[35m]    \033[36m%-10s     \033[37m%-14s       \033[1;33m%-10d  \033[1;32m%-9d   \033[m",
	      p[choice].num, p[choice].name, p[choice].msgbuy, p[choice].price, oldnum[choice]);

	    vmsg(p[choice].msguse);
	  }
	  else
	  {
	    vmsg("放棄買入...");
	  }
	}
      }
      else
      {
	sprintf(buf, "放棄買入.....");
	vmsg(buf);
      }
      break;

    case 's':
      sprintf(buf, "想要賣出啥呢？[0]放棄賣出 [1～%d]物品商號: ", oldnum[0]);
      choice = ians(b_lines - 2, 0, buf) - '0';
      if (choice >= 1 && choice <= oldnum[0])
      {
	sprintf(buf, "您要賣出物品 [%s] 多少個呢(1-%d)？[Q] ", p[choice].name, oldnum[choice]);
	vget(b_lines - 2, 0, buf, genbuf, 6, DOECHO);
	total = atoi(genbuf);

	if (total <= 0)
	{
	  vmsg("放棄賣出...");
	}
	else if (total > oldnum[choice])
	{
	  sprintf(buf, "您的 [%s] 沒有那麼多個喔", p[choice].name);
	  vmsg(buf);
	}
	else
	{
	  sprintf(buf, "確定賣出總價為 %d 的物品 [%s] 數量 %d 個嗎(Y/N)？[N] ", total * p[choice].price * 4 / 5, p[choice].name, total);
	  if (ians(b_lines - 2, 0, buf) == 'y')
	  {
	    oldnum[choice] -= total;
	    d.money += total * p[choice].price * 8 / 10;

	    /* itoc.010816: 更新擁有數量 */
	    move(7 + choice, 0);
	    prints("    \033[1;35m[\033[37m%2d\033[35m]    \033[36m%-10s     \033[37m%-14s       \033[1;33m%-10d  \033[1;32m%-9d   \033[m",
	      p[choice].num, p[choice].name, p[choice].msgbuy, p[choice].price, oldnum[choice]);

	    sprintf(buf, "老闆拿走了您的 %d 個%s", total,  p[choice].name);
	    vmsg(buf);
	  }
	  else
	  {
	    vmsg("放棄賣出...");
	  }
	}
      }
      else
      {
	sprintf(buf, "放棄賣出.....");
	vmsg(buf);
      }
      break;

    case 'q':
    case KEY_LEFT:
      sprintf(buf, "金錢交易共 %d 元,離開 %s ", oldmoney - d.money, shopname[mode]);
      vmsg(buf);
      break;
    }

    /* itoc.010816: 消掉 ians() vget() 留下的殘骸 */
    move (b_lines - 2, 0);
    clrtoeol();

  } while (ch != 'q' && ch != KEY_LEFT);

  return 0;
}


/*-------------------------------------------------------*/
/* 商店選單:食物 零食 大補丸 玩具 書本			 */
/*-------------------------------------------------------*/

/*-------------------------------------------------------*/
/* 函式庫                      				 */
/*-------------------------------------------------------*/

int
pip_store_food()
{
  int num[3];
  num[0] = 2;
  num[1] = d.food;
  num[2] = d.cookie;
  pip_buy_item(1, pipfoodlist, num);
  d.food = num[1];
  d.cookie = num[2];
  return 0;
}


int
pip_store_medicine()
{
  int num[7];
  num[0] = 6;
  num[1] = d.pill;  
  num[2] = d.medicine;
  num[3] = d.burger;
  num[4] = d.ginseng;
  num[5] = d.paste;
  num[6] = d.snowgrass;
  pip_buy_item(2, pipmedicinelist, num);
  d.pill = num[1];
  d.medicine = num[2];
  d.burger = num[3];
  d.ginseng = num[4];
  d.paste = num[5];
  d.snowgrass = num[6];
  return 0;
}


int
pip_store_other()
{
  int num[4];
  num[0] = 3;
  num[1] = d.book;
  num[2] = d.toy;
  num[3] = d.playboy;
  pip_buy_item(3, pipotherlist, num);
  d.book = num[1];
  d.toy = num[2];
  d.playboy = num[3];
  return 0;
}
#endif		/* HAVE_GAME */
