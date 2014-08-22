/*-------------------------------------------------------*/
/* util/translist.c                   			 */
/*-------------------------------------------------------*/
/* target : WD 至 Maple 3.02 特殊名單轉換           	 */
/* create : 02/01/26                     		 */
/* update :   /  /                   			 */
/* author : itoc.bbs@bbs.ee.nctu.edu.tw          	 */
/*-------------------------------------------------------*/
/* syntax : translist                     		 */
/*-------------------------------------------------------*/


#if 0

   1. 修改 translist()
   2. 只轉 list.1 ~ list.5

   ps. 使用前請先行備份，use on ur own risk. 程式拙劣請包涵 :p
   ps. 感謝 lkchu 的 Maple 3.02 for FreeBSD

#endif


#include "wd.h"

#ifdef HAVE_LIST

/* ----------------------------------------------------- */
/* 3.02 functions                                        */
/* ----------------------------------------------------- */


static int
acct_uno(userid)
  char *userid;
{  
  int fd;
  int userno;
  char fpath[64];

  usr_fpath(fpath, userid, FN_ACCT);
  fd = open(fpath, O_RDONLY);
  if (fd >= 0)
  {
    read(fd, &userno, sizeof(userno));
    close(fd);
    return userno;
  }
  return -1;
}  


/* ----------------------------------------------------- */
/* 轉換主程式                                            */
/* ----------------------------------------------------- */


static void
translist(userid)
  char *userid;
{
  ACCT acct;
  int pos, fd, friend_userno, i;
  char fpath[64], buf[64];
  PAL pal;
  FRIEND friend;

  /* sob 的 usr 目錄有分大小寫，所以要先取得大小寫 */
  usr_fpath(buf, userid, FN_ACCT);
  if ((fd = open(buf, O_RDONLY)) >= 0)
  {
    read(fd, &acct, sizeof(ACCT));
    close(fd);
  }

  for (i = 1; i <= 5; i++)	/* 只轉 list.1 ~ list.5 */
  {
    sprintf(buf, "%s.%d", FN_LIST, i);
    usr_fpath(fpath, userid, buf);				/* 新的特殊名單 */
    sprintf(buf, OLD_BBSHOME "/home/%s/list.%d", acct.userid, i);/* 舊的特殊名單 */

    if (dashf(fpath))
      unlink(fpath);		/* 清掉重建 */

    pos = 0;
    fd = open(buf, O_RDONLY);
    if (fd < 0)
      continue;

    while (fd)
    {
      lseek(fd, (off_t) (sizeof(FRIEND) * pos), SEEK_SET);
      if (read(fd, &friend, sizeof(FRIEND)) == sizeof(FRIEND))
      {
	if ((friend_userno = acct_uno(friend.userid)) >= 0)
	{
	  str_ncpy(pal.userid, friend.userid, sizeof(pal.userid));
	  pal.ftype = 0;		/* 特殊名單一律為好友 */
	  str_ncpy(pal.ship, friend.desc, sizeof(pal.ship));      
	  pal.userno = friend_userno;
	  rec_add(fpath, &pal, sizeof(PAL));
	}
	pos++;
      }
      else
      {
	close(fd);
	break;
      }
    }
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
    translist(argv[1]);
    exit(1);
  }

  /* 轉換使用者特殊名單 */
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

      translist(str);
    }

    closedir(dirp);
  }
  return 0;
}
#else
int
main()
{
  printf("You should define HAVE_LIST first.\n");
  return -1;
}
#endif	/* HAVE_LIST */
