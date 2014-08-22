/*-------------------------------------------------------*/
/* util/cola2post.c	( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : Cola 至 Maple 3.02 看板文章格式轉換		 */
/* create : 03/02/21					 */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"


#if 0

  以下這個是 ColaBBS 文章檔案的範例

*[m*[47;34m 作者 *[44;37m userid (暱稱)                                 *[47;34m 信區 *[44;37m SYSOP               *[m\n\r
*[47;34m 標題 *[44;37m Re: 啦啦啦啦啦啦                                               *[m\n\r
*[47;34m 時間 *[44;37m Fri Mar 15 11:33:20 2002                                       *[m\n\r
*[36m─────────────────────────────────*[m\n\r
\n\r
文章內容第一行\n\r
文章內容第二行\n\r

  要改成這樣

作者: userid (暱稱) 站內: SYSOP\n
標題: Re: 啦啦啦啦啦啦\n
時間: Fri Mar 15 11:33:20 2002\n
\n
文章內容第一行\n
文章內容第二行\n

#endif


static void
reaper(fpath)
  char *fpath;
{
  FILE *fpr, *fpw;
  char src[256], dst[256];
  char fnew[64];
  char *ptr;
  int i;

  if (!(fpr = fopen(fpath, "r")))
    return;

  sprintf(fnew, "%s.new", fpath);
  fpw = fopen(fnew, "w");

  i = 0;
  while (fgets(src, 256, fpr))
  {
    if (ptr = strchr(src, '\r'))
      *ptr = '\0';
    if (ptr = strchr(src, '\n'))
      *ptr = '\0';

    if (i < 4)	/* 前四行檔頭 */
    {
      i++;

      if (*src == '\033')	/* 此為檔頭 */
      {
        str_ansi(dst, src, sizeof(dst));	/* 去掉 ANSI */

	if (i <= 3 && dst[0] == ' ' && dst[5] == ' ')	/* 作者: */
	{
	  dst[5] = ':';
	  fprintf(fpw, "%.78s\n", dst + 1);	/* 去除第一格空白 */
	  continue;
	}
	else if (i == 4 && !strncmp(dst, "────", 8))
	{
	  /* 分隔線不要了 */
	  continue;
	}
      }
    }

    fprintf(fpw, "%s\n", src);	/* 內容照抄 */
  }

  fclose(fpr);
  fclose(fpw);

  unlink(fpath);
  rename(fnew, fpath);
}


static void
expireBrd(brdname)
  char *brdname;
{
  int fd;
  char folder[64], fpath[64];
  HDR hdr;

  printf("轉換 %s 看板\n", brdname);

  brd_fpath(folder, brdname, FN_DIR);

  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    while (read(fd, &hdr, sizeof(HDR)) == sizeof(HDR))
    {
      sprintf(fpath, "brd/%s/%c/%s", brdname, hdr.xname[7], hdr.xname);
      reaper(fpath);
    }
    close(fd);
  }
}


static void
expireGem(brdname)
  char *brdname;
{
  int i;
  char c;
  char fpath[64], *str;
  struct dirent *de;
  DIR *dirp;

  printf("轉換 %s 精華區\n", brdname);

  for (i = 0; i < 32; i++)
  {
    c = radix32[i];
    sprintf(fpath, "gem/brd/%s/%c", brdname, c);

    if (!(dirp = opendir(fpath)))
      continue;

    while (de = readdir(dirp))
    {
      str = de->d_name;
      if (*str <= ' ' || *str == '.' || *str == 'F')
        continue;

      sprintf(fpath, "gem/brd/%s/%c/%s", brdname, c, str);
      reaper(fpath);
    }

    closedir(dirp);
  }
}


static void
expireUsr(userid)
  char *userid;
{
  int fd;
  char folder[64], fpath[64];
  HDR hdr;

  printf("轉換 %s 信件\n", userid);

  usr_fpath(folder, userid, FN_DIR);

  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    while (read(fd, &hdr, sizeof(HDR)) == sizeof(HDR))
    {
      sprintf(fpath, "usr/%c/%s/@/%s", *userid, userid, hdr.xname);
      reaper(fpath);
    }
    close(fd);
  }
}


int
main()
{
  int fd;
  char c, *str, buf[64];
  BRD brd;
  struct dirent *de;
  DIR *dirp;

  chdir(BBSHOME);

  /* 轉換看板/精華區文章 */

  if ((fd = open(FN_BRD, O_RDONLY)) >= 0)
  {
    while (read(fd, &brd, sizeof(BRD)) == sizeof(BRD))
    {
      str = brd.brdname;
      expireBrd(str);
      expireGem(str);
    }
    close(fd);
  }

  /* 轉換使用者信件 */

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

      expireUsr(str);
    }

    closedir(dirp);
  }

  return 0;
}
