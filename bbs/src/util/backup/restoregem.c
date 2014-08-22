/*-------------------------------------------------------*/
/* util/restoregem.c    ( NTHU MapleBBS Ver 3.10 )       */
/*-------------------------------------------------------*/
/* target : 還原所有精華區資料                           */
/* create : 01/10/26                                     */
/* update :   /  /                                       */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#if 0

   BAKPATH 下會有很多不同日期備份的精華區資料，例如 gem010101 gem010102
   把要復原的那份更名為 gem  (mv gem010101 gem)
   執行本程式即可全部復原

#endif


#include "bbs.h"


int
main()
{
  struct dirent *de;
  DIR *dirp;
  char *ptr;
  char gempath[128], cmd[256];

  if (chdir(BAKPATH "/gem") || !(dirp = opendir(".")))
    exit(-1);

  strcpy(gempath, BBSHOME "/gem");
  mkdir(gempath, 0700);

  sprintf(cmd, "cp %s %s/", FN_DIR, gempath);
  system(cmd);

  /* 把 0~9 @ A~V 分別解壓縮回來 */
  while (de = readdir(dirp))
  {
    ptr = de->d_name;

    /* 看板的精華區另外解壓縮 */
    if (!strcmp(ptr, "brd"))
      continue;

    if (ptr[0] > ' ' && ptr[0] != '.')
    {
      sprintf(cmd, "tar xfz %s -C %s/", ptr, gempath);
      system(cmd);
    }
  }
  closedir(dirp);


  /* 回復看板 */

  if (chdir(BAKPATH "/gem/brd") || !(dirp = opendir(".")))
    exit(-1);

  strcat(gempath, "/brd");
  mkdir(gempath, 0700);

  /* 把各看板分別解壓縮回來 */
  while (de = readdir(dirp))
  {
    ptr = de->d_name;

    if (ptr[0] > ' ' && ptr[0] != '.')
    {
      sprintf(cmd, "tar xfz %s -C %s/", ptr, gempath);
      system(cmd);
    }
  }
  closedir(dirp);

  exit(0);
}
