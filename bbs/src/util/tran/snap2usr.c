/*-------------------------------------------------------*/
/* util/snap2usr.c	( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : M3 ACCT 轉換程式				 */
/* create : 98/12/15					 */
/* update : 02/04/29					 */
/* author : mat.bbs@fall.twbbs.org			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/
/* syntax : snap2usr [userid]				 */
/*-------------------------------------------------------*/


#include "snap.h"


static usint
transfer_ufo(oldufo)
  usint oldufo;
{
  usint ufo;

  ufo = 0;

  if (oldufo & HABIT_MOVIE)
    ufo |= UFO_MOVIE;

  if (oldufo & HABIT_BNOTE)
    ufo |= UFO_BRDNOTE;

  if (oldufo & HABIT_VEDIT)
    ufo |= UFO_VEDIT;

  if (oldufo & HABIT_MOTD)
    ufo |= UFO_MOTD;

  if (oldufo & HABIT_PAGER)
    ufo |= UFO_PAGER;

  if (oldufo & HABIT_QUIET)
    ufo |= UFO_QUIET;

  if (oldufo & HABIT_PAL)
    ufo |= UFO_PAL;

  if (oldufo & HABIT_ALOHA)
    ufo |= UFO_ALOHA;

  if (oldufo & HABIT_NWLOG)
    ufo |= UFO_NWLOG;

  if (oldufo & HABIT_NTLOG)
    ufo |= UFO_NTLOG;

  if (oldufo & HABIT_CLOAK)
    ufo |= UFO_CLOAK;

  if (oldufo & HABIT_ACL)
    ufo |= UFO_ACL;

  return ufo;
}


/* ----------------------------------------------------- */
/* 轉換主程式						 */
/* ----------------------------------------------------- */


static void
trans_acct(old, new)
  userec *old;
  ACCT *new;
{
  memset(new, 0, sizeof(ACCT));

  new->userno = old->userno;

  str_ncpy(new->userid, old->userid, sizeof(new->userid));
  str_ncpy(new->passwd, old->passwd, sizeof(new->passwd));
  str_ncpy(new->realname, old->realname, sizeof(new->realname));
  str_ncpy(new->username, old->username, sizeof(new->username));

  new->userlevel = old->userlevel;
  new->ufo = transfer_ufo(old->ufo);	/* itoc.010917: ufo 欄位未必一樣 */
  new->signature = old->signature;

  new->year = 0;		/* 給初始值 */
  new->month = 0;
  new->day = 0;
  new->sex = 0;
  new->money = 100;
  new->gold = 1;

  new->numlogins = old->numlogins;
  new->numposts = old->numposts;
  new->numemails = old->numemail;

  new->firstlogin = old->firstlogin;
  new->lastlogin = old->lastlogin;
  new->tcheck = old->tcheck;
  new->tvalid = old->tvalid;

  str_ncpy(new->lasthost, old->lasthost, sizeof(new->lasthost));
  str_ncpy(new->email, old->email, sizeof(new->email));
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  ACCT new;
  char c;

  if (argc > 2)
  {
    printf("Usage: %s [userid]\n", argv[0]);
    return -1;
  }

  for (c = 'a'; c <= 'z'; c++)
  {
    char buf[64];
    struct dirent *de;
    DIR *dirp;

    sprintf(buf, BBSHOME "/usr/%c", c);
    chdir(buf);

    if (!(dirp = opendir(".")))
      continue;

    while (de = readdir(dirp))
    {
      userec old;
      int fd;
      char *str;

      str = de->d_name;
      if (*str <= ' ' || *str == '.')
	continue;

      if ((argc == 2) && str_cmp(str, argv[1]))
	continue;

#ifdef MAK_DIRS
      sprintf(buf, "%s/MF", str);
      mkdir(buf, 0700);
      sprintf(buf, "%s/gem", str);
      mak_links(buf);
#endif

      sprintf(buf, "%s/" FN_ACCT, str);
      if ((fd = open(buf, O_RDONLY)) < 0)
	continue;

      read(fd, &old, sizeof(userec));
      close(fd);
      unlink(buf);			/* itoc.010831: 砍掉原來的 FN_ACCT */

      trans_acct(&old, &new);

      fd = open(buf, O_WRONLY | O_CREAT, 0600);	/* itoc.010831: 重建新的 FN_ACCT */
      write(fd, &new, sizeof(ACCT));
      close(fd);
    }

    closedir(dirp);    
  }

  return 0;
}
