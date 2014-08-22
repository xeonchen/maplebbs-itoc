/*-------------------------------------------------------*/
/* util/transusr.c					 */
/*-------------------------------------------------------*/
/* target : Maple Sob 2.36 至 Maple 3.02 使用者轉換	 */
/*          .PASSWDS => .USR .ACCT			 */
/* create : 98/06/14					 */
/* update : 02/10/26					 */
/* author : ernie@micro8.ee.nthu.edu.tw			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/
/* syntax : transusr					 */
/*-------------------------------------------------------*/


#if 0

   1. 修改 struct userec 及 creat_dirs()
      (userec 兩版定義的字串長度不一，請自行換成數字)
   2. 除 plans 檔名，好友名單、暫存檔等都不轉換
   3. Sob 有九個簽名檔，只轉前三個
   4. 信箱中的 internet mail 如有需要請先 chmod 644 `find PATH -perm 600`

   ps. 使用前請先行備份，use on ur own risk. 程式拙劣請包涵 :p
   ps. 感謝 lkchu 的 Maple 3.02 for FreeBSD

#endif


#include "sob.h"


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


#define LEVEL_BASIC	000000000001	/* 基本權力 */
#define LEVEL_CHAT	000000000002	/* 進入聊天室 */
#define LEVEL_PAGE	000000000004	/* 找人聊天 */
#define LEVEL_POST	000000000010	/* 發表文章 */
#define LEVEL_LOGINOK	000000000020	/* 註冊程序認證 */
#define LEVEL_MAILLIMIT	000000000040	/* 信件無上限 */
#define LEVEL_CLOAK	000000000100	/* 隱身術 */
#define LEVEL_SEECLOAK	000000000200	/* 看見忍者 */
#define LEVEL_XEMPT	000000000400	/* 永久保留帳號 */
#define LEVEL_BM	000000002000	/* 板主 */
#define LEVEL_ACCOUNTS	000000004000	/* 帳號總管 */
#define LEVEL_CHATROOM	000000010000	/* 聊天室總管 */
#define LEVEL_BOARD	000000020000	/* 看板總管 */
#define LEVEL_SYSOP	000000040000	/* 站長 */


static inline usint
trans_acct_level(perm)
  usint perm;
{
  usint userlevel;

  userlevel = 0;

  if (perm & LEVEL_BASIC)
    userlevel |= PERM_BASIC;

  if (perm & LEVEL_CHAT)
    userlevel |= PERM_CHAT;

  if (perm & LEVEL_PAGE)
    userlevel |= PERM_PAGE;

  if (perm & LEVEL_POST)
    userlevel |= PERM_POST;

  if (perm & LEVEL_LOGINOK)
    userlevel |= PERM_VALID;

  if (perm & LEVEL_MAILLIMIT)
    userlevel |= PERM_MBOX;

  if (perm & LEVEL_CLOAK)
    userlevel |= PERM_CLOAK;

  if (perm & LEVEL_SEECLOAK)
    userlevel |= PERM_SEECLOAK;

  if (perm & LEVEL_XEMPT)
    userlevel |= PERM_XEMPT;

  if (perm & LEVEL_BM)
    userlevel |= PERM_BM;

  if (perm & LEVEL_ACCOUNTS)
    userlevel |= PERM_ACCOUNTS;

  if (perm & LEVEL_CHATROOM)
    userlevel |= PERM_CHATROOM;

  if (perm & LEVEL_BOARD)
    userlevel |= PERM_BOARD;

  if (perm & LEVEL_SYSOP)
    userlevel |= PERM_SYSOP;

  return userlevel;
}


static inline void
creat_dirs(old)
  userec *old;
{
  ACCT new;
  SCHEMA slot;
  int fd;
  char fpath[64];

  memset(&new, 0, sizeof(new));
  memset(&slot, 0, sizeof(slot));

  str_ncpy(new.userid, old->userid, sizeof(new.userid));
  str_ncpy(new.passwd, old->passwd, sizeof(new.passwd));
  str_ncpy(new.realname, old->realname, sizeof(new.realname));
  str_ncpy(new.username, old->username, sizeof(new.username));
  new.userlevel = trans_acct_level(old->userlevel);
  new.ufo = UFO_DEFAULT_NEW;
  new.signature = 0;
  new.year = old->year;
  new.month = old->month;
  new.day = old->day;
  new.sex = old->sex ? 1 : 0;
  new.money = 1000;		/* 預設銀幣 = 1000 金幣 = 0 */
  new.gold = 0;
  new.numlogins = old->numlogins;
  new.numposts = old->numposts;
  new.numemails = 0;
  new.firstlogin = old->firstlogin;
  new.lastlogin = old->lastlogin;
  new.tcheck = time(&new.tvalid);
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
  usr_fpath(fpath, new.userid, "gem");		/* itoc.010727: 個人精華區 */
  mak_links(fpath);

  usr_fpath(fpath, new.userid, ".ACCT");
  fd = open(fpath, O_WRONLY | O_CREAT, 0600);
  write(fd, &new, sizeof(ACCT));
  close(fd);
}


/* ----------------------------------------------------- */
/* 轉換認證資料						 */
/* ----------------------------------------------------- */


static inline void
trans_justify(old)
  userec *old;
{
  char fpath[64];
  FILE *fp;

  usr_fpath(fpath, old->userid, FN_JUSTIFY);
  if (fp = fopen(fpath, "a"))
  {
    fprintf(fp, "RPY: %s\n", old->justify);	/* 轉換預設以 email 認證 */
    fclose(fp);
  }
}


/* ----------------------------------------------------- */
/* 轉換簽名檔、計畫檔					 */
/* ----------------------------------------------------- */


static inline void
trans_sig(old)
  userec *old;
{
  int i;
  char buf[64], fpath[64], f_sig[20];

  for (i = 1; i <= 3; i++)	/* Maple 3.0 只有三個簽名 */
  {
    sprintf(buf, OLD_BBSHOME "/home/%s/sig.%d", old->userid, i);	/* 舊的簽名檔 */
    if (dashf(buf))
    {
      sprintf(f_sig, "%s.%d", FN_SIGN, i);
      usr_fpath(fpath, old->userid, f_sig);
      f_cp(buf, fpath, O_TRUNC);
    }
  }

  return;
}


static inline void
trans_plans(old)
  userec *old;
{
  char buf[64], fpath[64];

  sprintf(buf, OLD_BBSHOME "/home/%s/plans", old->userid);
  if (dashf(buf))
  {
    usr_fpath(fpath, old->userid, FN_PLANS);
    f_cp(buf, fpath, O_TRUNC);
  }
  return;
}


/* ----------------------------------------------------- */
/* 轉換信件						 */
/* ----------------------------------------------------- */


static time_t
trans_hdr_chrono(filename)
  char *filename;
{
  char time_str[11];

  /* M.1087654321.A 或 M.987654321.A */
  str_ncpy(time_str, filename + 2, filename[2] == '1' ? 11 : 10);

  return (time_t) atoi(time_str);
}


static inline void
trans_mail(old)
  userec *old;
{
  int fd;
  char index[64], folder[64], buf[64], fpath[64];
  fileheader fh;
  HDR hdr;

  sprintf(index, OLD_BBSHOME "/home/%s/.DIR", old->userid);
  usr_fpath(folder, old->userid, FN_DIR);

  if ((fd = open(index, O_RDONLY)) >= 0)
  {
    while (read(fd, &fh, sizeof(fh)) == sizeof(fh))
    {
      sprintf(buf, OLD_BBSHOME "/home/%s/%s", old->userid, fh.filename);

      if (dashf(buf))     /* 文章檔案在才做轉換 */
      {
	time_t chrono;
	char new_name[10] = "@";      

	/* 轉換文章 .DIR */
	memset(&hdr, 0, sizeof(HDR));
	chrono = trans_hdr_chrono(fh.filename);
	new_name[1] = radix32[chrono & 31];
	archiv32(chrono, new_name + 1);

	hdr.chrono = chrono;
	str_ncpy(hdr.xname, new_name, sizeof(hdr.xname));
	str_ncpy(hdr.owner, strstr(fh.owner, "[備.") ? "[備忘錄]" : fh.owner, sizeof(hdr.owner));	/* [備.忘.錄] */
	str_ncpy(hdr.title, fh.title, sizeof(hdr.title));
	str_stamp(hdr.date, &hdr.chrono);
	hdr.xmode = (fh.filemode & 0x2) ? (MAIL_MARKED | MAIL_READ) : MAIL_READ;	/* 設為已讀 */

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
/* 轉換個人精華區					 */
/* ----------------------------------------------------- */


#ifdef HAVE_PERSONAL_GEM
static void
trans_man_stamp(folder, token, hdr, fpath, time)
  char *folder;
  int token;
  HDR *hdr;
  char *fpath;
  time_t time;
{
  char *fname, *family;
  int rc;

  fname = fpath;
  while (rc = *folder++)
  {
    *fname++ = rc;
    if (rc == '/')
      family = fname;
  }
  if (*family != '.')
  {
    fname = family;
    family -= 2;
  }
  else
  {
    fname = family + 1;
    *fname++ = '/';
  }

  *fname++ = token;

  *family = radix32[time & 31];
  archiv32(time, fname);

  if (rc = open(fpath, O_WRONLY | O_CREAT | O_EXCL, 0600))
  {
    memset(hdr, 0, sizeof(HDR));
    hdr->chrono = time;
    str_stamp(hdr->date, &hdr->chrono);
    strcpy(hdr->xname, --fname);
    close(rc);
  }
  return;
}


static void
transman(index, folder)
  char *index, *folder;
{
  static int count = 100;

  int fd;
  char *ptr, buf[256], fpath[64];
  fileheader fh;
  HDR hdr;
  time_t chrono;

  if ((fd = open(index, O_RDONLY)) >= 0)
  {
    while (read(fd, &fh, sizeof(fh)) == sizeof(fh))
    {
      strcpy(buf, index);
      ptr = strrchr(buf, '/') + 1;
      strcpy(ptr, fh.filename);

      if (*fh.filename == 'M' && dashf(buf))	/* 只轉 M.xxxx.A 及 D.xxxx.a */
      {
	/* 轉換文章 .DIR */
	memset(&hdr, 0, sizeof(HDR));
	chrono = trans_hdr_chrono(fh.filename);
	trans_man_stamp(folder, 'A', &hdr, fpath, chrono);
	hdr.xmode = 0;
	str_ncpy(hdr.owner, fh.owner, sizeof(hdr.owner));
	str_ncpy(hdr.title, fh.title + 3, sizeof(hdr.title));
	rec_add(folder, &hdr, sizeof(HDR));

	/* 拷貝檔案 */
	f_cp(buf, fpath, O_TRUNC);
      }
      else if (*fh.filename == 'D' && dashd(buf))
      {
	char sub_index[256];

	/* 轉換文章 .DIR */
	memset(&hdr, 0, sizeof(HDR));
	 chrono = ++count;		/* WD 的目錄命名比較奇怪，只好自己給數字 */
	trans_man_stamp(folder, 'F', &hdr, fpath, chrono);
	hdr.xmode = GEM_FOLDER;
	str_ncpy(hdr.owner, fh.owner, sizeof(hdr.owner));
	str_ncpy(hdr.title, fh.title + 3, sizeof(hdr.title));
	rec_add(folder, &hdr, sizeof(HDR));

	/* recursive 進去轉換子目錄 */
	strcpy(sub_index, buf);
	ptr = strrchr(sub_index, '/') + 1;
	sprintf(ptr, "%s/.DIR", fh.filename);
	transman(sub_index, fpath);
      }
    }
    close(fd);
  }
}
#endif


/* ----------------------------------------------------- */
/* 轉換主程式						 */
/* ----------------------------------------------------- */


static void
transusr(user)
  userec *user;
{
  char buf[64];

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

  sprintf(buf, OLD_BBSHOME "/home/%s", user->userid);
  if (!dashd(buf))
  {
    printf("%s 的檔案不存在\n", user->userid);
    return;
  }

  creat_dirs(user);
  trans_justify(user);
  trans_sig(user);
  trans_plans(user);
  trans_mail(user);


#ifdef HAVE_PERSONAL_GEM
  sprintf(buf, OLD_BBSHOME "/home/%s/man", user->userid);
  if (dashd(buf))
  {
    char index[64], folder[64];

    sprintf(index, "%s/.DIR", buf);
    usr_fpath(folder, user->userid, "gem/" FN_DIR);
    transman(index, folder);
  }
#endif
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
