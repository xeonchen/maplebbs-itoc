/*-------------------------------------------------------*/
/* util/transpal.c                   			 */
/*-------------------------------------------------------*/
/* target : SOB 至 Maple 3.02 (看板)好友名單轉換         */
/* create : 01/09/08                     		 */
/* update :   /  /                   			 */
/* author : itoc.bbs@bbs.ee.nctu.edu.tw          	 */
/*-------------------------------------------------------*/
/* syntax : transpal                     		 */
/*-------------------------------------------------------*/


#if 0

   1. 設定 OLD_BBSHOME (sob config.h)
   2. 修改 struct FRIEND 和 transfer_pal() transfer_brdpal()
   3. 轉換(看板)好友名單之前，您必須先轉換完看板及使用者。

   ps. 使用前請先行備份，use on ur own risk. 程式拙劣請包涵 :p
   ps. 感謝 lkchu 的 Maple 3.02 for FreeBSD

#endif


#include "bbs.h"


#define OLD_BBSHOME 	"/home/oldbbs"	/* 2.36 */


/* ----------------------------------------------------- */
/* 3.02 functions                    			 */
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


static void
transfer_pal(userid)
  char *userid;
{
  ACCT acct;
  FILE *fp;
  int fd, friend_userno;
  char fpath[64], buf[64], friend_userid[80];
  PAL pal;

  usr_fpath(fpath, userid, FN_PAL);         			/* 新的好友名單 */

  /* sob 的 usr 目錄有分大小寫，所以要先取得大小寫 */
  usr_fpath(buf, userid, FN_ACCT);
  if ((fd = open(buf, O_RDONLY)) >= 0)
  {
    read(fd, &acct, sizeof(ACCT));
    close(fd);
  }
  sprintf(buf, OLD_BBSHOME"/home/%s/overrides", acct.userid);	/* 舊的好友名單 */

  if (dashf(fpath))
    unlink(fpath);      /* 清掉重建 */

  if (!(fp = fopen(buf, "r")))
    return;

  while (fscanf(fp, "%s", friend_userid) == 1)
  {
    if ((friend_userno = acct_uno(friend_userid)) >= 0)
    {
      memset(&pal, 0, sizeof(PAL));
      str_ncpy(pal.userid, friend_userid, sizeof(pal.userid));
      pal.ftype = 0;
      str_ncpy(pal.ship, "", sizeof(pal.ship));
      pal.userno = friend_userno;
      rec_add(fpath, &pal, sizeof(PAL));
    }
  }

  fclose(fp);
}


static void
transfer_brdpal(userid)
  char *userid;
{
  FILE *fp;
  int friend_userno;
  char fpath[64], buf[64], friend_userid[80];
  PAL pal;

  brd_fpath(fpath, userid, FN_PAL);				/* 新的看板好友名單 */
  sprintf(buf, OLD_BBSHOME"/boards/%s/visable", userid);	/* 舊的看板好友名單 */

  if (dashf(fpath))
    unlink(fpath);              /* 清掉重建 */

  if (!(fp = fopen(buf, "r")))
    return;

  while (fscanf(fp, "%s", friend_userid) == 1)
  {
    if ((friend_userno = acct_uno(friend_userid)) >= 0)
    {
      memset(&pal, 0, sizeof(PAL));
      str_ncpy(pal.userid, friend_userid, sizeof(pal.userid));
      pal.ftype = 0;
      str_ncpy(pal.ship, "", sizeof(pal.ship));
      pal.userno = friend_userno;
      rec_add(fpath, &pal, sizeof(PAL));
    }
  }

  fclose(fp);
}


int  
main()
{
  char c, *str;
  char buf[64];
  struct dirent *de;
  DIR *dirp;

  chdir(BBSHOME);

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

      transfer_pal(str);
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

    transfer_brdpal(str);
  }
  closedir(dirp);  

  return 0;
}
