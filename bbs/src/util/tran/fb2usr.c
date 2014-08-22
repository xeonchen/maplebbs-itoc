/*-------------------------------------------------------*/
/* util/fb/fb2usr.c					 */
/*-------------------------------------------------------*/
/* target : firebird 3.0 轉 Maple 3.x 使用者資料	 */
/*          .PASSWDS => .USR .ACCT			 */
/* create : 00/11/22					 */
/* update :   /  /  					 */
/* author : hightman@263.net				 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#define TRANS_BITS_PERM

#include "fb.h"


static char uperid[80];		/* 大寫的 ID */


/* ----------------------------------------------------- */
/* 轉換 .ACCT						 */
/* ----------------------------------------------------- */


static inline int
is_bad_userid(userid)
  char *userid;
{
  register char ch;

  if (strlen(userid) < 2)
    return 1;

  if (!isalpha(*userid))
    return 1;

  if (!str_cmp(userid, "new"))
    return 1;

  while (ch = *(++userid))
  {
    if (!isalnum(ch))
      return 1;
  }
  return 0;
}


static inline int
uniq_userno(fd)
  int fd;
{
  char buf[4096];
  int userno, size;
  SCHEMA *sp;			/* record length 16 可整除 4096 */

  userno = 1;

  while ((size = read(fd, buf, sizeof(buf))) > 0)
  {
    sp = (SCHEMA *) buf;
    do
    {
      if (sp->userid[0] == '\0')
      {
	lseek(fd, -size, SEEK_CUR);
	return userno;
      }
      userno++;
      size -= sizeof(SCHEMA);
      sp++;
    } while (size);
  }

  return userno;
}


static void				/* 換大寫 */
str_uper(dst, src)
  char *dst, *src;
{
  int ch;
  do
  {
    ch = *src++;
    if (ch >= 'a' && ch <= 'z')
      ch = ch - 32;
    *dst++ = ch;
  } while (ch);
}


static inline void
creat_dirs(old)
  userec *old;
{
  ACCT new;
  SCHEMA slot;
  int fd;
  char fpath[64];
  BITS *p;

  memset(&new, 0, sizeof(ACCT));
  memset(&slot, 0, sizeof(slot));

  str_ncpy(new.userid, old->userid, sizeof(new.userid));
  str_ncpy(new.passwd, old->passwd, sizeof(new.passwd));
  str_ncpy(new.realname, old->realname, sizeof(new.realname));
  str_ncpy(new.username, old->username, sizeof(new.username));

  for (p = perm; p->old; p++)
  {
    if (old->userlevel & p->old)
      new.userlevel |= p->new;
  }
  new.ufo = UFO_DEFAULT_NEW;
  new.signature = old->signature;

  new.year = old->birthyear;
  new.month = old->birthmonth;
  new.day = old->birthday;
  new.sex = 1;
  new.money = 0;
  new.gold = 0;

  new.numlogins = old->numlogins;
  new.numposts = old->numposts;
  new.numemails = old->nummails;

  new.firstlogin = old->firstlogin;
  new.lastlogin = old->lastlogin;
  time(&new.tcheck);
  time(&new.tvalid);

  str_ncpy(new.lasthost, old->lasthost, sizeof(new.lasthost));
  str_ncpy(new.email, old->email, sizeof(new.email));

  slot.uptime = time(0);
  strcpy(slot.userid, new.userid);

  fd = open(FN_SCHEMA, O_RDWR | O_CREAT, 0600);
  new.userno = uniq_userno(fd);
  write(fd, &slot, sizeof(slot));
  close(fd);

  usr_fpath(fpath, new.userid, NULL);
  mkdir(fpath, 0700);
  strcat(fpath, "/@");
  mkdir(fpath, 0700);
  usr_fpath(fpath, new.userid, "MF");
  mkdir(fpath, 0700);
  usr_fpath(fpath, new.userid, "gem");	/* itoc.010727: 個人精華區 */
  mak_links(fpath);

  usr_fpath(fpath, new.userid, ".ACCT");
  fd = open(fpath, O_WRONLY | O_CREAT, 0600);
  write(fd, &new, sizeof(ACCT));
  close(fd);
}


/* ----------------------------------------------------- */
/* 轉換信件						 */
/* ----------------------------------------------------- */


static inline void
trans_mail(old)
  userec *old;
{
  int fd;
  char index[64], folder[64], buf[64], fpath[64];
  fileheader fh;
  HDR hdr;
  time_t chrono;

  time(&chrono);

  sprintf(index, OLD_BBSHOME "/mail/%c/%s/.DIR", *uperid, old->userid);
  usr_fpath(folder, old->userid, FN_DIR);

  if ((fd = open(index, O_RDONLY)) >= 0)
  {
    while (read(fd, &fh, sizeof(fh)) == sizeof(fh))
    {
      sprintf(buf, OLD_BBSHOME "/mail/%c/%s/%s", *uperid, old->userid, fh.filename);

      if (dashf(buf))     /* 文章檔案在才做轉換 */
      {
	char new_name[10] = "@";      

	/* 轉換文章 .DIR */
	memset(&hdr, 0, sizeof(HDR));
	chrono++;
	new_name[1] = radix32[chrono & 31];
	archiv32(chrono, new_name + 1);

	hdr.chrono = chrono;
	str_ncpy(hdr.xname, new_name, sizeof(hdr.xname));
	str_ncpy(hdr.owner, fh.owner, sizeof(hdr.owner));
	str_ncpy(hdr.title, fh.title, sizeof(hdr.title));
	str_stamp(hdr.date, &chrono);
	hdr.xmode = MAIL_READ;	/* 設為已讀 */

	rec_add(folder, &hdr, sizeof(HDR));

	/* 拷貝檔案 */
	usr_fpath(fpath, old->userid, "@/");
	strcat(fpath, new_name);
	f_cp(buf, fpath, O_TRUNC);
      }
    }
    close(fd);
  }


}


/* ----------------------------------------------------- */
/* 轉換主程式						 */
/* ----------------------------------------------------- */


static void
transusr(user)
  userec *user;
{
  char buf[64], fpath[64];

  printf("轉換 %s 使用者\n", user->userid);

  if (is_bad_userid(user->userid))
  {
    printf("%s 不是合法 ID\n", user->userid);
    return;
  }

  usr_fpath(buf, user->userid, NULL);
  if (dashd(buf))
  {
    printf("%s 已經有此 ID\n", user->userid);
    return;
  }

  /* FireBird 是用大寫 id */
  str_uper(uperid, user->userid);

  sprintf(buf, OLD_BBSHOME "/home/%c/%s", *uperid, user->userid);
  if (!dashd(buf))
  {
    printf("%s 的檔案不存在\n", user->userid);
    return;
  }

  /* 轉換 .ACCT */
  creat_dirs(user);
    
  /* 轉換計畫檔/簽名檔 */
  sprintf(buf, OLD_BBSHOME "/home/%c/%s/plans", *uperid, user->userid);
  if (dashf(buf))
  {
    usr_fpath(fpath, user->userid, FN_PLANS);
    f_cp(buf, fpath, O_TRUNC);
  }
  sprintf(buf, OLD_BBSHOME "/home/%c/%s/signatures", *uperid, user->userid);
  if (dashf(buf))
  {
    usr_fpath(fpath, user->userid, FN_SIGN".1");
    f_cp(buf, fpath, O_TRUNC);
  }

  /* 轉換信件 */
  trans_mail(user);
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int fd;
  userec user;

  /* argc == 1 轉全部使用者 */
  /* argc == 2 轉某特定使用者 */

  if (argc > 2)
  {
    printf("Usage: %s [target_user]\n", argv[0]);
    exit(-1);
  }

  chdir(BBSHOME);

  if (!dashf(FN_PASSWD))
  {
    printf("ERROR! Can't open " FN_PASSWD "\n");
    exit(-1);
  }
  if (!dashd(OLD_BBSHOME "/home"))
  {
    printf("ERROR! Can't open " OLD_BBSHOME "/home\n");
    exit(-1);
  }

  if ((fd = open(FN_PASSWD, O_RDONLY)) >= 0)
  {
    while (read(fd, &user, sizeof(user)) == sizeof(user))
    {
      if (argc == 1)
      {
	transusr(&user);
      }
      else if (!strcmp(user.userid, argv[1]))
      {
	transusr(&user);
	exit(1);
      }
    }
    close(fd);
  }

  exit(0);
}
