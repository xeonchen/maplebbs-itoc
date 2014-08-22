/*-------------------------------------------------------*/
/* util/backupbrd.c	( NTHU MapleBBS Ver 3.10 )       */
/*-------------------------------------------------------*/
/* target : 備份所有看板資料                             */
/* create : 01/10/19                                     */
/* update :   /  /                                       */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "bbs.h"


int
main()
{
  struct dirent *de;
  DIR *dirp;
  char *ptr;
  char bakpath[128], cmd[256];
  time_t now;
  struct tm *ptime;

  chdir(BBSHOME);
  umask(077);

  /* 建立備份路徑目錄 */
  time(&now);
  ptime = localtime(&now);
  sprintf(bakpath, "%s/brd%02d%02d%02d", BAKPATH, ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday);
  mkdir(bakpath, 0700);

  /* 備份 .BRD */
  sprintf(cmd, "%s/%s", bakpath, FN_BRD);
  f_cp(FN_BRD, cmd, O_EXCL);

  if (chdir(BBSHOME "/brd") || !(dirp = opendir(".")))
    exit(-1);

  /* 把各看板分別壓縮成一個壓縮檔 */
  while (de = readdir(dirp))
  {
    ptr = de->d_name;

    if (ptr[0] > ' ' && ptr[0] != '.')
    {
      sprintf(cmd, "tar cfz %s/%s.tgz ./%s", bakpath, ptr, ptr);
      system(cmd);
    }
  }
  closedir(dirp);

  exit(0);
}
