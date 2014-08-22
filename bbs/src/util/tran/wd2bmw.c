/*-------------------------------------------------------*/
/* util/transbmw.c                   			 */
/*-------------------------------------------------------*/
/* target : WD 至 Maple 3.02 水球記錄轉換           	 */
/* create : 02/01/22                     		 */
/* update :   /  /                   			 */
/* author : itoc.bbs@bbs.ee.nctu.edu.tw          	 */
/*-------------------------------------------------------*/
/* syntax : transbmw                     		 */
/*-------------------------------------------------------*/


#if 0

   1. 修改 transbmw()

   ps. 使用前請先行備份，use on ur own risk. 程式拙劣請包涵 :p
   ps. 感謝 lkchu 的 Maple 3.02 for FreeBSD

#endif


#include "wd.h"


/* ----------------------------------------------------- */
/* 3.02 functions                    			 */
/* ----------------------------------------------------- */


static void
_mail_self(userid, fpath, owner, title)		/* itoc.011115: 寄檔案給自己 */
  char *userid;		/* 收件者 */
  char *fpath;		/* 檔案路徑 */
  char *owner;		/* 寄件人 */
  char *title;		/* 郵件標題 */
{
  HDR fhdr;
  char folder[64];

  usr_fpath(folder, userid, FN_DIR);
  close(hdr_stamp(folder, HDR_LINK, &fhdr, fpath));
  str_ncpy(fhdr.owner, owner, sizeof(fhdr.owner));
  str_ncpy(fhdr.title, title, sizeof(fhdr.title));
  fhdr.xmode = 0;
  rec_add(folder, &fhdr, sizeof(fhdr));
}


/* ----------------------------------------------------- */
/* 轉換主程式                                            */
/* ----------------------------------------------------- */


static void
transbmw(userid)
  char *userid;
{
  ACCT acct;
  int fd;
  char buf[64];

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

  sprintf(buf, OLD_BBSHOME"/home/%s/writelog", acct.userid);	/* 舊的水球記錄 */

  if (dashf(buf))
    _mail_self(acct.userid, buf, "[備忘錄]", "熱線\033[41m記錄\033[m");
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
    transbmw(argv[1]);
    exit(1);
  }

  /* 轉換使用者水球記錄 */
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

      transbmw(str);
    }

    closedir(dirp);    
  }
  return 0;
}
