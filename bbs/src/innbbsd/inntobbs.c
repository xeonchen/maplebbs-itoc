/*-------------------------------------------------------*/
/* inntobbs.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : innbbsd INN to BBS				 */
/* create : 95/04/27					 */
/* update :   /  /  					 */
/* author : skhuang@csie.nctu.edu.tw			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "innbbsconf.h"
#include "bbslib.h"
#include "inntobbs.h"


typedef struct Header
{
  char *name;
  int id;
}      header_t;


enum HeaderValue	/* 所有有用到的 header */
{
  SUBJECT_H,
  FROM_H,
  DATE_H,
  PATH_H,
  GROUP_H,
  MSGID_H,

  SITE_H,
  POSTHOST_H,
  CONTROL_H,

  LASTHEADER
};


/* 只對這些檔頭有興趣 */
static header_t headertable[LASTHEADER] = 
{
  "Subject",			SUBJECT_H,
  "From",			FROM_H,
  "Date",			DATE_H,
  "Path",			PATH_H,
  "Newsgroups",			GROUP_H,
  "Message-ID",			MSGID_H,

  /* SITE_H (含) 以下為非必備檔頭 */
  "Organization",		SITE_H,
  "NNTP-Posting-Host",		POSTHOST_H,
  "Control",			CONTROL_H,
};


char *NODENAME;
char *BODY;
char *SUBJECT, *FROM, *DATE, *PATH, *GROUP, *MSGID, *POSTHOST, *SITE, *CONTROL;


static int 
header_cmp(a, b)
  header_t *a, *b;
{
  return str_cmp(a->name, b->name);
}


static int 
header_value(inputheader)
  char *inputheader;
{
  header_t key, *findkey;
  static int already_init = 0;

  if (!already_init)
  {
    qsort(headertable, sizeof(headertable) / sizeof(header_t), sizeof(header_t), header_cmp);
    already_init = 1;
  }

  key.name = inputheader;
  findkey = bsearch(&key, (char *) headertable, sizeof(headertable) / sizeof(header_t), sizeof(key), header_cmp);
  if (findkey != NULL)
    return findkey->id;

  return -1;
}


static int
is_loopback(path, token, len)
  char *path, *token;
  int len;
{
  int cc;

  if (!path)		/* 若沒有 PATH 則不檢查 */
    return 0;

  for (;;)
  {
    cc = path[len];
    if ((!cc || cc == '!') && !str_ncmp(path, token, len))
      return 1;

    for (;;)
    {
      cc = *path;
      if (!cc)
	return 0;
      path++;
      if (cc == '!')
	break;
    }
  }

  return 0;
}


int 			/* 1:成功 0:PATH包括自己 -1:檔頭不完整 */
readlines(data)		/* 讀入檔頭和內文 */
  char *data;
{
  int i;
  char *front, *ptr, *hptr;
  static char *HEADER[LASTHEADER];

  for (i = 0; i < LASTHEADER; i++)
    HEADER[i] = NULL;
  BODY = NULL;

  ptr = data;

  for (;;)
  {
    front = ptr + 1;
    if (*front == '\n')
    {
      /* skip leading empty lines */
      do
      {
	front++;
      } while (*front == '\n');

      BODY = front;
      break;
    }

    ptr = (char *) strchr(front, '\n');
    if (!ptr)
      break;
    *ptr = '\0';

    hptr = (char *) strchr(front, ':');
    if (hptr && hptr[1] == ' ')
    {
      *hptr = '\0';

      i = header_value(front);
      if (i >= 0)		/* 是有興趣的檔頭 */
      {
	HEADER[i] = hptr + 2;

	/* merge multi-line header */

	hptr = ptr;

	/* while (ptr[1] == ' ') */
        /* Thor.990110: 有的是用 \t 代表還有 */
	while (ptr[1] == ' ' || ptr[1] == '\t')
	{
          /* while (*++ptr == ' ') ; */
          do 
          {
            ++ptr;
          } while (*ptr == ' ' || *ptr == '\t');

	  for (;;)
	  {
	    i = *ptr;
	    if (i == '\n')
	      break;
	    if (i == '\0')
	    {
	      ptr--;
	      break;
	    }
	    ptr++;
	    *hptr++ = i;
	  }

	  *hptr = '\0';
	}

	/* well, ptr point to end of line */
      }
    }

    front = ptr;
  }

  /* 檢查檔頭欄位是否完整 */
  for (i = 0; i < POSTHOST_H; i++)	/* POSTHOST_H (含) 以下為非必備檔頭 */
  {
    if (!HEADER[i] || !*HEADER[i])
      return -1;
  }
  if (!BODY || !*BODY)
    return -1;

  SUBJECT = HEADER[SUBJECT_H];
  FROM = HEADER[FROM_H];
  DATE = HEADER[DATE_H];
  PATH = HEADER[PATH_H];
  GROUP = HEADER[GROUP_H];
  MSGID = HEADER[MSGID_H];
  SITE = HEADER[SITE_H];
  POSTHOST = HEADER[POSTHOST_H];
  CONTROL = HEADER[CONTROL_H];

  /* SITE_H (含) 以下為非必備檔頭，但仍要檢查是否為空字串 */
  if (SITE && !*SITE)
    SITE = NULL;
  if (POSTHOST && !*POSTHOST)
    POSTHOST = NULL;
  if (CONTROL && !*CONTROL)
    return -1;

  if (!CONTROL)		/* 一般信件 */
  {
    /* itoc.030223.註解: 看到 path 裡面有自己站的名稱以後，信就不會進來，
       避免站上的信被 bbslink 送去 news server 以後，又被自己用 bbsnnrp 取回 */
    if (is_loopback(PATH, MYBBSID, strlen(MYBBSID)))
      return 0;
  }

  return 1;
}
