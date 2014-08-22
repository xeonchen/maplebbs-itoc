/*-------------------------------------------------------*/
/* util/restorebrd.c	( NTHU MapleBBS Ver 3.10 )       */
/*-------------------------------------------------------*/
/* target : 還原所有看板資料                             */
/* create : 01/10/26                                     */
/* update :   /  /                                       */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#if 0

   BAKPATH 下會有很多不同日期備份的看板資料，例如 brd010101 brd010102
   把要復原的那份更名為 brd  (mv brd010101 brd)
   執行本程式即可全部復原

#endif


#include "bbs.h"


int
main()
{
  struct dirent *de;
  DIR *dirp;
  char *ptr;
  char brdpath[128], cmd[256];

  if (chdir(BAKPATH "/brd") || !(dirp = opendir(".")))
    exit(-1);

  sprintf(cmd, "cp %s %s/%s; chmod 600 %s/%s", FN_BRD, BBSHOME, FN_BRD, BBSHOME, FN_BRD);
  system(cmd);

  strcpy(brdpath, BBSHOME "/brd");
  mkdir(brdpath, 0700);

  while (de = readdir(dirp))
  {
    ptr = de->d_name;

    if (!strcmp(ptr, "BRD"))
      continue;

    if (ptr[0] > ' ' && ptr[0] != '.')
    {
      sprintf(cmd, "tar xfz %s -C %s/", ptr, brdpath);
      system(cmd);
    }
  }
  closedir(dirp);

  exit(0);
}
