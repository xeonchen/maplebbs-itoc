/*-------------------------------------------------------*/
/* util/mailpost.c	( NTHU CS MapleBBS Ver 2.36 )	 */
/*-------------------------------------------------------*/
/* target : 審核身分認證信函之回信			 */
/* create : 95/03/29				 	 */
/* update : 97/03/29				 	 */
/*-------------------------------------------------------*/
/* notice : brdshm (board shared memory) synchronize     */
/*-------------------------------------------------------*/


#include "bbs.h"


static void
mailog(msg)
  char *msg;
{
  FILE *fp;

  if (fp = fopen(BMTA_LOGFILE, "a"))
  {
    time_t now;
    struct tm *p;

    time(&now);
    p = localtime(&now);
    fprintf(fp, "%02d/%02d %02d:%02d:%02d <mailpost> %s\n",
      p->tm_mon + 1, p->tm_mday, 
      p->tm_hour, p->tm_min, p->tm_sec, 
      msg);
    fclose(fp);
  }
}


/* ----------------------------------------------------- */
/* 記錄驗證資料：user 有可能正在線上，故寫入檔案以保周全 */
/* ----------------------------------------------------- */


static int
is_badid(userid)
  char *userid;
{
  int ch;
  char *str;

  ch = strlen(userid);
  if (ch < 2 || ch > IDLEN)
    return 1;

  if (!is_alpha(*userid))
    return 1;

  str = userid;
  while (ch = *(++str))
  {
    if (!is_alnum(ch))
      return 1;
  }
  return 0;
}


static void
justify_user(userid, email)
  char *userid, *email;
{
  char fpath[64];
  HDR hdr;
  FILE *fp;

  /* 寄認證通過信給使用者 */
  usr_fpath(fpath, userid, FN_DIR);
  if (!hdr_stamp(fpath, HDR_LINK, &hdr, FN_ETC_JUSTIFIED))
  {
    strcpy(hdr.title, "您已經通過身分認證了！");
    strcpy(hdr.owner, STR_SYSOP);
    hdr.xmode = MAIL_NOREPLY;
    rec_add(fpath, &hdr, sizeof(HDR));
  }

  /* 記錄在 FN_JUSTIFY */
  usr_fpath(fpath, userid, FN_JUSTIFY);
  if (fp = fopen(fpath, "a"))
  {
    fprintf(fp, "RPY: %s\n", email);
    fclose(fp);
  }
}


static void
verify_user(str)
  char *str;
{
  int fd;
  char *ptr, *next, fpath[64];
  ACCT acct;

  /* itoc.註解: "userid(regkey) [VALID]" */

  if ((ptr = strchr(str, '(')) && (next = strchr(ptr + 1, ')')))
  {
    *ptr = '\0';
    *next = '\0';

    if (!is_badid(str) && !str_ncmp(next + 1, " [VALID]", 8))
    {
      /* 到此格式都正確 */

      usr_fpath(fpath, str, FN_ACCT);
      if ((fd = open(fpath, O_RDWR, 0600)) >= 0)
      {
	if (read(fd, &acct, sizeof(ACCT)) == sizeof(ACCT))
	{
	  if (str_hash(acct.email, acct.tvalid) == chrono32(ptr))	/* regkey 正確 */
	  {
	    /* 提升權限 */
	    acct.userlevel |= PERM_VALID;
	    time(&acct.tvalid);
	    lseek(fd, (off_t) 0, SEEK_SET);
	    write(fd, &acct, sizeof(ACCT));

	    justify_user(str, acct.email);
	  }
	}
	close(fd);
      }
    }
  }
}


/* ----------------------------------------------------- */
/* 主程式						 */
/* ----------------------------------------------------- */


static void
mailpost()
{
  int count;
  char *ptr, buf[512];

  /* 只需要找 Subject: 的檔頭 */

  count = 0;
  while ((++count < 20) && fgets(buf, sizeof(buf), stdin))	/* 最多 fgets 20 次，避免沒有 subject */
  {
    if (!str_ncmp(buf, "Subject: ", 9))
    {
      str_decode(buf);

      /* itoc.註解: mail.c: TAG_VALID " userid(regkey) [VALID]" */
      if (ptr = strstr(buf, TAG_VALID " "))
      {
	/* gslin.990101: TAG_VALID 長度不一定 */
	verify_user(ptr + sizeof(TAG_VALID));
      }
      break;
    }
  }
}


static void
sig_catch(sig)
  int sig;
{
  char buf[512];
  
  while (fgets(buf, sizeof(buf), stdin))
    ;
  sprintf(buf, "signal [%d]", sig);
  mailog(buf);
  exit(0);
}


int
main()
{
  char buf[512];

  setgid(BBSGID);
  setuid(BBSUID);
  chdir(BBSHOME);

  signal(SIGBUS, sig_catch);
  signal(SIGSEGV, sig_catch);
  signal(SIGPIPE, sig_catch);

  mailpost();

  /* eat mail queue */
  while (fgets(buf, sizeof(buf), stdin))
    ;

  exit(0);
}
