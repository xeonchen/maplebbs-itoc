/*-------------------------------------------------------*/
/* xover.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : board/mail interactive reading routines 	 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef MY_FAVORITE
#define MSG_ZONE_SWITCH	"快速切換：(A)精華 (B)文章 (C)看板 (F)最愛 (M)信件 (U)使用者 (W)水球："
#else
#define MSG_ZONE_SWITCH	"快速切換：(A)精華 (B)文章 (C)看板 (M)信件 (U)使用者 (W)水球："
#endif


/* ----------------------------------------------------- */
/* keep xover record					 */
/* ----------------------------------------------------- */


static XO *xo_root;		/* root of overview list */


XO *
xo_new(path)
  char *path;
{
  XO *xo;
  int len;

  len = strlen(path) + 1;

  xo = (XO *) malloc(sizeof(XO) + len);

  memcpy(xo->dir, path, len);

  return xo;
}


XO *
xo_get(path)
  char *path;
{
  XO *xo;

  for (xo = xo_root; xo; xo = xo->nxt)
  {
    if (!strcmp(xo->dir, path))
      return xo;
  }

  xo = xo_new(path);
  xo->nxt = xo_root;
  xo_root = xo;
  xo->xyz = NULL;
  xo->pos = XO_TAIL;		/* 第一次進入時，將游標放在最後面 */

  return xo;
}


#ifdef AUTO_JUMPPOST
XO *
xo_get_post(path, brd)		/* itoc.010910: 參考 xover.c xo_get()，為 XoPost 量身打造 */
  char *path;
  BRD *brd;
{
  XO *xo;
  time_t chrono;
  int fd;
  int pos, locus, mid;	/* locus:左指標 mid:中指標 pos:右指標 */

  for (xo = xo_root; xo; xo = xo->nxt)
  {
    if (!strcmp(xo->dir, path))
      return xo;
  }
   
  xo = xo_new(path);
  xo->nxt = xo_root;
  xo_root = xo;
  xo->xyz = NULL;

  /* 尚未更新 brd->blast 或 最後一篇已讀 或 只有一篇，則游標直接放最後 */
  if (brd->btime < 0 || !brh_unread(brd->blast) || 
    (pos = rec_num(path, sizeof(HDR))) <= 1 || (fd = open(path, O_RDONLY)) < 0)
  {
    xo->pos = XO_TAIL;	/* 游標放在最後面 */
    return xo;
  }

  /* 找第一篇未讀 binary search */
  pos--;
  locus = 0;
  while (1)
  {
    if (pos <= locus + 1)
      break;

    mid = locus + ((pos - locus) >> 1);
    lseek(fd, (off_t) (sizeof(HDR) * mid), SEEK_SET);
    if (read(fd, &chrono, sizeof(time_t)) == sizeof(time_t))
    {
      if (brh_unread(chrono))
	pos = mid;
      else
	locus = mid;
    }
    else
    {
      break;
    }
  }

  /* 特例: 如果右指標停留在 1，有二種可能，一是恰讀到第二篇，另一為連第一篇都沒讀 */
  if (pos == 1)
  {
    /* 檢查第一篇是否已讀 */
    lseek(fd, (off_t) 0, SEEK_SET);
    if (read(fd, &chrono, sizeof(time_t)) == sizeof(time_t))
    {
      if (brh_unread(chrono))	/* 若連第一篇也未讀，pos 調回去第一篇 */
	pos = 0;
    }
  }

  close(fd);
  xo->pos = pos;	/* 第一次進入時，將游標放在第一篇未讀 */

  return xo;
}
#endif


#if 0
void
xo_free(xo)
  XO *xo;
{
  char *ptr;

  if (ptr = xo->xyz)
    free(ptr);
  free(xo);
}
#endif


/* ----------------------------------------------------- */
/* interactive menu routines			 	 */
/* ----------------------------------------------------- */


char xo_pool[(T_LINES - 4) * XO_RSIZ];	/* XO's data I/O pool */


void
xo_load(xo, recsiz)
  XO *xo;
  int recsiz;
{
  int fd, max;

  max = 0;
  if ((fd = open(xo->dir, O_RDONLY)) >= 0)
  {
    int pos, top;
    struct stat st;

    fstat(fd, &st);
    max = st.st_size / recsiz;
    if (max > 0)
    {
      pos = xo->pos;
      if (pos <= 0)
      {
	pos = top = 0;
      }
      else
      {
	top = max - 1;
	if (pos > top)
	  pos = top;
	top = (pos / XO_TALL) * XO_TALL;
      }
      xo->pos = pos;
      xo->top = top;

      lseek(fd, (off_t) (recsiz * top), SEEK_SET);
      read(fd, xo_pool, recsiz * XO_TALL);
    }
    close(fd);
  }

  xo->max = max;
}


int					/* XO_LOAD:刪除  XO_FOOT:取消 */
xo_rangedel(xo, size, fchk, fdel)	/* itoc.031001: 區段刪除 */
  XO *xo;
  int size;
  int (*fchk) ();			/* 檢查區段中是否有被保護的記錄  0:刪除 1:保護 */
  void (*fdel) ();			/* 除了刪除記錄以外，還要做些什麼事 */
{
  char ans[8];
  int head, tail;

  vget(b_lines, 0, "[設定刪除範圍] 起點：", ans, 6, DOECHO);
  head = atoi(ans);
  if (head <= 0)
  {
    zmsg("起點有誤");
    return XO_FOOT;
  }

  vget(b_lines, 28, "終點：", ans, 6, DOECHO);
  tail = atoi(ans);
  if (tail < head)
  {
    zmsg("終點有誤");
    return XO_FOOT;
  }

  if (vget(b_lines, 41, msg_sure_ny, ans, 3, LCECHO) == 'y')
  {
    int fd, total;
    char *data, *phead, *ptail;
    struct stat st;

    if ((fd = open(xo->dir, O_RDONLY)) < 0)
      return XO_FOOT;

    fstat(fd, &st);
    total = st.st_size;
    head = (head - 1) * size;
    tail = tail * size;
    if (head > total)
    {
      close(fd);
      return XO_FOOT;
    }
    if (tail > total)
      tail = total;

    data = (char *) malloc(total);
    read(fd, data, total);
    close(fd);

    total -= tail;
    phead = data + head;
    ptail = data + tail;

    if (fchk || fdel)
    {
      char *ptr;
      for (ptr = phead; ptr < ptail; ptr += size)
      {
	if (fchk && fchk(ptr))		/* 這筆記錄保護不被砍 */
	{
	  memcpy(phead, ptr, size);
	  phead += size;
	  head += size;
	}
	else if (fdel)			/* 除了刪除記錄，還要做 fdel() */
	{
	  fdel(xo, ptr);
	}
      }
    }

    memcpy(phead, ptail, total);

    if ((fd = open(xo->dir, O_WRONLY | O_CREAT | O_TRUNC, 0600)) >= 0)
    {
      write(fd, data, total + head);
      close(fd);
    }

    free(data);
    return XO_LOAD;
  }
  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* Tag List 標籤					 */
/* ----------------------------------------------------- */


int TagNum;			/* tag's number */
TagItem TagList[TAG_MAX];	/* ascending list */


int
Tagger(chrono, recno, op)
  time_t chrono;
  int recno;
  int op;			/* op : TAG_NIN / TOGGLE / INSERT */
/* ----------------------------------------------------- */
/* return 0 : not found	/ full				 */
/* 1 : add						 */
/* -1 : remove						 */
/* ----------------------------------------------------- */
{
  int head, tail, pos, cmp;
  TagItem *tagp;

  for (head = 0, tail = TagNum - 1, tagp = TagList, cmp = 1; head <= tail;)
  {
    pos = (head + tail) >> 1;
    cmp = tagp[pos].chrono - chrono;
    if (!cmp)
    {
      break;
    }
    else if (cmp < 0)
    {
      head = pos + 1;
    }
    else
    {
      tail = pos - 1;
    }
  }

  if (op == TAG_NIN)
  {
    if (!cmp && recno)		/* 絕對嚴謹：連 recno 一起比對 */
      cmp = recno - tagp[pos].recno;
    return cmp;
  }

  tail = TagNum;

  if (!cmp)
  {
    if (op != TAG_TOGGLE)
      return 0;

    TagNum = --tail;
    memcpy(&tagp[pos], &tagp[pos + 1], (tail - pos) * sizeof(TagItem));
    return -1;
  }

  if (tail < TAG_MAX)
  {
    TagItem buf[TAG_MAX];

    TagNum = tail + 1;
    tail = (tail - head) * sizeof(TagItem);
    tagp += head;
    memcpy(buf, tagp, tail);
    tagp->chrono = chrono;
    tagp->recno = recno;
    memcpy(++tagp, buf, tail);
    return 1;
  }

  /* TagList is full */

  bell();
  return 0;
}


void
EnumTag(data, dir, locus, size)
  void *data;
  char *dir;
  int locus;
  int size;
{
  rec_get(dir, data, size, TagList[locus].recno);
}


int
AskTag(msg)
  char *msg;
/* ----------------------------------------------------- */
/* return value :					 */
/* -1	: 取消						 */
/* 0	: single article				 */
/* o.w.	: whole tag list				 */
/* ----------------------------------------------------- */
{
  char buf[80];
  int num;

  num = TagNum;

  if (num)	/* itoc.020130: 有 TagNum 才問 */
  {  
    sprintf(buf, "◆ %s A)單篇文章 T)標記文章 Q)離開？[%c] ", msg, num ? 'T' : 'A');
    switch (vans(buf))
    {
    case 'q':
      return -1;

    case 'a':
      return 0;
    }
  }
  return num;
}


/* ----------------------------------------------------- */
/* tag articles according to title / author		 */
/* ----------------------------------------------------- */


static int
xo_tag(xo, op)
  XO *xo;
  int op;
{
  int fsize, count;
  char *token, *fimage;
  HDR *head, *tail;

  fimage = f_map(xo->dir, &fsize);
  if (fimage == (char *) -1)
    return XO_NONE;

  head = (HDR *) xo_pool + (xo->pos - xo->top);
  if (op == Ctrl('A'))
  {
    token = head->owner;
    op = 0;
  }
  else
  {
    token = str_ttl(head->title);
    op = 1;
  }

  head = (HDR *) fimage;
  tail = (HDR *) (fimage + fsize);

  count = 0;

  do
  {
    if (!strcmp(token, op ? str_ttl(head->title) : head->owner))
    {
      if (!Tagger(head->chrono, count, TAG_INSERT))
	break;
    }
    count++;
  } while (++head < tail);

  munmap(fimage, fsize);
  return XO_BODY;
}


int					/* XO_LOAD:刪除  XO_FOOT/XO_NONE:取消 */
xo_prune(xo, size, fvfy, fdel)		/* itoc.031003: 標籤刪除 */
  XO *xo;
  int size;
  int (*fvfy) ();			/* 檢查區段中是否有被保護的記錄  0:刪除 1:保護 */
  void (*fdel) ();			/* 除了刪除記錄以外，還要做些什麼事 */
{
  int fd, total, pos;
  char *data, *phead, *ptail, *ptr;
  char buf[80];
  struct stat st;

  if (!TagNum)
    return XO_NONE;

  sprintf(buf, "確定要刪除 %d 篇標籤嗎(Y/N)？[N] ", TagNum);
  if (vans(buf) != 'y')
    return XO_FOOT;

  if ((fd = open(xo->dir, O_RDONLY)) < 0)
    return XO_FOOT;

  fstat(fd, &st);
  data = (char *) malloc(total = st.st_size);
  total = read(fd, data, total);
  close(fd);

  phead = data;
  ptail = data + total;
  pos = 0;
  total = 0;

  for (ptr = phead; ptr < ptail; ptr += size)
  {
    if (fvfy(ptr, pos))			/* 這筆記錄保護不被砍 */
    {
      memcpy(phead, ptr, size);
      phead += size;
      total += size;
    }
    else if (fdel)			/* 除了刪除記錄，還要做 fdel() */
    {
      fdel(xo, ptr);
    }
    pos++;
  }

  if ((fd = open(xo->dir, O_WRONLY | O_CREAT | O_TRUNC, 0600)) >= 0)
  {
    write(fd, data, total);
    close(fd);
  }

  free(data);

  TagNum = 0;
  
  return XO_LOAD;
}


/* ----------------------------------------------------- */
/* Tag's batch operation routines			 */
/* ----------------------------------------------------- */


extern BCACHE *bshm;    /* lkchu.981229 */


static int
xo_tbf(xo)
  XO *xo;
{
  char fpath[128], *dir;
  HDR *hdr, xhdr;
  int tag, locus, xmode;
  FILE *fp;

  if (!cuser.userlevel)
    return XO_NONE;

  tag = AskTag("拷貝到暫存檔");
  if (tag < 0)
    return XO_FOOT;

  if (!(fp = tbf_open()))
    return XO_FOOT;

  hdr = tag ? &xhdr : (HDR *) xo_pool + xo->pos - xo->top;

  locus = 0;
  dir = xo->dir;

  do
  {
    if (tag)
    {
      fputs(str_line, fp);
      EnumTag(hdr, dir, locus, sizeof(HDR));
    }

    xmode = hdr->xmode;

    /* itoc.000319: 修正限制級文章不得匯入暫存檔 */
    /* itoc.010602: GEM_RESTRICT 和 POST_RESTRICT 匹配，所以加密文章也不得匯入暫存檔 */
    if (xmode & (GEM_RESTRICT | GEM_RESERVED))
      continue;

    if (!(xmode & GEM_FOLDER))		/* 查 hdr 是否 plain text */
    {
      hdr_fpath(fpath, dir, hdr);
      f_suck(fp, fpath);
    }
  } while (++locus < tag);

  fclose(fp);
  zmsg("拷貝完成");

  return XO_FOOT;
}


static int
xo_forward(xo)
  XO *xo;
{
  static char rcpt[64];
  char fpath[64], folder[64], *dir, *title, *userid;
  HDR *hdr, xhdr;
  int tag, locus, userno, cc, xmode;
  int method;		/* 轉寄到 0:站外 >0:自己 <0:其他站內使用者 */

  if (!cuser.userlevel || HAS_PERM(PERM_DENYMAIL))
    return XO_NONE;

  tag = AskTag("轉寄");
  if (tag < 0)
    return XO_FOOT;

  if (!rcpt[0])
    strcpy(rcpt, cuser.email);

  if (!vget(b_lines, 0, "目的地：", rcpt, sizeof(rcpt), GCARRY))
    return XO_FOOT;

  userid = cuser.userid;
  userno = 0;

  if (!mail_external(rcpt))	/* 中途攔截 */
  {
    if (!str_cmp(rcpt, userid))
    {
      /* userno = cuser.userno; */	/* Thor.981027: 寄精選集給自己不通知自己 */
      method = 1;
    }
    else
    {
      if (!HAS_PERM(PERM_LOCAL))
	return XO_FOOT;

      if ((userno = acct_userno(rcpt)) <= 0)
      {
	zmsg(err_uid);
	return XO_FOOT;
      }
      method = -1;
    }

    usr_fpath(folder, rcpt, fn_dir);
  }
  else
  {
    if (!HAS_PERM(PERM_INTERNET))
    {
      vmsg("您無法寄信到站外");
      return XO_FOOT;
    }

    if (not_addr(rcpt))
    {
      zmsg(err_email);
      return XO_FOOT;
    }

    method = 0;
  }

  hdr = tag ? &xhdr : (HDR *) xo_pool + xo->pos - xo->top;

  dir = xo->dir;
  title = hdr->title;
  locus = 0;
  cc = -1;

  do
  {
    if (tag)
      EnumTag(hdr, dir, locus, sizeof(HDR));

    xmode = hdr->xmode;

    /* itoc.000319: 修正限制級文章不得轉寄 */
    /* itoc.010602: GEM_RESTRICT 和 POST_RESTRICT 匹配，所以加密文章也不得轉寄 */
    if (xmode & (GEM_RESTRICT | GEM_RESERVED))
      continue;     

    if (!(xmode & GEM_FOLDER))		/* 查 hdr 是否 plain text */
    {
      hdr_fpath(fpath, dir, hdr);

      if (method)		/* 轉寄站內 */
      {
	HDR mhdr;

	if ((cc = hdr_stamp(folder, HDR_COPY, &mhdr, fpath)) < 0)
	  break;

	if (method > 0)		/* 轉寄自己 */
	{
	  strcpy(mhdr.owner, "[精 選 集]");
	  mhdr.xmode = MAIL_READ | MAIL_NOREPLY;
	}
	else			/* 轉寄其他使用者 */
	{
	  strcpy(mhdr.owner, userid);
	}
	strcpy(mhdr.nick, cuser.username);
	strcpy(mhdr.title, title);
	if ((cc = rec_add(folder, &mhdr, sizeof(HDR))) < 0)
	  break;
      }
      else			/* 轉寄站外 */
      {
	if ((cc = bsmtp(fpath, title, rcpt, 0)) < 0)
	  break;
      }
    }
  } while (++locus < tag);

  if (userno > 0 && cc >= 0)
    m_biff(userno);

  zmsg(cc >= 0 ? msg_sent_ok : "部份信件無法寄達");

  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* 文章作者查詢、權限設定				 */
/* ----------------------------------------------------- */


int
xo_uquery(xo)
  XO *xo;
{
  HDR *hdr;
  char *userid;

  hdr = (HDR *) xo_pool + (xo->pos - xo->top);
  userid = hdr->owner;
  if (strchr(userid, '.'))
    return XO_NONE;

  move(1, 0);
  clrtobot();
  my_query(userid);
  return XO_HEAD;
}


int
xo_usetup(xo)
  XO *xo;
{
  HDR *hdr;
  char *userid;
  ACCT acct;

  if (!HAS_PERM(PERM_ALLACCT))
    return XO_NONE;

  hdr = (HDR *) xo_pool + (xo->pos - xo->top);
  userid = hdr->owner;
  if (strchr(userid, '.') || (acct_load(&acct, userid) < 0))
    return XO_NONE;

  move(3, 0);
  acct_setup(&acct, 1);
  return XO_HEAD;
}


/* ----------------------------------------------------- */
/* 主題式閱讀						 */
/* ----------------------------------------------------- */


#define RS_TITLE	0x001	/* author/title */
#define RS_FORWARD      0x002	/* backward */
#define RS_RELATED      0x004
#define RS_FIRST	0x008	/* find first article */
#define RS_CURRENT      0x010	/* match current read article */
#define RS_THREAD	0x020	/* search the first article */
#define RS_SEQUENT	0x040	/* sequential read */
#define RS_MARKED 	0x080	/* marked article */
#define RS_UNREAD 	0x100	/* unread article */
#define	RS_BOARD	0x1000	/* 用於 RS_UNREAD，跟前面的不可重疊 */

#define CURSOR_FIRST	(RS_RELATED | RS_TITLE | RS_FIRST)
#define CURSOR_NEXT	(RS_RELATED | RS_TITLE | RS_FORWARD)
#define CURSOR_PREV	(RS_RELATED | RS_TITLE)
#define RELATE_FIRST	(RS_RELATED | RS_TITLE | RS_FIRST | RS_CURRENT)
#define RELATE_NEXT	(RS_RELATED | RS_TITLE | RS_FORWARD | RS_CURRENT)
#define RELATE_PREV	(RS_RELATED | RS_TITLE | RS_CURRENT)
#define THREAD_NEXT	(RS_THREAD | RS_FORWARD)
#define THREAD_PREV	(RS_THREAD)

/* Thor: 前後找mark文章, 方便知道有什麼問題未處理 */

#define MARK_NEXT	(RS_MARKED | RS_FORWARD | RS_CURRENT)
#define MARK_PREV	(RS_MARKED | RS_CURRENT)


typedef struct
{
  int key;			/* key stroke */
  int map;			/* the mapped threading op-code */
}      KeyMap;


static KeyMap keymap[] =
{
  /* search title / author */

  '?', RS_TITLE | RS_FORWARD,
  '|', RS_TITLE,
  'A', RS_FORWARD,
  'Q', 0,

  /* thread : currtitle */

  '[', RS_RELATED | RS_TITLE | RS_CURRENT,
  ']', RS_RELATED | RS_TITLE | RS_FORWARD | RS_CURRENT,
  '=', RS_RELATED | RS_TITLE | RS_FIRST | RS_CURRENT,

  /* i.e. < > : make life easier */

  ',', RS_THREAD,
  '.', RS_THREAD | RS_FORWARD,

  /* thread : cursor */

  '-', RS_RELATED | RS_TITLE,
  '+', RS_RELATED | RS_TITLE | RS_FORWARD,
  '\\', RS_RELATED | RS_TITLE | RS_FIRST,

  /* Thor: marked : cursor */
  '\'', RS_MARKED | RS_FORWARD | RS_CURRENT,
  ';', RS_MARKED | RS_CURRENT,

  /* Thor: 向前找第一篇未讀的文章 */
  /* Thor.980909: 向前找首篇未讀, 或末篇已讀 */
  '`', RS_UNREAD /* | RS_FIRST */,

  /* sequential */

  ' ', RS_SEQUENT | RS_FORWARD,
  KEY_RIGHT, RS_SEQUENT | RS_FORWARD,
  KEY_PGDN, RS_SEQUENT | RS_FORWARD,
  KEY_DOWN, RS_SEQUENT | RS_FORWARD,
  /* Thor.990208: 為了方便看文章過程中, 移至下篇, 雖然上層被xover吃掉了:p */
  'j', RS_SEQUENT | RS_FORWARD,

  KEY_UP, RS_SEQUENT,
  KEY_PGUP, RS_SEQUENT,
  /* Thor.990208: 為了方便看文章過程中, 移至上篇, 雖然上層被xover吃掉了:p */
  'k', RS_SEQUENT,

  /* end of keymap */

  '\0', -1
};


static int
xo_keymap(key)
  int key;
{
  KeyMap *km;
  int ch;

  km = keymap;
  while (ch = km->key)
  {
    if (ch == key)
      break;
    km++;
  }
  return km->map;
}


/* itoc.010913: xo_thread() 回傳值                */
/*  XO_NONE: 沒找到或就是游標所在，不用清 b_lines */
/*  XO_FOOT: 沒找到或就是游標所在，需要清 b_lines */
/*  XO_BODY: 找到了，但在別頁                     */
/* -XO_NONE: 找到了，就在本頁，不用清 b_lines     */
/* -XO_FOOT: 找到了，就在本頁，需要清 b_lines     */


static int
xo_thread(xo, op)
  XO *xo;
  int op;
{
  static char s_author[16], s_title[32], s_unread[2] = "0";
  char buf[80];

  char *tag, *query, *title;
  const int origpos = xo->pos, origtop = xo->top, max = xo->max;
  int pos, match, near, neartop;	/* Thor: neartop 與 near 成對用 */
  int top, bottom, step, len;
  HDR *pool, *hdr;

  match = XO_NONE;
  pos = origpos;
  top = origtop;
  pool = (HDR *) xo_pool;
  hdr = pool + (pos - top);
  near = 0;
  step = (op & RS_FORWARD) - 1;		/* (op & RS_FORWARD) ? 1 : -1 */

  if (op & RS_RELATED)
  {
    tag = hdr->title;
    if (op & RS_CURRENT)
    {
      query = currtitle;
      if (op & RS_FIRST)
      {
	if (!strcmp(query, tag))	/* 目前的就是第一筆了 */
	  return XO_NONE;
	near = -1;
      }
    }
    else
    {
      title = str_ttl(tag);
      if (op & RS_FIRST)
      {
	if (title == tag)
	  return XO_NONE;
	near = -1;
      }
      strcpy(query = buf, title);
    }
  }
  else if (op & RS_UNREAD)
  {
    /* Thor.980909: 詢問 "首篇未讀" 或 "末篇已讀" */

    near = xo->dir[0];
    if (near != 'b' && near != 'u')	/* itoc.010913: 只允許在看板/信箱搜尋 */
      return XO_NONE;			/* itoc.040916.bug: 沒有限制在信箱精華區搜尋 */

    if (!vget(b_lines, 0, "向前找尋 0)首篇未讀 1)末篇已讀 ", s_unread, sizeof(s_unread), GCARRY))
      return XO_FOOT;

    if (*s_unread == '0')
      op |= RS_FIRST;

    if (near == 'b')		/* search board */
      op |= RS_BOARD;

    near = -1;
  }
  else if (!(op & (RS_THREAD | RS_SEQUENT | RS_MARKED)))
  {
    if (op & RS_TITLE)
    {
      title = "標題";
      tag = s_title;
      len = sizeof(s_title);
    }
    else
    {
      title = "作者";
      tag = s_author;
      len = sizeof(s_author);
    }

    sprintf(query = buf, "搜尋%s(%s)：", title, (step > 0) ? "↓" : "↑");
    if (!vget(b_lines, 0, query, tag, len, GCARRY))
      return XO_FOOT;

    str_lowest(query, tag);
  }

  bottom = top + XO_TALL;
  if (bottom > max)
    bottom = max;

  for (;;)
  {
    if (step > 0)
    {
      if (++pos >= max)
	break;
    }
    else
    {
      if (--pos < 0)
	break;
    }

    /* buffer I/O : shift sliding window scope */

    if (pos < top || pos >= bottom)
    {
      xo->pos = pos;
      xo_load(xo, sizeof(HDR));

      top = xo->top;
      bottom = top + XO_TALL;
      if (bottom > max)
	bottom = max;

      hdr = pool + (pos - top);
    }
    else
    {
      hdr += step;
    }

#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
      continue;
#endif

    if (op & RS_SEQUENT)
    {
      match = -1;
      break;
    }

    /* Thor: 前後 search marked 文章 */

    if (op & RS_MARKED)
    {
      if (hdr->xmode & POST_MARKED)
      {
	match = -1;
	break;
      }
      continue;
    }

    /* 向前找尋找尋未讀/已讀文章 */

    if (op & RS_UNREAD)
    {
#define UNREAD_FUNC()   (op & RS_BOARD ? brh_unread(hdr->chrono) : !(hdr->xmode & MAIL_READ))
      if (op & RS_FIRST)	/* 首篇未讀 */
      {
	if (UNREAD_FUNC())
	{
	  near = pos;
	  neartop = top;
	  continue;
	}
      }
      else			/* 末篇已讀 */
      {
	if (!UNREAD_FUNC())
	{
	  match = -1;
	  break;
	}
      }
      continue;
    }

    /* ------------------------------------------------- */
    /* 以下搜尋 title / author				 */
    /* ------------------------------------------------- */

    if (op & (RS_TITLE | RS_THREAD))
    {
      title = hdr->title;	/* title 指向 [title] field */
      tag = str_ttl(title);	/* tag 指向 thread's subject */

      if (op & RS_THREAD)
      {
	if (tag == title)
	{
	  match = -1;
	  break;
	}
	continue;
      }
    }
    else
    {
      tag = hdr->owner;	/* tag 指向 [owner] field */
    }

    if (((op & RS_RELATED) && !strncmp(tag, query, 40)) ||
      (!(op & RS_RELATED) && str_sub(tag, query)))
    {
      if ((op & RS_FIRST) && tag != title)
      {
	near = pos;		/* 記下最接近起點的位置 */
	neartop = top;
	continue;
      }

      match = -1;
      break;
    }
  }

  /* top = xo->top = buffering 的 top */
  /* 如果 match = -1 表示找到了，而 pos, top = 要去的地方 */
  /* 如果 RS_FIRST && near >= 0 表示找到了，而 near, neartop = 要去的地方 */

#define CLEAR_FOOT()	(!(op & RS_RELATED) && ((op & RS_UNREAD) || !(op & (RS_THREAD | RS_SEQUENT | RS_MARKED))))
  
  if (match < 0)			/* 找到了 */
  {
    xo->pos = pos;			/* 把要去的位置填進去 */

    if (top != origtop)			/* 在別頁找到了 */
      match = XO_BODY;
    else				/* 在本頁找到了 */
      match = CLEAR_FOOT() ? -XO_FOOT : -XO_NONE;
  }
  else if ((op & RS_FIRST) && near >= 0)/* 找到了 */
  {
    xo->pos = near;			/* 把要去的位置填進去 */

    /* 由於是 RS_FIRST 找第一篇，所以 buffering 的 top 可能找到比最後結果 neartop 更前面 */
    if (top != neartop)
      xo_load(xo, sizeof(HDR));

    if (xo->top != origtop)		/* 在別頁找到了 */
      match = XO_BODY;
    else				/* 在本頁找到了 */
      match = CLEAR_FOOT() ? -XO_FOOT : -XO_NONE;
  }
  else					/* 找不到 */
  {
    xo->pos = origpos;			/* 還原原來位置 */

    if (top != origtop)			/* 回目前所在頁 */
      xo_load(xo, sizeof(HDR));

    match = CLEAR_FOOT() ? XO_FOOT : XO_NONE;
  }

  return match;
}


/* Thor.990204: 為考慮more 傳回值, 以便看一半可以用 []... 
                ch 為先前more()中所按的key */   
int
xo_getch(xo, ch)
  XO *xo;
  int ch;
{
  int op;

  if (!ch)
    ch = vkey();

  op = xo_keymap(ch);
  if (op >= 0)
  {
    ch = xo_thread(xo, op);
    if (ch != XO_NONE)
      ch = XO_BODY;		/* 繼續瀏覽 */
  }

  return ch;
}


/* ----------------------------------------------------- */
/* XZ							 */
/* ----------------------------------------------------- */


extern KeyFunc pal_cb[];
extern KeyFunc bmw_cb[];
extern KeyFunc post_cb[];


XZ xz[] =
{
  {NULL, NULL, M_BOARD, FEETER_CLASS},		/* XZ_CLASS */
  {NULL, NULL, M_LUSERS, FEETER_ULIST},		/* XZ_ULIST */
  {NULL, pal_cb, M_PAL, FEETER_PAL},		/* XZ_PAL */
  {NULL, NULL, M_PAL, FEETER_ALOHA},		/* XZ_ALOHA */
  {NULL, NULL, M_VOTE, FEETER_VOTE},		/* XZ_VOTE */
  {NULL, bmw_cb, M_BMW, FEETER_BMW},		/* XZ_BMW */
  {NULL, NULL, M_MF, FEETER_MF},		/* XZ_MF */
  {NULL, NULL, M_COSIGN, FEETER_COSIGN},	/* XZ_COSIGN */
  {NULL, NULL, M_SONG, FEETER_SONG},		/* XZ_SONG */
  {NULL, NULL, M_READA, FEETER_NEWS},		/* XZ_NEWS */
  {NULL, NULL, M_READA, FEETER_XPOST},		/* XZ_XPOST */
  {NULL, NULL, M_RMAIL, FEETER_MBOX},		/* XZ_MBOX */
  {NULL, post_cb, M_READA, FEETER_POST},	/* XZ_POST */
  {NULL, NULL, M_GEM, FEETER_GEM}		/* XZ_GEM */
};


static int
xo_jump(pos, zone)
  int pos;			/* 移動游標到 number 所在的特定位置 */
  int zone;			/* itoc.010403: 把 zone 也傳進來 */
{
  char buf[6];

  buf[0] = pos;
  buf[1] = '\0';
  vget(b_lines, 0, "跳至第幾項：", buf, sizeof(buf), GCARRY);

#if 0
  move(b_lines, 0);
  clrtoeol();
#endif
  outf(xz[zone - XO_ZONE].feeter);	/* itoc.010403: 把 b_lines 填上 feeter */

  pos = atoi(buf);

  if (pos > 0)
    return XO_MOVE + pos - 1;

  return XO_NONE;
}


/* ----------------------------------------------------- */
/* interactive menu routines			 	 */
/* ----------------------------------------------------- */


void
xover(cmd)
  int cmd;
{
  int pos, num, zone, sysmode;
  XO *xo;
  KeyFunc *xcmd, *cb;

  for (;;)
  {
    while (cmd != XO_NONE)
    {
      if (cmd == XO_FOOT)
      {
	outf(xz[zone - XO_ZONE].feeter);	/* itoc.010403: 把 b_lines 填上 feeter */
	break;
      }

      if (cmd >= XO_ZONE)
      {
	/* --------------------------------------------- */
	/* switch zone					 */
	/* --------------------------------------------- */

	zone = cmd;
	cmd -= XO_ZONE;
	xo = xz[cmd].xo;
	xcmd = xz[cmd].cb;
	sysmode = xz[cmd].mode;

	TagNum = 0;		/* clear TagList */
	cmd = XO_INIT;
	utmp_mode(sysmode);
      }
      else if (cmd >= XO_MOVE - XO_TALL)
      {
	/* --------------------------------------------- */
	/* calc cursor pos and show cursor correctly	 */
	/* --------------------------------------------- */

	/* cmd >= XO_MOVE - XO_TALL so ... :chuan: 假設 cmd = -1 ?? */

	/* fix cursor's range */

	num = xo->max - 1;

	/* pos = (cmd | XO_WRAP) - (XO_MOVE + XO_WRAP); */
	/* cmd &= XO_WRAP; */
	/* itoc.020124: 修正在第一頁按 PGUP、最後一頁按 PGDN、第一項按 UP、最後一項按 DOWN 會把 XO_WRAP 旗標消失 */

	if (cmd > XO_MOVE + (XO_WRAP >> 1))     /* XO_WRAP >> 1 遠大過文章數 */
	{
	  pos = cmd - (XO_MOVE + XO_WRAP);
	  cmd = 1;				/* 按 KEY_UP 或 KEY_DOWN */
	}
	else
	{
	  pos = cmd - XO_MOVE;
	  cmd = 0;
	}

	/* pos: 要跳去的項目  cmd: 是否為 KEY_UP 或 KEY_DOWN */

	if (pos < 0)
	{
	  /* pos = (zone == XZ_POST) ? 0 : num; *//* itoc.000304: 閱讀到第一篇按 KEY_UP 或 KEY_PGUP 不會翻到最後 */
	  pos = num;	/* itoc.020124: 翻到最後比較方便，應該不會有人是往上讀的 :P */
	}
	else if (pos > num)
	{
	  if (zone == XZ_POST)
	    pos = num;	/* itoc.000304: 閱讀到最後一篇按 KEY_DOWN 或 KEY_PGDN 不會翻到最前 */
	  else
	    pos = (cmd || pos == num + XO_TALL) ? 0 : num;	/* itoc.020124: 要避免如果在倒數第二頁按 KEY_PGDN，
								   而最後一頁篇數太少會直接跳去第一頁，使用者會
								   不知道有最後一頁，故先在最後一項停一下 */
	}

	/* check cursor's range */

	cmd = xo->pos;

	if (cmd == pos)
	  break;

	xo->pos = pos;
	num = xo->top;
	if ((pos < num) || (pos >= num + XO_TALL))
	{
	  xo->top = (pos / XO_TALL) * XO_TALL;
	  cmd = XO_LOAD;	/* 載入資料並予以顯示 */
	}
	else
	{
	  move(3 + cmd - num, 0);
	  outc(' ');

	  break;		/* 只移動游標 */
	}
      }

      /* ----------------------------------------------- */
      /* 執行 call-back routines			 */
      /* ----------------------------------------------- */

      cb = xcmd;
      num = cmd | XO_DL; /* Thor.990220: for dynamic load */
      for (;;)
      {
	pos = cb->key;
#if 1
	/* Thor.990220: dynamic load , with key | XO_DL */
	if (pos == num)
	{
	  void *p = DL_get((char *) cb->func);
	  if (p) 
	  {
	    cb->func = p;
	    pos = cb->key = cmd;
	  }
	  else
	  {
	    cmd = XO_NONE;
	    break;
	  }
	}
#endif
	if (pos == cmd)
	{
	  cmd = (*(cb->func)) (xo);

	  if (cmd == XO_QUIT)
	    return;

	  break;
	}
	
	if (pos == 'h')		/* itoc.001029: 'h' 是一特例，代表 *_cb 的結束 */
	{
	  cmd = XO_NONE;	/* itoc.001029: 代表找不到 call-back, 不作了! */
	  break;
	}

	cb++;
      }

    } /* Thor.990220.註解: end of while (cmd!=XO_NONE) */

    utmp_mode(sysmode); 
    /* Thor.990220:註解:用來回復 event handle routine 回來後的模式 */

    pos = xo->pos;

    if (xo->max > 0)		/* Thor: 若是無東西就不show了 */
    {
      num = 3 + pos - xo->top;
      move(num, 0);
      outc('>');
    }

    cmd = vkey();

    /* itoc.註解: 以下定義了基本按鍵，所謂基本按鍵，就是移動的之類的，通用於所有 XZ_ 的地方 */  

    /* ------------------------------------------------- */
    /* 基本的游標移動 routines				 */
    /* ------------------------------------------------- */

    if (cmd == KEY_LEFT || cmd == 'e')
    {
      TagNum = 0;	/* itoc.050413: 從精華區回到文章列表時要清除 tag */
      return;
    }
    else if (xo->max <= 0)	/* Thor: 無東西則無法移游標 */
    {
      continue;
    }
    else if (cmd == KEY_UP || cmd == 'k')
    {
      cmd = pos - 1 + XO_MOVE + XO_WRAP;
    }
    else if (cmd == KEY_DOWN || cmd == 'j')
    {
      cmd = pos + 1 + XO_MOVE + XO_WRAP;
    }
    else if (cmd == ' ' || cmd == KEY_PGDN || cmd == 'N')
    {
      cmd = pos + XO_TALL + XO_MOVE;
    }
    else if (cmd == KEY_PGUP || cmd == 'P')
    {
      cmd = pos - XO_TALL + XO_MOVE;
    }
    else if (cmd == KEY_HOME || cmd == '0')
    {
      cmd = XO_MOVE;
    }
    else if (cmd == KEY_END || cmd == '$')
    {
      cmd = xo->max - 1 + XO_MOVE;
    }
    else if (cmd >= '1' && cmd <= '9')
    {
      cmd = xo_jump(cmd, zone);
    }
    else if (cmd == KEY_RIGHT || cmd == '\n')
    {
      cmd = 'r';
    }

    /* ------------------------------------------------- */
    /* switch Zone					 */
    /* ------------------------------------------------- */

#ifdef  EVERY_Z
    else if (cmd == Ctrl('Z'))
    {
      cmd = every_Z(zone);
    }
    else if (cmd == Ctrl('U'))
    {
      cmd = every_U(zone);
    }
#endif

    /* ------------------------------------------------- */
    /* 其餘的按鍵					 */
    /* ------------------------------------------------- */

    else
    {
      if (zone >= XZ_XPOST)		/* xo_pool 中放的是 HDR */
      {
	/* --------------------------------------------- */
	/* Tag						 */
	/* --------------------------------------------- */

	if (cmd == 'C')
	{
	  cmd = xo_tbf(xo);
	}
	else if (cmd == 'F')
	{
	  cmd = xo_forward(xo);
	}
	else if (cmd == Ctrl('C'))
	{
	  if (TagNum)
	  {
	    TagNum = 0;
	    cmd = XO_BODY;
	  }
	  else
	    cmd = XO_NONE;
	}
	else if (cmd == Ctrl('A') || cmd == Ctrl('T'))
	{
	  cmd = xo_tag(xo, cmd);
	}

	/* --------------------------------------------- */
	/* 主題式閱讀					 */
	/* --------------------------------------------- */

	if (zone == XZ_XPOST)		/* 串接中不支援主題式閱讀 */
	  continue;

	pos = xo_keymap(cmd);
	if (pos >= 0)			/* 如果不是按方向鍵 */
	{
	  cmd = xo_thread(xo, pos);	/* 去查查是哪一種 thread 搜尋 */	  

	  if (cmd < 0)		/* 在本頁找到 match */
	  {
	    move(num, 0);
	    outc(' ');
	    /* cmd = XO_NONE; */
	    /* itoc.010913: 某些搜尋要把 b_lines 填上 feeter */
	    cmd = -cmd;
	  }
	}
      }

      /* ----------------------------------------------- */
      /* 其他的交給 call-back routine 去處理		 */
      /* ----------------------------------------------- */

    } /* Thor.990220.註解: end of vkey() handling */
  }
}


/* ----------------------------------------------------- */
/* Thor.980725: ctrl Z everywhere			 */
/* ----------------------------------------------------- */


#ifdef EVERY_Z
int z_status = 0;	/* 進入幾層 */

int
every_Z(zone)
  int zone;				/* 傳入所在 XZ_ZONE，若傳入 0，表示不在 xover() 中 */
{
  int cmd, tmpbno, tmpmode;

  /* itoc.000319: 最多 every_Z 一層 */
  if (z_status >= 1)
    return XO_NONE;
  else
    z_status++;

  cmd = zone;
  outz(MSG_ZONE_SWITCH);

  tmpbno = vkey();	/* 借用 tmpbno 來換成小寫 */
  if (tmpbno >= 'A' && tmpbno <= 'Z')
    tmpbno |= 0x20;
  switch (tmpbno)
  {
  case 'a':
    cmd = XZ_GEM;
    break;

  case 'b':
    if (currbno >= 0)	/* 若已選定看板，進入看板，否則到看板列表 */
    {
      cmd = XZ_POST;
      break;
    }

  case 'c':
    cmd = XZ_CLASS;
    break;

#ifdef MY_FAVORITE
  case 'f':
    if (cuser.userlevel)
      cmd = XZ_MF;
    break;
#endif

  case 'm':
    if (cuser.userlevel)
      cmd = XZ_MBOX;
    break;

  case 'u':
    cmd = XZ_ULIST;
    break;

  case 'w':
    if (cuser.userlevel)
      cmd = XZ_BMW;
    break;
  }

  if (cmd == zone)		/* 和目前所在 zone 一樣，或取消 */
  {
    z_status--;
    return XO_FOOT;		/* 若在 xover() 中取消呼叫 every_Z() 則送回 XO_FOOT 即可重繪 */
  }

  if (cmd == XZ_POST)
    XoPost(currbno);

#ifdef MY_FAVORITE
  if (zone == XZ_POST && (cmd == XZ_CLASS || cmd == XZ_MF))
#else
  if (zone == XZ_POST && cmd == XZ_CLASS)
#endif
    tmpbno = currbno;
  else
    tmpbno = -1;

  tmpmode = bbsmode;
  xover(cmd);
  utmp_mode(tmpmode);

  if (tmpbno >= 0)		/* itoc.030731: 有可能進入別的板，就需要重新 XoPost，會再看一次進板畫面 */
    XoPost(tmpbno);

  z_status--;
  return XO_INIT;		/* 需要重新載入 xo_pool，若在 xover() 中也可藉此重繪 */
}


int
every_U(zone)
  int zone;			/* 傳入所在 XZ_ZONE，若傳入 0，表示不在 xover() 中 */
{
  /* itoc.000319: 最多 every_Z 一層 */
  if (z_status >= 1)
    return XO_NONE;

  if (zone != XZ_ULIST)
  {
    int tmpmode;

    z_status++;
    tmpmode = bbsmode;
    xover(XZ_ULIST);
    utmp_mode(tmpmode);
    z_status--;
  }
  return XO_INIT;
}
#endif


/* ----------------------------------------------------- */
/* 類 XZ_* 結構的游標移動				 */
/* ----------------------------------------------------- */


/* 傳入: ch, pagemax, num, pageno, cur, redraw */
/* 傳出: ch, pageno, cur, redraw */
int
xo_cursor(ch, pagemax, num, pageno, cur, redraw)
  int ch, pagemax, num;
  int *pageno, *cur, *redraw;
{
  switch (ch)
  {
  case KEY_LEFT:
  case 'q':
    return 'q';

  case KEY_PGUP:
    if (pagemax != 0)
    {
      if (*pageno)
      {
	(*pageno)--;
      }
      else
      {
	*pageno = pagemax;
	*cur = num % XO_TALL;
      }
      *redraw = 1;
    }
    break;

  case KEY_PGDN:
    if (pagemax != 0)
    {
      if (*pageno == pagemax)
      {
	/* 在最後一項停一下 */
	if (*cur != num % XO_TALL)
	{
	  *cur = num % XO_TALL;
	}
	else
	{
	  *pageno = 0;
	  *cur = 0;
	}
      }
      else
      {
	(*pageno)++;
	if (*pageno == pagemax && *cur > num % XO_TALL)
	  *cur = num % XO_TALL;
      }
      *redraw = 1;
    }
    break;

  case KEY_UP:
  case 'k':
    if (*cur == 0)
    {
      if (*pageno != 0)
      {
	*cur = XO_TALL - 1;
	*pageno = *pageno - 1;
      }
      else
      {
	*cur = num % XO_TALL;
	*pageno = pagemax;
      }
      *redraw = 1;
    }
    else
    {
      move(3 + *cur, 0);
      outc(' ');
      (*cur)--;
      move(3 + *cur, 0);
      outc('>');
    }
    break;

  case KEY_DOWN:
  case 'j':
    if (*cur == XO_TALL - 1)
    {
      *cur = 0;
      *pageno = (*pageno == pagemax) ? 0 : *pageno + 1;
      *redraw = 1;
    }
    else if (*pageno == pagemax && *cur == num % XO_TALL)
    {
      *cur = 0;
      *pageno = 0;
      *redraw = 1;
    }
    else
    {
      move(3 + *cur, 0);
      outc(' ');
      (*cur)++;
      move(3 + *cur, 0);
      outc('>');
    }
    break;

  case KEY_HOME:
  case '0':
    *pageno = 0;
    *cur = 0;
    *redraw = 1;
    break;

  case KEY_END:
  case '$':
    *pageno = pagemax;
    *cur = num % XO_TALL;
    *redraw = 1;
    break;

  default:
    if (ch >= '1' && ch <= '9')
    {
      int pos;
      char buf[6];

      buf[0] = ch;
      buf[1] = '\0';
      vget(b_lines, 0, "跳至第幾項：", buf, sizeof(buf), GCARRY);

      pos = atoi(buf);

      if (pos > 0)
      {
	pos--;
	if (pos >num)
	  pos = num;
	*pageno = pos / XO_TALL;
	*cur = pos % XO_TALL;
      }

      *redraw = 1;	/* 就算沒有換頁，也要重繪 feeter */
    }
  }

  return ch;
}


/* ----------------------------------------------------- */
/* 說明文件						 */
/* ----------------------------------------------------- */


void
xo_help(path)			/* itoc.021122: 說明文件 */
  char *path;
{
  /* itoc.030510: 放到 so 裡面 */
  DL_func("bin/help.so:vaHelp", path);
}
