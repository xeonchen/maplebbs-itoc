/* ----------------------------------------------------- */
/* pip_quest.c        ( NTHU CS MapleBBS Ver 3.10 )      */
/* ----------------------------------------------------- */
/* target : 戰鬥選單                                     */
/* create : 01/12/22                                     */
/* update :   /  /  		  			 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */  
/* ----------------------------------------------------- */


#include "bbs.h"

#ifdef HAVE_GAME

#include "pip.h"


/*-------------------------------------------------------*/
/* 所有任務: 回傳 1 表示完成   回傳 0 表示沒有完成       */
/*-------------------------------------------------------*/


/* 寫任務非常簡單：寫一個函式 pip_quest_??()，並加入 quest_cb，
   並改 PIPQUEST_NUM，最後編輯一個 etc/game/pip/quest/pic?? 即可 */


  /*-----------------------------------------------------*/
  /* pip_quest_1~99 取得物品的任務			 */
  /*-----------------------------------------------------*/


static int
pip_quest_1()		/* 取得基礎裝備 */
{
  /* 全身每個部位都穿著裝備就算合格 */
  if (d.weaponhead && d.weaponhand && d.weaponshield && d.weaponbody && d.weaponfoot)
  {
    /* 原裝備移除 */
    d.weaponhead = d.weaponhand = d.weaponshield = d.weaponbody = d.weaponfoot = 0;
    d.equiphead[0] = d.equiphand[0] = d.equipshield[0] = d.equipbody[0] = d.equipfoot[0] = '\0';

    d.tired = 0;
    vmsg("您不再感到疲憊了");
    return 1;
  }

  return 0;
}


static int
pip_quest_2()		/* 取得黃金裝備 */
{
  /* 全身每個部位的裝備名稱都包含「金」這字就算合格 */
  if (strstr(d.equiphead, "金") && strstr(d.equiphand, "金") &&
    strstr(d.equipshield, "金") && strstr(d.equipbody, "金") && strstr(d.equipfoot, "金"))
  {
    /* 原裝備移除 */
    d.weaponhead = d.weaponhand = d.weaponshield = d.weaponbody = d.weaponfoot = 0;
    d.equiphead[0] = d.equiphand[0] = d.equipshield[0] = d.equipbody[0] = d.equipfoot[0] = '\0';

    d.happy = 100;
    d.tired = 0;
    vmsg("您的精神百倍");
    return 1;
  }

  return 0;
}


static int
pip_quest_3()		/* 取得藥材 */
{
  /* 取得大還丹、靈芝、大補丸、千年人蔘、黑玉斷續膏、天山雪蓮各一 */
  if (d.pill && d.medicine && d.burger && d.ginseng && d.paste && d.snowgrass)
  {
    d.pill--;
    d.medicine--;
    d.burger--;
    d.ginseng--;
    d.paste--;
    d.snowgrass--;
    d.happy = 100;
    vmsg("助人為快樂之本");
    return 1;
  }

  return 0;
}


  /*-----------------------------------------------------*/
  /* pip_quest_101~199 屬性達成的任務			 */
  /*-----------------------------------------------------*/


static int
pip_quest_101()		/* 完全健康 */
{
  /* 達到不疲累、不生病、不骯髒 */
  if (d.tired == 0 && d.sick == 0 && d.shit == 0)
  {
    d.food++;
    d.cookie++;
    vmsg("真棒，女神賜您一些食物");
    return 1;
  }

  return 0;
}


  /*-----------------------------------------------------*/
  /* pip_quest_201~299 打敗怪物的任務			 */
  /*-----------------------------------------------------*/


static int
pip_quest_201()		/* 打倒病毒 */
{
  /* 打倒一隻能力比自己高的怪物 */

  int level;
  playrule m;

  strcpy(m.name, "變種病毒");  
  level = d.level + 5;
  m.hp = m.maxhp = 100 + level * level;
  m.attack = m.spirit = m.magic = m.armor = m.dodge = level * 15;
  m.money = 0;
  m.exp = 0;
  m.attribute = +7;		/* 專長: 究極法術 */ 
  m.pic = 004;

  if (pip_vs_man(m, 0))		/* 小雞對戰敵人 (借用武術大會) */
  {
    d.hexp += d.level;
    d.mexp += d.level;
    vmsg("您打敗了這隻變種病毒，評價提升");
    return 1;
  }

  return 0;
}


static int
pip_quest_202()		/* 十大惡人 */
{
  /* 依序打倒數隻怪物 */
  /* 若打敗十大惡人，名聲提高；反之，練成嫁衣神功 */

  int i;
  struct playrule badmanlist[] =	/* 十大惡人相當於等級 40 ~ 50 的怪物 */
  {
    /* name[13] attribute hp maxhp attack spirit magic armor dodge money exp pic */
    /* 愛用吸精 */	"李大嘴", +3, 1875, 1875, 500, 460, 350, 500, 500, 0, 220, 001,
    /* 愛用 blitz */	"陰九幽", +2, 1850, 1850, 520, 470, 320, 420, 630, 0, 220, 002,
    /* 愛用暗器 */	"哈哈兒", +7, 1750, 1750, 450, 450, 370, 400, 520, 0, 220, 003,
    /* 愛用炎系魔法 */	"屠嬌嬌", -4, 1635, 1635, 430, 340, 640, 360, 470, 0, 220, 004,
    /* 攻擊力特強 */	"杜  殺",  0, 2400, 2400, 610, 570, 280, 550, 500, 0, 250, 005,
  };

  if (ians(b_lines - 1, 0, "十大惡人看起來超強，您要找幫手嗎(Y/N)？[Y] ") != 'n')
  {
    if (ians(b_lines - 1, 0, "請誰當幫手？(1)燕東天 (2)燕西天 (3)燕南天 (4)燕北天 ") == '3')
    {
      /* itoc.050320: 若等級太低時接到這個任務會打不贏，所以要提供賤招 :p */
      vmsg("在大俠燕南天的幫助之下，您成功\地除去十大惡人");
      return 1;
    }
    else
    {
      vmsg("他拒絕了您的請求，看來您只好自己上了");
    }
  }

  for (i = 0; i< 5; i++)
  {
    if (!pip_vs_man(badmanlist[i], 0))	/* 小雞對戰敵人 (借用武術大會) */
    {
      vmsg("您被十大惡人圍攻，全身精脈俱斷");
      d.hp = 1;
      d.mp = 0;
      d.vp = 0;
      d.sp = 0;

      /* 嫁衣神功: 拿 maxmp 去換 maxsp */
      i = rand() % 10;
      if (d.maxmp > i)
      {
        d.maxmp -= i;
        d.maxsp += i;
        vmsg("在神醫萬春流的治療之下，您反練成嫁衣神功\");
      }

      return 0;
    }
  }

  d.hexp += 100;
  d.mexp += 100;
  vmsg("您成功\地除去惡人谷的所有壞蛋，名聲大幅提升");
  return 1;
}


  /*-----------------------------------------------------*/
  /* pip_quest_301~399 數學計算的任務			 */
  /*-----------------------------------------------------*/


static int
pip_quest_301()		/* 分配珠寶 */
{
  char ans[3];

  vget(b_lines, 0, "我應該可以分到幾個珠寶呢？", ans, 3, DOECHO);
  if (atoi(ans) == 2)
  {
    d.social += 10;
    vmsg("讓我想想看啊！噫，沒錯，您實在太聰明了");
    return 1;
  }

  return 0;
}


static int
pip_quest_302()		/* 0.9999... 循環小數 */
{
  int num;
  char ans1[4], ans2[4];

  if (vget(b_lines, 0, "循環小數 0.9999... 化為分數，分母是 ", ans1, 4, DOECHO) &&
    vget(b_lines, 0, "循環小數 0.9999... 化為分數，分子是 ", ans2, 4, DOECHO))
  {
    if ((num = atoi(ans1)) && (num == atoi(ans2)))	/* 分母不能為 0 */
    {
      d.wisdom += 10;
      vmsg("沒錯，0.9999... 就是 1");
      return 1;
    } 
  }

  return 0;
}


  /*-----------------------------------------------------*/
  /* pip_quest_401~499 格言欣賞的任務			 */
  /*-----------------------------------------------------*/


static int
pip_quest_401()		/* 態度百分百 */
{
  if (ians(b_lines - 1, 0, "1)知識 2)努力 3)態度 ") == '3')
  {
    d.affect += 5;
    d.toman += 5;
    vmsg("是的，唯有百分百的態度才能獲得眾人的尊敬");
    return 1;
  }

  return 0;
}


  /*-----------------------------------------------------*/
  /* quest_cb[] 任務列表			 	 */
  /*-----------------------------------------------------*/


#define PIPQUEST_NUM	9


/* static KeyFunc quest_cb[] = */
static KeyFunc quest_cb[PIPQUEST_NUM + 1] =	/* 把 PIPQUEST_NUM 指定進去，如果有錯，可以在 compile 中看出 */
{
  /* 尋找物品 */
  1, pip_quest_1,
  2, pip_quest_2,
  3, pip_quest_3,

  /* 屬性達成 */
  101, pip_quest_101,

  /* 打敗怪物 */
  201, pip_quest_201,
  202, pip_quest_202,

  /* 數學問題 */
  301, pip_quest_301,
  302, pip_quest_302,

  /* 格言欣賞 */
  401, pip_quest_401,
  
  0, NULL		/* 結束任務列表 */
};
    

/*-------------------------------------------------------*/
/* 任務函式                                              */
/*-------------------------------------------------------*/


static int		/* 1:查詢任務  0:沒有任務 */
pip_quest_query(quest)	/* 查詢舊任務 */
  int quest;		/* 任務編號 */
{
  if (!quest && !(quest = d.quest))
  {
    vmsg("您目前沒有任務在身");
    return 0;
  }
  show_quest_pic(quest);
  return 1;
}


int			/* 1:取得新任務  0:取消取得或已有任務 */
pip_quest_new()		/* 取得新任務 */
{
  if (ians(b_lines - 1, 0, "您已達升級標準，願意接受長老指派任務嗎(Y/N)？[N] ") == 'y')
  {
    d.quest = quest_cb[rand() % PIPQUEST_NUM].key;
    pip_quest_query(d.quest);
    vmsg("去吧，執行這個任務結束後我就賦予您更高的能力");
    return 1;
  }
  return 0;
}


static int		/* 1:任務完成  0:任務失敗 */
pip_quest_done()	/* 完成任務 */
{
  KeyFunc *cb;
  int key;

  if (!d.quest)
  {
    vmsg("您目前沒有任務在身");
    return 0;
  }

  /* itoc.註解: 是不是考慮換 binary search? */
  for (cb = quest_cb; (key = cb->key); cb++)
  {
    if (key == d.quest)
    {
      key = (*(cb->func)) ();	/* 1:完成 0:失敗 */
      pip_levelup(key);
      return key;
    }
  }

  vmsg("請告訴站長，找不到此任務的程式");	/* 應該不可能出現 */
  return 0;
}


static int		/* 1:放棄舊任務  0:取消放棄或沒有任務 */
pip_quest_abort()	/* 放棄舊任務 */
{
  if (!d.quest)
  {
    vmsg("您目前沒有任務在身");
  }
  else if (ians(b_lines - 1, 0, "您確定要放棄現有的任務嗎(Y/N)？[N] ") == 'y')
  {  
    pip_levelup(0);
    return 1;
  }
  else
  {
    vmsg("還是不要放棄好了");
  }
  return 0;
}


/*-------------------------------------------------------*/
/* 任務主選單                                            */
/*-------------------------------------------------------*/


int
pip_quest_menu()
{
  while (1)
  {
    out_cmd("", COLOR1 " 任務 " COLOR2 " [1]完成 [2]查詢 [3]放棄 [Q]跳出                                        \033[m");

    switch (vkey())
    {
      case 'q':
        return 0;

      case '1':
	pip_quest_done();
	break;
	
      case '2':
	pip_quest_query(0);
	break;

      case '3':
	pip_quest_abort();
	break;
    }
  }
}
#endif	/* HAVE_GAME */
