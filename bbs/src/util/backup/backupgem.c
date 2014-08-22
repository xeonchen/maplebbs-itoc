/*-------------------------------------------------------*/
/* util/backupgem.c	( NTHU MapleBBS Ver 3.10 )       */
/*-------------------------------------------------------*/
/* target : 備份所有精華區資料                           */
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
  char gempath[128], bakpath[128], cmd[256];
  time_t now;
  struct tm *ptime;

  chdir(BBSHOME);
  umask(077);

  /* 建立備份路徑目錄 */
  time(&now);
  ptime = localtime(&now);
  sprintf(bakpath, "%s/gem%02d%02d%02d", BAKPATH, ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday);
  mkdir(bakpath, 0700);

  if (chdir(BBSHOME "/gem") || !(dirp = opendir(".")))
    exit(-1);

  sprintf(cmd, "cp %s %s/", FN_DIR, bakpath);
  system(cmd);  

  /* 把 0~9 @ A~V 分別壓縮成一個壓縮檔 */
  while (de = readdir(dirp))
  {
    ptr = de->d_name;

    /* 看板的精華區另外備份 */
    if (!strcmp(ptr, "brd"))
      continue;

    if (ptr[0] > ' ' && ptr[0] != '.')
    {
      sprintf(cmd, "tar cfz %s/%s.tgz ./%s", bakpath, ptr, ptr);
      system(cmd);
    }
  }
  closedir(dirp);


  /* 備份看板 */

  if (chdir(BBSHOME "/gem/brd") || !(dirp = opendir(".")))
    exit(-1);

  /* 建立備份路徑目錄 */
  sprintf(gempath, "%s/brd", bakpath);
  mkdir(gempath, 0700);

  /* 把各看板分別壓縮成一個壓縮檔 */
  while (de = readdir(dirp))
  {
    ptr = de->d_name;

    if (ptr[0] > ' ' && ptr[0] != '.')
    {
      sprintf(cmd, "tar cfz %s/%s.tgz ./%s", gempath, ptr, ptr);
      system(cmd);
    }
  }
  closedir(dirp);

  exit(0);
}
