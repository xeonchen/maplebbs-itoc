/*-------------------------------------------------------*/
/* pip_ending.c       ( NTHU CS MapleBBS Ver 3.10 )      */
/*-------------------------------------------------------*/
/* target : 結局函式                                     */
/* create :   /  /                                       */
/* update : 01/08/14                                     */
/* author : dsyan.bbs@forever.twbbs.org                  */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME

#include "pip.h"


/* ------------------------------------------------------- */
/* 結局參數設定                                            */
/* ------------------------------------------------------- */


struct endingset
{
  char *girl;			/* 女生結局的職業 */
  char *boy;			/* 男生結局的職業 */
  int grade;			/* 評分 */
};
typedef struct endingset endingset;


/* 萬能類 */
struct endingset endmodeallpurpose[] = 
{
  "女性職業",		"男生職業",		0, 
  "成為這個國家新女王",	"成為這個國家新國王",	500, 
  "成為國家的宰相",	"成為國家的宰相",	400, 
  "成為教會中的大主教",	"成為教會中的教宗",	350, 
  "成為國家的大臣",	"成為國家的大臣",	320, 
  "成為一位博士",	"成為一位博士",		300, 
  "成為教會中的修女",	"成為教會中的神父",	150, 
  "成為法庭上的法官",	"成為法庭上的法官",	200, 
  "成為知名的學者",	"成為知名的學者",	120, 
  "成為一名女官",	"成為一名男官",		100, 
  "在育幼院工作",	"在育幼院工作",		100, 
  "在旅館工作",		"在旅館工作",		100, 
  "在農場工作",		"在農場工作",		100, 
  "在餐\廳工作",	"在餐\廳工作",		100, 
  "在教堂工作",		"在教堂工作",		100, 
  "在地攤工作",		"在地攤工作",		100, 
  "在伐木場工作",	"在伐木場工作",		100, 
  "在美容院工作",	"在美容院工作",		100, 
  "在狩獵區工作",	"在狩獵區工作",		100, 
  "在工地工作",		"在工地工作",		100, 
  "在墓園工作",		"在墓園工作",		100, 
  "擔任家庭教師工作",	"擔任家庭教師工作",	100, 
  "在酒家工作",		"在酒家工作",		100, 
  "在酒店工作",		"在酒店工作",		100, 
  "在大夜總會工作",	"在大夜總會工作",	100, 
  "在家中幫忙",		"在家中幫忙",		50, 
  "在育幼院兼差",	"在育幼院兼差",		50, 
  "在旅館兼差",		"在旅館兼差",		50, 
  "在農場兼差",		"在農場兼差",		50, 
  "在餐\廳兼差",	"在餐\廳兼差",		50, 
  "在教堂兼差",		"在教堂兼差",		50, 
  "在地攤兼差",		"在地攤兼差",		50, 
  "在伐木場兼差",	"在伐木場兼差",		50, 
  "在美容院兼差",	"在美容院兼差",		50, 
  "在狩獵區兼差",	"在狩獵區兼差",		50, 
  "在工地兼差",		"在工地兼差",		50, 
  "在墓園兼差",		"在墓園兼差",		50, 
  "擔任家庭教師兼差",	"擔任家庭教師兼差",	50, 
  "在酒家兼差",		"在酒家兼差",		50, 
  "在酒店兼差",		"在酒店兼差",		50, 
  "在大夜總會兼差",	"在大夜總會兼差",	50, 
  NULL,			NULL,			0
};


/* 戰鬥類 */
struct endingset endmodecombat[] = 
{
  "女性職業",		"男生職業",		0, 
  "被封為勇者 戰士型",	"被封為勇者 戰士型",	420, 
  "被拔擢為一國的將軍",	"被拔擢為一國的將軍",	300, 
  "當上國家近衛隊隊長",	"當上國家近衛隊隊長",	200, 
  "當了武術老師",	"當了武術老師",		150, 
  "變成騎士報效國家",	"變成騎士報效國家",	160, 
  "投身軍旅成為士兵",	"投身軍旅成為士兵",	80, 
  "變成獎金獵人",	"變成獎金獵人",		10, 
  "以傭兵工作維生",	"以傭兵工作維生",	10, 
  NULL,			NULL,			0
};


/* 魔法類 */
struct endingset endmodemagic[] = 
{
  "女性職業",		"男生職業",		0, 
  "被封為勇者 魔法型",	"被封為勇者 魔法型",	420, 
  "被聘為王宮魔法師",	"被聘為王官魔法師",	280, 
  "當了魔法老師",	"當了魔法老師",		160, 
  "變成一位魔導士",	"變成一位魔導士",	180, 
  "當了魔法師",		"當了魔法師",		120, 
  "以占卜幫人算命為生",	"以占卜幫人算命為生",	40, 
  "成為一個魔術師",	"成為一個魔術師",	20, 
  "成為街頭藝人",	"成為街頭藝人",		10, 
  NULL,			NULL,			0
};


/* 社交類 */
struct endingset endmodesocial[] = 
{
  "女性職業",		"男生職業",		0, 
  "成為國王的寵妃",	"成為女王的駙馬爺",	170, 
  "被挑選成為王妃",	"被選中當女王的夫婿",	260, 
  "成為伯爵的夫人",	"成為了女伯爵的夫婿",	130, 
  "成為富豪的妻子",	"成為女富豪的夫婿",	100, 
  "成為商人的妻子",	"成為女商人的夫婿",	80, 
  "成為農人的妻子",	"成為女農人的夫婿",	80, 
  "成為地主的情婦",	"成為女地主的情夫",	-40, 
  NULL,			NULL,			0
};


/* 藝術類 */
struct endingset endmodeart[] = 
{
  "女性職業",		"男生職業",		0, 
  "成為了小丑",		"成為了小丑",		100, 
  "成為了作家",		"成為了作家",		100, 
  "成為了畫家",		"成為了畫家",		100, 
  "成為了舞蹈家",	"成為了舞蹈家",		100, 
  NULL,			NULL,			0
};


/* 暗黑類 */
struct endingset endmodeblack[] = 
{
  "女性職業",		"男生職業",		0,
  "變成了女魔王",	"變成了大魔王",		-1000,
  "混成了太妹",		"混成了流氓",		-350,
  "做了ＳＭ女王的工作",	"做了ＳＭ國王的工作",	-150,
  "當了黑街的大姐",	"當了黑街的老大",	-500,
  "變成高級娼婦",	"變成高級情夫",		-350,
  "變成詐欺師詐欺別人",	"變成金光黨騙別人錢",	-350,
  "以流鶯的工作生活",	"以牛郎的工作生活",	-350,
  NULL,			NULL,			0
};


/* 家事類 */
struct endingset endmodefamily[] = 
{
  "女性職業",		"男生職業",		0,
  "正在新娘修行",	"正在新郎修行",		50,
  "正在家裡閒晃",	"正在家裡閒晃",		10,
  NULL,			NULL,			0
};


/*-------------------------------------------------------*/
/* 結局函式              				 */
/*-------------------------------------------------------*/


/* 工作場所判斷 */
static int
pip_max_worktime()		/* workind: 在哪裡工作最多次 */
{
  int workind, times;		/* 幾次 */

  times = 20;		/* 若沒有超過 20 次的工作場所，則傳回 workind = 0 */
  workind = 0;

  if (d.workA > times)
  {
    times = d.workA;
    workind = 1;
  }
  if (d.workB > times)
  {
    times = d.workB;
    workind = 2;
  }
  if (d.workC > times)
  {
    times = d.workC;
    workind = 3;
  }
  if (d.workD > times)
  {
    times = d.workD;
    workind = 4;
  }
  if (d.workE > times)
  {
    times = d.workE;
    workind = 5;
  }
  if (d.workF > times)
  {
    times = d.workF;
    workind = 6;
  }
  if (d.workG > times)
  {
    times = d.workG;
    workind = 7;
  }
  if (d.workH > times)
  {
    times = d.workH;
    workind = 8;
  }
  if (d.workI > times)
  {
    times = d.workI;
    workind = 9;
  }
  if (d.workJ > times)
  {
    times = d.workJ;
    workind = 10;
  }
  if (d.workK > times)
  {
    times = d.workK;
    workind = 11;
  }
  if (d.workL > times)
  {
    times = d.workL;
    workind = 12;
  }
  if (d.workM > times)
  {
    times = d.workM;
    workind = 13;
  }
  if (d.workN > times)
  {
    times = d.workN;
    workind = 14;
  }
  if (d.workO > times)
  {
    times = d.workO;
    workind = 16;
  }
  if (d.workP > times)
  {
    times = d.workP;
    workind = 16;
  }

  return workind;
}


/* 結局判斷 */
static int		/* return 1:暗黑 2:藝術 3:萬能 4:戰士 5:魔法 6:社交 7:家事 */
pip_future_decide(modeallpurpose)
  int *modeallpurpose;	/* 如果是萬能結局，那麼還要 return 是哪一類的萬能 */
{
  *modeallpurpose = 0;	/* 預設 0 */

  /* 暗黑 */
  if ((d.etchics == 0 && d.sin >= 100) || (d.etchics > 0 && d.etchics < 50 && d.sin >= 250))
  {
    return 1;
  }

  /* 藝術 */
  if (d.art > d.hexp && d.art > d.mexp && d.art > d.hskill &&
    d.art > d.mskill && d.art > d.social && d.art > d.family &&
    d.art > d.homework && d.art > d.wisdom && d.art > d.charm &&
    d.art > d.belief && d.art > d.manners && d.art > d.speech &&
    d.art > d.cook && d.art > d.love)
  {
    return 2;
  }

  /* 戰鬥 */
  if (d.hexp >= d.social && d.hexp >= d.mexp && d.hexp >= d.family)
  {
    *modeallpurpose = 1;
    if (d.hexp > d.social + 50 || d.hexp > d.mexp + 50 || d.hexp > d.family + 50)
      return 4;
    return 3;
  }

  /* 魔法 */
  if (d.mexp >= d.hexp && d.mexp >= d.social && d.mexp >= d.family)
  {
    *modeallpurpose = 2;
    if (d.mexp > d.hexp || d.mexp > d.social || d.mexp > d.family)
      return 5;
    return 3;
  }

  /* 社交 */
  if (d.social >= d.hexp && d.social >= d.mexp && d.social >= d.family)
  {
    *modeallpurpose = 3;
    if (d.social > d.hexp + 50 || d.social > d.mexp + 50 || d.social > d.family + 50)
      return 6;
    return 3;
  }

  /* 家事 */
  *modeallpurpose = 4;
  if (d.family > d.hexp + 50 || d.family > d.mexp + 50 || d.family > d.social + 50)
    return 7;
  return 3;
}


/* 結婚的判斷 */
static int		/* return grade */
pip_marry_decide()
{
  if (d.lover)		/* 商人 */
  {
    /* d.lover = 3 4 5 6 7:商人 */
    return 80;
  }

  if (d.royalJ >= d.relation)
  {
    if (d.royalJ >= 100)
    {
      d.lover = 1;	/* 王子 */
      return 200;
    }
  }
  else
  {
    if (d.relation >= 100)
    {
      d.lover = 2;	/* 父親或母親 */
      return 0;
    }
  }

  /* d.lover = 0; */	/* 單身 */
  return 40;
}


static int
pip_endwith_black(buf)	/* 暗黑 */
  char *buf;
{
  int m;

  if (d.sin > 500 && d.mexp > 500)		/* 魔王 */
    m = 1;
  else if (d.hexp > 600)			/* 流氓 */
    m = 2;
  else if (d.speech > 100 && d.art >= 80)	/* SM */
    m = 3;
  else if (d.hexp > 320 && d.character > 200 && d.charm < 200)	/* 黑街老大 */
    m = 4;
  else if (d.character > 200 && d.charm > 200 && d.speech > 70 && d.toman > 70)	/* 高級娼婦 */
    m = 5;
  else if (d.wisdom > 450)			/* 詐騙師 */
    m = 6;
  else						/* 流鶯 */
    m = 7;

  if (d.sex == 1)
    strcpy(buf, endmodeblack[m].boy);
  else
    strcpy(buf, endmodeblack[m].girl);

  return endmodeblack[m].grade;
}


static int
pip_endwith_social(buf)	/* 社交 */
  char *buf;
{
  int m;

  if (d.social > 600)
    m = (d.charm > 500) ? 1 : 2;
  else if (d.social > 450)
    m = 1;
  else if (d.social > 380)
    m = (d.character > d.charm) ?  3 : 4;
  else if (d.social > 250)
    m = (d.wisdom > d.affect) ? 5 : 6;
  else
    m = 7;

  d.lover = 10;

  if (d.sex == 1)
    strcpy(buf, endmodesocial[m].boy);
  else
    strcpy(buf, endmodesocial[m].girl);

  return endmodesocial[m].grade;
}


static int
pip_endwith_magic(buf)	/* 魔法 */
  char *buf;
{
  int m;

  if (d.mexp > 800)
    m = (d.affect > d.wisdom && d.affect > d.belief && d.etchics > 100) ? 1 : 2;
  else if (d.mexp > 600)
    m = (d.speech >= 350) ? 3 : 4;
  else if (d.mexp > 500)
    m = 5;
  else if (d.mexp > 300)
    m = 6;
  else
    m = (d.character > 200) ? 7 : 8;

  if (d.sex == 1)
    strcpy(buf, endmodemagic[m].boy);
  else
    strcpy(buf, endmodemagic[m].girl);

  return endmodemagic[m].grade;
}


static int
pip_endwith_combat(buf)	/* 戰鬥 */
  char *buf;
{
  int m;

  if (d.hexp > 1500)
    m = (d.affect > d.wisdom && d.affect > d.belief && d.etchics > 100) ? 1 : 2;
  else if (d.hexp > 1000)
    m = (d.character > 300 && d.etchics > 50) ? 3 : 4;
  else if (d.hexp > 800)
    m = (d.vp > 500) ? 5 : 6;
  else
    m = (d.attack > 200) ? 7 : 8;

  if (d.sex == 1)
    strcpy(buf, endmodecombat[m].boy);
  else
    strcpy(buf, endmodecombat[m].girl);

  return endmodecombat[m].grade;
}


static int
pip_endwith_family(buf)	/* 家事 */
  char *buf;
{
  int m;

  if (d.relation < 50)
    m = 1;
  else
    m = 2;

  if (d.sex == 1)
    strcpy(buf, endmodefamily[m].boy);
  else
    strcpy(buf, endmodefamily[m].girl);

  return endmodefamily[m].grade;
}


static int
pip_endwith_allpurpose(buf, mode)	/* 萬能 */
  char *buf;
  int mode;
{
  int m;
  int point, workind;

  /* 依是哪一類的萬能，來決定 point 點數 */

  if (mode == 1)
    point = d.hexp;
  else if (mode == 2)
    point = d.mexp;
  else if (mode == 3)
    point = d.social;
  else if (mode == 4)
    point = d.family;
  else
    point = -1;

  if (point > 1000)
  {
    m = (d.character > 1000) ? 1 : 2;
  }
  else if (point > 800)
  {
    m = (d.belief > d.etchics && d.belief > d.wisdom) ? 3 :
      (d.etchics > d.belief && d.etchics > d.wisdom) ? 4 : 5;
  }
  else if (point > 500)
  {
    m = (d.belief > d.etchics && d.belief > d.wisdom) ? 6 :
      (d.etchics > d.belief && d.etchics > d.wisdom) ? 7 : 8;
  }
  else if (point > 300)
  {
    workind = pip_max_worktime();
    m = (workind < 2) ? 9 : 8 + workind;
  }
  else
  {
    workind = pip_max_worktime();
    m = (workind < 2) ? 25 : 24 + workind;
  }

  if (d.sex == 1)
    strcpy(buf, endmodeallpurpose[m].boy);
  else
    strcpy(buf, endmodeallpurpose[m].girl);

  return endmodeallpurpose[m].grade;
}


static int
pip_endwith_art(buf)		/* 藝術 */
  char *buf;
{
  int m;

  if (d.speech > 100)
    m = 1;
  else if (d.wisdom > 400)
    m = 2;
  else if (d.classI > d.classJ)
    m = 3;
  else
    m = 4;

  if (d.sex == 1)
    strcpy(buf, endmodeart[m].boy);
  else
    strcpy(buf, endmodeart[m].girl);

  return endmodeart[m].grade;
}


/* ------------------------------------------------------- */
/* 結局決定                                                */
/* ------------------------------------------------------- */

static void
pip_endwith_decide(endbuf1, endbuf2, endmode, endgrade)
  char *endbuf1, *endbuf2;
  int *endmode, *endgrade;
{
  char *name[8][2] = 
  {
    {"男的", "女的"},
    {"嫁給王子", "娶了公主"},
    {"嫁給您", "娶了您"},
    {"嫁給商人Ａ", "娶了女商人Ａ"},
    {"嫁給商人Ｂ", "娶了女商人Ｂ"},
    {"嫁給商人Ｃ", "娶了女商人Ｃ"},
    {"嫁給商人Ｄ", "娶了女商人Ｄ"},
    {"嫁給商人Ｅ", "娶了女商人Ｅ"}
  };

  int modeallpurpose;

  /* 處理 endbuf1 */
  *endmode = pip_future_decide(&modeallpurpose);
  switch (*endmode)
  {
  /* 1:暗黑 2:藝術 3:萬能 4:戰士 5:魔法 6:社交 7:家事 */
  case 1:
    *endgrade = pip_endwith_black(endbuf1);
    break;
  case 2:
    *endgrade = pip_endwith_art(endbuf1);
    break;
  case 3:
    *endgrade = pip_endwith_allpurpose(endbuf1, modeallpurpose);
    break;
  case 4:
    *endgrade = pip_endwith_combat(endbuf1);
    break;
  case 5:
    *endgrade = pip_endwith_magic(endbuf1);
    break;
  case 6:
    *endgrade = pip_endwith_social(endbuf1);
    break;
  case 7:
    *endgrade = pip_endwith_family(endbuf1);
    break;
  }

  *endgrade += pip_marry_decide();

  /* 處理 endbuf2 */
  if (d.lover >= 1 && d.lover <= 7)
  {
    if (d.sex == 1)
      strcpy(endbuf2, name[d.lover][1]);
    else
      strcpy(endbuf2, name[d.lover][0]);
  }
  else if (d.lover == 10)			/* 社交類的職業同時也是婚姻狀況 */
  {
    strcpy(endbuf2, endbuf1);
  }
  else /* if (d.lover == 0) */
  {
    if (d.sex == 1)
      strcpy(endbuf2, "娶了同行的女孩");
    else
      strcpy(endbuf2, "嫁給了同行的男生");
  }
}


static void
pip_ending_grade(endgrade)
  int endgrade;
{
  clrfromto(1, 23);
  move(8, 17);
  outs("\033[1;36m感謝您玩完整個" BBSNAME "小雞的遊戲\033[m");
  move(10, 17);
  outs("\033[1;37m經過系統計算的結果：\033[m");
  move(12, 17);
  prints("\033[1;36m您的小雞\033[37m%s\033[36m總得分＝\033[1;5;33m%d\033[m", d.name, endgrade);
}


int
pip_ending_screen()		/* 結局畫面 */
{
  char endbuf1[50], endbuf2[50];
  int endgrade = 0;
  int endmode = 0;

  pip_endwith_decide(endbuf1, endbuf2, &endmode, &endgrade);

  clear();
  move(1, 0);
  outs("        \033[1;33m╔═══╗╔══╮╗╔═══╮╔═══╗╔══╮╗╭═══╮\033[m\n");
  outs("        \033[1;37m║      ║║    ║║║      ║║      ║║    ║║║      ║\033[m\n");
  outs("        \033[1;33m║    ═╣║    ║║║  ╭╮║╚═╗╔╝║    ║║║  ╔═╗\033[m\n");
  outs("        \033[1;32m║    ═╣║  ║  ║║  ╰╯║╔═╝╚╗║  ║  ║║  ╰╯║\033[m\n");
  outs("        \033[1;37m║      ║║  ║  ║║      ║║      ║║  ║  ║║      ║\033[m\n");
  outs("        \033[1;35m╚═══╝╚═╰═╝╚═══╯╚═══╝╚═╰═╝╰═══╯\033[m\n");
  outs("        \033[1;31m──────────\033[41;37m " BBSNAME PIPNAME "結局報告\033[0;1;31m──────────\033[m\n");
  outs("        \033[1;36m  這個時間不知不覺地還是到臨了..\033[m\n");
  prints("        \033[1;37m  \033[33m%s\033[37m 得離開您的溫暖懷抱，自己一隻雞在外面求生存了..\033[m\n", d.name);
  outs("        \033[1;36m  在您照顧教導他的這段時光，讓他接觸了很多領域，培養了很多的能力..\033[m\n");
  prints("        \033[1;37m  因為這些，讓小雞 \033[33m%s\033[37m 之後的生活，變得更多采多姿了..\033[m\n", d.name);
  outs("        \033[1;36m  對於您的關心，您的付出，您所有的愛..\033[m\n");
  prints("        \033[1;37m  \033[33m%s\033[37m 會永遠都銘記在心的..\033[m", d.name);
  vmsg("接下來看未來發展");

  clrfromto(7, 19);
  outs("        \033[1;34m──────────\033[44;37m " BBSNAME PIPNAME "未來發展\033[0;1;34m──────────\033[m\n");
  prints("        \033[1;36m  透過水晶球，讓我們一起來看 \033[33m%s\033[36m 的未來發展吧..\033[m\n", d.name);
  prints("        \033[1;37m小雞 \033[33m%s\033[37m 後來%s..\033[m\n", d.name, endbuf1);
  prints("        \033[1;37m至於小雞的婚姻狀況，他後來%s，婚姻算是很美滿..\033[m\n", endbuf2);
  outs("        \033[1;36m嗯..這是一個不錯的結局唷..\033[m");
  vmsg("我想您一定很感動吧");

  show_ending_pic(0);
  vmsg("看一看分數囉");

  pip_ending_grade(endgrade);
  vmsg("下一頁是小雞資料，趕快 copy 下來做紀念");

  pip_query_self();
  vmsg("歡迎再來挑戰..");

  pipdie("\033[1;31m遊戲結束囉..\033[m  ", 3);
  return 0;
}


/* ------------------------------------------------------- */
/* 隨機機緣                                                */
/* ------------------------------------------------------- */


int				/* 1:接受求婚  0:拒絕求婚 */
pip_marriage_offer()		/* 求婚 */
{
  char buf[128];
  int money, who;
  char *name[5][2] = {{"女商人Ａ", "商人Ａ"}, {"女商人Ｂ", "商人Ｂ"}, {"女商人Ｃ", "商人Ｃ"}, {"女商人Ｄ", "商人Ｄ"}, {"女商人Ｅ", "商人Ｅ"}};

  do
  {
    who = rand() % 5;
  } while (d.lover == (who + 3));	/* 來求婚者要不是現在的未婚妻 */

  money = rand() % 5000 + 4000;
  sprintf(buf, " %s帶來了金錢 %d，要向您的小雞求婚，您願意嗎(Y/N)？[N] ", name[who][d.sex - 1], money);
  if (ians(b_lines - 1, 0, buf) == 'y')
  {
    if (d.wantend != 1 && d.wantend != 4)
    {
      sprintf(buf, " ㄚ～之前已經有婚約了，您確定要解除舊婚約，改立婚約嗎(Y/N)？[N] ");
      if (ians(b_lines - 1, 0, buf) != 'y')
      {
	d.social += 10;			/* 維持舊婚約加社交 */
	vmsg("還是維持舊婚約好了..");
	return 0;
      }
      d.social -= rand() % 50 + 100;	/* 片面毀棄婚約降社交 */
    }
    d.charm -= rand() % 5 + 20;
    d.lover = who + 3;
    d.relation -= 20;
    if (d.relation < 0)
      d.relation = 0;
    if (d.wantend < 4)
      d.wantend = 2;
    else
      d.wantend = 5;
    vmsg("我想對方是一個很好的伴侶..");
    d.money += money;
    return 1;
  }
  else
  {
    d.charm += rand() % 5 + 20;
    d.relation += 20;
    if (d.wantend == 1 || d.wantend == 4)
      vmsg("我還年輕..心情還不定..");
    else
      vmsg("我早已有婚約了..對不起..");
    return 0;
  }
}


int			/* 1:被占卜 0:放棄或沒錢 */
pip_meet_divine()	/* 占卜師來訪 */
{
  char buf[80];
  int money;

  clrfromto(6, 17);
  move(7, 14);
  outs("\033[1;33;5m叩叩叩..\033[0;1;37m突然傳來陣陣的敲門聲..\033[m");
  move(9, 14);
  outs("\033[1;37;46m    原來是雲遊四海的占卜師來訪了.......   \033[m");
  vmsg("開門讓他進來吧..");

  money = 300 * (d.bbtime / 60 / 30 + 1);	/* 年紀越大，要花的錢越多 */
  if (d.money >= money)
  {
    sprintf(buf, "您要花 %d 元占卜嗎(Y/N)？[N] ", money);
    if (ians(11, 14, buf) == 'y')
    {
      sprintf(buf, "您的小雞%s以後可能的身分是", d.name);
      switch (rand() % 4)	/* 隨便弄一種 */
      {
        /* 以下所 strcat 的 end message 是男女相同的 */
      case 0:
	strcat(buf, endmodemagic[2 + rand() % 5].girl);
	break;
      case 1:
	strcat(buf, endmodecombat[2 + rand() % 6].girl);
	break;
      case 2:
	strcat(buf, endmodeart[2 + rand() % 6].girl);
	break;
      case 3:
	strcat(buf, endmodeallpurpose[6 + rand() % 15].girl);	
	break;
      }
      d.money -= money;
      
      move(13, 14);
      outs("\033[1;33m在我占卜結果看來..\033[m");
      move(15, 14);
      outs(buf);
      vmsg("謝謝惠顧，有緣再見面了，不準不能怪我喔");
      return 1;
    }
    else
    {
      vmsg("您不想占卜啊？..真可惜..那只有等下次吧..");
    }
  }
  else
  {
    vmsg("您的錢不夠喔..真是可惜..等下次吧..");
  }
  return 0;
}


int
pip_meet_sysop()	/* itoc.000416: 遇上站長大大 */
{
  char msg[5][40] =
  {
    "突然水面震動了起來..",   "一陣香氣隨風而至..",
    "喟..有人拍拍您的肩膀..", "周圍突然熱鬧了起來..",
    "遠遠地看到一個人影.."
  };

  clrfromto(6, 17);
  move(7, 14);
  outs(msg[rand() % 5]);	/* 亂數決定出場敘述 */
  vmsg("原來是赫赫有名的" SYSOPNICK "出現了");
  move(9, 14);

  switch (rand() % 4)		/* 隨機決定站長事件 (以不同亂數增加趣味) */
  {
  case 0:
    if (d.weight > 50)
    {
      d.weight -= 20;
      outs("您太胖了，我為您進行抽脂手術..");
    } 
    else
    {
      d.weight += 20;
      outs("您太瘦了，我為您進行增肥手術..");
    }
    break;

  case 1:
    d.money += 1000;
    outs("站長今天心情好，送您一些雞金當好料的..");
    break;

  case 2:
    d.hp = d.maxhp;
    d.mp = d.maxmp;
    d.vp = d.maxvp;
    d.sp = d.maxsp;
    outs(SYSOPNICK "送您一顆千年烏雞丸，您的體力完全恢復..");
    break;

  case 3:
    if (d.money >= 100000)
    {
      if (ians(9, 14, "您要花 100000 元接受密宗貫頂大法嗎(Y/N)？[N] ") == 'y')
      {
	/* 屬性上升 5% */
	d.money -= 100000;
	d.maxhp = d.maxhp * 105 / 100;
	d.maxmp = d.maxmp * 105 / 100;
	d.maxsp = d.maxsp * 105 / 100;
	d.maxsp = d.maxsp * 105 / 100;
	d.attack = d.attack * 105 / 100;
	d.resist = d.resist * 105 / 100;
	d.speed = d.speed * 105 / 100;
	d.character = d.character * 105 / 100;
	d.love = d.love * 105 / 100;
	d.wisdom = d.wisdom * 105 / 100;
	d.art = d.art * 105 / 100;
	d.brave = d.brave * 105 / 100;
	d.homework = d.homework * 105 / 100;
        move(11, 14);
	outs("經過大神的貫頂之後，您發現所有能力都上昇了..");
      }
      else
      {
        move(11, 14);
	outs("您不想要啊？..真可惜，那只有等下次吧..");
      }
    }    
    else
    {
      outs("大神覺得和您沒有緣份..");
    }
    break;
  }
  
  vmsg("一轉眼，" SYSOPNICK "就不見了..");
  return 0;
}


int
pip_meet_smith()	/* itoc.021101: 遇上鐵匠 */
{
  int randnum;
  char *equip;

  clrfromto(6, 17);
  move(7, 14);
  outs("小伙子要去哪兒啊？");
  vmsg("有人在背後叫了您一聲，原來鐵匠先生");
  move(9, 14);
  outs("您用的是什麼爛裝備，讓我送您一件新的吧");

  randnum = rand();

  switch (randnum % 5)
  {
  case 0:			/* 頭部武器 */
    randnum = randnum % 10;
    d.weaponhead += randnum;
    pip_weapon_wear(0, randnum);
    equip = d.equiphead;
    if (!*equip)
      strcpy(equip, "鐵匠的燈泡");
    break;

  case 1:			/* 手部武器 */
    randnum = randnum % 10;
    d.weaponhand += randnum;
    pip_weapon_wear(1, randnum);
    equip = d.equiphand;
    if (!*equip)
      strcpy(equip, "鐵匠的拳套");
    break;

  case 2:			/* 盾牌武器 */
    randnum = randnum % 10;
    d.weaponshield += randnum;
    pip_weapon_wear(2, randnum);
    equip = d.equipshield;
    if (!*equip)
      strcpy(equip, "鐵匠的盾甲");
    break;

  case 3:			/* 身體武器 */
    randnum = randnum % 10;
    d.weaponbody += randnum;
    pip_weapon_wear(3, randnum);
    equip = d.equipbody;
    if (!*equip)
      strcpy(equip, "鐵匠的道袍");
    break;

  case 4:			/* 腳部武器 */
    randnum = randnum % 10;
    d.weaponfoot += randnum;
    pip_weapon_wear(4, randnum);
    equip = d.equipfoot;
    if (!*equip)
      strcpy(equip, "鐵匠的護脛");
    break;
  }

  while (!vget(b_lines, 0, "請為裝備取個新名字：", equip, 11, GCARRY))
    ;

  vmsg("鐵匠拍拍您的肩膀，淡淡離開");
  return 0;
}


int
pip_meet_angel()	/* itoc.010814: 遇到天使 */
{
  clear();
  show_system_pic(0);
  move(17, 10);
  prints("\033[1;36m親愛的\033[1;33m%s ～\033[m", d.name);
  move(18, 10);
  outs("\033[1;37m看到您這樣努力的培養自己的能力  讓我心中十分的高興喔..\033[m");
  move(19, 10);
  outs("\033[1;36m小天使我決定給您獎賞鼓勵鼓勵  偷偷地幫助您一下..^_^\033[m");
  move(20, 10);

  switch (rand() % 8)
  {
  case 1:
    outs("\033[1;33m我將幫您的各項能力全部提升百分之五喔..\033[m");
    d.maxhp = d.maxhp * 105 / 100;
    d.hp = d.maxhp;
    d.maxmp = d.maxmp * 105 / 100;
    d.mp = d.maxmp;
    d.maxvp = d.maxvp * 105 / 100;
    d.vp = d.maxvp;
    d.maxsp = d.maxsp * 105 / 100;
    d.sp = d.maxsp;
    d.attack = d.attack * 105 / 100;
    d.resist = d.resist * 105 / 100;
    d.speed = d.speed * 105 / 100;
    d.character = d.character * 105 / 100;
    d.love = d.love * 105 / 100;
    d.wisdom = d.wisdom * 105 / 100;
    d.art = d.art * 105 / 100;
    d.brave = d.brave * 105 / 100;
    d.homework = d.homework * 105 / 100;
    break;

  case 2:
  case 3:
    outs("\033[1;33m我將幫您的戰鬥能力全部提升百分之十喔..\033[m");
    d.attack = d.attack * 110 / 100;
    d.resist = d.resist * 110 / 100;
    d.speed = d.speed * 110 / 100;
    d.brave = d.brave * 110 / 100;
    break;

  case 4:
  case 5:
    outs("\033[1;33m我將幫您的生命、法力、移動、內力全部提升百分之八喔..\033[m");
    d.maxhp = d.maxhp * 108 / 100;
    d.hp = d.maxhp;
    d.maxmp = d.maxmp * 108 / 100;
    d.mp = d.maxmp;
    d.maxvp = d.maxvp * 108 / 100;
    d.vp = d.maxvp;
    d.maxsp = d.maxsp * 108 / 100;
    d.sp = d.maxsp;
    break;

  case 6:
  case 7:
    outs("\033[1;33m我將幫您的感受能力全部提升百分之二十喔..\033[m");
    d.character = d.character * 120 / 100;
    d.love = d.love * 120 / 100;
    d.wisdom = d.wisdom * 120 / 100;
    d.art = d.art * 120 / 100;
    d.homework = d.homework * 120 / 100;
    break;
  }

  vmsg("請繼續加油喔..");
  return 0;
}
#endif	/* HAVE_GAME */
