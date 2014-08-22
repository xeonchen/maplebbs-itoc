/*-------------------------------------------------------*/
/* xyz.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : 雜七雜八的外掛				 */
/* create : 01/03/01					 */
/* update :   /  /  					 */
/*-------------------------------------------------------*/


#include "bbs.h"


#ifdef HAVE_TIP

/* ----------------------------------------------------- */
/* 每日小秘訣						 */
/* ----------------------------------------------------- */

int
x_tip()
{
  int i, j;
  char msg[128];
  FILE *fp;

  if (!(fp = fopen(FN_ETC_TIP, "r")))
    return XEASY;

  fgets(msg, 128, fp);
  j = atoi(msg);		/* 第一行記錄總篇數 */
  i = time(0) % j + 1;
  j = 0;

  while (j < i)			/* 取第 i 個 tip */
  {
    fgets(msg, 128, fp);
    if (msg[0] == '#')
      j++;
  }

  move(12, 0);
  clrtobot();
  fgets(msg, 128, fp);
  prints("\033[1;36m每日小祕訣：\033[m\n");
  prints("            %s", msg);
  fgets(msg, 128, fp);
  prints("            %s", msg);
  vmsg(NULL);
  fclose(fp);
  return 0;
}
#endif	/* HAVE_TIP */


#ifdef HAVE_LOVELETTER 

/* ----------------------------------------------------- */
/* 情書產生器						 */
/* ----------------------------------------------------- */

int
x_loveletter()
{
  FILE *fp;
  int start_show;	/* 1:開始秀 */
  int style;		/* 0:開頭 1:正文 2:結尾 */
  int line;
  char buf[128];
  char header[3][5] = {"head", "body", "foot"};	/* 開頭、正文、結尾 */
  int num[3];

  /* etc/loveletter 前段是#head 中段是#body 後段是#foot */
  /* 行數上限：#head五行  #body八行  #foot五行 */

  if (!(fp = fopen(FN_ETC_LOVELETTER, "r")))
    return XEASY;

  /* 前三行記錄篇數 */
  fgets(buf, 128, fp);
  num[0] = atoi(buf + 5);
  num[1] = atoi(buf + 5);
  num[2] = atoi(buf + 5);

  /* 決定要選第幾篇 */
  line = time(0);
  num[0] = line % num[0];
  num[1] = (line >> 1) % num[1];
  num[2] = (line >> 2) % num[2];

  vs_bar("情書產生器");

  start_show = style = line = 0;

  while (fgets(buf, 128, fp))
  {
    if (*buf == '#')
    {
      if (!strncmp(buf + 1, header[style], 4))  /* header[] 長度都是 5 bytes */
	num[style]--;

      if (num[style] < 0)	/* 已經 fget 到要選的這篇了 */
      {
	outc('\n');
	start_show = 1;
	style++;
      }
      else
      {
	start_show = 0;
      }
      continue;
    }

    if (start_show)
    {
      if (line >= (b_lines - 5))	/* 超過螢幕大小了 */
	break;

      outs(buf);
      line++;
    }
  }

  fclose(fp);
  vmsg(NULL);

  return 0;
}
#endif	/* HAVE_LOVELETTER */


/* ----------------------------------------------------- */
/* 密碼忘記，重設密碼					 */
/* ----------------------------------------------------- */


int
x_password()
{
  int i;
  ACCT acct;
  FILE *fp;
  char fpath[80], email[60], passwd[PSWDLEN + 1];
  time_t now;

  vmsg("當其他使用者忘記密碼時，重送新密碼至該使用者的信箱");

  if (acct_get(msg_uid, &acct) > 0)
  {
    time(&now);

    if (acct.lastlogin > now - 86400 * 10)
    {
      vmsg("該使用者必須十天以上未上站方可重送密碼");
      return 0;
    }

    vget(b_lines - 2, 0, "請輸入認證時的 Email：", email, 40, DOECHO);

    if (str_cmp(acct.email, email))
    {
      vmsg("這不是該使用者認證時用的 Email");
      return 0;
    }

    if (not_addr(email) || !mail_external(email))
    {
      vmsg(err_email);
      return 0;
    }

    vget(b_lines - 1, 0, "請輸入真實姓名：", fpath, RNLEN + 1, DOECHO);
    if (strcmp(acct.realname, fpath))
    {
      vmsg("這不是該使用者的真實姓名");
      return 0;
    }

    if (vans("資料正確，請確認是否產生新密碼(Y/N)？[N] ") != 'y')
      return 0;

    sprintf(fpath, "%s 改了 %s 的密碼", cuser.userid, acct.userid);
    blog("PASSWD", fpath);

    /* 亂數產生 A~Z 組合的密碼八碼 */
    for (i = 0; i < PSWDLEN; i++)
      passwd[i] = rnd(26) + 'A';
    passwd[PSWDLEN] = '\0';

    /* 重新 acct_load 載入一次，避免對方在 vans() 時登入會有洗錢的效果 */
    if (acct_load(&acct, acct.userid) >= 0)
    {
      str_ncpy(acct.passwd, genpasswd(passwd), PASSLEN + 1);
      acct_save(&acct);
    }

    sprintf(fpath, "tmp/sendpass.%s", cuser.userid);
    if (fp = fopen(fpath, "w"))
    {
      fprintf(fp, "%s 為您申請了新密碼\n\n", cuser.userid);
      fprintf(fp, BBSNAME "ID : %s\n\n", acct.userid);
      fprintf(fp, BBSNAME "新密碼 : %s\n", passwd);
      fclose(fp);

      bsmtp(fpath, BBSNAME "新密碼", email, 0);
      unlink(fpath);

      vmsg("新密碼已寄到該認證信箱");
    }
  }

  return 0;
}
