/*-------------------------------------------------------*/
/* bmw.c	( NTHU CS MapleBBS Ver 3.00 )		 */
/*-------------------------------------------------------*/
/* target : bmw routines		 		 */
/* create : 95/03/29				 	 */
/* update : 97/03/29				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern UCACHE *ushm;
extern XZ xz[];
extern char xo_pool[];


/* ----------------------------------------------------- */
/* BMW : bbs message write routines			 */
/* ----------------------------------------------------- */


#define BMW_FORMAT	"\033[1;33;46m★%s \033[37;45m %s \033[m"	/* 收到的水球 */
#define BMW_FORMAT2	"\033[1;33;41m☆%s \033[34;47m %s \033[m"	/* 送出的水球 */

static int bmw_locus = 0;		/* 總共保存幾個水球 (保留最近收到 BMW_LOCAL_MAX 個) */
static BMW bmw_lslot[BMW_LOCAL_MAX];	/* 保留收到的水球 */

static int bmw_locat = 0;		/* 總共保存幾個水球 (保留最近送出 BMW_LOCAL_MAX 個) */
static BMW bmw_lword[BMW_LOCAL_MAX];	/* 保留送出的水球 */


int			/* 1:可以傳水球給對方/與對方Talk  0:不能傳水球給對方/與對方Talk */
can_override(up)
  UTMP *up;
{
  int ufo;

  if (up->userno == cuser.userno)	/* 不能傳水球給自己(即使是分身) */
    return 0;

  ufo = up->ufo;

#ifdef HAVE_SUPERCLOAK
  if ((ufo & UFO_SUPERCLOAK) && !(cuser.ufo & UFO_SUPERCLOAK))	/* 紫隱只有紫隱的才看的見 */
    return 0;
#endif

  /* itoc.010909.註解: 站長可以傳水球給 鎖定/BBSNET... 的人，這樣好嗎？觀察中 */

  if (HAS_PERM(PERM_ALLACCT))	/* 站長、帳號管理員可以傳給任何人 */
    return 1;

  /* itoc.010909: 鎖定時不能被傳水球 */
  if ((ufo & UFO_QUIET) || (up->status & STATUS_REJECT))	/* 遠離塵囂/鎖定時 不能被傳 */
    return 0;

  if (!(up->ufo & UFO_CLOAK) || HAS_PERM(PERM_SEECLOAK))
  {
    /* itoc.001223: 用 is_ogood/is_obad 來做判斷 */
    if (ufo & UFO_PAGER)
      return is_ogood(up);		/* pager 關閉時只有被設好友能傳水球 */
    else
      return !is_obad(up);		/* pager 打開時只要沒有被設壞人即可傳水球 */
  }
  else
  {
    /* itoc.020321: 對方若隱形傳我水球，我也可以被動回 */
    BMW *bmw;

    for (ufo = bmw_locus - 1; ufo >= 0; ufo--)
    {
      bmw = &bmw_lslot[ufo];

      /* itoc.030718: 如果我重新上站了，那麼即使我上一次上站有丟對方水球，對方也不可以回我
         不過這檢查還是有個漏洞，就是如果重新上站以後又剛好坐同一個 ushm 的位置，那麼對方還是可以回我 */
      if (bmw->caller == up && bmw->sender == up->userno)
	return 1;
    }
  }

  return 0;
}


int			/* 1:可看見 0:不可看見 */
can_see(my, up)
  UTMP *my;
  UTMP *up;
{
  usint mylevel, myufo, urufo;

  if (my == cutmp)	/* 用 cuser. 來代替 cutmp-> */
  {
    mylevel = cuser.userlevel;
    myufo = cuser.ufo;
  }
  else
  {
    mylevel = my->userlevel;
    myufo = my->ufo;
  }
  urufo = up->ufo;

  if ((urufo & UFO_CLOAK) && !(mylevel & PERM_SEECLOAK))
    return 0;

#ifdef HAVE_SUPERCLOAK
  if ((urufo & UFO_SUPERCLOAK) && !(myufo & UFO_SUPERCLOAK))
    return 0;
#endif

#ifdef HAVE_BADPAL
  if (my == cutmp)	/* 檢查我可不可以看到對方 */
  {
    if (!(mylevel & PERM_SEECLOAK) && is_obad(up))
      return 0;
  }
  else			/* 檢查對方可不可以看到我 */
  {
    if (!(mylevel & PERM_SEECLOAK) && is_mybad(my->userno))
      return 0;
  }
#endif

  return 1;
}


int
bmw_send(callee, bmw)
  UTMP *callee;
  BMW *bmw;
{
  BMW *mpool, *mhead, *mtail, **mslot;
  int i;
  pid_t pid;
  time_t texpire;

  if ((callee->userno != bmw->recver) || (pid = callee->pid) <= 0)
    return 1;

  /* sem_lock(BSEM_ENTER); */

  /* find callee's available slot */

  mslot = callee->mslot;
  i = 0;

  for (;;)
  {
    if (mslot[i] == NULL)
      break;

    if (++i >= BMW_PER_USER)
    {
      /* sem_lock(BSEM_LEAVE); */
      return 1;
    }
  }

  /* find available BMW slot in pool */

  texpire = time(&bmw->btime) - BMW_EXPIRE;

  mpool = ushm->mpool;
  mhead = ushm->mbase;
  if (mhead < mpool)
    mhead = mpool;
  mtail = mpool + BMW_MAX;

  do
  {
    if (++mhead >= mtail)
      mhead = mpool;
  } while (mhead->btime > texpire);

  *mhead = *bmw;
  ushm->mbase = mslot[i] = mhead;
  /* Thor.981206: 需注意, 若ushm mapping不同, 
                  則不同隻 bbsd 互call會core dump,
                  除非這也用offset, 不過除了 -i, 應該是非必要 */


  /* sem_lock(BSEM_LEAVE); */
  return kill(pid, SIGUSR2);
}


#ifdef BMW_DISPLAY		
static void
bmw_display(max)	/* itoc.010313: display 以前的水球 */
  int max;
{
  int i;
  BMW *bmw;

  move(1, 0);
  clrtoeol();
  outs("\033[1;36m╭──────────────────────\033[37;44m [Ctrl-T]往上切換 \033[36;40m──────╮\033[m");

  i = 2;
  for (; max >= 0; max--)
  {	/* 從較新的水球往下印 */
    bmw = &bmw_lslot[max];
    move(i, 0);
    clrtoeol();
    prints("  " BMW_FORMAT, bmw->userid, bmw->msg);
    i++;
  }

  move(i, 0);
  clrtoeol();
  outs("\033[1;36m╰──────────────────────\033[37;44m [Ctrl-R]往下切換 \033[36;40m──────╯\033[m");
}
#endif


static int bmw_pos;	/* 目前指向 bmw_lslot 的哪一欄 */
static UTMP *bmw_up;	/* 目前回哪個 utmp */
static int bmw_request;	/* 1: 有新的水球進來 */


void
bmw_edit(up, hint, bmw)
  UTMP *up;		/* 送的對象，若是 NULL 表示廣播 */
  char *hint;
  BMW *bmw;
{
  int recver;
  screenline slp[3];
  char *userid, fpath[64];
  FILE *fp;

  if (bbsmode != M_BMW_REPLY)	/* 若是 reply 的話，在 bmw_reply() 會自行處理畫面重繪 */
    save_foot(slp);

  recver = up ? up->userno : 0;
  bmw->msg[0] = '\0';

  for (;;)
  {
    int ch;
    BMW *benz;

    ch = vget(0, 0, hint, bmw->msg, 62, GCARRY);

    if (!ch)		/* 沒輸入東西 */
    {
      if (bbsmode != M_BMW_REPLY)
	restore_foot(slp, 1);
      return;
    }

    if (ch != Ctrl('R') && ch != Ctrl('T'))	/* 完成水球輸入 */
      break;

    /* 有新的水球進來，重繪水球回顧，並將 bmw_pos 指向原來那個水球 */
    if (bmw_request)
    {
      bmw_request = 0;
      bmw_pos = bmw_locus - 1;
      if (cuser.ufo & UFO_BMWDISPLAY)
	bmw_display(bmw_pos);
      bmw_reply_CtrlRT(ch);
      continue;
    }

    /* 在 vget 中按 ^R 換 reply 別的水球 */
    benz = &bmw_lslot[bmw_pos];
    if (benz->sender != up->userno)	/* reply 不同人 */
    {
      up = bmw_up;
      recver = up->userno;
      sprintf(hint, "★[%s]", up->userid);
    }
  }

  sprintf(fpath, "確定要送出《水球》給 %s 嗎(Y/N)？[Y] ", up ? up->userid : "廣播");
  if (vans(fpath) != 'n')
  {
    int i;

    bmw->caller = cutmp;
    bmw->sender = cuser.userno;
    userid = cuser.userid;

    if (up)	/* 不是廣播 */
    {
      /* 送出水球 */
      bmw->recver = recver;
      strcpy(bmw->userid, userid);
      if (bmw_send(up, bmw))	/* 水球送不出去，不寫入水球紀錄檔 */
      {
	vmsg(MSG_USR_LEFT);
	if (bbsmode != M_BMW_REPLY)
	  restore_foot(slp, 2);
	return;
      }

      /* lkchu.990103: 若是自己送出的水球，存對方的 userid */
      strcpy(bmw->userid, up->userid);
    }
    else	/* 廣播 */
    {
      /* 送出廣播的程式，在 ulist_broadcast() 處理 */

      bmw->recver = 0;	/* 存 0 使不能 write 回廣播 */

      /* itoc.000213: 加 "> " 為了與一般水球區分 */
      sprintf(bmw->userid, "%s> ", cuser.userid);
    }
      
    time(&bmw->btime);
    usr_fpath(fpath, userid, fn_bmw);
    rec_add(fpath, bmw, sizeof(BMW));

    /* itoc.020126: 加入 FN_AMW */
    usr_fpath(fpath, userid, fn_amw);
    if (fp = fopen(fpath, "a"))
    {
      fprintf(fp, BMW_FORMAT2 " %s\n", bmw->userid, bmw->msg, Btime(&bmw->btime));
      fclose(fp);
    }

    /* itoc.030621: 保留送出的水球 */
    if (bmw_locat >= BMW_LOCAL_MAX)
    {
      /* 舊的往前挪 */
      i = BMW_LOCAL_MAX - 1;
      memcpy(bmw_lword, bmw_lword + 1, i * sizeof(BMW));
    }
    else
    {
      i = bmw_locat;
      bmw_locat++;
    }
    bmw_lword[i].recver = recver;
    strcpy(bmw_lword[i].msg, bmw->msg);
  }

  if (bbsmode != M_BMW_REPLY)
    restore_foot(slp, 2);
}


static void
bmw_outz()
{
  int i;
  BMW *bmw, *benz;

  /* 列印的位置要和 save/restore_foot 所重繪的部分是相同的 */

  bmw = &bmw_lslot[bmw_pos];
  move(b_lines, 0);
  clrtoeol();
  prints(BMW_FORMAT, bmw->userid, bmw->msg);

  /* itoc.030621: 由保留的送出水球中，找出上次回這人的水球是什麼 */
  for (i = bmw_locat; i >= 0; i--)
  {
    benz = &bmw_lword[i];
    if (benz->recver == bmw->sender)
      break;
  }
  move(b_lines - 1, 0);
  clrtoeol();
  prints(BMW_FORMAT2, bmw->userid, i >= 0 ? benz->msg : "【您最近沒有傳水球給這位使用者】");
}


static UTMP *
can_reply(uhead, pos)
  UTMP *uhead;
  int pos;
{
  int userno;
  BMW *bmw;
  UTMP *up;

  bmw = &bmw_lslot[pos];

  userno = bmw->sender;
  if (!userno)		/* Thor.980805: 防止系統協尋回扣 */
    return NULL;

  up = bmw->caller;
  if ((up < uhead) || (up > uhead + ushm->offset) || (up->userno != userno))
  {
    /* 如果 up-> 不在 ushm 內，或是 up-> 不是 call-in 我的人，表示這人下站了，
       但是他可能又上站或有 multi，所以重找一次 */
    if (!(up = utmp_find(userno)))	/* 如果再找一次還是沒有 */
      return NULL;
  }

  /* itoc.010909: 可以被動回給 隱形/遠離塵囂/關閉pager 的人，但是不能回給鎖定的人 */
  if (bmw->caller != up || up->status & STATUS_REJECT)
    return NULL;

  return up;
}


static UTMP *
bmw_lastslot(pos)	/* 找出最近一個可以回水球的對象 */
  int pos;
{
  int max, times;
  UTMP *up, *uhead;

  uhead = ushm->uslot;
  max = bmw_locus - 1;

  for (times = max; times >= 0; times--)
  {
    if (up = can_reply(uhead, pos))
    {
      bmw_pos = pos;
      return up;
    }

    /* 往下循環找一圈 */
    pos = (pos == 0) ? max : pos - 1;
  }

  return NULL;
}


static UTMP *
bmw_firstslot(pos)	/* 找出最遠一個可以回水球的對象 */
  int pos;
{
  int max, times;
  UTMP *up, *uhead;

  uhead = ushm->uslot;
  max = bmw_locus - 1;

  for (times = max; times >= 0; times--)
  {
    if (up = can_reply(uhead, pos))
    {
      bmw_pos = pos;
      return up;
    }

    /* 往上循環找一圈 */
    pos = (pos == max) ? 0 : pos + 1;
  }

  return NULL;
}


int
bmw_reply_CtrlRT(key)
  int key;
{
  int max, pos;

  max = bmw_locus - 1;
  if (max == 0)		/* 沒其他的水球可以選 */
    return 0;

  pos = bmw_pos;	/* 舊的 bmw_pos */

  if (key == Ctrl('R'))
    bmw_up = bmw_lastslot(pos == 0 ? max : pos - 1);	/* 由目前所在 pos 往下找一個可以回水球的對象 */
  else /* if (key == Ctrl('T')) */
    bmw_up = bmw_firstslot(pos == max ? 0 : pos + 1);	/* 由目前所在 pos 往上找一個可以回水球的對象 */

  if (!bmw_up)		/* 找不到別的水球 */
  {
    bmw_pos = pos;
    return 0;
  }

#ifdef BMW_DISPLAY
  if (cuser.ufo & UFO_BMWDISPLAY)
  {
    move(2 + max - pos, 0);
    outc(' ');
    move(2 + max - bmw_pos, 0);
    outc('>');
  }
#endif

  bmw_outz();
  return 1;
}


void
bmw_reply()
{
  int max, display, tmpmode;
  char buf[128];
  UTMP *up;
  BMW bmw;
#ifdef BMW_DISPLAY
  screenline slt[T_LINES];
#else
  screenline slt[3];
#endif

  cursor_save();

  max = bmw_locus - 1;
  if (!(up = bmw_lastslot(max)))
  {
    save_foot(slt);
    vmsg("先前並無水球呼叫，或對方皆已下站");
    restore_foot(slt, 2);
    cursor_restore();
    refresh();
    return;
  }

  tmpmode = bbsmode;	/* lkchu.981201: 儲存 bbsmode */
  utmp_mode(M_BMW_REPLY);

#ifdef BMW_DISPLAY
  display = cuser.ufo & UFO_BMWDISPLAY;
  if (display)
  {
    vs_save(slt);	/* itoc.010313: 記錄 bmd_display 之前的 screen */
    bmw_display(max);	/* itoc.010313: display 以前的水球 */
    move(2 + max - bmw_pos, 0);
    outc('>');
    bmw_request = 0;
  }
  else
#endif
    save_foot(slt);

  bmw_outz();

  sprintf(buf, "★[%s]", up->userid);
  bmw_edit(up, buf, &bmw);

#ifdef BMW_DISPLAY
  if (display)
  {
    cursor_restore();
    vs_restore(slt);	/* itoc.010313: 還原 bmw_display 之前的 screen */
  }
  else  
#endif
  {
    restore_foot(slt, 3);	/* 已 bmw_outz，要還原三列 */
    cursor_restore();
    refresh();
  }

  utmp_mode(tmpmode);	/* lkchu.981201: 回復 bbsmode */
}


void
bmw_rqst()
{
  int i, j, userno, locus;
  BMW bmw[BMW_PER_USER], *mptr, **mslot;

  /* download BMW slot first */

  i = j = 0;
  userno = cuser.userno;
  mslot = cutmp->mslot;

  while (mptr = mslot[i])
  {
    mslot[i] = NULL;
    if (mptr->recver == userno)
    {
      bmw[j++] = *mptr;
    }
    mptr->btime = 0;

    if (++i >= BMW_PER_USER)
      break;
  }

  /* process the request */

  if (j)
  {
    char buf[128];
    FILE *fp;

    locus = bmw_locus;
    i = locus + j - BMW_LOCAL_MAX;
    if (i >= 0)
    {
      locus -= i;
      memcpy(bmw_lslot, bmw_lslot + i, locus * sizeof(BMW));
    }

    /* itoc.020126: 加入 FN_AMW */
    usr_fpath(buf, cuser.userid, fn_amw);
    fp = fopen(buf, "a");

    i = 0;
    do
    {
      mptr = &bmw[i];

      /* lkchu.981230: 利用 xover 整合 bmw */
      usr_fpath(buf, cuser.userid, fn_bmw);
      rec_add(buf, mptr, sizeof(BMW));

      /* itoc.020126: 加入 FN_AMW */
      fprintf(fp, BMW_FORMAT " %s\n", mptr->userid, mptr->msg, Btime(&mptr->btime));

      bmw_lslot[locus++] = *mptr;	/* structure copy */
    } while (++i < j);

    fclose(fp);

    bmw_locus = locus;
    if (bbsmode == M_BMW_REPLY)
      bmw_request = 1;		/* 要求更新 */

    /* Thor.980827: 為了防止列印一半(more)時水球而後列印超過範圍踢人, 故存下游標位置 */
    cursor_save(); 

    sprintf(buf, BMW_FORMAT, mptr->userid, mptr->msg);
    outz(buf);

    /* Thor.980827: 為了防止列印一半(more)時水球而後列印超過範圍踢人, 故還原游標位置 */
    cursor_restore();

    refresh();
    bell();

#ifdef BMW_COUNT
    /* itoc.010312: 多中一個水球 */
    cutmp->bmw_count++;
#endif
  }
}


void
do_write(up)
  UTMP *up;
{
  if (can_override(up))
  {
    BMW bmw;
    char buf[20];

    sprintf(buf, "★[%s]", up->userid);
    bmw_edit(up, buf, &bmw);
  }
}


/* ----------------------------------------------------- */
/* 水球列表: 選單式操作界面描述 by lkchu		 */
/* ----------------------------------------------------- */


static void
bmw_item(num, bmw)
  int num;
  BMW *bmw;
{
  struct tm *ptime = localtime(&bmw->btime);

  if (bmw->sender == cuser.userno)	/* 送出的水球 */
  {
    prints("%6d%c\033[33m%-13s\033[36m%-*.*s\033[33m%02d:%02d\033[m\n",
      num, tag_char(bmw->btime), bmw->userid, d_cols + 53, d_cols + 53, bmw->msg, ptime->tm_hour, ptime->tm_min);
  }
  else					/* 收到的水球 */
  {
    prints("%6d%c%-13s\033[32m%-*.*s\033[m%02d:%02d\n",
      num, tag_char(bmw->btime), bmw->userid, d_cols + 53, d_cols + 53, bmw->msg, ptime->tm_hour, ptime->tm_min);
  }
}


static int
bmw_body(xo)
  XO *xo;
{
  BMW *bmw;
  int num, max, tail;

  max = xo->max;
  if (max <= 0)
  {
    vmsg("先前並無水球呼叫");
    return XO_QUIT;
  }

  bmw = (BMW *) xo_pool;
  num = xo->top;
  tail = num + XO_TALL;
  if (max > tail)
    max = tail;

  move(3, 0);
  do
  {
    bmw_item(++num, bmw++);
  } while (num < max);
  clrtobot();

  /* return XO_NONE; */
  return XO_FOOT;	/* itoc.010403: 把 b_lines 填上 feeter */
}


static int
bmw_head(xo)
  XO *xo;
{
  vs_head("察看水球", str_site);
  prints(NECKER_BMW, d_cols, "");
  return bmw_body(xo);
}


static int
bmw_load(xo)
  XO *xo;
{
  xo_load(xo, sizeof(BMW));
  return bmw_body(xo);
}


static int
bmw_init(xo)
  XO *xo;
{
  xo_load(xo, sizeof(BMW));
  return bmw_head(xo);
}


static int
bmw_delete(xo)
  XO *xo;
{
  if (vans(msg_del_ny) == 'y')
  {
    if (!rec_del(xo->dir, sizeof(BMW), xo->pos, NULL))
      return bmw_load(xo);
  }

  return XO_FOOT;
}


static int
bmw_rangedel(xo)	/* itoc.001126: 新增水球區段刪除 */
  XO *xo;
{
  return xo_rangedel(xo, sizeof(BMW), NULL, NULL);
}


static int
vfybmw(bmw, pos)
  BMW *bmw;
  int pos;
{
  return Tagger(bmw->btime, pos, TAG_NIN);
}


static int
bmw_prune(xo)
  XO *xo;
{
  return xo_prune(xo, sizeof(BMW), vfybmw, NULL);
}


static int
bmw_mail(xo)
  XO *xo;
{
  BMW *bmw;
  char *str, userid[IDLEN + 1];

  bmw = (BMW *) xo_pool + (xo->pos - xo->top);
  strcpy(userid, bmw->userid);
  if (str = strchr(userid, '>'))	/* 廣播 */
    *str = '\0';
  return my_send(userid);
}


static int
bmw_query(xo)
  XO *xo;
{
  BMW *bmw;
  char *str, userid[IDLEN + 1];

  bmw = (BMW *) xo_pool + (xo->pos - xo->top);
  move(1, 0);
  clrtobot();
  strcpy(userid, bmw->userid);
  if (str = strchr(userid, '>'))	/* 廣播 */
    *str = '\0';
  my_query(userid);
  return bmw_head(xo);
}


static int
bmw_write(xo)
  XO *xo;
{
  if (HAS_PERM(PERM_PAGE))
  {
    int userno;
    UTMP *up;
    BMW *bmw;

    bmw = (BMW *) xo_pool + (xo->pos - xo->top);

    /* itoc.010304: 讓傳訊的 bmw 也可以回 */
    /* 我送水球給別人，回給收訊者；別人送水球給我，回給送訊者 */
    userno = (bmw->sender == cuser.userno) ? bmw->recver : bmw->sender;
    if (!userno)
      return XO_NONE;

    if (up = utmp_find(userno))
      do_write(up);
  }
  return XO_NONE;
}


static void
bmw_store(fpath)
  char *fpath;
{
  int fd;
  FILE *fp;
  char buf[64], folder[64];
  HDR fhdr;

  /* itoc.020126.註解: 可以直接拿 FN_AMW 來儲存即可，
     可是如果用 FN_BMW 重做一次的話，可以讓使用者在 t_bmw() 中自由 d 掉不要的水球 */

  if ((fd = open(fpath, O_RDONLY)) < 0)
    return;

  usr_fpath(folder, cuser.userid, fn_dir);
  if (fp = fdopen(hdr_stamp(folder, 0, &fhdr, buf), "w"))
  {
    BMW bmw;

    fprintf(fp, "              == 水球記錄 %s ==\n\n", Now());

    while (read(fd, &bmw, sizeof(BMW)) == sizeof(BMW)) 
    {
      fprintf(fp, bmw.sender == cuser.userno ? BMW_FORMAT2 " %s\n" : BMW_FORMAT " %s\n",
	bmw.userid, bmw.msg, Btime(&bmw.btime));
    }
    fclose(fp);
  }

  close(fd);

  fhdr.xmode = MAIL_READ | MAIL_NOREPLY;
  strcpy(fhdr.title, "[備 忘 錄] 水球紀錄");
  strcpy(fhdr.owner, cuser.userid);
  rec_add(folder, &fhdr, sizeof(HDR));
}


static int
bmw_save(xo)
  XO *xo;
{
  if (vans("您確定要把水球存到信箱裡嗎(Y/N)？[N] ") == 'y')
  {
    char fpath[64];

    usr_fpath(fpath, cuser.userid, fn_bmw);
    bmw_store(fpath);
    unlink(fpath);
    usr_fpath(fpath, cuser.userid, fn_amw);
    unlink(fpath);
    return bmw_init(xo);
  }
  return XO_FOOT;
}


static int
bmw_save_user(xo)
  XO *xo;
{
  int fd;
  FILE *fp;
  char buf[64], folder[64];
  HDR fhdr;
  ACCT acct;

  if (acct_get(msg_uid, &acct) > 0 && acct.userno != cuser.userno)
  {
    usr_fpath(buf, cuser.userid, fn_bmw);
    if ((fd = open(buf, O_RDONLY)) >= 0)
    {
      usr_fpath(folder, cuser.userid, fn_dir);
      if (fp = fdopen(hdr_stamp(folder, 0, &fhdr, buf), "w"))
      {
	BMW bmw;

	fprintf(fp, "       == 與 %s 丟的水球紀錄 %s ==\n\n", acct.userid, Now());

	while (read(fd, &bmw, sizeof(BMW)) == sizeof(BMW)) 
	{
	  if (bmw.sender == acct.userno || bmw.recver == acct.userno)
	  {
	    fprintf(fp, bmw.sender == cuser.userno ? BMW_FORMAT2 " %s\n" : BMW_FORMAT " %s\n",
	      bmw.userid, bmw.msg, Btime(&bmw.btime));
	  }
	}
	fclose(fp);
      }
      close(fd);

      fhdr.xmode = MAIL_READ | MAIL_NOREPLY;
      strcpy(fhdr.title, "[備 忘 錄] 水球紀錄");
      strcpy(fhdr.owner, cuser.userid);
      rec_add(folder, &fhdr, sizeof(HDR));
      vmsg("水球紀錄已寄到信箱");
    }
  }

  return bmw_head(xo);
}


static int
bmw_clear(xo)
  XO *xo;
{
  if (vans("是否刪除所有水球紀錄(Y/N)？[N] ") == 'y')
  {
    char fpath[64];

    usr_fpath(fpath, cuser.userid, fn_bmw);
    unlink(fpath);
    usr_fpath(fpath, cuser.userid, fn_amw);
    unlink(fpath);
    return XO_QUIT;
  }

  return XO_FOOT;
}


static int
bmw_tag(xo)
  XO *xo;
{
  BMW *bmw;
  int tag, pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  bmw = (BMW *) xo_pool + cur;

  if (tag = Tagger(bmw->btime, pos, TAG_TOGGLE))
  {
    move(3 + cur, 6);
    outc(tag > 0 ? '*' : ' ');
  }

  /* return XO_NONE; */
  return xo->pos + 1 + XO_MOVE;	/* lkchu.981201: 跳至下一項 */
}


static int
bmw_help(xo)
  XO *xo;
{
  xo_help("bmw");
  return bmw_head(xo);
}


KeyFunc bmw_cb[] =
{
  XO_INIT, bmw_init,
  XO_LOAD, bmw_load,
  XO_HEAD, bmw_head,
  XO_BODY, bmw_body,
  
  'd', bmw_delete,
  'D', bmw_rangedel,
  'm', bmw_mail,
  'w', bmw_write,
  'r', bmw_query,
  Ctrl('Q'), bmw_query,
  's', bmw_init,
  'M', bmw_save,
  'u', bmw_save_user,
  't', bmw_tag,
  Ctrl('D'), bmw_prune,
  'C', bmw_clear,
  
  'h', bmw_help
};


int
t_bmw()
{
#if 0	/* itoc.010715: 由於 every_Z 要用，搬去 talk_main 常駐 */
  XO *xo;
  char fpath[64];

  usr_fpath(fpath, cuser.userid, fn_bmw);
  xz[XZ_BMW - XO_ZONE].xo = xo = xo_new(fpath);
  xover(XZ_BMW);
  free(xo);
#endif

  xover(XZ_BMW);
  return 0;
}


int
t_display()		/* itoc.020126: display FN_AMW */
{
  char fpath[64];

  usr_fpath(fpath, cuser.userid, fn_amw);
  return more(fpath, NULL);	/* Thor.990204: 只要不是 XEASY 就可以 reload menu 了 */
}


#ifdef RETAIN_BMW
static void
bmw_retain(fpath)
  char *fpath;
{
  char folder[64];
  HDR fhdr;

  usr_fpath(folder, str_sysop, fn_dir);
  hdr_stamp(folder, HDR_COPY, &fhdr, fpath);
  strcpy(fhdr.owner, cuser.userid);
  strcpy(fhdr.title, "水球存證");
  fhdr.xmode = 0;
  rec_add(folder, &fhdr, sizeof(HDR));
}
#endif


#ifdef LOG_BMW
void
bmw_log()
{
  int op;
  char fpath[64], buf[64];
  struct stat st;

  /* lkchu.981201: 放進私人信箱內/清除/保留 */  
  usr_fpath(fpath, cuser.userid, fn_bmw);

  if (!stat(fpath, &st) && S_ISREG(st.st_mode))
  {
    usr_fpath(buf, cuser.userid, fn_amw);

    if ((cuser.ufo & UFO_NWLOG) || !st.st_size)	/* itoc.000512: 不儲存水球記錄 */
    {						/* itoc.020711: 如果 bmw size 是 0 就清除 */
      op = 'c';
    }
    else
    {
      more(buf, (char *) -1);
#ifdef RETAIN_BMW
      op = vans("本次上站水球處理 (M)移至備忘錄 (R)保留 (C)清除 (S)存證？[R] ");
#else
      op = vans("本次上站水球處理 (M)移至備忘錄 (R)保留 (C)清除？[R] ");
#endif
    }
      
    switch (op)
    {
    case 'm':
      bmw_store(fpath);

    case 'c':
      unlink(fpath);
      unlink(buf);
      break;

#ifdef RETAIN_BMW
    case 's':
      if (vans("存證是把水球轉寄給站長以檢舉其他使用者，您確定要存證嗎(Y/N)？[N] ") == 'y')
	bmw_retain(buf);	/* 用不能改的 amw 來存證 */
      break;
#endif

    default:
      break;
    }
  }
}
#endif
