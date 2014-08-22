/*-------------------------------------------------------*/
/* util/transpal.c                                       */
/*-------------------------------------------------------*/
/* target : WD 至 Maple 3.02 (看板)好友名單轉換          */
/* create : 01/09/08                                     */
/* update :   /  /                                       */
/* author : itoc.bbs@bbs.ee.nctu.edu.tw                  */
/*-------------------------------------------------------*/
/* syntax : transpal                                     */
/*-------------------------------------------------------*/


#if 0

   1. 修改 struct FRIEND 和 transpal() transbrdpal()
   2. 轉換(看板)好友名單之前，您必須先轉換完看板及使用者。

   ps. 使用前請先行備份，use on ur own risk. 程式拙劣請包涵 :p
   ps. 感謝 lkchu 的 Maple 3.02 for FreeBSD

#endif


#include "wd.h"


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
transpal(userid)
  char *userid;
{
  ACCT acct;
  int fd, friend_userno;
  char fpath[64], buf[64];
  PAL pal;
  FRIEND friend;

  usr_fpath(fpath, userid, FN_PAL);			/* 新的好友名單 */

  /* sob 的 usr 目錄有分大小寫，所以要先取得大小寫 */
  usr_fpath(buf, userid, FN_ACCT);
  if ((fd = open(buf, O_RDONLY)) >= 0)
  {
    read(fd, &acct, sizeof(ACCT));
    close(fd);
  }
  else
  {
    return;
  }

  sprintf(buf, OLD_BBSHOME "/home/%s/pal", acct.userid);	/* 舊的好友名單 */

  if (dashf(fpath))
    unlink(fpath);		/* 清掉重建 */

  if ((fd = open(buf, O_RDONLY)) < 0)
    return;

  while (read(fd, &friend, sizeof(FRIEND)) == sizeof(FRIEND))
  {
    if ((friend_userno = acct_uno(friend.userid)) >= 0 &&
      strcmp(friend.userid, acct.userid))
    {
      memset(&pal, 0, sizeof(PAL));
      str_ncpy(pal.userid, friend.userid, sizeof(pal.userid));
      pal.ftype = (friend.savemode & 0x02) ? 0 : PAL_BAD;	/* 好友 vs 損友 */
      str_ncpy(pal.ship, friend.desc, sizeof(pal.ship));      
      pal.userno = friend_userno;
      rec_add(fpath, &pal, sizeof(PAL));
    }
  }
  close(fd);
}


static void
transbrdpal(brdname)
  char *brdname;
{
  int pos, fd, friend_userno;
  char fpath[64], buf[64];
  PAL pal;
  FRIEND friend;

  brd_fpath(fpath, brdname, FN_PAL);			/* 新的好友名單 */
  sprintf(buf, OLD_BBSHOME "/boards/%s/.LIST", brdname);/* 舊的好友名單 */

  if (dashf(fpath))
    unlink(fpath);		/* 清掉重建 */

  pos = 0;
  if ((fd = open(buf, O_RDONLY)) < 0)
    return;

  while (fd)
  {
    lseek(fd, (off_t) (sizeof(FRIEND) * pos), SEEK_SET);
    if (read(fd, &friend, sizeof(FRIEND)) == sizeof(FRIEND))
    {
      if ((friend_userno = acct_uno(friend.userid)) >= 0)
      {
	memset(&pal, 0, sizeof(PAL));
	str_ncpy(pal.userid, friend.userid, sizeof(pal.userid));
	pal.ftype = 0;		/* 看板好友一定是好友 */
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


int
main(argc, argv)
  int argc;
  char *argv[];
{
  char c, *str;
  char buf[64];
  struct dirent *de;
  DIR *dirp;

  /* argc == 1 轉全部使用者及看板 */
  /* argc == 2 轉某特定使用者 */

  if (argc > 2)
  {
    printf("Usage: %s [target_user]\n", argv[0]);
    exit(-1);
  }

  chdir(BBSHOME);

  if (argc == 2)
  {
    transpal(argv[1]);
    exit(1);
  }

  /* 轉換使用者好友名單 */
  for (c = 'a'; c <= 'z'; c++)
  {
    sprintf(buf, "usr/%c", c);

    if (!(dirp = opendir(buf)))
      continue;

    while (de = readdir(dirp))
    {
      str = de->d_name;
      if (*str <= ' ' || *str == '.')
	continue;

      transpal(str);
    }

    closedir(dirp);    
  }

  /* 轉換看板好友名單 */
  if (!(dirp = opendir("brd")))
    return 0;

  while (de = readdir(dirp))
  {
    str = de->d_name;
    if (*str <= ' ' || *str == '.')
      continue;

    transbrdpal(str);
  }

  closedir(dirp);

  return 0;
}
