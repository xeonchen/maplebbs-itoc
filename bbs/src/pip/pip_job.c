/*-------------------------------------------------------*/
/* pip_job.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : 打工                                         */
/* create :   /  /                                       */
/* update : 01/08/15                                     */
/* author : dsyan.bbs@forever.twbbs.org		 	 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME

#include "pip.h"


/*-------------------------------------------------------*/
/* 函式庫                                                */
/*-------------------------------------------------------*/


/* itoc.010815: 寫一隻公用的 function */

static int		/* 傳回: -1:放棄  0~100:成功狀況 */
pip_job_function(classgrade, tired_prob, tired_base, pic)
  int classgrade;	/* 身心狀況: 0% ~ 100 % */
  int tired_prob;	/* 計算疲勞的機率 */
  int tired_base;	/* 計算疲勞的底數 */
  int pic;		/* 要秀的圖 */
{
  int grade;

  /* 因為還沒 update，learn_skill 可能 < 0 */
  if (LEARN_LEVEL < 0)
  {
    vmsg("您已經累到爆了");
    return -1;
  }

  grade = classgrade * LEARN_LEVEL;

  /* grade 應該只從 0~100% */
  if (grade < 0)
    grade = 0;
  else if (grade > 100)
    grade = 100;

  /* 所有工作都會改變的屬性 */
  count_tired(tired_prob, tired_base, 1, 100, 1);	/* 增加疲勞與年齡有關 */
  d.shit += rand() % 3 + 5;
  d.hp -= rand() % 2 + 4;
  d.happy -= rand() % 3 + 4;
  d.satisfy -= rand() % 3 + 4;

  show_job_pic(pic);
  return grade;		/* 回傳工作結果: 0:失敗透了  ~   100:完全成功 */
}


/*-------------------------------------------------------*/
/* 打工選單:家事 苦工 家教 地攤				 */
/*-------------------------------------------------------*/


int
pip_job_workA()
{
  /* ├────┼──────────────────────┤ */
  /* │家庭管理│+ 待人接物 掃地洗衣 烹飪 親子關係 家事評價  │ */
  /* │        │- 感受                                      │ */
  /* ├────┼──────────────────────┤ */

  int class;

  class = d.hp * 100 / d.maxhp - d.tired;
  if ((class = pip_job_function(class, 2, 5, 11)) < 0)
    return 0;

  if (class >= 75)
  {
    class = 4;
    d.money += 80 + (d.homework + d.cook) / 50;
    vmsg("家事很成功\喔..多一點錢給您..");
  }
  else if (class >= 50)
  {
    class = 3;
    d.money += 60 + (d.homework + d.cook) / 55;
    vmsg("家事還蠻順利的唷..嗯嗯..");
  }
  else if (class >= 25)
  {
    class = 2;
    d.money += 40 + (d.homework + d.cook) / 60;
    vmsg("家事普普通通啦..可以更好的..加油..");
  }
  else
  {
    class = 1;
    d.money += 20 + (d.homework + d.cook) / 65;
    vmsg("家事很糟糕喔..這樣不行啦..");
  }

  d.toman += rand() % 2;
  d.homework += rand() % 2 + class;
  d.cook += rand() % 2 + class;
  d.relation += rand() % 2;
  d.family += class;

  d.affect -= rand() % 5;
  if (d.affect < 0)
    d.affect = 0;

  d.workA++;
  return 0;
}


int
pip_job_workB()
{
  /* ├────┼──────────────────────┤ */
  /* │育幼院  │+ 待人接物 愛心 感受                        │ */
  /* │        │- 魅力                                      │ */
  /* ├────┼──────────────────────┤ */

  int class;

  class = d.hp * 100 / d.maxhp - d.tired;
  if ((class = pip_job_function(class, 3, 7, 21)) < 0)
    return 0;

  if (class >= 75)
  {
    class = 4;
    d.money += 150 + (d.toman + d.love) / 50;
    vmsg("當保姆很成功\喔..下次再來喔..");
  }
  else if (class >= 50)
  {
    class = 3;
    d.money += 120 + (d.toman + d.love) / 55;
    vmsg("保姆還當的不錯唷..嗯嗯..");
  }
  else if (class >= 25)
  {
    class = 2;
    d.money += 100 + (d.toman + d.love) / 60;
    vmsg("小朋友很皮喔..加油..");
  }
  else
  {
    class = 1;
    d.money += 80 + (d.toman + d.love) / 65;
    vmsg("很糟糕喔..連小朋友都罩不住..");
  }

  d.toman += rand() % 3;
  d.love += rand() % 3 + class;
  d.affect += class;

  d.charm -= rand() % 5;
  if (d.charm < 0)
    d.charm = 0;

  /* itoc.010824: 亂數學會暗器 */
  if (rand() % 30 == 0)
    pip_learn_skill(7);

  d.workB++;
  return 0;
}


int
pip_job_workC()
{
  /* ├────┼──────────────────────┤ */
  /* │旅館    │+ 掃地洗衣 烹飪 家事評價                    │ */
  /* │        │- 無                                        │ */
  /* ├────┼──────────────────────┤ */

  int class;

  class = d.hp * 100 / d.maxhp - d.tired;
  if ((class = pip_job_function(class, 4, 8, 31)) < 0)
    return 0;

  if (class >= 75)
  {
    class = 4;
    d.money += 250 + (d.homework + d.cook) / 50;
    vmsg("旅館事業蒸蒸日上..希望您再過來幫忙..");
  }
  else if (class >= 50)
  {
    class = 3;
    d.money += 200 + (d.homework + d.cook) / 55;
    vmsg("旅館還蠻順利的唷..嗯嗯..");
  }
  else if (class >= 25)
  {
    class = 2;
    d.money += 150 + (d.homework + d.cook) / 60;
    vmsg("普普通通啦..可以更好的..加油..");
  }
  else
  {
    class = 1;
    d.money += 100 + (d.homework + d.cook) / 65;
    vmsg("這個很糟糕喔..這樣不行啦..");
  }

  d.homework += rand() % 2 + class;
  d.cook += rand() % 2 + class;
  d.family += class;

  d.workC++;
  return 0;
}


int
pip_job_workD()
{
  /* ├────┼──────────────────────┤ */
  /* │農場    │+ 無                                        │ */
  /* │        │- 氣質                                      │ */
  /* ├────┼──────────────────────┤ */

  int class;

  class = d.hp * 100 / d.maxhp - d.tired;
  if ((class = pip_job_function(class, 6, 12, 41)) < 0)
    return 0;

  if (class >= 75)
  {
    d.money += 250 + (d.attack + d.resist) / 50;
    vmsg("牛羊長的好好喔..希望您再來幫忙..");
  }
  else if (class >= 50)
  {
    d.money += 210 + (d.attack + d.resist) / 55;
    vmsg("呵呵..還不錯喔..");
  }
  else if (class >= 25)
  {
    d.money += 160 + (d.attack + d.resist) / 60;
    vmsg("普普通通啦..可以更好的..");
  }
  else
  {
    d.money += 120 + (d.attack + d.resist) / 65;
    vmsg("您不太適合農場的工作..");
  }

  d.character -= rand() % 5;
  if (d.character < 0)
    d.character = 0;

  d.workD++;
  return 0;
}


int
pip_job_workE()
{
  /* ├────┼──────────────────────┤ */
  /* │餐廳    │+ 掃地洗衣 烹飪                             │ */
  /* │        │- 無                                        │ */
  /* ├────┼──────────────────────┤ */

  int class;

  class = d.cook / 6 - d.tired;
  if ((class = pip_job_function(class, 4, 9, 51)) < 0)
    return 0;

  if (class >= 75)
  {
    class = 4;
    d.money += 250 + (d.homework + d.cook) / 50;
    vmsg("客人都說太好吃了..再來一盤吧..");
  }
  else if (class >= 50)
  {
    class = 3;
    d.money += 200 + (d.homework + d.cook) / 55;
    vmsg("煮的還不錯吃唷..");
  }
  else if (class >= 25)
  {
    class = 2;
    d.money += 150 + (d.homework + d.cook) / 60;
    vmsg("普普通通啦..可以更好的..");
  }
  else
  {
    class = 1;
    d.money += 100 + (d.homework + d.cook) / 65;
    vmsg("廚藝待加強喔..");
  }

  d.homework += rand() % 2 + class;
  d.cook += rand() % 5 + class;

  d.workE++;
  return 0;
}


int
pip_job_workF()
{
  /* ├────┼──────────────────────┤ */
  /* │教堂    │+ 愛心 道德 信仰                            │ */
  /* │        │- 罪孽                                      │ */
  /* ├────┼──────────────────────┤ */

  int class;

  class = d.hp * 100 / d.maxhp - d.tired;
  if ((class = pip_job_function(class, 3, 6, 61)) < 0)
    return 0;

  if (class >= 75)
  {
    class = 4;
    d.money += 100 + (d.etchics + d.belief) / 50;
    vmsg("非常謝謝您喔..真是得力的助手");
  }
  else if (class >= 50)
  {
    class = 3;
    d.money += 75 + (d.etchics + d.belief) / 55;
    vmsg("謝謝您的熱心幫忙..");
  }
  else if (class >= 25)
  {
    class = 2;
    d.money += 50 + (d.etchics + d.belief) / 60;
    vmsg("真的很有愛心啦..不過有點小累的樣子..");
  }
  else
  {
    class = 1;
    d.money += 25 + (d.etchics + d.belief) / 65;
    vmsg("來奉獻不錯..但也不能打混ㄚ..");
  }

  d.love += rand() % 2 + class;
  d.etchics += rand() % 4 + class;
  d.belief += rand() % 4 + class;

  d.sin -= rand() % 9;
  if (d.sin < 0)
    d.sin = 0;

  d.workF++;
  return 0;
}


int
pip_job_workG()
{
  /* ├────┼──────────────────────┤ */
  /* │地攤    │+ 待人接物 魅力 談吐 速度                   │ */
  /* │        │- 無                                        │ */
  /* ├────┼──────────────────────┤ */

  int class;

  class = d.hp * 100 / d.maxhp - d.tired;
  if ((class = pip_job_function(class, 5, 10, 71)) < 0)
    return 0;

  d.money += 200 + (d.charm + d.speech) * class / 5000;
  vmsg("擺\地攤要躲警察啦..:p");

  d.toman += rand() % 2;
  d.charm += rand() % 2;
  d.speed += rand() % 2 + 1;
  d.speech += rand() % 2 + 1;

  d.workG++;
  return 0;
}


int
pip_job_workH()
{
  /* ├────┼──────────────────────┤ */
  /* │伐木場  │+ 攻擊力                                    │ */
  /* │        │- 氣質                                      │ */
  /* ├────┼──────────────────────┤ */

  int class;

  if (d.bbtime < 1800 * 1)
  {
    vmsg("小雞太小了，一歲以後再來吧..");
    return 0;
  }

  class = d.hp * 100 / d.maxhp - d.tired;
  if ((class = pip_job_function(class, 7, 14, 81)) < 0)
    return 0;

  if (class >= 75)
  {
    class = 4;
    d.money += 350 + (d.maxhp + d.attack) / 50;
    vmsg("您腕力很好唷..");
  }
  else if (class >= 50)
  {
    class = 3;
    d.money += 300 + (d.maxhp + d.attack) / 55;
    vmsg("砍了不少樹喔..");
  }
  else if (class >= 25)
  {
    class = 2;
    d.money += 250 + (d.maxhp + d.attack) / 60;
    vmsg("普普通通啦..可以更好的..");
  }
  else
  {
    class = 1;
    d.money += 200 + (d.maxhp + d.attack) / 65;
    vmsg("待加強喔..鍛鍊再來吧..");
  }

  d.attack += rand() % 2 + class;

  d.character -= rand() % 5;
  if (d.character < 0)
    d.character = 0;

  d.workH++;
  return 0;
}


int
pip_job_workI()
{
  /* ├────┼──────────────────────┤ */
  /* │美容院  │+ 藝術 感受                                 │ */
  /* │        │- 無                                        │ */
  /* ├────┼──────────────────────┤ */

  int class;

  if (d.bbtime < 1800 * 1)
  {
    vmsg("小雞太小了，一歲以後再來吧..");
    return 0;
  }

  class = d.art / 6 - d.tired;
  if ((class = pip_job_function(class, 5, 10, 91)) < 0)
    return 0;

  if (class >= 75)
  {
    class = 4;
    d.money += 400 + (d.art + d.affect) / 50;
    vmsg("客人都很喜歡讓您做造型唷..");
  }
  else if (class >= 50)
  {
    class = 3;
    d.money += 360 + (d.art + d.affect) / 55;
    vmsg("做的不錯喔..頗有天份..");
  }
  else if (class >= 25)
  {
    class = 2;
    d.money += 320 + (d.art + d.affect) / 60;
    vmsg("馬馬虎虎啦..再加油一點..");
  }
  else
  {
    class = 1;
    d.money += 250 + (d.art + d.affect) / 65;
    vmsg("待加強喔..以後再來吧..");
  }

  d.art += rand() % 3 + class;
  d.affect += rand() % 2 + class;

  d.workI++;
  return 0;
}


int
pip_job_workJ()
{
  /* ├────┼──────────────────────┤ */
  /* │狩獵區  │+ 攻擊力 速度                               │ */
  /* │        │- 氣質 愛心                                 │ */
  /* ├────┼──────────────────────┤ */

  int class;

  if (d.bbtime < 1800 * 2)
  {
    vmsg("小雞太小了，二歲以後再來吧..");
    return 0;
  }

  class = d.hp * 100 / d.maxhp - d.tired;
  if ((class = pip_job_function(class, 6, 13, 101)) < 0)
    return 0;

  if (class >= 75)
  {
    class = 4;
    d.money += 300 + (d.attack + d.speed) / 50;
    vmsg("您是完美的獵人..");
  }
  else if (class >= 50)
  {
    class = 3;
    d.money += 270 + (d.attack + d.speed) / 55;
    vmsg("收獲還不錯喔..可以飽餐\一頓了..");
  }
  else if (class >= 25)
  {
    class = 2;
    d.money += 240 + (d.attack + d.speed) / 60;
    vmsg("狩獵是體力與智力的結合..");
  }
  else
  {
    class = 1;
    d.money += 210 + (d.attack + d.speed) / 65;
    vmsg("技術差強人意..再加油喔..");
  }

  d.attack += rand() % 2 + class;
  d.speed += rand() % 2 + class;

  d.character -= rand() % 5;
  if (d.character < 0)
    d.character = 0;
  d.love -= rand() % 5;
  if (d.love < 0)
    d.love = 0;

  d.workJ++;
  return 0;
}


int
pip_job_workK()
{
  /* ├────┼──────────────────────┤ */
  /* │工地    │+ 防禦力                                    │ */
  /* │        │- 魅力                                      │ */
  /* ├────┼──────────────────────┤ */

  int class;

  if (d.bbtime < 1800 * 2)
  {
    vmsg("小雞太小了，二歲以後再來吧..");
    return 0;
  }

  class = d.hp * 100 / d.maxhp - d.tired;
  if ((class = pip_job_function(class, 7, 15, 111)) < 0)
    return 0;

  if (class >= 75)
  {
    class = 4;
    d.money += 250 + (d.maxhp + d.resist) / 50;
    vmsg("工程很完美..謝謝了..");
  }
  else if (class >= 50)
  {
    class = 3;
    d.money += 220 + (d.maxhp + d.resist) / 55;
    vmsg("工程尚稱順利..辛苦了..");
  }
  else if (class >= 25)
  {
    class = 2;
    d.money += 200 + (d.maxhp + d.resist) / 60;
    vmsg("工程差強人意..再加油喔..");
  }
  else
  {
    class = 1;
    d.money += 160 + (d.maxhp + d.resist) / 65;
    vmsg("ㄜ..待加強待加強..");
  }

  d.resist += rand() % 2 + class;

  d.charm -= rand() % 5;
  if (d.charm < 0)
    d.charm = 0;

  d.workK++;
  return 0;
}


int
pip_job_workL()
{
  /* ├────┼──────────────────────┤ */
  /* │墓園    │+ 勇敢 抗魔能力 感受                        │ */
  /* │        │- 魅力                                      │ */
  /* ├────┼──────────────────────┤ */

  int class;

  if (d.bbtime < 1800 * 3)
  {
    vmsg("小雞太小了，三歲以後再來吧..");
    return 0;
  }

  class = d.hp * 100 / d.maxhp - d.tired;
  if ((class = pip_job_function(class, 4, 8, 121)) < 0)
    return 0;

  if (class >= 75)
  {
    class = 4;
    d.money += 200 + (d.brave + d.affect) / 50;
    vmsg("守墓成功\喔..多謝了");
  }
  else if (class >= 50)
  {
    class = 3;
    d.money += 150 + (d.brave + d.affect) / 55;
    vmsg("守墓還算成功\喔..謝啦..");
  }
  else if (class >= 25)
  {
    class = 2;
    d.money += 120 + (d.brave + d.affect) / 60;
    vmsg("守墓還算差強人意喔..加油..");
  }
  else
  {
    class = 1;
    d.money += 80 + (d.brave + d.affect) / 65;
    vmsg("我也不方便說啥了..請再加油..");
  }

  d.brave += rand() % 4 + class;
  d.immune += rand() % 3 + class;
  d.affect += class;

  d.charm -= rand() % 5;
  if (d.charm < 0)
    d.charm = 0;

  d.workL++;
  return 0;
}


int
pip_job_workM()
{
  /* ├────┼──────────────────────┤ */
  /* │家庭教師│+ 智力 談吐                                 │ */
  /* │        │- 無                                        │ */
  /* ├────┼──────────────────────┤ */

  int class;

  if (d.bbtime < 1800 * 4)
  {
    vmsg("小雞太小了，四歲以後再來吧..");
    return 0;
  }

  class = d.hp * 100 / d.maxhp - d.tired;
  if ((class = pip_job_function(class, 3, 7, 131)) < 0)
    return 0;

  d.money += 50 + (d.wisdom + d.character) * class / 5000;
  vmsg("家教輕鬆..當然錢就少一點囉");

  d.wisdom += rand() % 2 + 3;
  d.speech += rand() % 2 + 1;

  d.workM++;
  return 0;
}


int
pip_job_workN()
{
  /* ├────┼──────────────────────┤ */
  /* │酒店    │+ 魅力 談吐 烹飪                            │ */
  /* │        │- 智力 社交評價                             │ */
  /* ├────┼──────────────────────┤ */

  int class;

  if (d.bbtime < 1800 * 5)
  {
    vmsg("小雞太小了，五歲以後再來吧..");
    return 0;
  }

  class = d.charm / 6 - d.tired;
  if ((class = pip_job_function(class, 5, 11, 141)) < 0)
    return 0;

  if (class >= 75)
  {
    class = 4;
    d.money += 500 + (d.charm + d.speech) / 50;
    vmsg("很紅唷..");
  }
  else if (class >= 50)
  {
    class = 3;
    d.money += 400 + (d.charm + d.speech) / 55;
    vmsg("蠻受歡迎的耶..");
  }
  else if (class >= 25)
  {
    class = 2;
    d.money += 300 + (d.charm + d.speech) / 60;
    vmsg("很平凡啦..但馬馬虎虎..");
  }
  else
  {
    class = 1;
    d.money += 200 + (d.charm + d.speech) / 65;
    vmsg("媚力不夠啦..請加油..");
  }

  d.charm += rand() % 3 + class;
  d.speech += rand() % 2 + class;
  d.cook += class;

  d.wisdom -= rand() % 5;
  if (d.wisdom < 0)
    d.wisdom = 0;
  d.social -= rand() % 5;
  if (d.social < 0)
    d.social = 0;

  d.workN++;
  return 0;
}


int
pip_job_workO()
{
  /* ├────┼──────────────────────┤ */
  /* │酒家    │+ 魅力 罪孽                                 │ */
  /* │        │- 待人接物 道德 親子關係 信仰 社交評價      │ */
  /* ├────┼──────────────────────┤ */

  int class;

  if (d.bbtime < 1800 * 5)
  {
    vmsg("小雞太小了，五歲以後再來吧..");
    return 0;
  }

  class = d.charm / 6 - d.tired;
  if ((class = pip_job_function(class, 6, 12, 151)) < 0)
    return 0;

  if (class >= 75)
  {
    class = 4;
    d.money += 600 + (d.charm + d.speech) / 50;
    vmsg("您是本店的紅牌唷..");
  }
  else if (class >= 50)
  {
    class = 3;
    d.money += 500 + (d.charm + d.speech) / 55;
    vmsg("您蠻受歡迎的耶..");
  }
  else if (class >= 25)
  {
    class = 2;
    d.money += 400 + (d.charm + d.speech) / 60;
    vmsg("很平凡..但馬馬虎虎啦..");
  }
  else
  {
    class = 1;
    d.money += 300 + (d.charm + d.speech) / 65;
    vmsg("唉..媚力不夠啦..");
  }

  d.charm += rand() % 4 + class;
  d.sin += rand() % 4 + class;

  d.toman -= rand() % 5;
  if (d.toman < 0)
    d.toman = 0;
  d.etchics -= rand() % 5;
  if (d.etchics < 0)
    d.etchics = 0;
  d.relation -= rand() % 5;
  if (d.relation < 0)
    d.relation = 0;
  d.belief -= rand() % 5;
  if (d.belief < 0)
    d.belief = 0;
  d.social -= rand() % 5;
  if (d.social < 0)
    d.social = 0;

  d.workO++;
  return 0;
}


int
pip_job_workP()
{
  /* ├────┼──────────────────────┤ */
  /* │夜總會  │+ 魅力 談吐 罪孽                            │ */
  /* │        │- 待人接物 氣質 道德 親子關係 信仰 社交評價 │ */
  /* ├────┼──────────────────────┤ */

  int class;

  if (d.bbtime < 1800 * 6)
  {
    vmsg("小雞太小了，六歲以後再來吧..");
    return 0;
  }

  class = (d.charm + d.art - d.belief) / 6 - d.tired;
  if ((class = pip_job_function(class, 6, 12, 161)) < 0)
    return 0;

  if (class >= 75)
  {
    class = 4;
    d.money += 1000 + (d.charm + d.speech) / 50;
    vmsg("您是本夜總會最閃亮的星星唷..");
  }
  else if (class >= 50)
  {
    class = 3;
    d.money += 800 + (d.charm + d.speech) / 55;
    vmsg("嗯嗯..您蠻受歡迎的耶..");
  }
  else if (class >= 25)
  {
    class = 2;
    d.money += 600 + (d.charm + d.speech) / 60;
    vmsg("要加油了啦..但普普啦..");
  }
  else
  {
    class = 1;
    d.money += 400 + (d.charm + d.speech) / 65;
    vmsg("唉..不行啦..");
  }

  d.charm += rand() % 5 + class;
  d.speech += rand() % 2 + class;
  d.sin += rand() % 6 + class;

  d.toman -= rand() % 5;
  if (d.toman < 0)
    d.toman = 0;
  d.character -= rand() % 5;
  if (d.character < 0)
    d.character = 0;
  d.etchics -= rand() % 5;
  if (d.etchics < 0)
    d.etchics = 0;
  d.relation -= rand() % 5;
  if (d.relation < 0)
    d.relation = 0;
  d.belief -= rand() % 5;
  if (d.belief < 0)
    d.belief = 0;
  d.social -= rand() % 5;
  if (d.social < 0)
    d.social = 0;

  d.workP++;
  return 0;
}
#endif		/* HAVE_GAME */
