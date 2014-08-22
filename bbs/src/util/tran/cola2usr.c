/*-------------------------------------------------------*/
/* util/cola2usr.c  ( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : Cola 至 Maple 3.02 使用者轉換		 */
/* create : 03/02/11					 */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "cola.h"


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


static inline void
creat_dirs(old)
  userec *old;
{
  ACCT new;
  SCHEMA slot;
  int fd;
  char fpath[64];
  time_t now;

  time(&now);

  memset(&new, 0, sizeof(new));
  memset(&slot, 0, sizeof(slot));

  str_ncpy(new.userid, old->userid, sizeof(new.userid));
  str_ncpy(new.passwd, old->passwd, sizeof(new.passwd));
  str_ncpy(new.username, old->username, sizeof(new.username));
  str_ncpy(new.realname, old->realname, sizeof(new.realname));
  new.userlevel = PERM_DEFAULT;
  new.ufo = UFO_DEFAULT_NEW;
  new.numlogins = 1;
  new.firstlogin = now;
  new.lastlogin = now;
  new.tcheck = now;
  new.tvalid = now;

  slot.uptime = now;
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
/* 轉換簽名檔、計畫檔					 */
/* ----------------------------------------------------- */


static inline void
trans_sig(old)
  userec *old;
{
  char buf[64], fpath[64], f_sig[20];

  sprintf(buf, COLABBS_HOME "/%s/signatures", old->blank2);
  if (dashf(buf))
  {
    sprintf(f_sig, "%s.1", FN_SIGN);
    usr_fpath(fpath, old->userid, f_sig);
    f_cp(buf, fpath, O_TRUNC);
  }
}


static inline void
trans_plans(old)
  userec *old;
{
  char buf[64], fpath[64];

  sprintf(buf, COLABBS_HOME "/%s/PLANS", old->blank2);
  if (dashf(buf))
  {
    usr_fpath(fpath, old->userid, FN_PLANS);
    f_cp(buf, fpath, O_TRUNC);
  }
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
  char *ptr, index[64], folder[64], buf[64], fpath[64];
  fileheader fh;
  HDR hdr;
  time_t chrono;

  sprintf(index, COLABBS_HOME "/%s/mail/.DIR", old->blank2);
  usr_fpath(folder, old->userid, FN_DIR);

  if ((fd = open(index, O_RDONLY)) >= 0)
  {
    while (read(fd, &fh, sizeof(fh)) == sizeof(fh))
    {
      sprintf(buf, COLABBS_HOME "/%s/mail/%s", old->blank2, fh.filename);

      if (dashf(buf))     /* 文章檔案在才做轉換 */
      {
	char new_name[10] = "@";      

	/* 轉換文章 .DIR */
	memset(&hdr, 0, sizeof(HDR));
	chrono = trans_hdr_chrono(fh.filename);
	new_name[1] = radix32[chrono & 31];
	archiv32(chrono, new_name + 1);

	hdr.chrono = chrono;
	str_ncpy(hdr.xname, new_name, sizeof(hdr.xname));
	str_ncpy(hdr.owner, fh.owner, sizeof(hdr.owner));
	if (ptr = strchr(hdr.owner, ' '))
	  *ptr = '\0';
	str_ncpy(hdr.title, fh.title + 3, sizeof(hdr.title));
	str_stamp(hdr.date, &hdr.chrono);
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

  creat_dirs(user);
  trans_plans(user);
  trans_sig(user);
  trans_mail(user);
}


int
main()
{
  char *str, buf[64];
  struct dirent *de;
  DIR *dirp;
  userec user;

  chdir(BBSHOME);

  if (!(dirp = opendir(COLABBS_HOME)))
    return -1;

  while (de = readdir(dirp))
  {
    str = de->d_name;
    if (*str <= ' ' || *str == '.')
      continue;

    sprintf(buf, COLABBS_HOME "/%s/USERDATA.DAT", str);
    rec_get(buf, &user, sizeof(user), 0);    
    strcpy(user.blank2, str);			/* 借用做為 path */

    transusr(&user);
  }

  closedir(dirp);

  return 0;
}
