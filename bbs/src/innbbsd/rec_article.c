/*-------------------------------------------------------*/
/* rec_article.c( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : innbbsd receive article			 */
/* create : 95/04/27					 */
/* update :   /  /  					 */
/* author : skhuang@csie.nctu.edu.tw			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#if 0
   收到之文章內容和檔頭分別在
   內文 (body)   在 char *BODY
   檔頭 (header) 在 char *SUBJECT, *FROM, *SITE, *DATE, *PATH, *GROUP, *MSGID, *POSTHOST, *CONTROL;
#endif


#include "innbbsconf.h"
#include "bbslib.h"
#include "inntobbs.h"


/* ----------------------------------------------------- */
/* board：shm 部份須與 cache.c 相容			 */
/* ----------------------------------------------------- */


static BCACHE *bshm;


void
init_bshm()
{
  /* itoc.030727: 在開啟 bbsd 之前，應該就要執行過 account，
     所以 bshm 應該已設定好 */

  bshm = shm_new(BRDSHM_KEY, sizeof(BCACHE));

  if (bshm->uptime <= 0)	/* bshm 未設定完成 */
    exit(0);
}


/* ----------------------------------------------------- */
/* 處理 DATE						 */
/* ----------------------------------------------------- */


#if 0	/* itoc.030303.註解: RFC 822 的 DATE 欄位；RFC 1123 將 year 改成 4-DIGIT */

date-time := [ wday "," ] date time ; dd mm yy, hh:mm:ss zzz 
wday      :=  "Mon" / "Tue" / "Wed" / "Thu" / "Fri" / "Sat" / "Sun" 
date      :=  1*2DIGIT month 4DIGIT ; mday month year
month     :=  "Jan" / "Feb" / "Mar" / "Apr" / "May" / "Jun" / "Jul" / "Aug" / "Sep" / "Oct" / "Nov" / "Dec" 
time      :=  hour zone ; ANSI and Military 
hour      :=  2DIGIT ":" 2DIGIT [":" 2DIGIT] ; 00:00:00 - 23:59:59 
zone      :=  "UT" / "GMT" / "EST" / "EDT" / "CST" / "CDT" / "MST" / "MDT" / "PST" / "PDT" / 1ALPHA / ( ("+" / "-") 4DIGIT )

#endif

static time_t datevalue;

static void
parse_date()		/* 把符合 "dd mmm yyyy hh:mm:ss" 的格式，轉成 time_t */
{
  static char months[12][4] = {"jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec"};
  int i;
  char *ptr, *str, buf[80];
  struct tm ptime;

  str_ncpy(buf, DATE, sizeof(buf));
  str_lower(buf, buf);			/* 通通換小寫，因為 Dec DEC dec 各種都有人用 */

  str = buf + 2;
  for (i = 0; i < 12; i++)
  {
    if (ptr = strstr(str, months[i]))
      break;
  }

  if (ptr)
  {
    ptr[-1] = ptr[3] = ptr[8] = ptr[11] = ptr[14] = ptr[17] = '\0';

    ptime.tm_sec = atoi(ptr + 15);
    ptime.tm_min = atoi(ptr + 12);
    ptime.tm_hour = atoi(ptr + 9);
    ptime.tm_mday = (ptr == buf + 2 || ptr == buf + 7) ? atoi(ptr - 2) : atoi(ptr - 3);	/* RFC 822 允許 mday 是 1- 或 2- DIGIT */
    ptime.tm_mon = i;
    ptime.tm_year = atoi(ptr + 4) - 1900;
    ptime.tm_isdst = 0;
#ifndef CYGWIN
    ptime.tm_zone = "GMT";
    ptime.tm_gmtoff = 0;
#endif

    datevalue = mktime(&ptime);
    str = ptr + 18;
    if (ptr = strchr(str, '+'))
    {
      /* 如果有 +0100 (MET) 等註明時區，先調回 GMT 時區 */
      datevalue -= ((ptr[1] - '0') * 10 + (ptr[2] - '0')) * 3600 + ((ptr[3] - '0') * 10 + (ptr[4] - '0')) * 60;
    }
    else if (ptr = strchr(str, '-'))
    {
      /* 如果有 -1000 (HST) 等註明時區，先調回 GMT 時區 */
      datevalue += ((ptr[1] - '0') * 10 + (ptr[2] - '0')) * 3600 + ((ptr[3] - '0') * 10 + (ptr[4] - '0')) * 60;
    }
    datevalue += 28800;		/* 台灣所在的 CST 時區比 GMT 快八小時 */
  }
  else
  {
    /* 如果分析失敗，那麼拿現在時間來當發文時間 */
    time(&datevalue);
    /* bbslog("<rec_article> :Warn: parse_date 錯誤：%s\n", DATE); */
  }
}


/* ----------------------------------------------------- */
/* process post write					 */
/* ----------------------------------------------------- */


static void
update_btime(brdname)
  char *brdname;
{
  BRD *brdp, *bend;

  brdp = bshm->bcache;
  bend = brdp + bshm->number;
  do
  {
    if (!strcmp(brdname, brdp->brdname))
    {
      brdp->btime = -1;
      break;
    }
  } while (++brdp < bend);
}


static void
bbspost_add(board, addr, nick)
  char *board, *addr, *nick;
{
  int cc;
  char *str;
  char folder[64], fpath[64];
  HDR hdr;
  FILE *fp;

  /* 寫入文章內容 */

  brd_fpath(folder, board, FN_DIR);

  if (fp = fdopen(hdr_stamp(folder, 'A', &hdr, fpath), "w"))
  {
    fprintf(fp, "發信人: %.50s 看板: %s\n", FROM, board);
    fprintf(fp, "標  題: %.70s\n", SUBJECT);
    if (SITE)
      fprintf(fp, "發信站: %.27s (%.40s)\n\n", SITE, DATE);
    else
      fprintf(fp, "發信站: %.40s\n\n", DATE);

    /* chuan: header 跟 body 要空行隔開 */

    /* fprintf(fp, "%s", BODY); */

    for (str = BODY; cc = *str; str++)
    {
      if (cc == '.')
      {
	/* for line beginning with a period, collapse the doubled period to a single one. */
	if (str >= BODY + 2 && str[-1] == '.' && str[-2] == '\n')
	  continue;
      }

      fputc(cc, fp);
    }

    fclose(fp);
  }

  /* 造 HDR */

  hdr.xmode = POST_INCOME;

  /* Thor.980825: 防止字串太長蓋過頭 */
  str_ncpy(hdr.owner, addr, sizeof(hdr.owner));
  str_ncpy(hdr.nick, nick, sizeof(hdr.nick));
  str_stamp(hdr.date, &datevalue);	/* 依 DATE: 欄位的日期，與 hdr.chrono 不同步 */
  str_ncpy(hdr.title, SUBJECT, sizeof(hdr.title));

  rec_bot(folder, &hdr, sizeof(HDR));

  update_btime(board);

  HISadd(MSGID, board, hdr.xname);
}


/* ----------------------------------------------------- */
/* process cancel write					 */
/* ----------------------------------------------------- */


#ifdef _KEEP_CANCEL_
static inline void
move_post(hdr, board, filename)
  HDR *hdr;
  char *board, *filename;
{
  HDR post;
  char folder[64];

  brd_fpath(folder, board, FN_DIR);
  hdr_stamp(folder, HDR_LINK | 'A', &post, filename);
  unlink(filename);

  /* 直接複製 trailing data */

  memcpy(post.owner, hdr->owner, TTLEN + 140);

  sprintf(post.title, "[cancel] %-60.60s", FROM);

  rec_bot(folder, &post, sizeof(HDR));
}
#endif


static void
bbspost_cancel(board, chrono, fpath)
  char *board;
  time_t chrono;
  char *fpath;
{
  HDR hdr;
  struct stat st;
  long size;
  int fd, ent;
  char folder[64];
  off_t off, len;

  /* XLOG("cancel [%s] %d\n", board, time); */

  brd_fpath(folder, board, FN_DIR);
  if ((fd = open(folder, O_RDWR)) == -1)
    return;

  /* flock(fd, LOCK_EX); */
  /* Thor.981205: 用 fcntl 取代flock, POSIX標準用法 */
  f_exlock(fd);

  fstat(fd, &st);
  size = sizeof(HDR);
  ent = ((long) st.st_size) / size;

  /* itoc.030307.註解: 去 .DIR 中藉由比對 chrono 找出是哪一篇 */

  while (1)
  {
    /* itoc.030307.註解: 每 16 篇為一個 block */
    ent -= 16;
    if (ent <= 0)
      break;

    lseek(fd, size * ent, SEEK_SET);
    if (read(fd, &hdr, size) != size)
      break;

    if (hdr.chrono <= chrono)	/* 落在這個 block 裡 */
    {
      do
      {
	if (hdr.chrono == chrono)
	{
	  /* Thor.981014: mark 的文章不被 cancel */
	  if (hdr.xmode & POST_MARKED)
	    break;

#ifdef _KEEP_CANCEL_
	  /* itoc.030613: 保留被 cancel 的文章於 [deleted] */
	  move_post(&hdr, BN_DELETED, fpath);
#else
	  unlink(fpath);
#endif

	  update_btime(board);

	  /* itoc.030307: 被 cancel 的文章不保留 header */

	  off = lseek(fd, 0, SEEK_CUR);
	  len = st.st_size - off;

	  board = (char *) malloc(len);
	  read(fd, board, len);

	  lseek(fd, off - size, SEEK_SET);
	  write(fd, board, len);
	  ftruncate(fd, st.st_size - size);

	  free(board);
	  break;
	}

	if (hdr.chrono > chrono)
	  break;
      } while (read(fd, &hdr, size) == size);

      break;
    }
  }

  /* flock(fd, LOCK_UN); */
  /* Thor.981205: 用 fcntl 取代flock, POSIX標準用法 */
  f_unlock(fd);

  close(fd);
  return;
}


int			/* 0:cancel success  -1:cancel fail */
cancel_article(msgid)
  char *msgid;
{
  int fd;
  char fpath[64], cancelfrom[128], buffer[128];
  char board[BNLEN + 1], xname[9];

  /* XLOG("cancel %s <%s>\n", FROM, msgid); */

  if (!HISfetch(msgid, board, xname))
    return -1;

  str_from(FROM, cancelfrom, buffer);

  /* XLOG("cancel %s (%s)\n", cancelfrom, buffer); */

  sprintf(fpath, "brd/%s/%c/%s", board, xname[7], xname);	/* 去找出那篇文章 */

  /* XLOG("cancel fpath (%s)\n", fpath); */

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    int len;

    len = read(fd, buffer, sizeof(buffer));
    close(fd);

    /* Thor.981221.註解: 外來文章才能被 cancel */
    if ((len > 10) && !memcmp(buffer, "發信人: ", 8))
    {
      char *xfrom, *str;

      xfrom = buffer + 8;
      if (str = strchr(xfrom, ' '))
      {
	*str = '\0';

#ifdef _NoCeM_
	/* gslin.000607: ncm_issuer 可以砍別站發的信 */
	if (strcmp(xfrom, cancelfrom) && !search_issuer(FROM, NULL))
#else
	if (strcmp(xfrom, cancelfrom))
#endif
	{
	  /* itoc.030107.註解: 若 cancelfrom 和本地文章 header 記錄的 xfrom 不同，就是 fake cancel */
	  bbslog("<rec_article> :Warn: 無效的 cancel：%s, sender: %s, path: %s\n", xfrom, FROM, PATH);
	  return -1;
	}

	bbspost_cancel(board, chrono32(xname), fpath);
      }
    }
  }

  return 0;
}


/* ----------------------------------------------------- */
/* check spam rule					 */
/* ----------------------------------------------------- */


static int		/* 1: 符合擋信規則 */
is_spam(board, addr, nick)
  char *board, *addr, *nick;
{
  spamrule_t *spam;
  int i, xmode;
  char *compare, *detail;

  for (i = 0; i < SPAMCOUNT; i++)
  {
    spam = SPAMRULE + i;

    compare = spam->path;
    if (*compare && strcmp(compare, NODENAME))
      continue;

    compare = spam->board;
    if (*compare && strcmp(compare, board))
      continue;

    xmode = spam->xmode;
    detail = spam->detail;

    if (xmode & INN_SPAMADDR)
      compare = addr;
    else if (xmode & INN_SPAMNICK)
      compare = nick;
    else if (xmode & INN_SPAMSUBJECT)
      compare = SUBJECT;
    else if (xmode & INN_SPAMPATH)
      compare = PATH;
    else if (xmode & INN_SPAMMSGID)
      compare = MSGID;
    else if (xmode & INN_SPAMBODY)
      compare = BODY;
    else if (xmode & INN_SPAMSITE && SITE)		/* SITE 可以是 NULL */
      compare = SITE;
    else if (xmode & INN_SPAMPOSTHOST && POSTHOST)	/* POSTHOST 可以是 NULL */
      compare = POSTHOST;
    else
      continue;

    if (str_sub(compare, detail))
      return 1;
  }
  return 0;
}


/* ----------------------------------------------------- */
/* process receive article				 */
/* ----------------------------------------------------- */


#ifndef _NoCeM_
static 
#endif
newsfeeds_t *
search_newsfeeds_bygroup(newsgroup)
  char *newsgroup;
{
  newsfeeds_t nf, *find;

  str_ncpy(nf.newsgroup, newsgroup, sizeof(nf.newsgroup));
  find = bsearch(&nf, NEWSFEEDS_G, NFCOUNT, sizeof(newsfeeds_t), nf_bygroupcmp);
  if (find && !(find->xmode & INN_NOINCOME))
    return find;
  return NULL;
}


int				/* 0:success  -1:fail */
receive_article()
{
  newsfeeds_t *nf;
  char myaddr[128], mynick[128], mysubject[128], myfrom[128], mydate[80];
  char poolx[256];
  char *group;
  int firstboard = 1;

  /* try to split newsgroups into separate group */
  for (group = strtok(GROUP, ","); group; group = strtok(NULL, ","))
  {
    if (!(nf = search_newsfeeds_bygroup(group)))
      continue;

    if (firstboard)	/* opus: 第一個板才需要處理 */
    {
      /* Thor.980825: gc patch: lib/str_decode 只能接受 decode 完 strlen < 256 */ 

      str_ncpy(poolx, SUBJECT, 255);
      str_decode(poolx);
      str_ansi(mysubject, poolx, 70);	/* 70 是 bbspost_add() 標題所需的長度 */
      SUBJECT = mysubject;

      str_ncpy(poolx, FROM, 255);
      str_decode(poolx);
      str_ansi(myfrom, poolx, 128);	/* 雖然 bbspost_add() 發信人所需的長度只需要 50，但是 str_from() 需要長一些 */
      FROM = myfrom;

      /* itoc.030218.註解: 處理「發信站」中的時間 */
      parse_date();
      strcpy(mydate, (char *) Btime(&datevalue));
      DATE = mydate;

      if (*nf->charset == 'g')
      {
	gb2b5(BODY);
	gb2b5(FROM);
	gb2b5(SUBJECT);
	if (SITE)
	  gb2b5(SITE);
      }

      strcpy(poolx, FROM);
      str_from(poolx, myaddr, mynick);

      if (is_spam(nf->board, myaddr, mynick))
      {
#ifdef _KEEP_CANCEL_
	bbspost_add(BN_DELETED, myaddr, mynick);
#endif
	break;
      }

      firstboard = 0;
    }

    bbspost_add(nf->board, myaddr, mynick);
  }		/* for board1,board2,... */

  return 0;
}
