/*-------------------------------------------------------*/
/* bbslib.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : innbbsd library				 */
/* create : 95/04/27					 */
/* update :   /  /  					 */
/* author : skhuang@csie.nctu.edu.tw			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "innbbsconf.h"
#include "bbslib.h"

#ifdef _NoCeM_
#include "nocem.h"
#endif

#include <stdarg.h>


/* ----------------------------------------------------- */
/* read nodelist.bbs					 */
/* ----------------------------------------------------- */


int NLCOUNT;
nodelist_t *NODELIST = NULL;


int
nl_bynamecmp(a, b)
  nodelist_t *a, *b;
{
  return str_cmp(a->name, b->name);
}


static int		/* 0:success  -1:fail */
read_nodelist()
{
  int fd, size;
  struct stat st;

  if ((fd = open("innd/nodelist.bbs", O_RDONLY)) < 0)
    return -1;

  fstat(fd, &st);
  if ((size = st.st_size) <= 0)
  {
    close(fd);
    return -1;
  }
  NODELIST = !NODELIST ? (nodelist_t *) malloc(size) : (nodelist_t *) realloc(NODELIST, size);
  read(fd, NODELIST, size);
  close(fd);

  NLCOUNT = size / sizeof(nodelist_t);
  if (NLCOUNT > 1)
  {
    /* 將 NODELIST[] 依 name 排序，那麼在 search_nodelist_byname() 可以找快一點 */
    qsort(NODELIST, NLCOUNT, sizeof(nodelist_t), nl_bynamecmp);
  }

  return 0;
}


/* ----------------------------------------------------- */
/* read newsfeeds.bbs					 */
/* ----------------------------------------------------- */


int NFCOUNT;
newsfeeds_t *NEWSFEEDS = NULL;
newsfeeds_t *NEWSFEEDS_B = NULL;
newsfeeds_t *NEWSFEEDS_G = NULL;


int
nf_byboardcmp(a, b)
  newsfeeds_t *a, *b;
{
  return str_cmp(a->board, b->board);
}


int
nf_bygroupcmp(a, b)
  newsfeeds_t *a, *b;
{
  return str_cmp(a->newsgroup, b->newsgroup);
}


static int		/* 0:success  -1:fail */
read_newsfeeds()
{
  int fd, size;
  struct stat st;

  if ((fd = open("innd/newsfeeds.bbs", O_RDONLY)) < 0)
    return -1;

  fstat(fd, &st);
  if ((size = st.st_size) <= 0)
  {
    close(fd);
    return -1;
  }
  NEWSFEEDS = !NEWSFEEDS ? (newsfeeds_t *) malloc(size) : (newsfeeds_t *) realloc(NEWSFEEDS, size);
  read(fd, NEWSFEEDS, size);
  close(fd);

  /* 另外準備二份相同的資訊，但是排序方法不同 */
  NEWSFEEDS_B = !NEWSFEEDS_B ? (newsfeeds_t *) malloc(size) : (newsfeeds_t *) realloc(NEWSFEEDS_B, size);
  memcpy(NEWSFEEDS_B, NEWSFEEDS, size);
  NEWSFEEDS_G = !NEWSFEEDS_G ? (newsfeeds_t *) malloc(size) : (newsfeeds_t *) realloc(NEWSFEEDS_G, size);
  memcpy(NEWSFEEDS_G, NEWSFEEDS, size);

  NFCOUNT = size / sizeof(newsfeeds_t);
  if (NFCOUNT > 1)
  {
    /* NEWSFEEDS[] 不變動，預設依站台名稱排序 */

    /* 將 NEWSFEEDS_B[] 依 board 排序，那麼在 search_newsfeeds_byboard() 可以找快一點 */
    qsort(NEWSFEEDS_B, NFCOUNT, sizeof(newsfeeds_t), nf_byboardcmp);

    /* 將 NEWSFEEDS_G[] 依 group 排序，那麼在 search_newsfeeds_bygroup() 可以找快一點 */
    qsort(NEWSFEEDS_G, NFCOUNT, sizeof(newsfeeds_t), nf_bygroupcmp);
  }

  return 0;
}


#ifdef _NoCeM_
/* ----------------------------------------------------- */
/* read ncmperm.bbs					 */
/* ----------------------------------------------------- */


ncmperm_t *NCMPERM = NULL;
int NCMCOUNT = 0;


int			/* 0:success  -1:fail */
read_ncmperm()
{
  int fd, size;
  struct stat st;

  if ((fd = open("innd/ncmperm.bbs", O_RDONLY)) < 0)
    return -1;

  fstat(fd, &st);
  if ((size = st.st_size) <= 0)
  {
    close(fd);
    return -1;
  }
  NCMPERM = !NCMPERM ? (ncmperm_t *) malloc(size) : (ncmperm_t *) realloc(NCMPERM, size);
  read(fd, NCMPERM, size);
  close(fd);

  NCMCOUNT = size / sizeof(ncmperm_t);

  return 0;
}
#endif	/* _NoCeM_ */


/* ----------------------------------------------------- */
/* read spamrule.bbs					 */
/* ----------------------------------------------------- */


spamrule_t *SPAMRULE = NULL;
int SPAMCOUNT = 0;


static int		/* 0:success  -1:fail */
read_spamrule()
{
  int fd, size;
  struct stat st;
  spamrule_t *spam;
  char *detail;

  if ((fd = open("innd/spamrule.bbs", O_RDONLY)) < 0)
    return -1;

  fstat(fd, &st);
  if ((size = st.st_size) <= 0)
  {
    close(fd);
    return -1;
  }
  SPAMRULE = !SPAMRULE ? (spamrule_t *) malloc(size) : (spamrule_t *) realloc(SPAMRULE, size);
  read(fd, SPAMRULE, size);
  close(fd);

  SPAMCOUNT = size / sizeof(spamrule_t);
  for (fd = 0; fd < SPAMCOUNT; fd++)
  {
    /* 將 SPAMRULE[] 都變成小寫，這樣比對時就可以大小寫通吃 */
    spam = SPAMRULE + fd;
    detail = spam->detail;
    str_lowest(detail, detail);
  }

  return 0;
}


/* ----------------------------------------------------- */
/* initail INNBBSD					 */
/* ----------------------------------------------------- */


int			/* 1:success  0:failure */
initial_bbs()
{
  chdir(BBSHOME);		/* chdir to bbs_home first */

  /* 依序載入 nodelist.bbs、newsfeeds.bbs、ncmperm.bbs、spamrule.bbs */

  if (read_nodelist() < 0)
  {
    printf("請檢查 nodelist.bbs，無法讀檔\n");
    return 0;
  }

  if (read_newsfeeds() < 0)
  {
    printf("請檢查 newsfeeds.bbs，無法讀檔\n");
    return 0;
  }

#ifdef _NoCeM_
  if (read_ncmperm() < 0)
  {
    printf("請檢查 ncmperm.bbs，無法讀檔；如果您不想設定 NoCeM，那麼請忽略此訊息\n");
    /* return 0; */	/* ncmperm.bbs 可以是空的 */
  }
#endif

  if (read_spamrule() < 0)
  {
    printf("請檢查 spamrule.bbs，無法讀檔；如果您不想設定擋信規則，那麼請忽略此訊息\n");
    /* return 0; */	/* spamrule.bbs 可以是空的 */
  }

  return 1;
}


/* ----------------------------------------------------- */
/* log function						 */
/* ----------------------------------------------------- */


void
bbslog(char *fmt, ...)
{
  va_list args;
  char datebuf[40];
  time_t now;
  FILE *fp;

  if (fp = fopen(LOGFILE, "a"))
  {
    time(&now);
    strftime(datebuf, sizeof(datebuf), "%b %d %X ", localtime(&now));
    fprintf(fp, "%s ", datebuf);

    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);

    fclose(fp);
  }
}
