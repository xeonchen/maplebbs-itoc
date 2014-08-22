/*-------------------------------------------------------*/
/* util/bquota.c	( NTHU CS MapleBBS Ver 3.00 )	 */
/*-------------------------------------------------------*/
/* target : BBS user quota maintain & mail expire	 */
/* create : 95/03/29				 	 */
/* update : 97/03/29				 	 */
/*-------------------------------------------------------*/
/* syntax : bquota					 */
/*-------------------------------------------------------*/


#include "bbs.h"


#ifdef OVERDUE_MAILDEL

#include <sys/resource.h>

#undef	VERBOSE
#define	LOG_FILE	"run/bquota.log"


#define	MAX_SIZE	20000	/* 超過 20k bytes 就刪除 */


/* itoc.011002.註解: XXXX_DUE 在 config.h 中定義 */
/* itoc.011002: 有必要把暑假的 bquota 延長時間嗎？ */

static time_t file_due;
static time_t mail_due;
static time_t mark_due;


static FILE *flog;


/* ----------------------------------------------------- */
/* synchronize folder & files				 */
/* ----------------------------------------------------- */


#define	SYNC_DB_SIZE	1024


typedef struct
{
  time_t chrono;
  char exotic;
}      SyncMail;


static SyncMail *xlist;
static int xsize;


static int
sync_cmp(s1, s2)
  SyncMail *s1, *s2;
{
  return s1->chrono - s2->chrono;
}


static int
bquota(userid)
  char *userid;
{
  HDR hdr;
  char *fname, *str, fpath[80], fnew[80], fold[80];
  int bonus, cc, count, xmode;
  time_t due, chrono;
  FILE *fpr, *fpw;
  struct stat st;
  DIR *dirp;
  struct dirent *de;

  SyncMail *tlist, *tsync;
  int tsize;

  bonus = 0;
  sprintf(fpath, "%s/@", userid);

  if (!(dirp = opendir(fpath)))
  {
    fprintf(flog, "dir: %s\n", fpath);
    rmdir(userid);
    return bonus;
  }

  fname = strchr(fpath, '@') + 1;
  *fname++ = '/';
  count = 0;

  tlist = xlist;
  tsize = xsize;

  while (de = readdir(dirp))
  {
    str = de->d_name;
    cc = *str;
    if (cc == '.')
      continue;

    strcpy(fname, str);

    if (cc != '@')
    {
      unlink(fpath);
#ifdef VERBOSE
      fprintf(flog, "NAME: %s\n", fpath);
#endif
      continue;
    }

    chrono = chrono32(str);

    if (chrono < mark_due)
    {
#ifdef VERBOSE
      fprintf(flog, "OLD : %s\n", fpath);
#endif
      unlink(fpath);
      bonus++;
      continue;
    }

    if (chrono < file_due)
    {
      if (!stat(fpath, &st) && (st.st_size >= MAX_SIZE || st.st_size <= 0))
      {
#ifdef VERBOSE
	fprintf(flog, "BIG : %s\n", fpath);
#endif
	unlink(fpath);
	bonus++;
	continue;
      }
    }

    if (count >= tsize)
    {
      xsize = (tsize += count);
      xlist = tlist = (SyncMail *) realloc(tlist, sizeof(SyncMail) * tsize);
    }
    tlist[count].chrono = chrono;
    tlist[count].exotic = 1;
    count++;
  }
  closedir(dirp);

  if (count > 1)
    qsort(tlist, count, sizeof(SyncMail), sync_cmp);

  sprintf(fold, "%s/.DIR", userid);

  if (fpr = fopen(fold, "r"))
  {
    cc = 0;
    if (fpw = (FILE *) f_new(fold, fnew))
    {
      while (fread(&hdr, sizeof(HDR), 1, fpr) == 1)
      {
	tsync = (SyncMail *) bsearch(&hdr.chrono,
	  tlist, count, sizeof(SyncMail), sync_cmp);

	if (!tsync)
	  continue;

	tsync->exotic = 0;

	xmode = hdr.xmode;

	if (xmode & MAIL_MARKED)
	  due = mark_due;
	else
	  due = mail_due;

	if (hdr.chrono < due)
	{
	  strcpy(fname, hdr.xname);
	  unlink(fpath);
	  bonus++;
#ifdef VERBOSE
	  fprintf(flog, "DUE : %s\n", fpath);
#endif
	}
	else
	{
	  if (fwrite(&hdr, sizeof(HDR), 1, fpw) != 1)
	  {
	    cc = count = -1;
	    break;
	  }
	  cc++;
	}
      }

      fclose(fpw);
      if (count < 0)
	unlink(fnew);
    }
    fclose(fpr);

    if (cc > 0)
      rename(fnew, fold);
    else
      unlink(fold);
  }

  *fname++ = '@';
  for (cc = 0; cc < count; cc++)
  {
    if (tlist[cc].exotic)
    {
      archiv32(tlist[cc].chrono, fname);
      unlink(fpath);
      bonus++;
#ifdef VERBOSE
      fprintf(flog, "SYNC: %s\n", fpath);
#endif
    }
  }

  /* maintain userid/... */

  fname -= 3;
  *fname = '\0';
  if (dirp = opendir(fpath))
  {
    due = file_due;
    while (de = readdir(dirp))
    {
      str = de->d_name;

      if (str[0] == 'b' && str[1] == 'u')	/* buf.? */
      {
	strcpy(fname, str);

	stat(fpath, &st);
	if ((st.st_mtime < due) || (st.st_size > 3000) || (st.st_size <= 0))
	{
#ifdef VERBOSE
	  fprintf(flog, "FILE: %s\n", fpath);
#endif
	  unlink(fpath);
	  bonus++;
	  continue;
	}
      }
      else if (!strcmp(str, "log"))
      {
	strcpy(fname, str);
	stat(fpath, &st);
	if ((st.st_mtime < due) || (st.st_size <= 0))
	{
	  unlink(fpath);
	  bonus++;
	  continue;
	}

#define	ULOG_SIZE 	3072

	if ((st.st_size >= ULOG_SIZE) && (cc = open(fpath, O_RDWR)) >= 0)
	{
	  char buf[ULOG_SIZE];

	  lseek(cc, st.st_size - ULOG_SIZE, 0);
	  count = read(cc, buf, sizeof(buf));
	  if (str = strchr(buf, '\n'))
	  {
	    str++;
	    count -= str - buf;
	    lseek(cc, 0, 0);
	    write(cc, str, count);
	    ftruncate(cc, count);
	    bonus++;
	  }
	  close(cc);
	}

#undef	ULOG_SIZE

	continue;
      }
    }

    closedir(dirp);
  }

  return bonus;
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  DIR *dirp;
  struct dirent *de;
  char *fname, fpath[64];
  time_t start, end;
  int bonus, visit;
  struct rlimit rl;

  setuid(BBSUID);
  setgid(BBSGID);

  chdir(BBSHOME);

  rl.rlim_cur = rl.rlim_max = 0;
  setrlimit(RLIMIT_CORE, &rl);
  time(&start);

  if (argc == 2 && (argc = argv[1][0]) >= 'a' && argc <= 'z')
  {
    flog = stderr;
  }
  else
  {
    flog = fopen(LOG_FILE, "w");
    argc = 'a' + (start / 86400) % 26;	/* 每隔 26 天 bquota 一次 */
  }

  /* visit the second hierarchy */

  sprintf(fpath, "usr/%c", argc);
  fprintf(flog, "# visit: %s\n\n", fpath);

  if (chdir(fpath) || (!(dirp = opendir("."))))
  {
    fprintf(flog, "## unable to enter user home\n");
    fclose(flog);
    exit(-1);
  }

  /* traverse user home */

  file_due = start - FILE_DUE * 86400;
  mail_due = start - MAIL_DUE * 86400;
  mark_due = start - MARK_DUE * 86400;

  xlist = (SyncMail *) malloc(SYNC_DB_SIZE * sizeof(SyncMail));
  xsize = SYNC_DB_SIZE;

  bonus = 0;
  visit = 0;

  fprintf(flog, "\nbegin\n");

  while (de = readdir(dirp))
  {
    fname = de->d_name;
    if (*fname != '.' && *fname > ' ')
    {
#ifdef VERBOSE
      fprintf(flog, "\n[%s]\n", fname);
#endif
      bonus += bquota(fname);
      visit++;
    }
  }
  closedir(dirp);

  time(&end);
  fprintf(flog, "# 開始時間：%s\n", Btime(&start));
  fprintf(flog, "# 結束時間：%s\n", Btime(&end));
  end -= start;
  start = end % 60;
  end /= 60;
  fprintf(flog, "# 總計耗時：%d:%d:%d\n", end / 60, end % 60, start);
  fprintf(flog, "# 註冊人數：%d\n", visit);
  fprintf(flog, "# 清除檔案：%d\n", bonus);
  fclose(flog);
  exit(0);
}

#else
int
main()
{
  printf("You should define OVERDUE_MAILDEL first.\n");
  return -1;
}
#endif	/* HAVE_NETTOOL */
