/*-------------------------------------------------------*/
/* pip_prac.c         ( NTHU CS MapleBBS Ver 3.10 )      */
/*-------------------------------------------------------*/
/* target : 修行選單                                     */
/* create :   /  /                                       */
/* update : 01/08/14                                     */
/* author : dsyan.bbs@forever.twbbs.org                  */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME

#include "pip.h"


/*-------------------------------------------------------*/
/* 修行選單:念書 練武 修行     				 */
/*-------------------------------------------------------*/

/*-------------------------------------------------------*/
/* 資料庫                      			 	 */
/*-------------------------------------------------------*/

static char *classrank[6] = {"沒有", "初級", "中級", "高級", "進階", "專業"};

static int classmoney[11][2] = 
{
  {0, 0}, {60, 110}, {70, 120}, {70, 120}, {80, 130}, {70, 120},
  {60, 110}, {90, 140}, {70, 120}, {70, 120}, {80, 130}
};

static int classvariable[11][4] = 
{
  {0, 0, 0, 0},
  {5, 5, 4, 4}, {5, 7, 6, 4}, {5, 7, 6, 4}, {5, 6, 5, 4}, {7, 5, 4, 6},
  {7, 5, 4, 6}, {6, 5, 4, 6}, {6, 6, 5, 4}, {5, 5, 4, 7}, {7, 5, 4, 7}
};

static char classword[11][5][41] = 	/* 限二十個中文字 */
{
  {"課名", "成功\一", "成功\二", "失敗一", "失敗二"},

  {"自然科學", "正在用功\讀書中..", "我是聰明滿分雞",
  "這題怎麼看不懂咧..怪了", "唸不完了 :~~~~~~"},

  {"唐詩宋詞", "床前明月光..疑是地上霜..", "紅豆生南國..春來發幾枝..",
  "ㄟ..上課不要流口水", "還混喔..快背唐詩三百首"},

  {"神學教育", "哈雷路亞  哈雷路亞", "讓我們迎接天堂之門",
  "ㄟ..在幹嘛ㄚ？還不好好唸", "神學很嚴肅的..請好好學..:("},

  {"軍學教育", "孫子兵法是中國兵法書..", "從軍報國，我要帶兵去打仗",
  "什麼陣形ㄚ？混亂陣形？ @_@", "連三國志都玩不好，還想打仗？"},

  {"劍道技術", "看我的厲害..", "我刺 我刺 我刺刺刺..",
  "劍要拿穩一點啦..", "在刺地鼠ㄚ？劍拿高一點"},

  {"格鬥戰技", "肌肉是肌肉  呼呼..", "十八銅人行氣散..",
  "腳再踢高一點啦..", "拳頭怎麼這麼沒力ㄚ.."},

  {"魔法教育", "我變 我變 我變變變..", "蛇膽＋蟋蜴尾＋鼠牙＋蟾蜍＝？？",
  "小心掃帚不要亂揮..", "ㄟ～口水不要流到水晶球上.."},

  {"禮儀教育", "要當隻有禮貌的雞..", "歐嗨唷..ㄚ哩ㄚ豆..",
  "怎麼學不會ㄚ？天呀..", "走起路來沒走樣..天ㄚ.."},

  {"繪畫技巧", "很不錯唷..有美術天份..", "這幅畫的顏色搭配的很好..",
  "不要鬼畫符啦..要加油..", "不要咬畫筆啦..壞壞小雞喔.."},

  {"舞蹈技巧", "美得就像一隻天鵝喔..", "舞蹈細胞很好喔..",
  "身體再柔軟一點..", "拜託不要這麼粗魯.."}
};


/*-------------------------------------------------------*/
/* 函式庫                                                */
/*-------------------------------------------------------*/


static int
pip_practice_gradeup(classnum, classgrade, newgrade)	/* 修行等級提升 */
  int classnum;		/* 課號 */
  int classgrade;	/* 年級 */
  int newgrade;		/* 新年級 */
{
  /* itoc.0108802: 為省計算，newgrade 從 0 開始算，classgrade 從 1 開始算 */
  if (newgrade >= classgrade && newgrade < 5)
  {
    char buf[80];
    sprintf(buf, "下次換上 [%8s%4s課程]", classword[classnum][0], classrank[newgrade + 1]);
    vmsg(buf);
  }
  return 0;
}


/* 傳入:課號 等級 生命 快樂 滿足 髒髒 傳回:變數12345   傳回: -1:放棄 0:失敗 1:成功 */
static int
pip_practice_function(classnum, classgrade, pic1, pic2, change1, change2, change3, change4, change5)
  int classnum;			/* 修行種類 */
  int classgrade;		/* 修行等級 */
  int pic1, pic2;		/* 圖檔 */
  int *change1;			/* 主要屬性增加 */
  int *change2;			/* 次要屬性增加 */
  int *change3;			/* 附加屬性增加 */
  int *change4;			/* 相剋屬性減少 */
  int *change5;			/* 相斥屬性減少 */
{
  int grade, success;
  char buf[80];

  /* itoc.010803: 檢查 classgrade，避免意外 */
  /* 因為還沒 update，learn_skill 可能 < 0 */
  if (LEARN_LEVEL < 0)
  {
    vmsg("您已經累到爆了");
    return -1;
  }

  /* itoc.010803: classgrade 應該只從 1~5 級 */
  if (classgrade < 0)
    grade = 1;
  else if (classgrade > 5)
    grade = 5;
  else
    grade = classgrade;

  /* 錢的算法 */
  success = grade * classmoney[classnum][0] + classmoney[classnum][1];	/* 借用 success */
  sprintf(buf, "  [%8s%4s課程]要花 %d元，確定要嗎(Y/N)？[Y] ", classword[classnum][0], classrank[grade], success);

  if (ians(b_lines - 2, 0, buf) == 'n')
    return -1;
  if (d.money < success)
  {
    vmsg("很抱歉，您的錢不夠喔");
    return -1;
  }
  count_tired(4, 5, 1, 100, 1);
  d.money -= success;

  /* 成功與否的判斷 */
  success = (d.hp / 2 + rand() % 20 > d.tired);		/* 1: 成功   0: 失敗 */

  d.hp -= rand() % 5 + classvariable[classnum][0];
  d.happy -= rand() % 5 + classvariable[classnum][1];
  d.satisfy -= rand() % 5 + classvariable[classnum][2];
  d.shit += rand() % 5 + classvariable[classnum][3];

  /* 加的點數成功是失敗的 1.5 倍，扣的點數失敗是成功的 1.5 倍 */
  /* learn_skill 可從 2%~100% */
  *change1 = (7 + 6 * (rand() % grade)) * 2 * LEARN_LEVEL / (3 - success);	/* 主要屬性加期望值 3*classgrade+4 (若修行成功且假設 learn_level = 100%) */
  *change2 = (5 + 4 * (rand() % grade)) * 2 * LEARN_LEVEL / (3 - success);	/* 次要屬性加期望值 2*classgrade+3 (若修行成功且假設 learn_level = 100%) */
  *change3 = (3 + 2 * (rand() % grade)) * 2 * LEARN_LEVEL / (3 - success);	/* 附加屬性加期望值 classgrade+2   (若修行成功且假設 learn_level = 100%) */

  *change4 = (5 + rand() % grade) * 2 / (1 + success);			/* 相剋屬性扣期望值 classgrade/2+4.5 (若修行成功) */
  *change5 = (5 + rand() % grade) * 2 / (2 + success);			/* 相斥屬性扣期望值 classgrade/3+3 (若修行成功) */

  /* 亂數選一個圖來秀 */
  if (rand() % 2)
    show_practice_pic(pic1);
  else
    show_practice_pic(pic2);

  vmsg(classword[classnum][3 - 2 * success + rand() % 2]);	/* 第一二個是成功訊息，三四個是失敗訊息 */
  return success;
}


/*-------------------------------------------------------*/
/* 修行選單:念書 練武 修行     				 */
/*-------------------------------------------------------*/


/* itoc.010802: 各類 classgrage 的界定是小雞的某項屬性 / 200，當然也可以加權處理 */

int
pip_practice_classA()
{
  /* ┌────┬─────────────────┐ */
  /* │自然科學│正屬性：智力、抗魔、從缺          │ */
  /* │        ├─────────────────┤ */
  /* │        │負屬性：信仰、從缺                │ */
  /* └────┴─────────────────┘ */

  int class;
  int change1, change2, change3, change4, change5;

  class = (d.wisdom * 3 + d.immune * 2) / 1000 + 1;	/* 科學 */

  if (pip_practice_function(1, class, 11, 12, &change1, &change2, &change3, &change4, &change5) < 0)
    return 0;

  d.wisdom += change1;
  d.immune += change2;
  d.belief -= change4;
  d.classA++;

  if (d.belief < 0)
    d.belief = 0;

  /* itoc.010802: 亂數學會究極法術 */
  if (rand() % 30 == 0)
    pip_learn_skill(-7);

  pip_practice_gradeup(1, class, (d.wisdom * 3 + d.immune * 2) / 1000);
  return 0;
}


int
pip_practice_classB()
{
  /* ┌────┬─────────────────┐ */
  /* │  詩詞  │正屬性：感受、氣質、藝術          │ */
  /* │        ├─────────────────┤ */
  /* │        │負屬性：從缺、抗魔                │ */
  /* └────┴─────────────────┘ */

  int class;
  int change1, change2, change3, change4, change5;

  class = (d.affect * 3 + d.character * 2 + d.art) / 1200 + 1;	/* 詩詞 */

  if (pip_practice_function(2, class, 21, 22, &change1, &change2, &change3, &change4, &change5) < 0)
    return 0;

  d.affect += change1;
  d.character += change2;
  d.art += change3;
  d.immune -= change5;
  d.classB++;

  if (d.immune < 0)
    d.immune = 0;

  /* itoc.010814: 亂數學會心法 */
  if (rand() % 10 == 0)
    pip_learn_skill(3);

  pip_practice_gradeup(2, class, (d.affect * 3 + d.character * 2 + d.art) / 1200);
  return 0;
}


int
pip_practice_classC()
{
  /* ┌────┬─────────────────┐ */
  /* │  神學  │正屬性：信仰、抗魔、智力          │ */
  /* │        ├─────────────────┤ */
  /* │        │負屬性：攻擊、從缺                │ */
  /* └────┴─────────────────┘ */
  
  int class;
  int change1, change2, change3, change4, change5;

  class = (d.belief * 3 + d.immune * 2 + d.wisdom) / 1200 + 1;	/* 神學 */

  if (pip_practice_function(3, class, 31, 32, &change1, &change2, &change3, &change4, &change5) < 0)
    return 0;

  d.belief += change1;
  d.immune += change2;
  d.wisdom += change3;
  d.attack -= change4;
  d.classC++;

  if (d.attack < 0)
    d.attack = 0;

  /* itoc.010802: 亂數學會治療法術 */
  if (rand() % 10 == 0)
    pip_learn_skill(-1);

  pip_practice_gradeup(3, class, (d.belief * 3 + d.immune * 2 + d.wisdom) / 1200);
  return 0;
}


int
pip_practice_classD()
{
  /* ┌────┬─────────────────┐ */
  /* │  軍學  │正屬性：戰鬥技術、智力、從缺      │ */
  /* │        ├─────────────────┤ */
  /* │        │負屬性：感受、從缺                │ */
  /* └────┴─────────────────┘ */

  int class;
  int change1, change2, change3, change4, change5;

  class = (d.hskill * 3 + d.wisdom * 2) / 1000 + 1;

  if (pip_practice_function(4, class, 41, 42, &change1, &change2, &change3, &change4, &change5) < 0)
    return 0;

  d.hskill += change1;
  d.wisdom += change2;
  d.affect -= change4;
  d.classD++;

  if (d.affect < 0)
    d.affect = 0;

  /* itoc.010814: 亂數學會護身 */
  if (rand() % 10 == 0)
    pip_learn_skill(1);

  pip_practice_gradeup(4, class, (d.hskill * 3 + d.wisdom * 2) / 1000);
  return 0;
}


int
pip_practice_classE()
{
  /* ┌────┬─────────────────┐ */
  /* │  劍術  │正屬性：攻擊、戰鬥技術、防禦      │ */
  /* │        ├─────────────────┤ */
  /* │        │負屬性：感受、從缺                │ */
  /* └────┴─────────────────┘ */

  int class;
  int change1, change2, change3, change4, change5;

  class = (d.attack * 3 + d.hskill * 2 + d.resist) / 1200 + 1;

  if (pip_practice_function(5, class, 51, 52, &change1, &change2, &change3, &change4, &change5) < 0)
    return 0;

  d.attack += change1;
  d.hskill += change2;
  d.resist += change3;
  d.affect -= change4;
  d.classE++;

  if (d.affect < 0)
    d.affect = 0;

  /* itoc.010802: 亂數學會劍法 */
  if (rand() % 10 == 0)
    pip_learn_skill(5);

  pip_practice_gradeup(5, class, (d.attack * 3 + d.hskill * 2 + d.resist) / 1200);
  return 0;
}


int
pip_practice_classF()
{
  /* ┌────┬─────────────────┐ */
  /* │  格鬥  │正屬性：防禦、速度、攻擊          │ */
  /* │        ├─────────────────┤ */
  /* │        │負屬性：感受、從缺                │ */
  /* └────┴─────────────────┘ */

  int class;
  int change1, change2, change3, change4, change5;

  class = (d.resist * 3 + d.speed * 2 + d.attack) / 1200 + 1;

  if (pip_practice_function(6, class, 61, 62, &change1, &change2, &change3, &change4, &change5) < 0)
    return 0;

  d.resist += change1;
  d.speed += change2;
  d.attack += change3;
  d.affect -= change4;
  d.classF++;

  if (d.affect < 0)
    d.affect = 0;

  /* itoc.010802: 亂數學會拳法 */
  if (rand() % 10 == 0)
    pip_learn_skill(4);

  pip_practice_gradeup(6, class, (d.resist * 3 + d.speed * 2 + d.attack) / 1200);
  return 0;
}


int
pip_practice_classG()
{
  /* ┌────┬─────────────────┐ */
  /* │  魔法  │正屬性：魔法技術、抗魔、從缺      │ */
  /* │        ├─────────────────┤ */
  /* │        │負屬性：攻擊、速度                │ */
  /* └────┴─────────────────┘ */

  int class;
  int change1, change2, change3, change4, change5;

  class = (d.mskill * 3 + d.immune * 2) / 1000 + 1;

  if (pip_practice_function(7, class, 71, 72, &change1, &change2, &change3, &change4, &change5) < 0)
    return 0;

  d.mskill += change1;
  d.immune += change2;
  d.attack -= change4;
  d.speed -= change5;
  d.classG++;

  if (d.attack < 0)
    d.attack = 0;
  if (d.speed < 0)
    d.speed = 0;

  /* itoc.010802: 亂數學會五系魔法之一 */
  if (rand() % 7 == 0)
    pip_learn_skill(- 2 - rand() % 5);

  pip_practice_gradeup(7, class, (d.mskill * 3 + d.immune * 2) / 1000);
  return 0;
}


int
pip_practice_classH()
{
  /* ┌────┬─────────────────┐ */
  /* │  禮儀  │正屬性：禮儀、氣質、談吐          │ */
  /* │        ├─────────────────┤ */
  /* │        │負屬性：速度、從缺                │ */
  /* └────┴─────────────────┘ */

  int class;
  int change1, change2, change3, change4, change5;

  class = (d.manners * 3 + d.character * 2 + d.speech) / 1200 + 1;

  if (pip_practice_function(8, class, 81, 82, &change1, &change2, &change3, &change4, &change5) < 0)
    return 0;

  d.manners += change1;
  d.character += change2;
  d.speech += change3;
  d.speed -= change4;
  d.classH++;

  if (d.speed < 0)
    d.speed = 0;

  pip_practice_gradeup(8, class, (d.manners * 3 + d.character * 2 + d.speech) / 1200);
  return 0;
}


int
pip_practice_classI()
{
  /* ┌────┬─────────────────┐ */
  /* │  繪畫  │正屬性：藝術、感受、從缺          │ */
  /* │        ├─────────────────┤ */
  /* │        │負屬性：從缺、從缺                │ */
  /* └────┴─────────────────┘ */

  int class;
  int change1, change2, change3, change4, change5;

  class = (d.art * 3 + d.character * 2) / 1000 + 1;

  if (pip_practice_function(9, class, 91, 92, &change1, &change2, &change3, &change4, &change5) < 0)
    return 0;

  d.art += change1;
  d.character += change2;
  d.classI++;

  /* itoc.010814: 亂數學會刀法 */
  if (rand() % 10 == 0)
    pip_learn_skill(6);

  pip_practice_gradeup(9, class, (d.art * 3 + d.character * 2) / 1000);
  return 0;
}


int
pip_practice_classJ()
{
  /* ┌────┬─────────────────┐ */
  /* │  舞蹈  │正屬性：藝術、魅力、氣質          │ */
  /* │        ├─────────────────┤ */
  /* │        │負屬性：攻擊、魔法技術            │ */
  /* └────┴─────────────────┘ */

  int class;
  int change1, change2, change3, change4, change5;

  class = (d.art * 3 + d.charm * 2 + d.character) / 1200 + 1;

  if (pip_practice_function(10, class, 101, 102, &change1, &change2, &change3, &change4, &change5) < 0)
    return 0;

  d.art += change1;
  d.charm += change2;
  d.character += change3;
  d.attack -= change4;
  d.mskill -= change5;
  d.classJ++;

  if (d.attack < 0)
    d.attack = 0;
  if (d.mskill < 0)
    d.mskill = 0;

  /* itoc.010802: 亂數學會輕功 */
  if (rand() % 10 == 0)
    pip_learn_skill(2);

  pip_practice_gradeup(10, class, (d.art * 3 + d.charm * 2 + d.character) / 1200);
  return 0;
}
#endif		/* HAVE_GAME */
