/*-------------------------------------------------------*/
/* util/redir.c		( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : 自動重建.DIR程式				 */
/* create : 99/10/07					 */
/* update : 04/11/29					 */
/* author : Thor.bbs@bbs.cs.nthu.edu.tw			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/
/* input  : scan current directory			 */
/* output : generate .DIR.re				 */
/*-------------------------------------------------------*/


#if 0

  此程式可以重建看板、精華區、信箱的 .DIR
  會 scan current directory 來產生 .DIR.re
  以 chrono 來排序

#endif


#include "bbs.h"


#define FNAME_DB_SIZE   2048


typedef char FNAME[9];
static FNAME *n_pool;
static int n_size, n_head;


static int
pool_add(fname)
  FNAME fname;
{
  char *p;

  /* initial pool */
  if (!n_pool)
  {
    n_pool = (FNAME *) malloc(FNAME_DB_SIZE * sizeof(FNAME));
    n_size = FNAME_DB_SIZE;
    n_head = 0;
  }

  if (n_head >= n_size)
  {
    n_size += (n_size >> 1);
    n_pool = (FNAME *) realloc(n_pool, n_size * sizeof(FNAME));
  }

  p = n_pool[n_head];

  if (strlen(fname) != 8)
    return -1;

  strcpy(p, fname);

  n_head++;

  return 0;
}


static int type;		/* 'b':看板 'g':精華區 'm':信箱 */


static HDR *
article_parse(fname)
  char *fname;
{
  char buf[ANSILINELEN], *ptr1, *ptr2, *ptr3;
  FILE *fp;
  static HDR hdr;

  memset(&hdr, 0, sizeof(HDR));

  /* fill in chrono/date/xmode/xid/xname */
  hdr.chrono = chrono32(fname);
  str_stamp(hdr.date, &hdr.chrono);
  strcpy(hdr.xname, fname);
  if (type == 'm')
    hdr.xmode = MAIL_READ;

  if (*fname == 'F')	/* 如果是卷宗，由於沒有其他資訊了，所以只能隨便給 */
  {
    hdr.xmode = GEM_FOLDER;
    strcpy(hdr.owner, STR_SYSOP);
    strcpy(hdr.nick, SYSOPNICK);
    strcpy(hdr.title, "滄海拾遺");
    return &hdr;
  }

  sprintf(buf, "%c/%s", (type == 'm') ? '@' : fname[7], fname);
  if (fp = fopen(buf, "r"))
  {
    if (fgets(buf, sizeof(buf), fp))
    {
      if (ptr1 = strchr(buf, '\n'))
	ptr1 = '\0';

      if (!strncmp(buf, STR_AUTHOR1 " ", LEN_AUTHOR1 + 1))
	ptr1 = buf + LEN_AUTHOR1 + 1;
      else if (!strncmp(buf, STR_AUTHOR2 " ", LEN_AUTHOR2 + 1))
	ptr1 = buf + LEN_AUTHOR2 + 1;
      else
	ptr1 = NULL;

      if (ptr1 && *ptr1)
      {
	ptr2 = strchr(ptr1 + 1, '@');
	ptr3 = strchr(ptr1 + 1, '(');

	if (ptr2 && (!ptr3 || ptr2 < ptr3))	/* 在暱稱裡面的 @ 不算是 email */
	{
	  str_from(ptr1, hdr.owner, hdr.nick);
	  hdr.xmode |= POST_INCOME;	/* also MAIL_INCOME */
	}
	else if (ptr3)
	{
	  ptr3[-1] = '\0';
	  str_ncpy(hdr.owner, ptr1, sizeof(hdr.owner));
	  if (ptr2 = strchr(ptr3 + 1, ')'))
	  {
	    *ptr2 = '\0';
	    str_ncpy(hdr.nick, ptr3 + 1, sizeof(hdr.nick));
	  }
	}
      }

      if (fgets(buf, sizeof(buf), fp))
      {
	if (ptr1 = strchr(buf, '\n'))
	  *ptr1 = '\0';

	if (!strncmp(buf, "標題: ", LEN_AUTHOR1 + 1))
	  ptr1 = buf + LEN_AUTHOR1 + 1;
	else if (!strncmp(buf, "標  題: ", LEN_AUTHOR2 + 1))
	  ptr1 = buf + LEN_AUTHOR2 + 1;
	else
	  ptr1 = NULL;

	if (ptr1 && *ptr1)	/* 有標題欄位 */
	  str_ncpy(hdr.title, ptr1, sizeof(hdr.title));
      }
    }

    fclose(fp);
  }

  return &hdr;
}


static char *allindex = ".DIR.tmp";


static void
allindex_collect()
{
  int i;
  char *fname, fpath[64];
  FILE *fp;

  /* 將所有的 F* 都寫入一個暫存檔 */
  if (fp = fopen(allindex, "w"))
  {
    for (i = 0; i < n_head; i++)
    {
      fname = n_pool[i];

      if (*fname != 'F')
	continue;

      sprintf(fpath, "%c/%s", fname[7], fname);
      f_suck(fp, fpath);
    }
    fclose(fp);
  }
}


static int
allindex_search(fname)
  char *fname;
{
  HDR old;
  int fd;
  int rc = 0;

  if ((fd = open(allindex, O_RDONLY)) >= 0)
  {
    while (read(fd, &old, sizeof(HDR)) == sizeof(HDR))
    {
      if (!strcmp(fname, old.xname))
      {
	rc = 1;
	break;
      }
    }
    close(fd);
  }
  return rc;
}


static int
fname_cmp(s1, s2)
  char *s1, *s2;
{
  return strcmp(s1 + 1, s2 + 1);
}


static void
usage(argv)
  char *argv[];
{
  char *str = argv[0];

  printf("Usage: 請指定參數\n");
  printf("  重建 看板文章 索引請執行 %s -b\n", str);
  printf("  重建精華區文章索引請執行 %s -g\n", str);
  printf("  重建 信箱信件 索引請執行 %s -m\n", str);
  printf("執行結束以後，再將 .DIR.re 覆蓋\ .DIR 即可\n");

  exit(0);
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int i;
  char *fname, buf[10];
  FILE *fp;
  HDR *hdr;
  struct dirent *de;
  DIR *dirp;

  if (argc != 2)
    usage(argv);

  type = argv[1][1];
  if (type != 'b' && type != 'g' && type != 'm')
    usage(argv);

  /* readdir 0-9A-V(看板、精華區) 或 @(信箱) */

  buf[1] = '\0';

  for (i = 0; i < 32; i++)
  {
    *buf = (type == 'm') ? '@' : radix32[i];

    if (dirp = opendir(buf))
    {
      while (de = readdir(dirp))
      {
	fname = de->d_name;
	if (type == 'b' && *fname != 'A')			/* 看板必是 A1234567 */
	  continue;
	if (type == 'g' && *fname != 'A' && *fname != 'F')	/* 精華區必是 A1234567 或 F1234567 */
	  continue;
	if (type == 'm' && *fname != '@')			/* 信箱必是 @1234567 */
	  continue;

	if (pool_add(fname) < 0)
	  printf("Bad article/folder name %c/%s\n", *buf, fname);
      }
      closedir(dirp);
    }

    if (type == 'm')	/* 信箱只需要掃 @ 這個目錄 */
      break;
  }

  if (n_head)
  {
    /* 看板/信箱 就直接把 n_pool 裡面的所有檔案加進 .DIR 即可 */
    /* 精華區的話，則是只需要把沒有在其他 F* 裡面的加入 .DIR */

    /* qsort chrono */
    if (n_head > 1)
      qsort(n_pool, n_head, sizeof(FNAME), fname_cmp);

    if (type == 'g')
      allindex_collect();

    /* generate .DIR.re */
    if (fp = fopen(".DIR.re", "w"))
    {
      /* for each file/folder */
      for (i = 0; i < n_head; i++)
      {
	fname = n_pool[i];

	if (type == 'g' && allindex_search(fname))	/* 已經在其他卷宗裡面了，那就不會是在 .DIR 裡面 */
	  continue;

	/* parse header */
	if (hdr = article_parse(fname))
	  fwrite(hdr, sizeof(HDR), 1, fp);
      }
      fclose(fp);
    }

    if (type == 'g')
      unlink(allindex);
  }

  return 0;
}
