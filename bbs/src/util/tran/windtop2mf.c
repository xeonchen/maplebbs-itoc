/*-------------------------------------------------------*/
/* util/windtop2usr.c					 */
/*-------------------------------------------------------*/
/* target : WindTop favorite 轉換			 */
/* create : 06/08/02					 */
/* update :   /  /  					 */
/* author : dingyuchi.bbs@bbs.scu.edu.tw                 */
/*-------------------------------------------------------*/


#include "windtop.h"


int
main()
{
  char c;
  mfheader favor;
  MF mf;

  for (c = 'a'; c <= 'z'; c++)
  {
    char buf[128];
    struct dirent *de;
    DIR *dirp;

    sprintf(buf, BBSHOME "/usr/%c", c);
    chdir(buf);

    if (!(dirp = opendir(".")))
      continue;

    while (de = readdir(dirp))
    {
      int fd;
      char *str;

      str = de->d_name;
      if (*str <= ' ' || *str == '.')
	continue;

      /* 建立我的最愛目錄 */
      sprintf(buf, "%s/MF", str);
      mkdir(buf, 0700);

      /* delete MF */
      sprintf(buf,"%s/MF/" FN_MF, str);
      unlink(buf);

      sprintf(buf, "%s/" FN_OLD_FAVORITE, str);
      if ((fd = open(buf, O_RDONLY)) < 0)
	continue;

      while (read(fd, &favor, sizeof(mfheader)) == sizeof(mfheader))
      {
        memset(&mf, 0, sizeof(MF));

        /* 轉換的動作在此 */
        str_ncpy(mf.xname, favor.xname, sizeof(mf.xname));
        mf.mftype = MF_BOARD;
        sprintf(buf,"%s/MF/" FN_MF, str);
        rec_add(buf, &mf, sizeof(MF));
      }

      /* 刪除舊的 */
      sprintf(buf,"%s/" FN_OLD_FAVORITE, str);
      unlink(buf);
      unlink(FN_OLDIMG_FAVORITE);

      close(fd);
    }
  }
}
