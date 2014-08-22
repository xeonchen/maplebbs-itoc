/*-------------------------------------------------------*/
/* util/poststat.c	( NTHU CS MapleBBS Ver 3.00 )	 */
/*-------------------------------------------------------*/
/* target : 統計今日、週、月、年熱門話題		 */
/* create : 95/03/29				 	 */
/* update : 97/08/29				 	 */
/*-------------------------------------------------------*/
/* syntax : poststat [day]				 */
/*-------------------------------------------------------*/


#include "bbs.h"


static char *myfile[] = {"day", "week", "month", "year"};
static int mycount[4] = {7, 4, 12};
static int mytop[] = {10, 50, 100, 100};
static char *mytitle[] = {"日十", "週五十", "月百", "年度百"};


#define FN_RUN_POST_AUTHOR	"run/var/post.author"
#define FN_RUN_POST_OLD		"run/var/post.old"
#define FN_RUN_DAY_0		"run/var/day.0"
  

#define HASHSIZE		1024		/* 2's power */
#define TOPCOUNT		200
#define	LOWER_BOUND		4


/* POSTLOG + next rec */
static
struct postrec
{
  char author[IDLEN + 1];	/* author name */
  char board[BNLEN + 1];	/* board name */
  char title[66];		/* title name */
  time_t date;			/* last post's date */
  int number;			/* post number */
  struct postrec *next;		/* next rec */
}      *bucket[HASHSIZE];


static POSTLOG top[TOPCOUNT], *tp;          


static int
hash(key)
  char *key;
{
  int i, ch, value = 0;

  for (i = 0; (ch = key[i]) && i < 80; i++)
  {
    value = (value << 5) - value + ch;
  }

  return value;
}


/* ---------------------------------- */
/* hash structure : array + link list */
/* ---------------------------------- */


static void
search(t)
  POSTLOG *t;
{
  struct postrec *p, *q, *s;
  int i, found = 0;

  i = hash(t->title) & (HASHSIZE - 1);
  q = NULL;
  p = bucket[i];
  while (p && (!found))
  {
    if (!strcmp(p->title, t->title) && !strcmp(p->board, t->board))
      found = 1;
    else
    {
      q = p;
      p = p->next;
    }
  }
  if (found)
  {
    p->number += t->number;
    if (p->date < t->date)	/* 取較近日期 */
      p->date = t->date;
  }
  else
  {
    s = (struct postrec *) malloc(sizeof(struct postrec));
    memcpy(s, t, sizeof(POSTLOG));
    s->next = NULL;
    if (q == NULL)
      bucket[i] = s;
    else
      q->next = s;
  }
}


static int
sort(pp, count)
  struct postrec *pp;
  int count;
{
  int i, j;

  for (i = 0; i <= count; i++)
  {
    if (pp->number > top[i].number)
    {
      if (count < TOPCOUNT - 1)
	count++;
      for (j = count - 1; j >= i; j--)
	memcpy(&top[j + 1], &top[j], sizeof(POSTLOG));

      memcpy(&top[i], pp, sizeof(POSTLOG));
      break;
    }
  }
  return count;
}


static void
load_stat(fname)
  char *fname;
{
  FILE *fp;

  if (fp = fopen(fname, "r"))
  {
    int count = fread(top, sizeof(POSTLOG), TOPCOUNT, fp);
    fclose(fp);
    while (count)
      search(&top[--count]);
  }
}


static void
poststat(mytype)
  int mytype;
{
  FILE *fp;
  char buf[40], *p;
  char curfile[80] = FN_RUN_DAY_0;
  struct postrec *pp;
  int i, j;

  if (mytype < 0)
  {
    FILE *fpw;

    /* --------------------------------------- */
    /* load .post and statictic processing     */
    /* --------------------------------------- */

    unlink(FN_RUN_POST_OLD);
    rename(FN_RUN_POST, FN_RUN_POST_OLD);
    if (!(fp = fopen(FN_RUN_POST_OLD, "r")))
      return;

    if (!(fpw = fopen(FN_RUN_POST_AUTHOR, "a")))
    {
      fclose(fp);
      return;
    }

    mytype = 0;
    load_stat(curfile);

    while (fread(top, sizeof(POSTLOG), 1, fp) == 1)
    {
      fwrite(top, sizeof(POSTLOG), 1, fpw);
      search(top);
    }
    fclose(fp);
    fclose(fpw);
  }
  else
  {
    /* ---------------------------------------------- */
    /* load previous results and statictic processing */
    /* ---------------------------------------------- */

    i = mycount[mytype];
    p = myfile[mytype];
    while (i)
    {
      sprintf(buf, "run/var/%s.%d", p, i);
      sprintf(curfile, "run/var/%s.%d", p, --i);
      load_stat(curfile);
      rename(curfile, buf);
    }
    mytype++;
  }

  /* ---------------------------------------------- */
  /* sort top 100 issue and save results            */
  /* ---------------------------------------------- */

  memset(top, 0, sizeof(top));
  for (i = j = 0; i < HASHSIZE; i++)
  {
    for (pp = bucket[i]; pp; pp = pp->next)
    {

#ifdef DEBUG
      printf("Title : %s, Board: %s\nPostNo : %d, Author: %s\n", 
	pp->title, pp->board, pp->number, pp->author);
#endif

      j = sort(pp, j);
    }
  }

  p = myfile[mytype];
  sprintf(curfile, "run/var/%s.0", p);
  if (fp = fopen(curfile, "w"))
  {
    fwrite(top, sizeof(POSTLOG), j, fp);
    fclose(fp);
  }

  /* --------------------------------------------------- */
  /* report file : gem/@/@-				 */
  /* --------------------------------------------------- */

  sprintf(curfile, BBSHOME "/gem/@/@-%s", p);
  if (fp = fopen(curfile, "w"))
  {
    int max, cnt;

    fprintf(fp, "%16s\033[1;34m-----\033[37m=====\033[41m 本%s大熱門話題 \033[40m=====\033[34m-----\033[m\n\n", 
      "", mytitle[mytype]);

    max = mytop[mytype];
    p = buf + 5;
    for (i = cnt = 0; (cnt < max) && (i < j); i++)
    {
      tp = &top[i];
      strcpy(buf, Btime(&(tp->date)));
      buf[23] = '\0';
      fprintf(fp,
	"\033[1;31m%3d. \033[33m看板 : \033[32m%-16s\033[35m《%s》\033[36m%4d 篇\033[33m%16s\n"
	"     \033[33m標題 : \033[0;44;37m%-60.60s\033[40m\n", 
	++cnt, tp->board, p, tp->number, tp->author, tp->title);
    }
    fclose(fp);
  }

  /* free statistics */

  for (i = 0; i < HASHSIZE; i++)
  {
    struct postrec *next;

    for (pp = bucket[i]; pp; pp = next)
    {
      next = pp->next;
      free(pp);
    }
    bucket[i] = NULL;
  }
}


#include "splay.h"


typedef struct PostText
{
  struct PostText *ptnext;
  int count;
  char title[0];
}	PostText;


typedef struct PostAuthor
{
  struct PostAuthor *panext;
  PostText *text;
  int count;
  int hash;
  char author[0];
}	PostAuthor;


static int
pa_cmp(x, y)
 PostAuthor *x;
 PostAuthor *y;
{
  int dif;

  dif = y->count - x->count;
  if (dif)
    return dif;
  return strcmp(x->author, y->author);
}


static void
pa_out(top, fp)
  SplayNode *top;
  FILE *fp;
{
  PostAuthor *pa;
  PostText *text;

  if (top == NULL)
    return;

  pa_out(top->left, fp);

  pa = (PostAuthor *) top->data;
  if (pa->count <= LOWER_BOUND)
    return;

  fprintf(fp, "\n>%6d %s\n", pa->count, pa->author);
  for (text = pa->text; text; text = text->ptnext)
  {
    fprintf(fp, "%7d %.70s\n", text->count, text->title);
  }

  pa_out(top->right, fp);
}


static void
post_author()
{
  int cc, i, len;
  char *str;
  FILE *fp;
  POSTLOG post;
  PostAuthor **paht, *pahe;
  PostText *text;
  SplayNode *patop;

  unlink(FN_RUN_POST_OLD);
  rename(FN_RUN_POST_AUTHOR, FN_RUN_POST_OLD);
  if (!(fp = fopen(FN_RUN_POST_OLD, "r")))
    return;

  paht = (PostAuthor **) calloc(sizeof(PostAuthor *), HASHSIZE);

  while (fread(&post, sizeof(post), 1, fp) == 1)
  {
    cc = hash(str = post.author);
    pahe = paht[i = cc & (HASHSIZE - 1)];

    for (;;)
    {
      if (pahe == NULL)
      {
	len = strlen(str) + 1;
	pahe = (PostAuthor *) malloc(sizeof(PostAuthor) + len);
	pahe->panext = paht[i];
	pahe->text = NULL;
	pahe->count = 1;
	pahe->hash = cc;
	memcpy(pahe->author, str, len);
	paht[i] = pahe;
	break;
      }

      if ((cc == pahe->hash) && !strcmp(str, pahe->author))
      {
        pahe->count++;
	break;
      }

      pahe = pahe->panext;
    }

    text = pahe->text;
    str = post.title;
    for (;;)
    {
      if (text == NULL)
      {
	len = strlen(str) + 1;
	text = (PostText *) malloc(sizeof(PostText) + len + BNLEN + 1);
	text->ptnext = pahe->text;
	text->count = 1;
	sprintf(text->title, "%-13s%s", post.board, str);
	/* memcpy(text->title, str, len); */
	pahe->text = text;
	break;
      }

      if (!strcmp(str, text->title + BNLEN + 1))
      {
	text->count ++;
	break;
      }

      text = text->ptnext;
    }
  }
  fclose(fp);

  /* splay sort */

  patop = NULL;

  for (i = 0; i < HASHSIZE; i++)
  {
    for (pahe = paht[i]; pahe; pahe = pahe->panext)
    {
      patop = splay_in(patop, pahe, pa_cmp);
    }
  }

  /* report */

  if (fp = fopen(FN_RUN_POST_LOG, "w"))
  {
    pa_out(patop, fp);
    fclose(fp);
  }
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  time_t now;
  struct tm *ptime;

  chdir(BBSHOME);
  umask(077);

  if (argc == 2)
  {
    argc = atoi(argv[1]);
    if (argc == 100)
      post_author();
    else
      poststat(argc);
    exit(0);
  }

  time(&now);
  ptime = localtime(&now);
  argc = ptime->tm_hour;

  if (argc == 0)
  {
    if (ptime->tm_mday == 1)
      poststat(2);
    if (ptime->tm_wday == 0)
      poststat(1);
    poststat(0);
  }

  poststat(-1);

  if (argc == 23)
    post_author();

  exit(0);
}
