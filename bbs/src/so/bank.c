/*-------------------------------------------------------*/
/* bank.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : 銀行、購買權限功能				 */
/* create : 01/07/16					 */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_BUY

static void
x_give()
{
  int way, dollar;
  char userid[IDLEN + 1], buf[80];
  char folder[64], fpath[64], reason[40];
  HDR hdr;
  FILE *fp;
  time_t now;
  PAYCHECK paycheck;

  if (!vget(13, 0, "您要把錢轉給誰呢？", userid, IDLEN + 1, DOECHO))
    return;

  if (acct_userno(userid) <= 0)
  {
    vmsg(err_uid);
    return;
  }

  way = vget(15, 0, "轉帳 1)轉銀幣 2)轉金幣：", buf, 3, DOECHO) - '1';
  if (way < 0 || way > 1)
    return;

  do
  {
    if (!vget(17, 0, "要轉多少錢過去？", buf, 9, DOECHO))	/* 最多轉 99999999 避免溢位 */
      return;

    dollar = atoi(buf);

    if (!way)
    {
      if (dollar > cuser.money)
	dollar = cuser.money;	/* 全轉過去 */
    }
    else
    {
      if (dollar > cuser.gold)
	dollar = cuser.gold;	/* 全轉過去 */
    }
  } while (dollar <= 1);	/* 不能只轉 1，會全變手續費 */

  if (!vget(19, 0, "請輸入理由：", reason, 40, DOECHO))
    strcpy(reason, "錢太多");

  sprintf(buf, "是否要轉帳給 %s %s幣 %d (Y/N)？[N] ", userid, !way ? "銀" : "金", dollar);
  if (vget(21, 0, buf, fpath, 3, LCECHO) == 'y')
  {
    if (!way)
      cuser.money -= dollar;
    else
      cuser.gold -= dollar;

    dollar -= dollar / 10 + ((dollar % 10) ? 1 : 0);	/* 10% 手續費 */

    /* itoc.020831: 加入匯錢記錄 */
    time(&now);
    sprintf(buf, "%-13s轉給 %-13s計 %d %s (%s)\n",
      cuser.userid, userid, dollar, !way ? "銀" : "金", Btime(&now));
    f_cat(FN_RUN_BANK_LOG, buf);

    usr_fpath(folder, userid, fn_dir);
    if (fp = fdopen(hdr_stamp(folder, 0, &hdr, fpath), "w"))
    {
      fprintf(fp, "%s %s (%s)\n標題: 轉帳通知\n時間: %s\n\n", 
	str_author1, cuser.userid, cuser.username, Btime(&now));
      fprintf(fp, "%s\n他的理由是：%s\n\n請您至金融中心將支票兌現", buf, reason);
      fclose(fp);      

      strcpy(hdr.title, "轉帳通知");
      strcpy(hdr.owner, cuser.userid);
      rec_add(folder, &hdr, sizeof(HDR));
    }

    memset(&paycheck, 0, sizeof(PAYCHECK));
    time(&paycheck.tissue);
    if (!way)
      paycheck.money = dollar;
    else
      paycheck.gold = dollar;
    sprintf(paycheck.reason, "[轉帳] %s", cuser.userid);
    usr_fpath(fpath, userid, FN_PAYCHECK);
    rec_add(fpath, &paycheck, sizeof(PAYCHECK));

    sprintf(buf, "您身上有銀幣 %d 元，金幣 %d 元", cuser.money, cuser.gold);
    vmsg(buf);
  }
  else
  {
    vmsg("取消交易");
  }
}


#define GOLD2MONEY	900000	/* 金幣→銀幣 匯率 */
#define MONEY2GOLD	1100000	/* 銀幣→金幣 匯率 */

static void
x_exchange()
{
  int way, gold, money;
  char buf[80], ans[8];

  move(13, 0);
  prints("銀幣→金幣 = %d：1  金幣→銀幣 = 1：%d", MONEY2GOLD, GOLD2MONEY);

  way = vget(15, 0, "匯兌 1)銀幣→金幣 2)金幣→銀幣：", ans, 3, DOECHO) - '1';

  if (!way)
    money = cuser.money / MONEY2GOLD;
  else if (way == 1)
    money = cuser.gold;
  else
    return;

  if (!way)
    sprintf(buf, "您要將銀幣兌換成多少個金幣呢？[1 - %d] ", money);
  else
    sprintf(buf, "您要兌換多少個金幣成為銀幣呢？[1 - %d] ", money);
    
  if (!vget(17, 0, buf, ans, 4, DOECHO))	/* 長度比較短，避免溢位 */
    return;

  gold = atoi(ans);
  if (gold <= 0 || gold > money)
    return;

  if (!way)
  {
    if (gold > (INT_MAX - cuser.gold))
    {
      vmsg("您換太多錢囉～會溢位的！");
      return;
    }
    money = gold * MONEY2GOLD;
    sprintf(buf, "是否要兌換銀幣 %d 元 為金幣 %d (Y/N)？[N] ", money, gold);
  }
  else
  {
    money = gold * GOLD2MONEY;
    if (money > (INT_MAX - cuser.money))
    {
      vmsg("您換太多錢囉～會溢位的！");
      return;
    }
    sprintf(buf, "是否要兌換金幣 %d 元 為銀幣 %d (Y/N)？[N] ", gold, money);
  }

  if (vget(19, 0, buf, ans, 3, LCECHO) == 'y')
  {
    if (!way)
    {
      cuser.money -= money;
      addgold(gold);
    }
    else
    {
      cuser.gold -= gold;
      addmoney(money);
    }
    sprintf(buf, "您身上有銀幣 %d 元，金幣 %d 元", cuser.money, cuser.gold);
    vmsg(buf);
  }
  else
  {
    vmsg("取消交易");
  }
}


static void
x_cash()
{
  int fd, money, gold;
  char fpath[64], buf[64];
  FILE *fp;
  PAYCHECK paycheck;

  usr_fpath(fpath, cuser.userid, FN_PAYCHECK);
  if ((fd = open(fpath, O_RDONLY)) < 0)
  {
    vmsg("您目前沒有支票未兌現");
    return;
  }

  usr_fpath(buf, cuser.userid, "cashed");
  fp = fopen(buf, "w");
  fputs("以下是您的支票兌換清單：\n\n", fp);

  money = gold = 0;
  while (read(fd, &paycheck, sizeof(PAYCHECK)) == sizeof(PAYCHECK))
  {
    if (paycheck.money < (INT_MAX - money))	/* 避免溢位 */
      money += paycheck.money;
    else
      money = INT_MAX;
    if (paycheck.gold < (INT_MAX - gold))	/* 避免溢位 */
      gold += paycheck.gold;
    else
      gold = INT_MAX;

    fprintf(fp, "%s %s %d 銀 %d 金\n", 
      Btime(&paycheck.tissue), paycheck.reason, paycheck.money, paycheck.gold);
  }
  close(fd);
  unlink(fpath);

  fprintf(fp, "\n您共兌現 %d 銀 %d 金\n", money, gold);
  fclose(fp);

  addmoney(money);
  addgold(gold);

  more(buf, NULL);
  unlink(buf);
}


int
x_bank()
{
  char ans[3];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  vs_bar("信託銀行");
  move(2, 0);

  /* itoc.011208: 以防萬一 */
  if (cuser.money < 0)
    cuser.money = 0;
  if (cuser.gold < 0)
    cuser.gold = 0;

  outs("\033[1;36m  ╭═════════════════════════════╮\n");
  prints("  ║\033[32m您現在有銀幣 \033[33m%12d\033[32m 元，金幣 \033[33m%12d\033[32m 元\033[36m        ║\n", 
    cuser.money, cuser.gold);
  outs("  ╠═════════════════════════════╣\n"
    "  ║ 目前銀行提供下列幾項服務：                               ║\n"
    "  ║\033[33m1.\033[37m 轉帳 -- 轉帳給其他人   (抽取 10% 手續費) \033[36m              ║\n"
    "  ║\033[33m2.\033[37m 匯兌 -- 銀幣/金幣 兌換 (抽取 10% 手續費) \033[36m              ║\n"
    "  ║\033[33m3.\033[37m 兌現 -- 支票兌現                         \033[36m              ║\n"
    "  ╰═════════════════════════════╯\033[m");

  vget(11, 0, "請輸入您需要的服務：", ans, 3, DOECHO);
  if (ans[0] == '1')
    x_give();
  else if (ans[0] == '2')
    x_exchange();
  else if (ans[0] == '3')
    x_cash();

  return 0;
}


int
b_invis()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (cuser.ufo & UFO_CLOAK)
  {
    if (vans("是否現身(Y/N)？[N] ") != 'y')
      return XEASY; 
    /* 現身免費 */
  }
  else
  {
    if (HAS_PERM(PERM_CLOAK))
    {
      if (vans("是否隱形(Y/N)？[N] ") != 'y')
	return XEASY;
      /* 有無限隱形權限者免費 */
    }
    else
    {
      if (cuser.gold < 10)
      {
	vmsg("要 10 金幣才能隱形喔");
	return XEASY;
      }
      if (vans("是否花 10 金幣隱形(Y/N)？[N] ") != 'y')
	return XEASY;
      cuser.gold -= 10;
    }
  }

  cuser.ufo ^= UFO_CLOAK;
  cutmp->ufo ^= UFO_CLOAK;	/* ufo 要同步 */

  return XEASY;
}


static void
buy_level(userlevel)		/* itoc.010830: 只存 level 欄位，以免變動到在線上更動的認證欄位 */
  usint userlevel;
{
  if (!HAS_STATUS(STATUS_DATALOCK))	/* itoc.010811: 要沒有被站長鎖定，才能寫入 */
  {
    int fd;
    char fpath[80];
    ACCT tuser;

    usr_fpath(fpath, cuser.userid, fn_acct);
    fd = open(fpath, O_RDWR);
    if (fd >= 0)
    {
      if (read(fd, &tuser, sizeof(ACCT)) == sizeof(ACCT))
      {
	tuser.userlevel |= userlevel;
	lseek(fd, (off_t) 0, SEEK_SET);
	write(fd, &tuser, sizeof(ACCT));
	vmsg("您已經獲得權限，請重新上站");
      }
      close(fd);
    }
  }
}


int
b_cloak()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (HAS_PERM(PERM_CLOAK))
  {
    vmsg("您已經能無限隱形了");
  }
  else
  {
    if (cuser.gold < 1000)
    {
      vmsg("要 1000 金幣才能購買無限隱形權限喔");
    }
    else if (vans("是否花 1000 金幣購買無限隱形權限(Y/N)？[N] ") == 'y')
    {
      cuser.gold -= 1000;
      buy_level(PERM_CLOAK);
    }
  }

  return XEASY;
}


int
b_mbox()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (HAS_PERM(PERM_MBOX))
  {
    vmsg("您的信箱已經沒有上限了");
  }
  else
  {
    if (cuser.gold < 1000)
    {
      vmsg("要 1000 金幣才能購買信箱無限權限喔");
    }
    else if (vans("是否花 1000 金幣購買信箱無限權限(Y/N)？[N] ") == 'y')
    {
      cuser.gold -= 1000;
      buy_level(PERM_MBOX);
    }
  }

  return XEASY;
}


int
b_xempt()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (HAS_PERM(PERM_XEMPT))
  {
    vmsg("您的帳號已經永久保留了");
  }
  else
  {
    if (cuser.gold < 1000)
    {
      vmsg("要 1000 金幣才能購買帳號永久保留權限喔");
    }
    else if (vans("是否花 1000 金幣購買帳號永久保留權限(Y/N)？[N] ") == 'y')
    {
      cuser.gold -= 1000;
      buy_level(PERM_XEMPT);
    }
  }

  return XEASY;
}


#if 0	/* 不提供購買自殺功能 */
int
b_purge()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (HAS_PERM(PERM_PURGE))
  {
    vmsg("系統在下次定期清帳號時，將清除此 ID");
  }
  else
  {
    if (cuser.gold < 1000)
    {
      vmsg("要 1000 金幣才能自殺喔");
    }
    else if (vans("是否花 1000 金幣自殺(Y/N)？[N] ") == 'y')
    {
      cuser.gold -= 1000;
      buy_level(PERM_PURGE);
    }
  }

  return XEASY;
}
#endif
#endif	/* HAVE_BUY */
