/* ----------------------------------------------------- */
/* pip_struct.h     ( NTHU CS MapleBBS Ver 3.10 )        */
/* ----------------------------------------------------- */
/* target : 小雞 data structure                          */
/* create :   /  /                                       */
/* update : 01/08/14                                     */
/* author : dsyan.bbs@@forever.twbbs.org                 */  
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/* ----------------------------------------------------- */


#ifndef	_PIP_STRUCT_H_
#define	_PIP_STRUCT_H_


/* ------------------------------------------------------- */
/* 小雞參數設定                   			   */
/* ------------------------------------------------------- */


struct CHICKEN
{
  /* ---姓名及生日--- */
  char name[IDLEN + 1];		/* 姓    名 */
  char birth[9];		/* 生    日 */

  /* ---小雞的時間--- */        
  time_t bbtime;		/* 玩小雞的總時間(秒) */
  				/* itoc.010804: 目前的設定是 30 分(即30*60秒) 為一歲 */

  /* 以下全是 integer */
  /* 每一種類型都保留至十欄以準備擴充 */
  /* 沒有註解的這些都是保留，沒有使用的欄位 */

  /* ---基本資料--- */
  int year;			/* 生日  年 */
  int month;			/* 生日  月 */
  int day;			/* 生日  日 */
  int sex;			/* 性    別 1:♂ 2:♀  */
  int death;			/* 狀    態 1:死亡 2:拋棄 3:結束 */
  int liveagain;		/* 復活次數 */
  int wantend;			/* 20歲結局 1:不要且未婚 2:不要且已婚  3:不要且當第三者 4:要且未婚  5:要且已婚 6:要且當第三者 */
  int lover;			/* 愛人     0:沒有 1:魔王 2:龍族 3:A 4:B 5:C 6:D 7:E */
  int seeroyalJ;		/* 是否可以遇上王子/公主  1:可以(王子已經回國了) 0:不能(王子還在邊疆) */
  int quest;			/* 任    務 0:無任務 !=0:任務編號 */

  /* ---狀態指數--- */
  /* itoc.010730: 這些指數在工作/學習/遊玩中改變 */
  int relation;			/* 親子關係 (人和寵物的互動關係) */
  int happy;			/* 快 樂 度 */
  int satisfy;			/* 滿 意 度 */
  int fallinlove;		/* 戀愛指數 */
  int belief;			/* 信    仰 */
  int sin;			/* 罪    孽 */
  int affect;			/* 感    受 */
  int state7;
  int state8;
  int state9;
  
  /* ---健康指數--- */
  /* itoc.010730: 這些指數在工作/學習/遊玩中改變 */
  int weight;			/* 體    重 */
  int tired;			/* 疲 勞 度 */
  int sick;			/* 病    氣 */
  int shit;			/* 清 潔 度 */
  int body4;
  int body5;
  int body6;
  int winn;			/* 猜拳贏的次數 */
  int losee;			/* 猜拳輸的次數 */
  int tiee;			/* 猜拳平手的次數 */

  /* ---評價參數--- */
  int social;			/* 社交評價 */
  int family;			/* 家事評價 */
  int hexp;			/* 戰鬥評價 */
  int mexp;			/* 魔法評價 */
  int value4;
  int value5;
  int value6;
  int value7;
  int value8;
  int value9;

  /* ---能力參數--- */
  /* itoc.010730: 這些指數在學習中大量改變，在工作中微量調整 */
  int toman;			/* 待人接物 */
  int character;		/* 氣 質 度 */
  int love;			/* 愛    心 */
  int wisdom;			/* 智    力 */
  int art;			/* 藝術能力 */
  int etchics;			/* 道    德 */
  int brave;			/* 勇    敢 */
  int homework;			/* 掃地洗衣 */
  int charm;			/* 魅    力 */
  int manners;			/* 禮    儀 */
  int speech;			/* 談    吐 */
  int cook;			/* 烹    飪 */
  int attack;			/* 攻 擊 力 */
  int resist;			/* 防 禦 力 */
  int speed;			/* 速    度 */
  int hskill;			/* 戰鬥技術 */
  int mskill;			/* 魔法技術 */
  int immune;			/* 抗魔能力 */
  int learn18;
  int learn19;

  /* ---戰鬥指標--- (隨升級而增加) */
  /* itoc.010730: 為了增加戰鬥的必要性，maxhp maxmp maxvp maxsp
     這些屬性應該只在 exp 增加升級後，才能大量增加 */
  /* itoc.010804: 目前容許在某些狀況下 maxhp 以 0~3 點的速度增加 */

  int level;			/* 等    級 */
  int exp;			/* 經 驗 值 */
  int hp;			/* Health Point 血 */
  int maxhp;			/* 最大血 */
  int mp;			/* Mana Point 法力 */
  int maxmp;			/* 最大法力 */
  int vp;			/* moVe Point 移動力 */
  int maxvp;			/* 最大移動力 */
  int sp;			/* Spirit Point 內力 */
  int maxsp;			/* 最大內力 */

  /* ---所學會技能--- */	/* bitwise operation */
  usint skillA;			/* 技能: 護身 */
  usint skillB;			/* 技能: 輕功 */
  usint skillC;			/* 技能: 心法 */
  usint skillD;			/* 技能: 拳法 */
  usint skillE;			/* 技能: 劍法 */
  usint skillF;			/* 技能: 刀法 */
  usint skillG;			/* 技能: 暗器、毒 */
  usint skill7;
  usint skill8;
  usint skillXYZ;		/* 特殊技能 */
  usint spellA;			/* 治療法術 */
  usint spellB;			/* 雷系法術 */
  usint spellC;			/* 冰系法術 */
  usint spellD;			/* 炎系法術 */
  usint spellE;			/* 土系法術 */
  usint spellF;			/* 風系法術 */
  usint spellG;			/* 究極法術 */
  usint spell7;
  usint spell8;
  usint spell9;

  /* ---武器的參數--- */
  int weaponhead;		/* 頭部武器 */
  int weaponhand;		/* 手部武器 */
  int weaponshield;		/* 盾牌武器 */
  int weaponbody;		/* 身體武器 */
  int weaponfoot;		/* 腳部武器 */
  int weapon5;
  int weapon6;
  int weapon7;
  int weapon8;
  int weapon9;

  /* ---吃的東西--- */
  int food;			/* 食    物 */
  int cookie;			/* 零    食 */  
  int eat2;
  int pill;			/* 大 還 丹 : 補血 */
  int medicine;			/* 靈    芝 : 補法力 */
  int burger;			/* 大 補 丸 : 補移動力 */
  int ginseng;			/* 千年人蔘 : 補內力 */
  int paste;			/* 黑玉斷續膏 : 血全滿 */
  int snowgrass;		/* 天山雪蓮 : 通通全滿 */
  int eat9;

  /* ---擁有的東西--- */
  int money;			/* 金    錢 */  
  int book;			/* 書    本 */
  int toy;			/* 玩    具 */  
  int playboy;			/* 課外讀物 */
  int thing4;
  int thing5;
  int thing6;
  int thing7;
  int thing8;
  int thing9;  

  /* ---參見王臣-- */
  int royalA;			/* 和 守衛 的好感 */
  int royalB;			/* 和 近衛 的好感 */
  int royalC;			/* 和 將軍 的好感 */
  int royalD;			/* 和 大臣 的好感 */
  int royalE;			/* 和 祭司 的好感 */
  int royalF;			/* 和 寵妃 的好感 */
  int royalG;			/* 和 王妃 的好感 */
  int royalH;			/* 和 國王 的好感 */
  int royalI;			/* 和 小丑 的好感 */
  int royalJ;			/* 和 王子/公主 的好感 */

  /* -------工作次數-------- */
  int workA;			/* 家事 */
  int workB;			/* 保姆 */
  int workC;			/* 旅店 */
  int workD;			/* 農場 */
  int workE;			/* 餐廳 */
  int workF;			/* 教堂 */
  int workG;			/* 地攤 */
  int workH;			/* 伐木 */
  int workI;			/* 美髮 */
  int workJ;			/* 獵人 */
  int workK;			/* 工地 */
  int workL;			/* 守墓 */
  int workM;			/* 家教 */
  int workN;			/* 酒家 */
  int workO;			/* 酒店 */
  int workP;			/* 夜總會 */
  int work16;
  int work17;
  int work18;
  int work19;

  /* -------上課次數-------- */
  int classA;			/* 自然科學 */
  int classB;			/* 唐詩宋詞 */
  int classC;			/* 神學教育 */
  int classD;			/* 軍學教育 */
  int classE;			/* 劍道技術 */
  int classF;			/* 格鬥戰技 */
  int classG;			/* 魔法教育 */
  int classH;			/* 禮儀教育 */
  int classI;			/* 繪畫技巧 */
  int classJ;			/* 舞蹈技巧 */
  int class10;
  int class11;
  int class12;
  int class13;
  int class14;
  int class15;
  int class16;
  int class17;
  int class18;
  int class19;

  /* ---武器的名稱--- */
  char equiphead[11];		/* 頭部武器名稱 */
  char equiphand[11];		/* 手部武器名稱 */
  char equipshield[11];		/* 盾牌武器名稱 */
  char equipbody[11];		/* 身體武器名稱 */
  char equipfoot[11];		/* 腳部武器名稱 */
  char equip5[11];
  char equip6[11];
  char equip7[11];
  char equip8[11];
  char equip9[11];
};
typedef struct CHICKEN CHICKEN;


/* ------------------------------------------------------- */
/* 物品參數設定                                            */
/* ------------------------------------------------------- */

struct itemset
{
  int num;			/* 編號 */
  char *name;			/* 名字 */
  char *msgbuy;			/* 功用 */
  char *msguse;			/* 說明 */
  int price;			/* 價格 */
};
typedef struct itemset itemset;


/* ------------------------------------------------------- */
/* 參見王臣參數設定                                        */
/* ------------------------------------------------------- */

struct royalset
{
  char *num;			/* 代碼 */
  char *name;			/* 王臣的名字 */
  int needmode;			/* 需要的mode *//* 0:不需要 1:禮儀 2:談吐 */
  int needvalue;		/* 需要的value */
  int addtoman;			/* 最大的增加量 */
  int maxtoman;			/* 庫存量 */
  char *words1;
  char *words2;
};
typedef struct royalset royalset;


/* ------------------------------------------------------- */
/* 技能參數設定                                            */
/* ------------------------------------------------------- */


#if 0		/* itoc.010729.說明 */

  smode 要對應 struce CHICKEN 的的 skill/spell
  smode = +1 為 skillA，smode = +2 為 skillB
  smode = -1 為 spellA，smode = -2 為 spellB

  sno 是此技能的編號，例如 0x01 是劍法甲，0x02 是劍法乙，0x04 是劍法丙 (注意是 bit operation)
  sbasic 則是此技能的基本技能，若要先學習劍法甲丙以後才能學習劍法丁，那麼劍法丁的 sbasic = 0x01 | 0x04 = 0x05

  needhp/nedmp/addtired 若是正的，就是扣血/扣法力/加疲勞

#endif


struct skillset
{
  int smode;			/* skill mode  0:特殊  >0:武功  <0:魔法 */
  usint sno;			/* skill number */
  usint sbasic;			/* basic skill */
  char name[13];		/* 技能的名字，限制六個中文字 */
  int needhp;			/* 生命力的改變 */
  int needmp;			/* 法力的改變 */
  int needvp;			/* 移動力的改變 */
  int needsp;			/* 內力的改變 */
  int addtired;			/* 疲勞值的改變 */
  int effect;			/* 效果/強弱 */
  int pic;			/* 圖檔 */
  char msg[41];			/* 使用技能的說明，限制20個中文字 */  
};
typedef struct skillset skillset;


/* ------------------------------------------------------- */
/* 選單的設定                                              */
/* ------------------------------------------------------- */

struct pipcommands
{
  int (*fptr) ();
  int key;
};
typedef struct pipcommands pipcommands;


/* ------------------------------------------------------- */
/* 怪物參數設定                                            */
/* ------------------------------------------------------- */

struct playrule
{
  /* itoc.010731: 怪物只有 hp 這種血，不需要 mp/vp/sp/tired，
    它的技能攻擊無限，但是怪物的攻擊模式是由亂數決定，如此可簡單化 */

  /* itoc.010731: 等級為 n 級的怪物，其期望
     maxhp = 0.75*(n^2)   (和玩家一樣)
     attack/spirit/magic/armor/dodge = 10*n
     money = 10*n
     exp = 5*n (原則上打20隻怪物升一級) */
               
  char name[13];		/* 名字，限制六個中文字 */
  int attribute;		/* 愛用的攻擊技能  0:無  >0:武功  <0:魔法 */
  int hp;			/* 血 */
  int maxhp;			/* 最大血 */

  /* 分別與怪物 物理/武功/魔法 攻擊小雞正相關 */
  int attack;			/* 物理攻擊能力 */
  int spirit;			/* 內力指數，技能攻擊能力 */
  int magic;			/* 魔法指數，法術攻擊能力 */

  /* 分別與小雞打怪物的 傷害/命中率 負相關 */
  int armor;			/* 防護指數，承受攻擊的能力 */
  int dodge;			/* 閃避指數，閃避攻擊的能力 */

  /* 打死怪物的獎勵 */
  int money;			/* 打死怪物得到的財寶 */
  int exp;			/* 打死怪物得到的經驗值 */

  int pic;			/* 圖檔 */
};
typedef struct playrule playrule;


/* ------------------------------------------------------- */
/* 武器參數設定                                            */
/* ------------------------------------------------------- */

struct weapon
{
  char name[11];		/* 名稱，限制五個中文字 */
  int quality;			/* 品質 */
  int cost;			/* 價格 */
};
typedef struct weapon weapon;


/* ------------------------------------------------------- */
/* PK 對戰參數設定                                         */
/* ------------------------------------------------------- */


#define	MAX_PIPPK_USER	10	/* 最多同時有 10 人在 PK 場中 */

struct PTMP
{
  char inuse;			/* 0:未使用 1:蓄勢待發 2:下挑戰書 -1:戰鬥中 */
  char done;			/* 0:未行動 1:已行動 */ 
  char name[IDLEN + 1];		/* 姓名 */
  char userid[IDLEN + 1];	/* 自己的 ID */
  char mateid[IDLEN + 1];	/* 對手的 ID */

  int sex;			/* 性別 */
  int level;			/* 等級 */

  int hp;			/* Health Point 血 */
  int maxhp;			/* 最大血 */
  int mp;			/* Mana Point 法力 */
  int maxmp;			/* 最大法力 */
  int vp;			/* moVe Point 移動力 */
  int maxvp;			/* 最大移動力 */
  int sp;			/* Spirit Point 內力 */
  int maxsp;			/* 最大內力 */

  int combat;			/* 物理身段: 決定「肉搏、防禦」的強度 */
  int magic;			/* 魔法造詣: 決定「法術-各系」的強度 */
  int speed;			/* 敏捷技巧: 決定「技能-護身、技能-輕功、技能-劍法」的強度 */
  int spirit;			/* 內力強度: 決定「技能-心法、技能-拳法、技能-刀法」的強度 */
  int charm;			/* 動感魅力: 決定「魅惑、召喚」的強度 */
  int oral;			/* 口若懸河: 決定「說服、煽動」的強度 */
  int cook;			/* 美味烹調: 決定「技能-暗器、法術-治療」 */
};
typedef struct PTMP PTMP;


struct PCACHE
{
  PTMP pslot[MAX_PIPPK_USER];	/* PTMP slots */
};
typedef struct PCACHE PCACHE;

#endif		/* _PIP_STRUCT_H_ */
