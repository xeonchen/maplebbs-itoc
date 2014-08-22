/*-------------------------------------------------------*/
/* pip_basic.c         ( NTHU CS MapleBBS Ver 3.10 )     */
/*-------------------------------------------------------*/
/* target : 基本選單                                     */
/* create :   /  /                                       */
/* update : 01/08/14                                     */
/* author : dsyan.bbs@forever.twbbs.org                  */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME

#include "pip.h"


/*-------------------------------------------------------*/
/* 基本選單:餵食 清潔 親親 休息	換雞金			 */
/*-------------------------------------------------------*/


int				/* 1: 沒吃食物，放棄  0: 吃了 */
pip_basic_feed()		/* 餵食 */
{
  int ch;
  int nodone;			/* itoc.010731: 記錄是否有行動 */

  nodone = 1;

  do
  {
    out_cmd(COLOR1 " 一般 " COLOR2 " [1]吃飯 [2]零食 [3]書本 [4]玩具 [5]讀物 [Q]跳出                        \033[m", 
      COLOR1 " 藥品 " COLOR2 " [a]大還 [b]靈芝 [c]補丸 [d]人蔘 [e]黑玉 [f]雪蓮 [Q]跳出                \033[m");

    switch (ch = vkey())
    {
    case '1':		/* 吃飯 */
      if (d.food <= 0)
      {
	vmsg("沒有食物囉..快去買吧！");
	break;
      }
      d.food--;
      d.hp += 50;
      if (d.hp > d.maxhp)
      {
	d.hp = d.maxhp;
	d.weight += rand() % 2;
      }
      nodone = 0;
      if ((d.bbtime / 60 / 30) < 3)		/* 未滿三歲 */
	show_feed_pic(11);
      else
	show_feed_pic(12);
      vmsg("每吃一次食物會恢復體力50喔!");
      break;

    case '2':		/* 零食 */
      if (d.cookie <= 0)
      {
	vmsg("零食吃光囉..快去買吧！");
	break;
      }
      d.cookie--;
      d.hp += 100;
      if (d.hp > d.maxhp)
      {
	d.hp = d.maxhp;
	d.weight += rand() % 2 + 2;
      }
      else
      {
	d.weight += (rand() % 2 + 1);
      }
      d.happy += (rand() % 3 + 4);
      d.satisfy += rand() % 3 + 2;
      nodone = 0;
      if (rand() % 2)
	show_feed_pic(21);
      else
	show_feed_pic(22);
      vmsg("吃零食容易胖喔...");
      break;

    case '3':		/* 書本 */
      if (d.book <= 0)
      {
	vmsg("沒有書本囉..快去買吧！");
	break;
      }
      d.book--;
      d.happy -= 5;
      d.wisdom+= 20;
      d.art += 20;
      nodone = 0;
      show_feed_pic(31);
      vmsg("開卷有益");
      break;

    case '4':		/* 玩具 */
      if (d.toy <= 0)
      {
	vmsg("沒有玩具囉..快去買吧！");
	break;
      }
      d.toy--;
      d.happy += 20;
      d.satisfy += 20;
      nodone = 0;
      show_feed_pic(41);
      vmsg("玩玩具的小孩不會變壞");
      break;

    case '5':		/* 讀物 */
      if (d.playboy <= 0)
      {
	vmsg("沒有課外讀物囉..快去買吧！");
	break;
      }
      if ((d.bbtime / 60 / 30) < 5)
      {
        /* itoc.010801: 五歲以後才能看 :p */
        vmsg("封面上寫著 5 禁 Ｘ");
        break;
      }
      d.playboy--;
      /* itoc.010801: 增加罪惡，但快樂/滿意大量上升 */
      d.happy = 100;
      d.satisfy = 100;
      d.art += 5;
      d.sin += 30;
      nodone = 0;
      show_feed_pic(51);
      vmsg("呼呼，還好沒人看見");
      break;

    case 'a':		/* 大還 */
      if (d.pill <= 0)
      {
	vmsg("沒有大還丹囉..快去買吧！");
	break;
      }
      d.pill--;
      d.hp += 1000;
      if (d.hp > d.maxhp)
	d.hp = d.maxhp;
      nodone = 0;
      show_feed_pic(61);
      vmsg("每吃一次大還丹會恢復體力 1000 喔!");
      break;

    case 'b':
      if (d.medicine <= 0)
      {
	vmsg("沒有靈芝囉..快去買吧！");
	break;
      }
      d.medicine--;
      d.mp += 1000;
      if (d.mp > d.maxmp)
	d.mp = d.maxmp;
      nodone = 0;
      show_feed_pic(71);
      vmsg("每吃一次靈芝會恢復法力 1000 喔!");
      break;

    case 'c':		/* 補丸 */
      if (d.burger <= 0)
      {
	vmsg("沒有大補丸了耶! 快去買吧..");
	break;
      }
      d.burger--;
      d.vp += 1000;
      if (d.vp > d.maxvp)
	d.vp = d.maxvp;
      nodone = 0;
      show_feed_pic(81);
      vmsg("每吃一次補丸會恢復移動 1000 喔!");
      break;

    case 'd':		/* 人蔘 */
      if (d.ginseng <= 0)
      {
	vmsg("沒有千年人蔘耶! 快去買吧..");
	break;
      }
      d.ginseng--;
      d.sp += 1000;
      if (d.sp > d.maxsp)
        d.sp = d.maxsp;
      nodone = 0;
      show_feed_pic(91);
      vmsg("每吃一次人蔘會恢復內力 1000 喔!");
      break;

    case 'e':		/* 黑玉 */
      if (d.paste <= 0)
      {
	vmsg("沒有黑玉斷續膏耶! 快去買吧..");
	break;
      }
      d.snowgrass--;
      d.hp = d.maxhp;
      d.tired = 0;
      d.sick = 0;
      nodone = 0;
      show_feed_pic(101);
      vmsg("黑玉斷續膏..超極棒的唷...");
      break;

    case 'f':		/* 雪蓮 */
      if (d.snowgrass <= 0)
      {
	vmsg("沒有天山雪蓮耶! 快去買吧..");
	break;
      }
      d.snowgrass--;
      d.hp = d.maxhp;
      d.mp = d.maxmp;
      d.vp = d.maxvp;
      d.sp = d.maxsp;
      d.tired = 0;
      d.sick = 0;
      nodone = 0;
      show_feed_pic(111);
      vmsg("天山雪蓮..超極棒的唷...");
      break;
    }
  } while (ch != 'q' && ch != KEY_LEFT);

  return nodone;
}


int
pip_basic_takeshower()		/* 洗澡 */
{
  d.shit -= 20;
  if (d.shit < 0)
    d.shit = 0;

  d.hp -= rand() % 2 + 3;

  switch(rand() % 3)
  {
  case 0:
    show_usual_pic(1);
    vmsg("我是乾淨的小雞  cccc....");
    break;

  case 1:
    show_usual_pic(7);
    vmsg("馬桶 嗯～～");
    break;

  case 2: 
    show_usual_pic(2);
    vmsg("我愛洗澡 lalala....");
    break;
  }
  return 0;
}


int
pip_basic_takerest()		/* 休息 */
{
  count_tired(5, 20, 1, 100, 0);	/* 恢復疲勞 */
  d.shit++;
  d.hp += d.maxhp / 10;
  if (d.hp > d.maxhp)
    d.hp = d.maxhp;

  show_usual_pic(5);
  vmsg("再按一下我就起床囉....");
  show_usual_pic(6);
  vmsg("喂喂喂..該起床囉......");
  return 0;
}


int
pip_basic_kiss()		/* 親親 */
{
  if (rand() % 2)
  {
    d.happy += rand() % 3 + 4;
    d.satisfy += rand() % 2 + 1;
  }
  else
  {
    d.happy += rand() % 2 + 1;
    d.satisfy += rand() % 3 + 4;
  }
  count_tired(1, 2, 0, 100, 1);
  d.shit += rand() % 5 + 4;
  d.relation += rand() % 2;

  show_usual_pic(3);

  if (d.shit < 60)
    vmsg("來嘛! 啵一個.....");
  else
    vmsg("親太多也是會髒死的喔....");

  return 0;
}


int
pip_money()
{
  int money;
  char buf[80];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return 0;
  }

  /* itoc.註解: 之所以不提供小雞幣換銀幣的原因是因為小雞可以儲存/讀取進度 */

  clrfromto(6, 18);
  prints("您身上有 %d 銀幣,雞金 %d 元\n", cuser.money, d.money);
  outs("\n一銀幣換一雞金唷！\n");

  do
  {
    if (!vget(10, 0, "要換多少銀幣？[Q] ", buf, 10, DOECHO))
      return 0;
    money = atol(buf);
  } while (money <= 0 || money > cuser.money);

  sprintf(buf, "是否要轉換 %d 銀幣 為 %d 雞金(Y/N)？[N] ", money, money);
  if (ians(11, 0, buf) == 'y')
  {
    cuser.money -= money;
    d.money += money;
    sprintf(buf, "您身上有 %d 次銀幣,雞金 %d 元", cuser.money, d.money);
    vmsg(buf);
    return 1;
  }
  vmsg("取消....");
  return 0;
}
#endif		/* HAVE_GAME */
