/*-------------------------------------------------------*/
/* util/transpip.c                   			 */
/*-------------------------------------------------------*/
/* target : WD 至 Maple 3.02 小雞資料轉換           	 */
/* create : 02/01/26                     		 */
/* update :   /  /                   			 */
/* author : itoc.bbs@bbs.ee.nctu.edu.tw          	 */
/*-------------------------------------------------------*/
/* syntax : transpip                     		 */
/*-------------------------------------------------------*/


#if 0

   1. 修改 transpip()
   2. 只轉 chicken，chicken.bak* 就不轉了

   ps. 使用前請先行備份，use on ur own risk. 程式拙劣請包涵 :p
   ps. 感謝 lkchu 的 Maple 3.02 for FreeBSD

#endif


#include "windtop.h"

#ifdef HAVE_GAME

#include "../../pip/pipstruct.h"	/* 引入新小雞的參數設定 */


/* ----------------------------------------------------- */
/* 舊小雞參數設定					 */
/* ----------------------------------------------------- */


struct chicken 
{
 /* ---基本的資料--- */
 char name[20];		/* 姓    名 */
 char birth[21];	/* 生    日 */
 int year;		/* 生日  年 */
 int month;		/* 生日  月 */
 int day; 		/* 生日  日 */
 int sex;		/* 性    別 1:♂   2:♀   */
 int death;             /* 1:  死亡 2:拋棄 3:結局 */
 int nodone;		/* 1:  未做 */
 int relation;		/* 兩人關係 */
 int liveagain;		/* 復活次數 */
 int dataB;
 int dataC;
 int dataD;
 int dataE;
  
 /* ---身體的參數--- */
 int hp;		/* 體    力 */
 int maxhp;             /* 最大體力 */
 int weight;            /* 體    重 */
 int tired;		/* 疲 勞 度 */
 int sick;		/* 病    氣 */
 int shit;		/* 清 潔 度 */ 
 int wrist;		/* 腕    力 */
 int bodyA;
 int bodyB;
 int bodyC;
 int bodyD;
 int bodyE;
 
 /* ---評價的參數--- */
 int social;		/* 社交評價 */
 int family;		/* 家事評價 */
 int hexp;		/* 戰鬥評價 */
 int mexp;		/* 魔法評價 */
 int tmpA;
 int tmpB;
 int tmpC;
 int tmpD;
 int tmpE;
 
 /* ---戰鬥用參數--- */
 int mp;		/* 法    力 */
 int maxmp;             /* 最大法力 */
 int attack;		/* 攻 擊 力 */
 int resist;		/* 防 禦 力 */
 int speed;		/* 速    度 */
 int hskill;		/* 戰鬥技術 */
 int mskill;		/* 魔法技術 */
 int mresist;		/* 抗魔能力 */
 int magicmode;		/* 魔法型態 */
 int fightB;
 int fightC;
 int fightD;
 int fightE;
 

 /* ---武器的參數--- */
 int weaponhead;	/* 頭部武器 */
 int weaponrhand;	/* 右手武器 */
 int weaponlhand;	/* 左手武器 */
 int weaponbody;	/* 身體武器 */
 int weaponfoot;	/* 腳的武器 */ 
 int weaponA;
 int weaponB;
 int weaponC;
 int weaponD;
 int weaponE;
 
 /* ---各能力參數--- */
 int toman;		/* 待人接物 */ 
 int character;		/* 氣 質 度 */ 
 int love;		/* 愛    心 */ 
 int wisdom;		/* 智    慧 */
 int art;		/* 藝術能力 */
 int etchics;		/* 道    德 */
 int brave;		/* 勇    敢 */
 int homework;		/* 掃地洗衣 */
 int charm;		/* 魅	力 */
 int manners;		/* 禮    儀 */
 int speech;		/* 談	吐 */
 int cookskill;		/* 烹    飪 */
 int learnA;
 int learnB;
 int learnC;
 int learnD;
 int learnE;
 
 
 /* ---各狀態數值--- */
 int happy;		/* 快 樂 度 */
 int satisfy;		/* 滿 意 度 */
 int fallinlove;	/* 戀愛指數 */
 int belief;		/* 信    仰 */
 int offense;		/* 罪    孽 */
 int affect;		/* 感    受 */
 int stateA;
 int stateB;
 int stateC;
 int stateD;
 int stateE;
 
 /* ---吃的東西啦--- */
 int food;		/* 食    物 */
 int medicine;          /* 靈    芝 */
 int bighp;             /* 大 補 丸 */
 int cookie;		/* 零    食 */
 int ginseng;		/* 千年人蔘 */
 int snowgrass;		/* 天山雪蓮 */
 int eatC;
 int eatD;
 int eatE;
 
 /* ---擁有的東西--- */
 int book;		/* 書    本 */
 int playtool; 		/* 玩    具 */
 int money;		/* 金    錢 */
 int thingA;		
 int thingB;		
 int thingC;
 int thingD;
 int thingE; 
 
 /* ---猜拳的參數--- */
 int winn;		
 int losee;
 
 /* ---參見王臣-- */
 int royalA;		/* from守衛 */
 int royalB;		/* from近衛 */
 int royalC;		/* from將軍 */
 int royalD;		/* from大臣 */
 int royalE;		/* from祭司 */ 		
 int royalF;		/* from寵妃 */
 int royalG;		/* from王妃 */
 int royalH;  		/* from國王 */
 int royalI;		/* from小丑 */
 int royalJ;		/* from王子 */
 int seeroyalJ;		/* 是否已經看過王子了 */
 int seeA;
 int seeB;
 int seeC;
 int seeD;
 int seeE;

 /* ---結局---- */
 int wantend;		/* 20歲結局 1:不要且未婚 2:不要且已婚  3:不要且當第三者 4:要且未婚  5:要且已婚 6:要且當第三者 */
 int lover;		/* 愛人 0:沒有 1:魔王 2:龍族 3:A 4:B 5:C 6:D 7:E  */ 
 
 /* -------工作次數-------- */
 int workA;		/* 家事 */
 int workB;		/* 保姆 */
 int workC;		/* 旅店 */
 int workD;		/* 農場 */
 int workE;		/* 餐廳 */
 int workF;		/* 教堂 */
 int workG;		/* 地攤 */
 int workH;		/* 伐木 */
 int workI;		/* 美髮 */
 int workJ;		/* 獵人 */
 int workK;		/* 工地 */
 int workL;		/* 守墓 */
 int workM;		/* 家教 */
 int workN;		/* 酒家 */
 int workO;		/* 酒店 */
 int workP;		/* 夜總會 */
 int workQ;
 int workR;
 int workS;
 int workT;
 int workU;
 int workV;
 int workW;
 int workX;
 int workY;
 int workZ;
 
 /* -------上課次數-------- */
 int classA;
 int classB;
 int classC;
 int classD;
 int classE;
 int classF;
 int classG;
 int classH; 
 int classI;
 int classJ;
 int classK;
 int classL;
 int classM;
 int classN;
 int classO;
 
 /* ---小雞的時間--- */
 time_t bbtime;
};
typedef struct chicken chicken;


/* ----------------------------------------------------- */
/* 轉換主程式                                            */
/* ----------------------------------------------------- */


static void
transpip(userid)
  char *userid;
{
  int fd;
  char fpath[64], buf[20];
  FILE *fp;
  struct chicken d;	/* 舊小雞 */
  struct CHICKEN p;	/* 新小雞 */
  

  usr_fpath(fpath, userid, "chicken");

  if (fp = fopen(fpath, "r"))
  {
    /* 讀出舊小雞資料 */

    fgets(buf, 20, fp);
    d.bbtime = (time_t) atoi(buf);

    fscanf(fp,
      "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "
      "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "
      "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "
      "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "
      "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "
      "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "
      "%d %s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "
      "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "
      "%d %d %d",
      &(d.year), &(d.month), &(d.day), &(d.sex), &(d.death), &(d.nodone), &(d.relation), &(d.liveagain), &(d.dataB), &(d.dataC), &(d.dataD), &(d.dataE),
      &(d.hp), &(d.maxhp), &(d.weight), &(d.tired), &(d.sick), &(d.shit), &(d.wrist), &(d.bodyA), &(d.bodyB), &(d.bodyC), &(d.bodyD), &(d.bodyE),
      &(d.social), &(d.family), &(d.hexp), &(d.mexp), &(d.tmpA), &(d.tmpB), &(d.tmpC), &(d.tmpD), &(d.tmpE),
      &(d.mp), &(d.maxmp), &(d.attack), &(d.resist), &(d.speed), &(d.hskill), &(d.mskill), &(d.mresist), &(d.magicmode), &(d.fightB), &(d.fightC), &(d.fightD), &(d.fightE),
      &(d.weaponhead), &(d.weaponrhand), &(d.weaponlhand), &(d.weaponbody), &(d.weaponfoot), &(d.weaponA), &(d.weaponB), &(d.weaponC), &(d.weaponD), &(d.weaponE),
      &(d.toman), &(d.character), &(d.love), &(d.wisdom), &(d.art), &(d.etchics), &(d.brave), &(d.homework), &(d.charm), &(d.manners), &(d.speech), &(d.cookskill), &(d.learnA), &(d.learnB), &(d.learnC), &(d.learnD), &(d.learnE),
      &(d.happy), &(d.satisfy), &(d.fallinlove), &(d.belief), &(d.offense), &(d.affect), &(d.stateA), &(d.stateB), &(d.stateC), &(d.stateD), &(d.stateE),
      &(d.food), &(d.medicine), &(d.bighp), &(d.cookie), &(d.ginseng), &(d.snowgrass), &(d.eatC), &(d.eatD), &(d.eatE),
      &(d.book), &(d.playtool), &(d.money), &(d.thingA), &(d.thingB), &(d.thingC), &(d.thingD), &(d.thingE),
      &(d.winn), &(d.losee),
      &(d.royalA), &(d.royalB), &(d.royalC), &(d.royalD), &(d.royalE), &(d.royalF), &(d.royalG), &(d.royalH), &(d.royalI), &(d.royalJ), &(d.seeroyalJ), &(d.seeA), &(d.seeB), &(d.seeC), &(d.seeD), &(d.seeE),
      &(d.wantend), &(d.lover), d.name,
      &(d.classA), &(d.classB), &(d.classC), &(d.classD), &(d.classE),
      &(d.classF), &(d.classG), &(d.classH), &(d.classI), &(d.classJ),
      &(d.classK), &(d.classL), &(d.classM), &(d.classN), &(d.classO),
      &(d.workA), &(d.workB), &(d.workC), &(d.workD), &(d.workE),
      &(d.workF), &(d.workG), &(d.workH), &(d.workI), &(d.workJ),
      &(d.workK), &(d.workL), &(d.workM), &(d.workN), &(d.workO),
      &(d.workP), &(d.workQ), &(d.workR), &(d.workS), &(d.workT),
      &(d.workU), &(d.workV), &(d.workW), &(d.workX), &(d.workY), &(d.workZ));

    fclose(fp);

    /* 轉換小雞資料 */

    memset(&p, 0, sizeof(p));

    str_ncpy(p.name, d.name, sizeof(p.name));
    sprintf(p.birth, "%02d/%02d/%02d", d.year % 100, d.month, d.day);

    p.bbtime = d.bbtime;

    p.year = d.year;
    p.month = d.month;
    p.day = d.day;
    p.sex = d.sex;
    p.death = d.death;
    p.liveagain = d.liveagain;
    p.wantend = d.wantend;
    p.lover = d.lover;
    p.seeroyalJ = d.seeroyalJ;
    p.quest = 0;		/* 舊電子雞沒有任務 */

    p.relation = d.relation;
    p.happy = d.happy;
    p.satisfy = p.satisfy;
    p.fallinlove = d.fallinlove;
    p.belief = d.belief;
    p.sin = d.offense;
    p.affect = d.affect;

    p.weight = d.weight;
    p.tired = d.tired;
    p.sick = d.sick;
    p.shit = d.shit;

    p.social = d.social;
    p.family = d.family;
    p.hexp = d.hexp;
    p.mexp = d.mexp;

    p.toman = d.toman;
    p.character = d.character;
    p.love = d.love;
    p.wisdom = d.wisdom;
    p.art = d.art;
    p.etchics = d.etchics;
    p.brave = d.brave;
    p.homework = d.homework;
    p.charm = d.charm;
    p.manners = d.manners;
    p.speech = d.speech;
    p.cook = d.cookskill;
    p.attack = d.attack;
    p.resist = d.resist;
    p.speed = d.speed;
    p.hskill = d.hskill;
    p.mskill = d.mskill;
    p.immune = d.mresist;

    p.level = 1;		/* 從 1 級開始 */
    p.exp = 0;
    p.hp = d.hp;
    p.maxhp = d.maxhp;
    p.mp = d.mp;
    p.maxmp = d.maxmp;
    p.vp = d.hp;		/* 舊電子雞沒有 vp/sp 拿 hp/mp 來套 */
    p.maxvp = d.maxhp;
    p.sp = d.mp;
    p.maxsp = d.maxmp;

    /* 舊電子雞沒有技能，預設為 0 */

    /* 武器通通重置為 0，以免武器列表不同 */

    p.food = d.food;
    p.cookie = d.cookie;
    p.pill = 0;
    p.medicine = d.medicine;
    p.burger = d.bighp;
    p.ginseng = d.ginseng;
    p.paste = 0;
    p.snowgrass = d.snowgrass;

    p.money = d.money;
    p.book = d.book;
    p.toy = d.playtool;
    p.playboy = 0;

    p.royalA = d.royalA;
    p.royalB = d.royalB;
    p.royalC = d.royalC;
    p.royalD = d.royalD;
    p.royalE = d.royalE;
    p.royalF = d.royalF;
    p.royalG = d.royalG;
    p.royalH = d.royalH;
    p.royalI = d.royalI;
    p.royalJ = d.royalJ;

    p.workA = d.workA;
    p.workB = d.workB;
    p.workC = d.workC;
    p.workD = d.workD;
    p.workE = d.workE;
    p.workF = d.workF;
    p.workG = d.workG;
    p.workH = d.workH;
    p.workI = d.workI;
    p.workJ = d.workJ;
    p.workK = d.workK;
    p.workL = d.workL;
    p.workM = d.workM;
    p.workN = d.workN;
    p.workO = d.workO;
    p.workP = d.workP;

    p.winn = d.winn;
    p.losee = d.losee;
    p.classA = d.classA;
    p.classB = d.classB;
    p.classC = d.classC;
    p.classD = d.classD;
    p.classE = d.classE;
    p.classF = d.classF;
    p.classG = d.classG;
    p.classH = d.classH;
    p.classI = d.classI;
    p.classJ = d.classJ;    
    
    /* 寫入新小雞資料 */

    unlink(fpath);		/* 重建個新的 */
    fd = open(fpath, O_WRONLY | O_CREAT, 0600);
    write(fd, &p, sizeof(CHICKEN));
    close(fd);    
  }
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  char c;
  char buf[64];
  struct dirent *de;
  DIR *dirp;

  /* argc == 1 轉全部使用者 */
  /* argc == 2 轉某特定使用者 */

  if (argc > 2)
  {
    printf("Usage: %s [target_user]\n", argv[0]);
    exit(-1);
  }

  chdir(BBSHOME);

  if (argc == 2)
  {
    transpip(argv[1]);
    exit(1);
  }

  /* 轉換使用者小雞資料 */
  for (c = 'a'; c <= 'z'; c++)
  {
    sprintf(buf, "usr/%c", c);

    if (!(dirp = opendir(buf)))
      continue;

    while (de = readdir(dirp))
    {
      char *str;

      str = de->d_name;
      if (*str <= ' ' || *str == '.')
	continue;

      transpip(str);
    }

    closedir(dirp);
  }
  return 0;
}
#else
int
main()
{
  printf("You should define HAVE_GAME first.\n");
  return -1;
}
#endif	/* HAVE_GAME */
