/*-------------------------------------------------------*/
/* util/fb/fb2gem.c					 */
/*-------------------------------------------------------*/
/* target : firebird 3.0 轉 Maple 3.x 精華區		 */
/*          0Announce => gem/@Class			 */
/* create : 00/11/22					 */
/* update :   /  /  					 */
/* author : hightman@263.net				 */
/*----------------------------------------------------------*/


#include "fb.h"


static int ndir;
static int nfile;
static time_t chrono;
static int level;
static char gpath[1024];
static char tmp_name[128];

static int mandex(char *board, char *fpath, int deep);
static void Names_class(char *fpath);

static char *fn_names = ".Names";


/* ------------------------------------------------------------- */
/* 建立 gem/.DIR 及 gem/@/@Class                                 */
/* ------------------------------------------------------------- */


static void
new_class()
{
  HDR hdr;

  memset(&hdr, 0, sizeof(HDR));
  time(&hdr.chrono);
  strcpy(hdr.owner, STR_SYSOP);
  strcpy(hdr.nick, SYSOPNICK);
  str_stamp(hdr.date, &hdr.chrono);
  strcpy(hdr.xname, "@Class");
  strcpy(hdr.title, "Class/     看板精華區");
  hdr.xmode = GEM_FOLDER;
  rec_add("gem/.DIR", &hdr, sizeof(HDR));
}


/* ------------------------------------------------------------- */
/* Tran_Group         						 */
/* ------------------------------------------------------------- */


static void
tran_group(title, fname, flag)
  char *title;
  char *fname;
  int flag;
{
  HDR hdr;
  char buf[1024];

  memset(&hdr, 0, sizeof(HDR));
  time(&hdr.chrono);
  strcpy(hdr.owner, STR_SYSOP);
  strcpy(hdr.nick, SYSOPNICK);
  str_stamp(hdr.date, &hdr.chrono);

  if (flag == 1)		/* 卷宗 */
  {
    strcpy(tmp_name, fname);

    sprintf(hdr.xname, "@%s", fname);
    str_ncpy(hdr.title, title, sizeof(hdr.title));
    hdr.xmode = GEM_FOLDER;
    rec_add("gem/@/@Class", &hdr, sizeof(HDR));

    sprintf(buf, "%s/0Announce/groups/%s/%s", OLD_BBSHOME, fname, fn_names);
    Names_class(buf);
  }
  else if (flag == 2)		/* 看板 */
  {
    strcpy(hdr.xname, fname);
    str_ncpy(hdr.title, title, sizeof(hdr.title));
    hdr.xmode = GEM_BOARD | GEM_FOLDER;

    sprintf(buf, "gem/@/@%s", tmp_name);
    rec_add(buf, &hdr, sizeof(HDR));

    sprintf(buf, "%s/0Announce/groups/%s/%s/%s", OLD_BBSHOME, tmp_name, fname, fn_names);
    mandex(fname, buf, 0);
  }
}


/* ------------------------------------------------------------- */
/* Names Path ...                                                */
/* ------------------------------------------------------------- */


static void
Names_class(fpath)
  char *fpath;
{
  FILE *fp;
  char *ptr, buf[256];
  char title[TTLEN + 1], fname[32];
  int flag;

  if (fp = fopen(fpath, "r"))
  {
    while (fgets(buf, sizeof(buf), fp))
    {
      if (ptr = strchr(buf, '\n'))
	*ptr = '\0';

      if (!memcmp(buf, "Name=", 5))
      {
	str_ncpy(title, buf + 5, sizeof(title));
      }
      else if (!memcmp(buf, "Path=", 5))
      {
	ptr = buf + 7;
	if (!memcmp(ptr, "../", 3) || (*ptr == '/') || !strcmp(ptr, ".index") || (*ptr == '\0'))
	  continue;

	str_ncpy(fname, ptr, sizeof(fname));
	if (strstr(fname, "GROUP"))
	  flag = 1;
	else
	  flag = 2;

	tran_group(title, fname, flag);
      }
    }

    fclose(fp);
  }
}


/* ------------------------------------------------------------- */
/* Group							 */
/* ------------------------------------------------------------- */


static void
group_class()
{
  char fpath[1024];

  sprintf(fpath, "%s/0Announce/groups/%s", OLD_BBSHOME, fn_names);
  Names_class(fpath);
}


/* ------------------------------------------------------------- */
/* 0Announce							 */
/* ------------------------------------------------------------- */


static int
mandex(board, fpath, deep)
  char *board;
  char *fpath;
  int deep;
{
  FILE *fgem, *fxx;
  char *fname, *gname, *ptr, *str, buf[256], site[64];
  struct stat st;
  int cc, xmode, gport;
  HDR ghdr;

  if (board)
  {
    level = 0;
    if (strcmp(board, NOBOARD))
    {
      sprintf(gpath, "gem/brd/%s", board);
      mkdir(gpath, 0700);
    }
    cc = '0';
    for (;;)
    {
      if (!strcmp(board, NOBOARD))
	sprintf(gpath, "gem/%c", cc);
      else
	sprintf(gpath, "gem/brd/%s/%c", board, cc);
      mkdir(gpath, 0700);
      if (cc == '9')
	cc = '@';
      else
      {
	if (++cc == 'W')
	  break;
      }
    }
    if (!strcmp(board, NOBOARD))
      sprintf(gpath, "gem/.DIR");
    else
      sprintf(gpath, "gem/brd/%s/.DIR", board);
    ndir = nfile = 0;
    chrono = 10000;
  }

  if (!(gname = strrchr(gpath, '/')))
    return -1;
  if (gname[1] == '.')
    gname++;
  else
    gname--;

  if (!(fxx = fopen(gpath, "a")))
    return -1;

  if (!(fgem = fopen(fpath, "r")))
  {
    fclose(fxx);
    return -1;
  }

  if (deep > level)
    level = deep;

  gport = -1;
  fname = strrchr(fpath, '.');

  while (fgets(buf, sizeof(buf), fgem))
  {
    if (gport < 0)
    {
      *fname = xmode = gport = 0;
      memset(&ghdr, 0, sizeof(HDR));
    }
    ptr = buf + 5;
    if (str = strchr(buf, '\n'))
      *str = '\0';
    if (!memcmp(buf, "Name=", 5))
    {
      if (!memcmp(ptr, "⊙ ", 3) || !memcmp(ptr, "↑ ", 3) || !memcmp(ptr, "♂", 3) ||
	!memcmp(ptr, "♀ ", 3) || !memcmp(ptr, "× ", 3))
      {
	ptr += 3;
      }
      else if (!memcmp(ptr, "↓ ", 3) || !memcmp(ptr, "← ", 3))
      {
	ptr += 3;
	gport = 70;
      }
      if (*ptr == '#')
      {
	if (*++ptr == ' ')
	  ptr++;
	xmode = GEM_RESTRICT;
      }
      str_ncpy(ghdr.title, ptr, sizeof(ghdr.title));
    }
    else if (!memcmp(buf, "Edit=", 5))	/* server */
    {
      if (gport)
      {
	str = site;
	do
	{
	  cc = *ptr++;
	  if (cc >= 'A' && cc <= 'Z')
	    cc |= 0x20;
	  *str++ = cc;
	} while (cc);
      }
      else
      {
	str_ncpy(ghdr.owner, ptr, sizeof(ghdr.owner));
      }
    }
    else if (!memcmp(buf, "Date=", 5))	/* port */
    {
      if (gport)
	gport = atoi(ptr);
      else if (ptr[2] != '/' || ptr[5] != '/')
	str_ncpy(ghdr.date, ptr, sizeof(ghdr.date));
    }
    else if (!memcmp(buf, "Path=", 5))
    {
      ptr += 2;
      if (!memcmp(ptr, "../", 3) || (*ptr == '/') || !strcmp(ptr, ".index"))
      {
	printf("\tskip: %s, %s\n", fpath, ptr);
	gport = -1;
	continue;
      }
      if (!memcmp(ptr, "groups", 6) && deep == 0)
      {
	printf("GROUPS! wait...\n");
	group_class();
	gport = -1;
	continue;
      }
      ghdr.chrono = ++chrono;

      if (gport)		/* Gopher 就當成一般文章或卷宗 */
      {
	xmode = 0;
	if (*ptr == '1')
	  xmode |= GEM_FOLDER;
	sprintf(ghdr.xname, "%s/%s", site, ptr);
      }
      else
      {
	strcpy(fname, ptr);
	if (stat(fpath, &st))
	{
	  gport = -1;
	  continue;
	}
	archiv32(chrono, gname + 3);
	*gname = gname[9];
	gname[1] = '/';
	str = ghdr.date;
	if (!*str)
	{
	  struct tm *p = localtime(&st.st_mtime);
	  sprintf(str, "%02d/%02d/%02d", p->tm_year % 100, p->tm_mon + 1, p->tm_mday);
	}
	if (S_ISREG(st.st_mode))/* file */
	{
	  gname[2] = 'A';
	  strcpy(ghdr.xname, gname + 2);
	  link(fpath, gpath);
	  nfile++;
	}
	else if (S_ISDIR(st.st_mode))	/* dir */
	{
	  gname[2] = 'F';
	  strcpy(ghdr.xname, gname + 2);
	  strcat(fpath, "/");
	  strcat(fpath, fn_names);
	  if (mandex(NULL, fpath, deep + 1))
	  {
	    gport = -1;
	    continue;
	  }
	  xmode |= GEM_FOLDER;
	  ndir++;
	}
	else
	{
	  gport = -1;
	  continue;
	}
      }
      ghdr.xmode = xmode;
      fwrite(&ghdr, sizeof(HDR), 1, fxx);
      gport = -1;
    }
  }
  fclose(fxx);
  fclose(fgem);

  if (board)			/* report */
    printf("\td:%d\tf:%d\t(%d)\n", ndir, nfile, level);

  return 0;
}


int
main()
{
  char fpath[1024];

  chdir(BBSHOME);

  new_class();

  sprintf(fpath, "%s/0Announce/%s", OLD_BBSHOME, fn_names);
  mandex(NOBOARD, fpath, 0);

  return 0;
}
