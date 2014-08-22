/*-------------------------------------------------------*/
/* util/gem-index.c	( NTHU CS MapleBBS Ver 2.39 )	 */
/*-------------------------------------------------------*/
/* target : 精華區索引程式 (man index)			 */
/* create : 95/03/29				 	 */
/* update : 95/08/08				 	 */
/*-------------------------------------------------------*/
/* syntax : gem-index [board]				 */
/*          [board] 有值 ==> 只跑該 board		 */
/*           空的 ==> 所有的 boards 都跑		 */
/*-------------------------------------------------------*/


#include	"bbs.h"

#define	COLOR_INDEX	/* Thor.980307: 加上顏色試試是否比較易找 */


#define GINDEX_LOG      (BBSHOME "/run/gindex.log")


#define	CHRONO_INDEX	1
#define	CHRONO_LOG	2


static char fn_index[] = "@/@index";
static char fn_log[] = "@/@log";


static int gem_default;
static int ndir;
static int nfile;
static char pgem[128], pndx[128], pool[128];
static FILE *flog;


/* visit the hierarchy recursively */


static void
gindex(level, toc, fpath, fndx)
  int level;
  char *toc;
  char *fpath;
  FILE *fndx;
{
  int count, xmode;
  char *fname, *ptr, buf[128];
  FILE *fgem;
  HDR hdr;

  if (level > 7)		/* endless loop ? */
  {
    fprintf(flog, "level: %d [%s]\n", level, fpath);
    return;
  }

  if (!level)
  {
    fprintf(flog, "%-14s", fpath);	/* report */
    sprintf(pool, "%s/.DIR", fpath);
    fpath = pool;
    strcpy(pgem, fpath);
  }

  if (!(fgem = fopen(fpath, "r")))
    return;

  fname = fpath;
  while (xmode = *fname++)
  {
    if (xmode == '/')
      ptr = fname;
  }
  if (*ptr != '.')
    ptr -= 2;
  fname = ptr;

  if (!fndx)
  {
    strcpy(fname, "@/@ing--");
    fndx = fopen(fpath, "w");
    if (!fndx)
    {
      fclose(fgem);
      return;
    }
    fprintf(fndx, "序號\t\t\t精華區主題\n"
      "-------------------------------------------------------------\n");
    strcpy(pndx, fpath);
    gem_default = ndir = nfile = 0;
  }

  count = 0;
  while (fread(&hdr, sizeof(hdr), 1, fgem) == 1)
  {
    count++;
    xmode = hdr.xmode;

    /* 檢查是否為索引、異動 */
    if (!level && hdr.chrono <= CHRONO_LOG)
    {
      gem_default |= hdr.chrono;
      continue;
    }

    if (xmode & GEM_FOLDER)
      ndir++;
    else
      nfile++;

    if (xmode & GEM_RESTRICT)
      continue;

    sprintf(buf, "%.*s%3d. ", level * 4, toc, count);

#ifdef COLOR_INDEX
    /* Thor.980307: 加上顏色試試是否比較易找 */
    if (xmode & GEM_FOLDER) 
      fprintf(fndx, "%s\033[1;37;%dm%s\033[m\n", buf, 41 + (level % 6) , hdr.title);
    else
      fprintf(fndx, "%s%s\n", buf, hdr.title);
#else
    if (xmode & GEM_FOLDER) 
      fprintf(fndx, "*%s%s\n", buf + 1, hdr.title);
    else
      fprintf(fndx, "%*d. %s\n", 4 * level + 3, count, hdr.title);
#endif

    if ((xmode & (GEM_FOLDER | GEM_BOARD)) == GEM_FOLDER)	/* 一般卷宗 */
    {
      ptr = hdr.xname;		/* F1234567 */
      sprintf(fname, "%c/%s", (*ptr == '@' ? '@' : ptr[7]), ptr);
      gindex(level + 1, buf, fpath, fndx);
    }
  }

  if (!level)
  {
    fclose(fndx);
    strcpy(fname, fn_index);
    rename(pndx, fpath);

    /* report */

    fprintf(flog, "==> d: %d\tf: %d\n", ndir, nfile);

    xmode = gem_default;
    if (xmode != (CHRONO_INDEX | CHRONO_LOG))	/* 已有索引及異動 */
    {
      sprintf(pool, "%s.o", pgem);
      sprintf(pndx, "%s.n", pgem);
      if (fndx = fopen(pndx, "w"))
      {
	memset(&hdr, 0, sizeof(HDR));
	hdr.xmode = GEM_RESERVED;
	strcpy(hdr.owner, SYSOPNICK);

	if (!(xmode & CHRONO_INDEX))
	{
	  hdr.chrono = CHRONO_INDEX;
	  strcpy(hdr.xname, fn_index + 2);
	  strcpy(hdr.title, "精華區索引");
	  fwrite(&hdr, sizeof(hdr), 1, fndx);
	}

	if (!(xmode & CHRONO_LOG))
	{
	  hdr.chrono = CHRONO_LOG;
	  strcpy(hdr.xname, fn_log + 2);
	  strcpy(hdr.title, "精華區異動");
	  fwrite(&hdr, sizeof(hdr), 1, fndx);
	}

	fseek(fgem, (off_t) 0, SEEK_SET);

	while (fread(&hdr, sizeof(hdr), 1, fgem) == 1)
	{
	  fwrite(&hdr, sizeof(hdr), 1, fndx);
	}

	fclose(fndx);
	fclose(fgem);
	rename(pgem, pool);
	rename(pndx, pgem);
	return;
      }
    }
  }

  fclose(fgem);
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  DIR *dirp;
  struct dirent *de;
  char *fname, fpath[80];

  umask(077);
  chdir(BBSHOME "/gem");

  if (argc > 1)
  {
    flog = stderr;
    /* 避免輸入過長字串 嚴謹一點的話 應該要再檢查合法字元 */
    if (strlen(argv[1]) > BNLEN)
      return -1;
    sprintf(fpath, "brd/%s", argv[1]);
    gindex(0, "", fpath, NULL);
    exit(0);
  }

  if (!(flog = fopen(GINDEX_LOG, "w")))
    exit(-1);

  /* visit the top folder */

  gindex(0, "", ".", NULL);

  /* visit the second hierarchy for all boards */

  strcpy(fpath, "brd");
  if (!(dirp = opendir(fpath)))
  {
    fprintf(flog, "## unable to visit [gem/brd]\n");
    fclose(flog);
    exit(-1);
  }

  fpath[3] = '/';
  while (de = readdir(dirp))
  {
    fname = de->d_name;
    if (fname[1] && *fname != '.')
    {
      strcpy(fpath + 4, fname);
      gindex(0, "", fpath, NULL);
    }
  }
  closedir(dirp);
  fclose(flog);
  exit(0);
}
