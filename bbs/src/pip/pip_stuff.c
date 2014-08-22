/*-------------------------------------------------------*/
/* pip_stuff.c         ( NTHU CS MapleBBS Ver 3.10 )     */
/*-------------------------------------------------------*/
/* target : 升級、系統、特殊選單等雜七雜八函式           */
/* create :   /  /                                       */
/* update : 01/08/14                                     */
/* author : dsyan.bbs@forever.twbbs.org                  */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME

#include "pip.h"


/*-------------------------------------------------------*/
/* 資料存取						 */
/*-------------------------------------------------------*/


void
pip_write_file()		/* 遊戲寫資料入檔案 */
{
  int fd;
  char fpath[64];

  usr_fpath(fpath, cuser.userid, fn_pip);
  fd = open(fpath, O_WRONLY | O_CREAT, 0600);	/* fpath 不必已經存在 */
  write(fd, &d, sizeof(CHICKEN));
  close(fd);
}


int		/* >=0:成功  <0:失敗 */
pip_read_file(userid, p)	/* 遊戲讀資料出檔案 */
  char *userid;
  struct CHICKEN *p;
{
  int fd;
  char fpath[64];

  usr_fpath(fpath, userid, fn_pip);
  fd = open(fpath, O_RDONLY);		/* fpath 必須已經存在 */
  if (fd >= 0)
  {
    read(fd, p, sizeof(CHICKEN));
    close(fd);
  }
  return fd;
}


int			/* 1: 成功寫入  0: 放棄 */
pip_write_backup()	/* 小雞進度備份 */
{
  char *files[4] = {"沒有", "進度一", "進度二", "進度三"};
  char buf[80], fpath[64];
  int ch;

  show_basic_pic(101);

  ch = ians(b_lines - 2, 0, "儲存 [1] 進度一 [2] 進度二 [3] 進度三 [Q] 放棄：[Q] ") - '0';

  if (ch < 1 || ch > 3)
  {
    vmsg("放棄儲存遊戲備份");
    return 0;
  }

  sprintf(buf, "確定要儲存於 [%s] 檔案嗎(Y/N)？[N] ", files[ch]);
  if (ians(b_lines - 2, 0, buf) != 'y')
  {
    vmsg("放棄儲存檔案");
    return 0;
  }

  sprintf(buf, "儲存 [%s] 檔案完成了", files[ch]);
  vmsg(buf);

  sprintf(buf, "%s.bak%d", fn_pip, ch);
  usr_fpath(fpath, cuser.userid, buf);
  ch = open(fpath, O_WRONLY | O_CREAT, 0600);	/* fpath 不必已經存在 */
  write(ch, &d, sizeof(CHICKEN));
  close(ch);

  return 1;
}


int			/* 1: 成功讀出  0: 放棄 */
pip_read_backup()	/* 小雞備份讀取 */
{
  char *files[4] = {"沒有", "進度一", "進度二", "進度三"};
  char buf[80], fpath[64];
  int ch, fd;

  show_basic_pic(102);

  ch = ians(b_lines - 2, 0, "讀取 [1] 進度一 [2] 進度二 [3] 進度三 [Q] 放棄：[Q] ") - '0';

  if (ch < 1 || ch > 3)
  {
    vmsg("放棄讀取遊戲備份");
    return 0;
  }

  sprintf(buf, "%s.bak%d", fn_pip, ch);
  usr_fpath(fpath, cuser.userid, buf);

  fd = open(fpath, O_RDONLY);		/* fpath 必須已經存在 */
  if (fd >= 0)
  {
    sprintf(buf, "確定要讀取於 [%s] 檔案嗎(Y/N)？[N] ", files[ch]);
    if (ians(b_lines - 2, 0, buf) == 'y')
    {
      read(fd, &d, sizeof(CHICKEN));
      close(fd);
      sprintf(buf, "讀取 [%s] 檔案完成了", files[ch]);
      vmsg(buf);
      return 1;
    }
    vmsg("放棄讀取檔案");
    close(fd);
    return 0;
  }
  else
  {
    sprintf(buf, "檔案 [%s] 不存在", files[ch]);
    vmsg(buf);
    return 0;
  }
}


/*-------------------------------------------------------*/
/* 小雞狀態函式					 	 */
/*-------------------------------------------------------*/


void
pipdie(msg, diemode)	/* 小雞死亡 */
  char *msg;
  int diemode;
{
  vs_head("電子養小雞", str_site);

  if (diemode == 1)
  {
    show_die_pic(1);
    vmsg("死神來帶走小雞了");
    vs_head("電子養小雞", str_site);
    show_die_pic(2);
    move(14, 20);
    prints("可憐的小雞\033[1;31m%s\033[m", msg);
    vmsg(BBSNAME "哀悼中....");
  }
  else if (diemode == 2)
  {
    show_die_pic(3);
    vmsg("嗚嗚嗚..我被丟棄了.....");
  }
  else if (diemode == 3)
  {
    show_die_pic(0);
    vmsg("遊戲結束囉..");
  }

  d.death = diemode;
  pip_write_file();
}


int
count_tired(prob, base, mode, mul, cal)		/* itoc.010803: 依照傳入的引數來增減疲勞度 */
  int prob;				/* 機率 */
  int base;				/* 底數 */
  int mode;				/* 類型 1:和年齡有關  0:和年齡無關  */
  int mul;				/* 加權 (以 % 來計 100->1) */
  int cal;				/* 1:加疲勞  0:減疲勞 */
{
  int tiredvary;       /* 改變值 */

  /* 先算改變量 */  
  tiredvary = rand() % prob + base;  

  if (mode)            /* 和年齡有關 */
  {
    int tm;            /* 年齡 */
    tm = d.bbtime / 60 / 30;

    /* itoc.010803: 年紀越小，加疲勞比較少，恢復疲勞也比較快 */
    /* 注意不能寫成 tiredvary *= 6 / 5; 喔 :p */

    if (tm <= 3)       /* 0~3 歲 */
    {
      tiredvary = cal ? tiredvary * 16 / 15 : tiredvary * 6 / 5;
    }
    else if (tm <= 7)  /* 4~7 歲 */
    {
      tiredvary = cal ? tiredvary * 11 / 10 : tiredvary * 8 / 7;
    }
    else if (tm <= 10) /* 8~10 歲 */
    {
      tiredvary = cal ? tiredvary * 8 / 7 : tiredvary * 11 / 10;
    }
    else               /* 11 歲以上 */
    {
      tiredvary = cal ? tiredvary * 6 / 5 : tiredvary * 16 / 15;
    }
  }

  /* 再算加權 */
  if (cal)
  {
    d.tired += tiredvary * mul / 100;
    if (d.tired > 100)
      d.tired = 100;
  }
  else
  {
    d.tired -= tiredvary;      /* 扣值不再加權了 */
    if (d.tired < 0)
      d.tired = 0;
  }
}


/*-------------------------------------------------------*/
/* 特殊選單:看病 減肥 				         */
/*-------------------------------------------------------*/


int				/* 1:看完醫生  0:沒病來醫院惡搞 */
pip_see_doctor()		/* 看醫生 */
{
  char buf[256];
  int savemoney;
  savemoney = d.sick * 25;
  if (d.sick <= 0)
  {
    vmsg("哇哩..沒病來醫院幹嘛..被罵了..嗚~~");
    d.character -= rand() % 3 + 1;
    if (d.character < 0)
      d.character = 0;
    d.happy -= (rand() % 3 + 3);
    d.satisfy -= rand() % 3 + 2;
  }
  else if (d.money < savemoney)
  {
    sprintf(buf, "您的病要花 %d 元喔....您不夠錢啦...", savemoney);
    vmsg(buf);
  }
  else
  {
    d.tired -= rand() % 10 + 20;
    if (d.tired < 0)
      d.tired = 0;
    d.sick = 0;
    d.money = d.money - savemoney;
    move(4, 0);
    show_special_pic(1);
    vmsg("藥到病除..沒有副作用!!");
    return 1;
  }
  return 0;
}


int				/* 1:整容  0:沒有整容 */
pip_change_weight()		/* 增胖/減肥 */
{
  char buf[80];
  int weightmp;

  show_special_pic(2);

  out_cmd("", COLOR1 " 美容 " COLOR2 " [1]傳統增胖 [2]快速增胖 [3]傳統減肥 [4]快速減肥 [Q]跳出                \033[m");

  switch (vkey())
  {
  case '1':
    if (d.money < 80)
    {
      vmsg("傳統增胖要80元喔....您不夠錢啦...");
    }
    else
    {
      if (ians(b_lines - 1, 0, " 需花費 80 元（3～5公斤），確定嗎(Y/N)？[N] ") == 'Y')
      {
	weightmp = 3 + rand() % 3;
	d.weight += weightmp;
	d.money -= 80;
	d.hp -= rand() % 2 + 3;
	show_special_pic(3);
	sprintf(buf, "總共增加了 %d 公斤", weightmp);
	vmsg(buf);
	return 1;
      }
      else
      {
	vmsg("回心轉意囉.....");
      }
    }
    break;

  case '2':
    vget(b_lines - 1, 0, " 增一公斤要 30 元，您要增多少公斤呢？[請填數字]：", buf, 4, DOECHO);
    weightmp = atoi(buf);
    if (weightmp <= 0)
    {
      vmsg("輸入有誤..放棄囉...");
    }
    else if (d.money > (weightmp * 30))
    {
      sprintf(buf, " 增加 %d 公斤，總共需花費了 %d 元，確定嗎(Y/N)？[N] ", weightmp, weightmp * 30);      
      if (ians(b_lines - 1, 0, buf) == 'y')
      {
	d.money -= weightmp * 30;
	d.weight += weightmp;
	count_tired(5, 8, 0, 100, 1);
	d.hp -= (rand() % 2 + 3);
	d.sick += rand() % 10 + 5;
	show_special_pic(3);
	sprintf(buf, "總共增加了 %d 公斤", weightmp);
	vmsg(buf);
	return 1;
      }
      else
      {
	vmsg("回心轉意囉.....");
      }
    }
    else
    {
      vmsg("您錢沒那麼多啦.......");
    }
    break;

  case '3':
    if (d.money < 80)
    {
      vmsg("傳統減肥要80元喔....您不夠錢啦...");
    }
    else
    {
      if (ians(b_lines - 1, 0, "需花費 80元(3～5公斤)，確定嗎(Y/N)？[N] ") == 'y')
      {
	weightmp = 3 + rand() % 3;
	d.weight -= weightmp;
	if (d.weight <= 0)
	  d.weight = 1;
	d.money -= 100;
	d.hp -= rand() % 2 + 3;
	show_special_pic(4);
	sprintf(buf, "總共減少了 %d 公斤", weightmp);
	vmsg(buf);
	return 1;
      }
      else
      {
	vmsg("回心轉意囉.....");
      }
    }
    break;

  case '4':
    vget(b_lines - 1, 0, " 減一公斤要 30 元，您要減多少公斤呢？[請填數字]：", buf, 4, DOECHO);
    weightmp = atoi(buf);
    if (weightmp <= 0)
    {
      vmsg("輸入有誤..放棄囉...");
    }
    else if (d.weight <= weightmp)
    {
      vmsg("您沒那麼重喔.....");
    }
    else if (d.money > (weightmp * 30))
    {
      sprintf(buf, " 減少 %d 公斤，總共需花費了 %d 元，確定嗎(Y/N)？[N] ", weightmp, weightmp * 30);
      if (ians(b_lines - 1, 0, buf) == 'y')
      {
	d.money -= weightmp * 30;
	d.weight -= weightmp;
	count_tired(5, 8, 0, 100, 1);
	d.hp -= (rand() % 2 + 3);
	d.sick += rand() % 10 + 5;
	show_special_pic(4);
	sprintf(buf, "總共減少了 %d 公斤", weightmp);
	vmsg(buf);
	return 1;
      }
      else
      {
	vmsg("回心轉意囉.....");
      }
    }
    else
    {
      vmsg("您錢沒那麼多啦.......");
    }
    break;
  }
  return 0;
}


/*-------------------------------------------------------*/
/* 系統選單: 個人資料 拜訪 小雞放生  特別服務		 */
/*-------------------------------------------------------*/


static int			/* 0: 沒有養小雞  1: 有養小雞 */
pip_data_list(userid)		/* 看某人小雞詳細資料 */
  char *userid;
{
  char buf1[20], buf2[20], buf3[20], buf4[20];
  int ch, page;
  struct CHICKEN chicken;

  if (!strcmp(cuser.userid, userid))	/* itoc.021031: 如果查詢自己，從記憶體叫現在值 */
  {
    memcpy(&chicken, &d, sizeof(CHICKEN));
  }
  else if (pip_read_file(userid, &chicken) < 0)
  {
    vmsg("他沒有養小雞喔");
    return 0;
  }

  page = 1;

  do
  {
    clear();
    move(1, 0);

    /* itoc,010802: 為了看清楚一點，所以 prints() 裡面的引數就不斷行寫在該列最後 */

    if (page == 1)
    {
      outs("\033[1;31m ╭┤\033[41;37m 基本資料 \033[0;1;31m├────────────────────────────╮\033[m\n");
      prints("\033[1;31m │\033[33m﹟姓    名 : \033[37m%-10s\033[33m﹟生    日 : \033[37m%-10s\033[33m﹟性    別 : \033[37m%-10s\033[31m │\033[m\n", chicken.name, chicken.birth, chicken.sex == 1 ? "♂" : "♀");
      prints("\033[1;31m │\033[33m﹟狀    態 : \033[37m%-10s\033[33m﹟復活次數 : \033[37m%-10d\033[33m﹟年    齡 : \033[37m%-10d\033[31m │\033[m\n", chicken.death == 1 ? "死亡" : chicken.death == 2 ? "拋棄" : chicken.death == 3 ? "結束" : "正常", chicken.liveagain, chicken.bbtime / 60 / 30);

      outs("\033[1;31m ├┤\033[41;37m 狀態指數 \033[0;1;31m├────────────────────────────┤\033[m\n");
      prints("\033[1;31m │\033[33m﹟親子關係 : \033[37m%-10d\033[33m﹟快 樂 度 : \033[37m%-10d\033[33m﹟滿 意 度 : \033[37m%-10d\033[31m │\033[m\n", chicken.relation, chicken.happy, chicken.satisfy);
      prints("\033[1;31m │\033[33m﹟戀愛指數 : \033[37m%-10d\033[33m﹟信    仰 : \033[37m%-10d\033[33m﹟罪    孽 : \033[37m%-10d\033[31m │\033[m\n", chicken.fallinlove, chicken.belief, chicken.sin);
      prints("\033[1;31m │\033[33m﹟感    受 : \033[37m%-10d\033[33m﹟         : \033[37m%-10s\033[33m﹟         : \033[37m%-10s\033[31m │\033[m\n", chicken.affect, "", "");

      outs("\033[1;31m ├┤\033[41;37m 健康指數 \033[0;1;31m├────────────────────────────┤\033[m\n");
      prints("\033[1;31m │\033[33m﹟體    重 : \033[37m%-10d\033[33m﹟疲 勞 度 : \033[37m%-10d\033[33m﹟病    氣 : \033[37m%-10d\033[31m │\033[m\n", chicken.weight, chicken.tired, chicken.sick);
      prints("\033[1;31m │\033[33m﹟清 潔 度 : \033[37m%-10d\033[33m﹟         : \033[37m%-10s\033[33m﹟         : \033[37m%-10s\033[31m │\033[m\n", chicken.shit, "", "");

      outs("\033[1;31m ├┤\033[41;37m 遊戲背景 \033[0;1;31m├────────────────────────────┤\033[m\n");
      prints("\033[1;31m │\033[33m﹟猜 拳 贏 : \033[37m%-10d\033[33m﹟猜 拳 輸 : \033[37m%-10d\033[33m﹟猜拳平手 : \033[37m%-10d\033[31m │\033[m\n", chicken.winn, chicken.losee, chicken.tiee);

      outs("\033[1;31m ├┤\033[41;37m 評價參數 \033[0;1;31m├────────────────────────────┤\033[m\n");
      prints("\033[1;31m │\033[33m﹟社交評價 : \033[37m%-10d\033[33m﹟家事評價 : \033[37m%-10d\033[33m﹟戰鬥評價 : \033[37m%-10d\033[31m │\033[m\n", chicken.social, chicken.family, chicken.hexp);
      prints("\033[1;31m │\033[33m﹟魔法評價 : \033[37m%-10d\033[33m﹟         : \033[37m%-10s\033[33m﹟         : \033[37m%-10s\033[31m │\033[m\n", chicken.hexp, "", "");

      outs("\033[1;31m ╰───────────────────────────────────╯\033[m\n");
      move(b_lines - 2, 0);
      outs("                                                             \033[1;36m第一頁\033[37m/\033[36m共三頁\033[m\n");
    }
    else if (page == 2)
    {
      outs("\033[1;31m ╭┤\033[41;37m 能力參數 \033[0;1;31m├────────────────────────────╮\033[m\n");
      prints("\033[1;31m │\033[33m﹟待人接物 : \033[37m%-10d\033[33m﹟氣 質 度 : \033[37m%-10d\033[33m﹟愛    心 : \033[37m%-10d\033[31m │\033[m\n", chicken.toman, chicken.character, chicken.love);
      prints("\033[1;31m │\033[33m﹟智    力 : \033[37m%-10d\033[33m﹟藝術能力 : \033[37m%-10d\033[33m﹟道    德 : \033[37m%-10d\033[31m │\033[m\n", chicken.wisdom, chicken.art, chicken.etchics);
      prints("\033[1;31m │\033[33m﹟勇    敢 : \033[37m%-10d\033[33m﹟掃地洗衣 : \033[37m%-10d\033[33m﹟魅    力 : \033[37m%-10d\033[31m │\033[m\n", chicken.brave, chicken.homework, chicken.charm);
      prints("\033[1;31m │\033[33m﹟禮    儀 : \033[37m%-10d\033[33m﹟談    吐 : \033[37m%-10d\033[33m﹟烹    飪 : \033[37m%-10d\033[31m │\033[m\n", chicken.manners, chicken.speech, chicken.cook);
      prints("\033[1;31m │\033[33m﹟攻 擊 力 : \033[37m%-10d\033[33m﹟防 禦 力 : \033[37m%-10d\033[33m﹟速    度 : \033[37m%-10d\033[31m │\033[m\n", chicken.attack, chicken.resist, chicken.speed);
      prints("\033[1;31m │\033[33m﹟戰鬥技術 : \033[37m%-10d\033[33m﹟魔法技術 : \033[37m%-10d\033[33m﹟抗魔能力 : \033[37m%-10d\033[31m │\033[m\n", chicken.hskill, chicken.mskill, chicken.immune);

      sprintf(buf1, "%d%s/%d%s", chicken.hp > 1000 ? chicken.hp / 1000 : chicken.hp, chicken.hp > 1000 ? "K" : "",	/* HP */
	chicken.maxhp > 1000 ? chicken.maxhp / 1000 : chicken.maxhp, chicken.maxhp > 1000 ? "K" : "");
      sprintf(buf2, "%d%s/%d%s", chicken.mp > 1000 ? chicken.mp / 1000 : chicken.mp, chicken.mp > 1000 ? "K" : "",	/* MP */      
	chicken.maxmp > 1000 ? chicken.maxmp / 1000 : chicken.maxmp, chicken.maxmp > 1000 ? "K" : "");
      sprintf(buf3, "%d%s/%d%s", chicken.vp > 1000 ? chicken.vp / 1000 : chicken.vp, chicken.vp > 1000 ? "K" : "",	/* VP */
	chicken.maxvp > 1000 ? chicken.maxvp / 1000 : chicken.maxvp, chicken.maxvp > 1000 ? "K" : "");
      sprintf(buf4, "%d%s/%d%s", chicken.sp > 1000 ? chicken.sp / 1000 : chicken.sp, chicken.sp > 1000 ? "K" : "",	/* SP */      
	chicken.maxsp > 1000 ? chicken.maxsp / 1000 : chicken.maxsp, chicken.maxsp > 1000 ? "K" : "");            

      outs("\033[1;31m ├┤\033[41;37m 戰鬥指標 \033[0;1;31m├────────────────────────────┤\033[m\n");
      prints("\033[1;31m │\033[33m﹟等    級 : \033[37m%-10d\033[33m﹟經 驗 值 : \033[37m%-10d\033[33m﹟   血    : \033[37m%-10s\033[31m │\033[m\n", chicken.level, chicken.exp, buf1);
      prints("\033[1;31m │\033[33m﹟法    力 : \033[37m%-10s\033[33m﹟移 動 力 : \033[37m%-10s\033[33m﹟內    力 : \033[37m%-10s\033[31m │\033[m\n", buf2, buf3, buf4);

      outs("\033[1;31m ├┤\033[41;37m 食物庫存 \033[0;1;31m├────────────────────────────┤\033[m\n");
      prints("\033[1;31m │\033[33m﹟食    物 : \033[37m%-10d\033[33m﹟零    食 : \033[37m%-10d\033[33m﹟         : \033[37m%-10s\033[31m │\033[m\n", chicken.food, chicken.cookie, "");
      prints("\033[1;31m │\033[33m﹟大 還 丹 : \033[37m%-10d\033[33m﹟靈    芝 : \033[37m%-10d\033[33m﹟大 補 丸 : \033[37m%-10d\033[31m │\033[m\n", chicken.pill, chicken.medicine, chicken.burger);
      prints("\033[1;31m │\033[33m﹟人    蔘 : \033[37m%-10d\033[33m﹟斷 續 膏 : \033[37m%-10d\033[33m﹟雪    蓮 : \033[37m%-10d\033[31m │\033[m\n", chicken.ginseng, chicken.paste, chicken.snowgrass);

      outs("\033[1;31m ├┤\033[41;37m 物品庫存 \033[0;1;31m├────────────────────────────┤\033[m\n");
      prints("\033[1;31m │\033[33m﹟金    錢 : \033[37m%-10d\033[33m﹟         : \033[37m%-10s\033[33m﹟         : \033[37m%-10s\033[31m │\033[m\n", chicken.money, "", "");
      prints("\033[1;31m │\033[33m﹟書    本 : \033[37m%-10d\033[33m﹟玩    具 : \033[37m%-10d\033[33m﹟課外讀物 : \033[37m%-10d\033[31m │\033[m\n", chicken.book, chicken.toy, chicken.playboy);

 
      outs("\033[1;31m ╰───────────────────────────────────╯\033[m\n");
      move(b_lines - 2, 0);
      outs("                                                             \033[1;36m第二頁\033[37m/\033[36m共三頁\033[m\n");
    }
    else /* if (page == 3) */
    {
      outs("\033[1;31m ╭┤\033[41;37m 參見王臣 \033[0;1;31m├────────────────────────────╮\033[m\n");
      prints("\033[1;31m │\033[33m﹟守衛好感 : \033[37m%-10d\033[33m﹟近衛好感 : \033[37m%-10d\033[33m﹟將軍好感 : \033[37m%-10d\033[31m │\033[m\n", chicken.royalA, chicken.royalB, chicken.royalC);
      prints("\033[1;31m │\033[33m﹟大臣好感 : \033[37m%-10d\033[33m﹟祭司好感 : \033[37m%-10d\033[33m﹟寵妃好感 : \033[37m%-10d\033[31m │\033[m\n", chicken.royalD, chicken.royalE, chicken.royalF);
      prints("\033[1;31m │\033[33m﹟王妃好感 : \033[37m%-10d\033[33m﹟國王好感 : \033[37m%-10d\033[33m﹟小丑好感 : \033[37m%-10d\033[31m │\033[m\n", chicken.royalG, chicken.royalH, chicken.royalI);

      outs("\033[1;31m ├┤\033[41;37m 工作次數 \033[0;1;31m├────────────────────────────┤\033[m\n");
      prints("\033[1;31m │\033[33m﹟家事次數 : \033[37m%-10d\033[33m﹟保姆次數 : \033[37m%-10d\033[33m﹟旅店次數 : \033[37m%-10d\033[31m │\033[m\n", chicken.workA, chicken.workB, chicken.workC);
      prints("\033[1;31m │\033[33m﹟農場次數 : \033[37m%-10d\033[33m﹟餐\廳次數 : \033[37m%-10d\033[33m﹟教堂次數 : \033[37m%-10d\033[31m │\033[m\n", chicken.workD, chicken.workE, chicken.workF);
      prints("\033[1;31m │\033[33m﹟地攤次數 : \033[37m%-10d\033[33m﹟伐木次數 : \033[37m%-10d\033[33m﹟美髮次數 : \033[37m%-10d\033[31m │\033[m\n", chicken.workG, chicken.workH, chicken.workI);
      prints("\033[1;31m │\033[33m﹟獵人次數 : \033[37m%-10d\033[33m﹟工地次數 : \033[37m%-10d\033[33m﹟守墓次數 : \033[37m%-10d\033[31m │\033[m\n", chicken.workJ, chicken.workK, chicken.workL);
      prints("\033[1;31m │\033[33m﹟家教次數 : \033[37m%-10d\033[33m﹟酒家次數 : \033[37m%-10d\033[33m﹟酒店次數 : \033[37m%-10d\033[31m │\033[m\n", chicken.workM, chicken.workN, chicken.workO);
      prints("\033[1;31m │\033[33m﹟夜 總 會 : \033[37m%-10d\033[33m﹟         : \033[37m%-10s\033[33m﹟         : \033[37m%-10s\033[31m │\033[m\n", chicken.workP, "", "");

      outs("\033[1;31m ├┤\033[41;37m 上課次數 \033[0;1;31m├────────────────────────────┤\033[m\n");
      prints("\033[1;31m │\033[33m﹟自然科學 : \033[37m%-10d\033[33m﹟唐詩宋詞 : \033[37m%-10d\033[33m﹟神學教育 : \033[37m%-10d\033[31m │\033[m\n", chicken.classA, chicken.classB, chicken.classC);
      prints("\033[1;31m │\033[33m﹟軍學教育 : \033[37m%-10d\033[33m﹟劍道技術 : \033[37m%-10d\033[33m﹟格鬥戰技 : \033[37m%-10d\033[31m │\033[m\n", chicken.classD, chicken.classE, chicken.classF);
      prints("\033[1;31m │\033[33m﹟魔法教育 : \033[37m%-10d\033[33m﹟禮儀教育 : \033[37m%-10d\033[33m﹟繪畫技巧 : \033[37m%-10d\033[31m │\033[m\n", chicken.classG, chicken.classH, chicken.classI);
      prints("\033[1;31m │\033[33m﹟舞蹈技巧 : \033[37m%-10d\033[33m﹟         : \033[37m%-10s\033[33m﹟         : \033[37m%-10s\033[31m │\033[m\n", chicken.classJ, "", "");

      outs("\033[1;31m ├┤\033[41;37m 裝備列表 \033[0;1;31m├────────────────────────────┤\033[m\n");
      prints("\033[1;31m │\033[33m﹟頭部裝備 : \033[37m%-10s\033[33m﹟手部裝備 : \033[37m%-10s\033[33m﹟盾牌裝備 : \033[37m%-10s\033[31m │\033[m\n", chicken.equiphead, chicken.equiphand, chicken.equipshield);
      prints("\033[1;31m │\033[33m﹟身體裝備 : \033[37m%-10s\033[33m﹟腳部裝備 : \033[37m%-10s\033[33m﹟           \033[37m%-10s\033[31m │\033[m\n", chicken.equipbody, chicken.equipfoot, "");

      outs("\033[1;31m ╰───────────────────────────────────╯\033[m\n");
      move(b_lines - 2, 0);
      outs("                                                             \033[1;36m第三頁\033[37m/\033[36m共三頁\033[m\n");
    }

    out_cmd("", COLOR1 " 查詢 " COLOR2 " [↑/PgUP]往上一頁 [↓/PgDN]往下一頁 [Q]離開                            \033[m");

    switch (ch = vkey())
    {
    case KEY_UP:
    case KEY_PGUP:
      if (page > 1)
        page--;
      break;

    default:
      if (page < 3)
	page++;
      break;
    }
  } while (ch != 'q' && ch != KEY_LEFT);

  return 1;
}


int
pip_query_self()		/* 查詢自己 */
{
  pip_data_list(cuser.userid);
  return 0;
}


int				/* 1:拜訪成功  0:沒這隻雞 */
pip_query()			/* 拜訪小雞 */
{
  int uno;
  char uid[IDLEN + 1];

  vs_bar("拜訪同伴");
  if (vget(1, 0, msg_uid, uid, IDLEN + 1, GET_USER))
  {
    move(2, 0);
    if (uno = acct_userno(uid))
    {
      pip_data_list(uid);
      return 1;
    }
    else
    {
      outs(err_uid);
      clrtoeol();
    }
  }
  return 0;
}


int				/* 1:放生  0:續養 */
pip_system_freepip()
{
  char buf[80];

  if (ians(b_lines - 2, 0, "真的要放生嗎(Y/N)？[N] ") == 'y')
  {
    sprintf(buf, "\033[1;31m%s 被狠心的 %s 丟掉了~\033[m", d.name, cuser.userid);
    pipdie(buf, 2);
    return 1;
  }
  return 0;
}


int
pip_system_service()
{
  int choice;
  char buf[128];

  out_cmd("", COLOR1 " 服務 " COLOR2 " [1]命名大師 [2]變性手術 [3]結局設局 [Q]離開                            \033[m");

  switch (vkey())
  {
  case '1':
    vget(b_lines - 2, 0, "幫小雞重新取個好名字：", buf, 11, DOECHO);
    if (!buf[0])
    {
      vmsg("等一下想好再來好了  :)");
    }
    else
    {
      strcpy(d.name, buf);
      vmsg("嗯嗯  換一個新的名字喔...");
    }
    break;

  case '2':			/* 變性 */
    if (d.sex == 1)	/* 1:公 2:母 */
    {
      choice = 2;		/* 公-->母 */
      sprintf(buf, "將小雞由♂變性成♀的嗎(Y/N)？[N] ");
    }
    else
    {
      choice = 1;		/* 母-->公 */
      sprintf(buf, "將小雞由♀變性成♂的嗎(Y/N)？[N] ");
    }
    if (ians(b_lines - 2, 0, buf) == 'y')
    {
      d.sex = choice;
      vmsg("變性手術完畢...");
    }
    break;

  case '3':
    /* 1:不要且未婚 4:要且未婚 */
    if (d.wantend == 1 || d.wantend == 2 || d.wantend == 3)
    {
      choice = 3;		/* 沒有-->有 */
      sprintf(buf, "將小雞遊戲改成【有20歲結局】(Y/N)？[N] ");
    }
    else
    {
      choice = -3;		/* 有-->沒有 */
      sprintf(buf, "將小雞遊戲改成【沒有20歲結局】(Y/N)？[N] ");
    }
    if (ians(b_lines - 2, 0, buf) == 'y')
    {
      d.wantend += choice;
      vmsg("遊戲結局設定完畢...");
    }
    break;
  }
  return 0;
}
#endif	/* HAVE_GAME */
