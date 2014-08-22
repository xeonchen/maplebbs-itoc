/*-------------------------------------------------------*/
/* util/transfavor.c                   			 */
/*-------------------------------------------------------*/
/* target : WD 至 Maple 3.02 我的最愛轉換           	 */
/* create : 01/09/15                     		 */
/* update :   /  /                   			 */
/* author : itoc.bbs@bbs.ee.nctu.edu.tw          	 */
/*-------------------------------------------------------*/
/* syntax : transfavor                     		 */
/*-------------------------------------------------------*/


#if 0

   1. 修改 transmf()
   2. 轉換好友名單之前，您必須先轉換完看板及使用者。

   ps. 使用前請先行備份，use on ur own risk. 程式拙劣請包涵 :p
   ps. 感謝 lkchu 的 Maple 3.02 for FreeBSD

#endif


#include "ats.h"

#ifdef MY_FAVORITE


/* ----------------------------------------------------- */
/* 3.02 functions                    			 */
/* ----------------------------------------------------- */


static void
_mf_fpath(fpath, userid, fname)
  char *fpath;
  char *userid;		/* lower ID */
  char *fname;  
{
  if (fname)
    sprintf(fpath, "usr/%c/%s/MF/%s", userid[0], userid, fname);
  else
    sprintf(fpath, "usr/%c/%s/MF", userid[0], userid);
}


/* ----------------------------------------------------- */
/* 轉換主程式                                            */
/* ----------------------------------------------------- */


static void
transmf(userid)
  char *userid;
{
  ACCT acct;
  FILE *fp;
  int fd, num;
  char fpath[64], buf[64];
  char *str, brdname[IDLEN + 1];
  MF mf;

  /* 建立目錄 */
  _mf_fpath(fpath, userid, NULL);
  mkdir(fpath, 0700);

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

  sprintf(buf, OLD_BBSHOME"/home/%s/favor_boards", acct.userid);  /* 舊的我的最愛 */

  printf("轉換 %s ：我的最愛\n",acct.userid);

  if (!(fp = fopen(buf, "r")))
    return;

  _mf_fpath(fpath, userid, FN_MF);
  num = 0;

  while (fgets(brdname, IDLEN + 1, fp))
  {
    for (str = brdname; *str; str++)
    {
      if (*str <= ' ')
      {
	*str = '\0';
	break;
      }
    }

    brd_fpath(buf, brdname, NULL);
    if (dashd(buf))			/* 的確有這個板 */
    {
      mf.chrono = ++num;      
      mf.mftype = MF_BOARD;
      str_ncpy(mf.xname, brdname, sizeof(mf.xname));
      mf.title[0] = '\0';		/* 看板捷徑沒有 mf.title */
      rec_add(fpath, &mf, sizeof(MF));
    }
  }

  fclose(fp);
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
    transmf(argv[1]);
    exit(1);
  }

  /* 轉換使用者我的最愛 */
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

      transmf(str);
    }

    closedir(dirp);    
  }
  return 0;
}

#else
int
main()
{
  printf("You should define MY_FAVORITE first.\n");
  return -1;
}
#endif		/* MY_FAVORITE */
