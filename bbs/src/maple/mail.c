/*-------------------------------------------------------*/
/* mail.c	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : local/internet mail routines	 	 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern XZ xz[];
extern char xo_pool[];


extern UCACHE *ushm;


/* ----------------------------------------------------- */
/* Link List routines					 */
/* ----------------------------------------------------- */


LinkList *ll_head;		/* head of link list */
static LinkList *ll_tail;	/* tail of link list */


void
ll_new()
{
  LinkList *list, *next;

  list = ll_head;

  while (list)
  {
    next = list->next;
    free(list);
    list = next;
  }

  ll_head = ll_tail = NULL;
}


void
ll_add(name)
  char *name;
{
  LinkList *node;
  int len;

  len = strlen(name) + 1;
  node = (LinkList *) malloc(sizeof(LinkList) + len);
  node->next = NULL;
  memcpy(node->data, name, len);

  if (ll_head)
    ll_tail->next = node;
  else
    ll_head = node;
  ll_tail = node;
}


int
ll_del(name)
  char *name;
{
  LinkList *list, *prev, *next;

  prev = NULL;
  for (list = ll_head; list; list = next)
  {
    next = list->next;
    if (!strcmp(list->data, name))
    {
      if (prev == NULL)
	ll_head = next;
      else
	prev->next = next;

      if (list == ll_tail)
	ll_tail = prev;

      free(list);
      return 1;
    }
    prev = list;
  }
  return 0;
}


int
ll_has(name)
  char *name;
{
  LinkList *list;

  for (list = ll_head; list; list = list->next)
  {
    if (!strcmp(list->data, name))
      return 1;
  }
  return 0;
}


void
ll_out(row, column, msg)
  int row, column;
  char *msg;
{
  int len;
  LinkList *list;

  move(row, column);
  clrtobot();
  outs(msg);

  column = 80;
  for (list = ll_head; list; list = list->next)
  {
    msg = list->data;
    len = strlen(msg) + 1;
    if (column + len > 78)
    {
      if (++row > b_lines - 2)
      {
	move(b_lines, 60);
	outs("族繁不及備載...");
	break;
      }
      else
      {
	column = len;
	outc('\n');
      }
    }
    else
    {
      column += len;
      outc(' ');
    }
    outs(msg);
  }
}


/* ----------------------------------------------------- */
/* (direct) SMTP					 */
/* ----------------------------------------------------- */


int				/* >=0:成功 <0:失敗 */
bsmtp(fpath, title, rcpt, method)
  char *fpath, *title, *rcpt;
  int method;
{
  int sock;
  time_t stamp;
  FILE *fp, *fr, *fw;
  char *str, buf[512], from[80], msgid[80];
#ifdef EMAIL_JUSTIFY
  char subject[80];
#endif

  cuser.numemails++;		/* 記錄使用者共寄出幾封 Internet E-mail */
  time(&stamp);

#ifdef EMAIL_JUSTIFY

  /* --------------------------------------------------- */
  /* 身分認證信函					 */
  /* --------------------------------------------------- */

  if (method & MQ_JUSTIFY)
  {
    fpath = FN_ETC_VALID;
    title = subject;
    sprintf(from, "bbsreg@%s", str_host);
    /* itoc.010820: 用 cuser.tvalid 來做 time-seed，可以省掉 cuser.vtime 欄位，
       還可以使在同一次認證時，所發出去的認證信標題都相同，避免有人老是不知道要回最後一封 */
    archiv32(str_hash(rcpt, cuser.tvalid), buf);
    sprintf(title, TAG_VALID " %s(%s) [VALID]", cuser.userid, buf);
  }
  else
#endif
  {
    sprintf(from, "%s.bbs@%s", cuser.userid, str_host);
  }

  str = strchr(rcpt, '@') + 1;
  sock = dns_smtp(str);
  if (sock >= 0)
  {
    move(b_lines, 0);
    clrtoeol();
    prints("★ 寄信給 %s \033[5m...\033[m", rcpt);
    refresh();

    sleep(1);			/* wait for mail server response */

    fr = fdopen(sock, "r");
    fw = fdopen(sock, "w");

    fgets(buf, sizeof(buf), fr);
    if (memcmp(buf, "220", 3))
      goto smtp_error;
    while (buf[3] == '-')
      fgets(buf, sizeof(buf), fr);

    fprintf(fw, "HELO %s\r\n", str_host);
    fflush(fw);
    do
    {
      fgets(buf, sizeof(buf), fr);
      if (memcmp(buf, "250", 3))
	goto smtp_error;
    } while (buf[3] == '-');

    fprintf(fw, "MAIL FROM:<%s>\r\n", from);
    fflush(fw);
    do
    {
      fgets(buf, sizeof(buf), fr);
      if (memcmp(buf, "250", 3))
	goto smtp_error;
    } while (buf[3] == '-');

    fprintf(fw, "RCPT TO:<%s>\r\n", rcpt);
    fflush(fw);
    do
    {
      fgets(buf, sizeof(buf), fr);
      if (memcmp(buf, "250", 3))
	goto smtp_error;
    } while (buf[3] == '-');

    fprintf(fw, "DATA\r\n");
    fflush(fw);
    do
    {
      fgets(buf, sizeof(buf), fr);
      if (memcmp(buf, "354", 3))
	goto smtp_error;
    } while (buf[3] == '-');

    /* ------------------------------------------------- */
    /* begin of mail header				 */
    /* ------------------------------------------------- */

    archiv32(stamp, msgid);

    /* Thor.990125: 儘可能的像 RFC 822 及 sendmail 的作法, 免得別人不接:p */
    fprintf(fw, "From: \"%s\" <%s>\r\nTo: %s\r\n", 
#ifdef EMAIL_JUSTIFY
      method & MQ_JUSTIFY ? "BBS Register" : 
#endif
      cuser.username, from, rcpt);

    /* itoc.030411: mail 輸出 RFC 2047 */
    output_rfc2047_qp(fw, "Subject: ", title, MYCHARSET, "\r\n");

    fprintf(fw, "Date: %s\r\nMessage-Id: <%s@%s>\r\n", Atime(&stamp), msgid, str_host);

    /* itoc.030323: mail 輸出 RFC 2045 */
    fprintf(fw, "Mime-Version: 1.0\r\n"
      "Content-Type: %s; charset="MYCHARSET"\r\n"
      "Content-Transfer-Encoding: %s\r\n",
      method & MQ_ATTACH ? "application/x-compressed" : "text/plain",
      method & MQ_ATTACH ? "base64" : "8bit");
    if (method & MQ_ATTACH)
      fprintf(fw, "Content-Disposition: attachment; filename=%s.tgz\r\n", cuser.userid);

    fprintf(fw, "X-Sender: %s (%s)\r\n"
      "X-Disclaimer: [%s] 對本信內容恕不負責\r\n\r\n",
      cuser.userid, cuser.username, str_site);

#ifdef EMAIL_JUSTIFY
    if (method & MQ_JUSTIFY)	/* 身分認證信函 */
    {
      fprintf(fw, " ID: %s (%s)  E-mail: %s\r\n\r\n",
	cuser.userid, cuser.username, rcpt);
    }
#endif

    /* ------------------------------------------------- */
    /* begin of mail body				 */
    /* ------------------------------------------------- */

    if (fp = fopen(fpath, "r"))
    {
      char *ptr;

      str = buf;
      *str++ = '.';
      if (method & MQ_ATTACH)	/* LHD.061222: 去掉開頭 begin 那行 */
	fgets(str, sizeof(buf) - 3, fp);
      while (fgets(str, sizeof(buf) - 3, fp))
      {
	if (ptr = strchr(str, '\n'))
	{
	  *ptr++ = '\r';
	  *ptr++ = '\n';
	  *ptr = '\0';
	}
	fputs((*str == '.' ? buf : str), fw);
      }
      fclose(fp);
    }
#ifdef HAVE_SIGNED_MAIL
    if (!(method & MQ_JUSTIFY) && !rec_get(FN_RUN_PRIVATE, buf, 8, 0))
    /* Thor.990413: 除了認證函外, 其他信件都要加sign */
    {
      time_t prichro;

      buf[8] = '\0';	/* Thor.990413: buf 用不到了，借來用用 */
      prichro = chrono32(buf);
      archiv32(str_hash(msgid, prichro), buf);
      fprintf(fw,"※ X-Sign: %s$%s %s\r\n", 
	msgid, genpasswd(buf), Btime(&stamp));
    }
#endif
    fputs("\r\n.\r\n", fw);
    fflush(fw);

    fgets(buf, sizeof(buf), fr);
    if (memcmp(buf, "250", 3))
      goto smtp_error;

    fputs("QUIT\r\n", fw);
    fflush(fw);
    fclose(fw);
    fclose(fr);
    goto smtp_log;

smtp_error:

    /* itoc.041128.註解: 凡是走到這裡的，buf 是對方回覆的錯誤訊息 */
    fclose(fr);
    fclose(fw);
    sprintf(from, "\t%.70s\n", buf);
    sock = -1;
  }
  else
  {
    strcpy(from, "\tSMTP 連線失敗\n");
  }

smtp_log:

  /* --------------------------------------------------- */
  /* 記錄寄信						 */
  /* --------------------------------------------------- */

  sprintf(buf, "%s %-13s%c> %s\n%s\t%s\n\t%s\n", 
    Btime(&stamp), cuser.userid, (method & MQ_JUSTIFY) ? '=' : '-', rcpt, 
    sock >= 0 ? "" : from, title, fpath);
  f_cat(FN_RUN_MAIL_LOG, buf);

  return sock;
}


#ifdef HAVE_MAIL_ZIP

/* ----------------------------------------------------- */
/* Zip mbox & board gem					 */
/* ----------------------------------------------------- */


static void
do_forward(title, mode)
  char *title;
  int mode;
{
  int rc;
  char *userid;
  char addr[64], fpath[64], cmd[256];

  strcpy(addr, cuser.email);

  if (!vget(b_lines, 0, "請輸入轉寄地址：", addr, 60, GCARRY))
    return;

  if (not_addr(addr))
  {
    zmsg(err_email);
    return;
  }

  sprintf(fpath, "確定寄給 [%s] 嗎(Y/N)？[N] ", addr);
  if (vans(fpath) != 'y')
    return;

  userid = strchr(addr, '@') + 1;
  if (dns_smtp(userid) >= 0)	 /* itoc.註解: 雖然 bsmtp() 也會做，但是這邊先做，以免壓縮完才知道是無效的工作站地址 */
  {
    userid = cuser.userid;

    if (mode == '1')		/* 個人信件 */
    {
      /* usr_fpath(fpath, userid, "@"); */
      /* 因為 .DIR 不在 @/ 裡，要一起打包 */
      usr_fpath(cmd, userid, fn_dir);
      usr_fpath(fpath, userid, "@");
      strcat(fpath, " ");
      strcat(fpath, cmd);
    }
    else if (mode == '2')	/* 個人精華區 */
    {
      usr_fpath(fpath, userid, "gem");
    }
    else if (mode == '3')	/* 看板文章 */
    {
      brd_fpath(fpath, currboard, NULL);
    }
    else /* if (mode == '4') */	/* 看板精華區 */
    {
      gem_fpath(fpath, currboard, NULL);
    }

    sprintf(cmd, "tar cfz - %s | uuencode -m %s.tgz > tmp/%s.tgz", fpath, userid, userid);
    system(cmd);

    sprintf(fpath, "tmp/%s.tgz", userid);
    rc = bsmtp(fpath, title, addr, MQ_ATTACH);
    unlink(fpath);

    if (rc >= 0)
    {
      vmsg(msg_sent_ok);
      return;
    }
  }

  vmsg("信件無法寄達");
}


int
m_zip()			/* itoc.010228: 打包資料 */
{
  int ans;
  char *name, *item, buf[80];

  ans = vans("打包資料 1)個人信件 2)個人精華區 3)看板文章 4)看板精華區 [Q] ");

  if (ans == '1' || ans == '2')
  {
    name = cuser.userid;
    item = (ans == '1') ? "個人信件" : "個人精華區";
  }
  else if (ans == '3' || ans == '4')
  {
    /* itoc.註解: 限定只能打包目前閱讀的看板，不能閱讀秘密看板的人就不能打包該板/精華區 */
    /* itoc.020612: 為了防止 POST_RESTRICT/GEM_RESTRICT 的文章外流，非板主就不能打包 */

    if (currbno < 0)
    {
      vmsg("請先進入您要打包的看板，再來此打包");
      return XEASY;
    }

    if ((ans == '3' && !(bbstate & STAT_BM)) || (ans == '4' && !(bbstate & STAT_BOARD)))
    {
      vmsg("只有板主才能打包看板文章及看板精華區");
      return XEASY;
    }

    name = currboard;
    item = (ans == '3') ? "看板文章" : "看板精華區";
  }
  else
  {
    return XEASY;
  }

  sprintf(buf, "確定要打包 %s %s嗎(Y/N)？[N] ", name, item);
  if (vans(buf) == 'y')
  {
    sprintf(buf, "【" BBSNAME "】%s %s", name, item);
    do_forward(buf, ans);
  }

  return XEASY;
}
#endif	/* HAVE_MAIL_ZIP */


#ifdef HAVE_SIGNED_MAIL		/* Thor.990413: 提供驗證功能 */
int
m_verify()
{
  time_t prichro;	/* 系統的 private chrono */
  time_t chrono;
  char key[9], sign[68], *p;

  vmsg("請輸入信末 X-Sign 以進行驗證");

  if (!vget(b_lines, 0, "※ X-Sign: ", sign, sizeof(sign), DOECHO))
    return XEASY;

  str_trim(sign);	/* Thor: 去尾巴，for ptelnet 自動加空白 */
  p = sign;
  while (*p == ' ')	/* Thor: 去前頭的空白 */
    p++;

  if (strlen(p) < 7 + 1 + PASSLEN + 1)
  {
    vmsg("電子簽章有誤");
    return XEASY;
  }

  /* 取得系統的 private chrono */
  if (rec_get(FN_RUN_PRIVATE, key, 8, 0))
  {
    zmsg("本系統並無電子簽章，請洽站長");
    return XEASY;
  }
  key[8] = '\0';
  prichro = chrono32(key);

  /* 取得該信的 chrono */
  p[7] = '\0';
  strcpy(key + 1, p);		/* +1 for chrono32() 所要的 prefix */
  chrono = chrono32(key);

  /* 取得該信的密鑰 */
  archiv32(str_hash(p, prichro), key);
  p[7 + PASSLEN + 1] = '\0';

  if (chkpasswd(p + 8, key) || strcmp(p + 8 + PASSLEN + 1, Btime(&chrono)))
    vmsg("此信並非由本站所發，請查照！");
  else
    vmsg("此信由本站所發出");

  return XEASY;
}
#endif


/* ----------------------------------------------------- */
/* mail routines					 */
/* ----------------------------------------------------- */


static struct
{
  XO mail_xo;
  char dir[32];
}      cmbox;


#ifdef OVERDUE_MAILDEL
usint
m_quota()
{
  usint status;
  int fd, count, fsize, limit, xmode;
  time_t mail_due, mark_due;
  struct stat st;
  HDR *head, *tail;
  char *base, *folder, date[9];

  if ((fd = open(folder = cmbox.dir, O_RDWR)) < 0)
    return 0;

  status = 0;
  fsize = limit = 0;

  if (!fstat(fd, &st) && (fsize = st.st_size) >= sizeof(HDR) &&
    (base = (char *) malloc(fsize)))
  {

    /* flock(fd, LOCK_EX); */
    /* Thor.981205: 用 fcntl 取代flock, POSIX標準用法 */
    f_exlock(fd);

    if ((fsize = read(fd, base, fsize)) >= sizeof(HDR))
    {
      int prune;		/* number of pruned mail */

      limit = time(0);
      mail_due = limit - MAIL_DUE * 86400;
      mark_due = limit - MARK_DUE * 86400;
      st.st_mtime = limit + CHECK_PERIOD;
      str_stamp(date, &st.st_mtime);

      limit = cuser.userlevel;
      limit = (limit & (PERM_ALLADMIN | PERM_MBOX)) ? MAX_BBSMAIL : (limit & PERM_VALID) ? MAX_VALIDMAIL : MAX_NOVALIDMAIL;

      count = fsize / sizeof(HDR);

      head = (HDR *) base;
      tail = (HDR *) (base + fsize);

      prune = 0;

      do
      {
	/* itoc.011013.註解: 這次加上了 MAIL_DELETE，要過 CHECK_PERIOD 下一次才會真正刪除 */
	/* itoc.011013.註解: 除了 m_quota() 以外，在 bpop3.c 裡面，若取信時要求從伺服器上刪除，也會加上 MAIL_DELETE 的旗標 */

	xmode = head->xmode;
	if (xmode & MAIL_DELETE)
	{
	  char fpath[64];

	  hdr_fpath(fpath, folder, head);
	  unlink(fpath);
	  prune++;
	  continue;
	}

	if (!(xmode & MAIL_READ))
	  status |= STATUS_BIFF;

	if ((count > limit) ||
	  (head->chrono <= (xmode & MAIL_MARKED ? mark_due : mail_due)))
	{
	  count--;
	  head->xmode = xmode | MAIL_DELETE;
	  strcpy(head->date, date);
	  status |= STATUS_MQUOTA;
	}

	if (prune)
	  memcpy(head - prune, head, sizeof(HDR));

      } while (++head < tail);

      fsize -= (prune * sizeof(HDR));
      if ((fsize > 0) && (prune || (status & STATUS_MQUOTA)))
      {
	lseek(fd, 0, SEEK_SET);
	write(fd, base, fsize);
	ftruncate(fd, fsize);
      }
    }

    /* flock(fd, LOCK_UN); */
    /* Thor.981205: 用 fcntl 取代flock, POSIX標準用法 */
    f_unlock(fd);

    free(base);
  }

  close(fd);

  if (fsize > limit * sizeof(HDR))
    status ^= STATUS_MAILOVER;
  else if (fsize < sizeof(HDR))
    unlink(folder);

  return status;
}
#endif


usint
m_query(userid)
  char *userid;
{
  usint status;
  int fsize, limit;
  HDR *head, *tail;
  char folder[64];

  status = 0;
  usr_fpath(folder, userid, fn_dir);
  if (head = (HDR *) f_img(folder, &fsize))
  {
    fsize /= sizeof(HDR);
    tail = head + fsize;

    while (--tail >= head)
    {
      if (!(tail->xmode & MAIL_READ))
      {
	status = STATUS_BIFF;
	break;
      }
    }
    free(head);

    limit = HAS_PERM(PERM_ALLADMIN | PERM_MBOX) ? MAX_BBSMAIL : HAS_PERM(PERM_VALID) ? MAX_VALIDMAIL : MAX_NOVALIDMAIL;
    if (fsize > limit)
      status ^= STATUS_MAILOVER;
  }

  return status;
}


void
m_biff(userno)
  int userno;
{
  UTMP *utmp, *uceil;

  utmp = ushm->uslot;
  uceil = (void *) utmp + ushm->offset;
  do
  {
    if (utmp->userno == userno)
    {
      utmp->status |= STATUS_BIFF;

#ifdef	BELL_ONCE_ONLY
      return;
#endif
    }
  } while (++utmp <= uceil);
}


void
mail_hold(fpath, rcpt, title, hold)
  char *fpath;
  char *rcpt;
  char *title;
  int hold;		/* -1:當寄信失敗時可以強迫保留 */
{
  if (cuser.userlevel && (hold < 0 || vans("是否自存底稿(Y/N)？[N] ") == 'y'))
  {
    char buf[256];

    sprintf(buf, "<%s> %s", rcpt, title);
    buf[TTLEN] = '\0';

    mail_self(fpath, "[備 忘 錄]", buf, MAIL_READ | MAIL_NOREPLY);
  }
}


/* ----------------------------------------------------- */
/* in boards/mail 回信給原作者，轉信站亦可		 */
/* ----------------------------------------------------- */


static inline int
is_host_alias(addr)
  char *addr;
{
  int i;
  char *str;
  static char *alias[] = HOST_ALIASES;

  /* check the aliases */

  for (i = 0; str = alias[i]; i++)
  {
    if (*str && !str_cmp(addr, str))
      return 1;
  }
  return 0;
}


/* static inline */
int				/* 1:internet mail   0:站內寄信(傳回addr為userid) */
mail_external(addr)
  char *addr;
{
  char *str;

  str = strchr(addr, '@');	/* itoc.020125.註解: email 位址只有個 ID，表示是站內寄信 */
  if (!str)
    return 0;

  if (!is_host_alias(str + 1))	/* itoc.020125: 如果不在 HOST_ALIAS 中，為站外寄信 */
    return 1;

  /* 站內寄信: 攔截 xyz.bbs@mydomain，但不包括 xyz.brd@mydomain */

  *str = '\0';
  if (str > addr + 4 && !strcmp(str - 4, ".bbs"))	/* ".bbs" => 4 */
  {
    str[-4] = '\0';
    return 0;
  }

  return 1;	/* 若不是 *.bbs@mydomain，則丟回去讓 not_addr() 發生錯誤 */
}


int		/* >=0:成功 -1:失敗或取消 */
mail_send(rcpt)
  char *rcpt;
{
  /* Thor.981105: 進入前需設好 quote_file */
  /* itoc.041116: 進入前需設好 ve_title (之所以用 ve_title 是希望在 vedit 改標題時，也能一起改信件檔頭的標題) */
  HDR hdr;
  char fpath[64], folder[64], *msg;
  int userno;				/* 0:internet_mail >0:userno */
  FILE *fp;

  if (!mail_external(rcpt))	/* 中途攔截 */
  {
    if ((userno = acct_userno(rcpt)) <= 0)
    {
      zmsg(err_uid);
      return -1;
    }
  }
  else
  {
    if (not_addr(rcpt))
    {
      zmsg(err_email);
      return -1;
    }
    userno = 0;
  }

  utmp_mode(M_SMAIL);
  fpath[0] = '\0';
  curredit = EDIT_MAIL;		/* Thor.981105: 直接指定寫信 */

  if (vedit(fpath, userno ? 1 : 2) < 0)
  {
    unlink(fpath);
    vmsg(msg_cancel);
    return -1;
  }

  if (userno)
  {
    usr_fpath(folder, rcpt, fn_dir);
    hdr_stamp(folder, HDR_LINK, &hdr, fpath);
    strcpy(hdr.owner, cuser.userid);
    strcpy(hdr.nick, cuser.username);	/* chuan: 加入 nick */
    strcpy(hdr.title, ve_title);
    rec_add(folder, &hdr, sizeof(HDR));

    if (fp = fopen(FN_RUN_MAIL_LOG, "a"))
    {
      fprintf(fp, "%s %-13s-> %s\n\t%s\n",
        Btime(&hdr.chrono), cuser.userid, rcpt, ve_title);
      fclose(fp);
    }

    msg = msg_sent_ok;
    m_biff(userno);
    mail_hold(fpath, rcpt, ve_title, 0);
  }
  else
  {
    clear();
    prints("信件即將寄給 %s\n標題為：%s\n確定要寄出嗎(Y/N)？[Y] ", rcpt, ve_title);
    switch (vkey())
    {
    case 'n':
    case 'N':
      msg = msg_cancel;
      userno = -1;
      break;

    default:
      outs("Y\n請稍候，信件傳遞中...\n");
      refresh();
      userno = bsmtp(fpath, ve_title, rcpt, 0);
      msg = (userno >= 0) ? msg_sent_ok : "信件無法寄達，底稿備份在信箱";
      mail_hold(fpath, rcpt, ve_title, userno);
    }
  }

  unlink(fpath);
  vmsg(msg);
  return userno;
}


static void
mail_reply(hdr)
  HDR *hdr;
{
  int xmode, prefix;

  vs_bar("回  信");

  /* make the title */

  sprintf(ve_title, "Re: %.64s", str_ttl(hdr->title));
  if (!vget(2, 0, "標題：", ve_title, TTLEN + 1, GCARRY))
    return;

  prints("\n收信人: %s (%s)\n標  題: %s\n", quote_user, quote_nick, ve_title);

  /* Thor: 為了省一次 rec_put 回信則假設看過內容 */

  xmode = hdr->xmode | MAIL_READ;
  prefix = quote_file[0];

  /* edit, then send the mail */

  if (mail_send(quote_user) >= 0)
    xmode |= MAIL_REPLIED;

  if (prefix == 'u')  /* user mail 看信時才標 r */
    hdr->xmode = xmode;
}


int
my_send(userid)		/* 站內寄信給 userid */
  char *userid;
{
  if (HAS_PERM(PERM_DENYMAIL) || !HAS_PERM(PERM_LOCAL))
    return XO_NONE;

  vs_bar("寄  信");
  prints("收信人：%s", userid);
  if (vget(2, 0, "標題：", ve_title, TTLEN + 1, DOECHO))
  {
    *quote_file = '\0';
    mail_send(userid);
  }
  return XO_HEAD;
}


int
m_send()
{
  ACCT acct;

  /* itoc.050604: 雖然在 my_send() 裡面會檢查，但是應該在此就先擋掉，連 ID 都無法輸入 */
  if (HAS_PERM(PERM_DENYMAIL))
    return XEASY;

  if (acct_get(msg_uid, &acct) > 0)
    my_send(acct.userid);
  return 0;
}


int
m_internet()
{
  char rcpt[60];

  if (HAS_PERM(PERM_DENYMAIL))
    return XEASY;

  move(MENU_XPOS, 0);
  clrtobot();

  if (vget(15, 0, "收信人：", rcpt, sizeof(rcpt), DOECHO) &&
    vget(17, 0, "標題：", ve_title, TTLEN + 1, DOECHO))
  {
    *quote_file = '\0';
    mail_send(rcpt);
  }

  return 0;
}


int
m_sysop()
{
  int fd;

  if ((fd = open(FN_ETC_SYSOP, O_RDONLY)) >= 0)
  {
    int i, j;
    char *ptr, *str;

    struct SYSOPLIST
    {
      char userid[IDLEN + 1];
      char duty[40];
    }	 sysoplist[7];	/* 假設 7 個足矣 */

    j = 0;
    mgets(-1);
    while (str = mgets(fd))
    {
      if (ptr = strchr(str, ':'))
      {
	*ptr = '\0';
	do
	{
	  i = *++ptr;
	} while (i == ' ' || i == '\t');

	if (i)
	{
	  strcpy(sysoplist[j].userid, str);
	  strcpy(sysoplist[j++].duty, ptr);
	}
      }
    }
    close(fd);

    move(12, 0);
    clrtobot();
    prints("%16s   %-18s權責劃分\n%s\n", "編號", "站長 ID", msg_seperator);

    for (i = 0; i < j; i++)
    {
      prints("%15d.   \033[1;%dm%-16s%s\033[m\n",
	i + 1, 31 + i, sysoplist[i].userid, sysoplist[i].duty);
    }
    prints("%-14s0.   \033[1;%dm離開\033[m", "", 31 + j);

    i = vans("請輸入代碼：[0] ") - '1';
    if (i >= 0 && i < j)
    {
      vs_bar("寄  信");
      prints("收信人：%s", sysoplist[i].userid);

      if (vget(2, 0, "標題：", ve_title, TTLEN + 1, DOECHO))
      {
	*quote_file = '\0';
        mail_send(sysoplist[i].userid);
      }
    }
  }
  return 0;
}


void
mail_self(fpath, owner, title, xmode)		/* itoc.011115: 寄檔案給自己 */
  char *fpath;		/* 檔案路徑 */
  char *owner;		/* 寄件人 */
  char *title;		/* 郵件標題 */
  usint xmode;
{
  HDR hdr;
  char *folder;

  folder = cmbox.dir;
  hdr_stamp(folder, HDR_COPY, &hdr, fpath);
  strcpy(hdr.owner, owner);
  strcpy(hdr.title, title);
  hdr.xmode = xmode;
  rec_add(folder, &hdr, sizeof(HDR));
}


int
mail_him(fpath, rcpt, title, xmode)		/* itoc.041111: 寄檔案給他人 */
  char *fpath;		/* 檔案路徑 */
  char *rcpt;		/* 收件人 */
  char *title;		/* 郵件標題 */
  usint xmode;
{
  int userno;
  HDR hdr;
  char folder[64];

  if ((userno = acct_userno(rcpt)) > 0)		/* 收件人有可能不存在 */
  {
    usr_fpath(folder, rcpt, fn_dir);
    hdr_stamp(folder, HDR_COPY, &hdr, fpath);
    strcpy(hdr.owner, cuser.userid);
    strcpy(hdr.title, title);
    hdr.xmode = xmode;
    rec_add(folder, &hdr, sizeof(HDR));
    m_biff(userno);
  }
  return userno;
}


/* ----------------------------------------------------- */
/* 群組寄信、回信					 */
/* ----------------------------------------------------- */


#ifdef MULTI_MAIL	/* Thor.981009: 防止愛情幸運信 */

static int
multi_send(title)
  char *title;
{
  FILE *fp;
  HDR hdr;
  char buf[128], fpath[64], *userid;
  int userno, reciper, listing, row;
  LinkList *wp;

  vs_bar(title ? "群組回信" : "群組寄信");

  ll_new();
  listing = reciper = 0;

  /* 回信時讀取 mail list 名單 */

  if (*quote_file)
  {
    ll_add(quote_user);
    reciper = 1;

    fp = fopen(quote_file, "r");
    while (fgets(buf, sizeof(buf), fp))
    {
      if (memcmp(buf, "※ ", 3))
      {
	if (listing)
	  break;
      }
      else
      {
	userid = buf + 3;
	if (listing)
	{
	  strtok(userid, " \n\r");
	  do
	  {
	    if ((userno = acct_userno(userid)) && (userno != cuser.userno) &&
	      !ll_has(userid))
	    {
	      ll_add(userid);
	      reciper++;
	    }
	  } while (userid = (char *) strtok(NULL, " \n\r"));
	}
	else if (!memcmp(userid, "[通告]", 6))
	  listing = 1;
      }
    }
    fclose(fp);
    ll_out(3, 0, MSG_LL);
  }

  /* 設定 mail list 的名單 */

  reciper = pal_list(reciper);

  /* 開始寄信 */

  move(1, 0);
  clrtobot();

  if (reciper == 1)
  {
    if (title)
      sprintf(ve_title, "Re: %.64s", str_ttl(title));

    if (!vget(2, 0, "標題：", ve_title, TTLEN + 1, title ? GCARRY : DOECHO))
      return -1;
    mail_send(ll_head->data);
  }
  else if (reciper >= 2 && ve_subject(2, title, "[通告] "))
  {
    usr_fpath(fpath, cuser.userid, fn_note);

    if (fp = fopen(fpath, "w"))
    {
      fprintf(fp, "※ [通告] 共 %d 人收件", reciper);
      listing = 80;
      wp = ll_head;

      do
      {
	userid = wp->data;
	reciper = strlen(userid) + 1;

	if (listing + reciper > 75)
	{
	  listing = reciper;
	  fprintf(fp, "\n※");
	}
	else
	{
	  listing += reciper;
	}

	fprintf(fp, " %s", userid);
      } while (wp = wp->next);

      memset(buf, '-', 75);
      buf[75] = '\0';
      fprintf(fp, "\n%s\n\n", buf);
      fclose(fp);
    }

    utmp_mode(M_SMAIL);
    curredit = EDIT_MAIL | EDIT_LIST;

    if (vedit(fpath, 1) < 0)
    {
      vmsg(msg_cancel);
      unlink(fpath);
      return -1;
    }
    else
    {
      vs_bar("寄信中...");

      listing = 80;
      wp = ll_head;
      title = ve_title;
      row = 2;	/* 印到哪一列 */

      do
      {
	userid = wp->data;
	if (row < b_lines)	/* 最多印到 b_lines - 1 */
	{
	  /* 印出目前寄到哪一個 id */
	  reciper = strlen(userid) + 1;
	  if (listing + reciper > 75)
	  {
	    listing = reciper;
	    outc('\n');
	    row++;
	  }
	  else
	  {
	    listing += reciper;
	    outc(' ');
	  }
	  outs(userid);
	}

	usr_fpath(buf, userid, fn_dir);
	hdr_stamp(buf, HDR_LINK, &hdr, fpath);
	strcpy(hdr.owner, cuser.userid);
	strcpy(hdr.title, title);
	hdr.xmode = MAIL_MULTI;
	rec_add(buf, &hdr, sizeof(HDR));

	if ((userno = acct_userno(userid)) > 0)
	  m_biff(userno);
      } while (wp = wp->next);

      if (fp = fopen(FN_RUN_MAIL_LOG, "a"))
      {
	fprintf(fp, "%s %-13s-> %s\n\t%s\n",
	  Btime(&hdr.chrono), cuser.userid, "群組寄信", title);
	fclose(fp);
      }

      mail_hold(fpath, "群組寄信", title, 0);
      unlink(fpath);
      vmsg(msg_sent_ok);
    }
  }
  else
  {
    vmsg(msg_cancel);
    return -1;
  }
  
  return 0;
}


static void
multi_reply(hdr)
  HDR *hdr;
{
  if (!multi_send(hdr->title))
    hdr->xmode |= (MAIL_REPLIED | MAIL_READ);
}


int
m_list()
{
  if (HAS_PERM(PERM_DENYMAIL))
    return XEASY;

  multi_send(NULL);

  return 0;
}

#endif


int
do_mreply(hdr, noreply)
  HDR *hdr;
  int noreply;		/* 1:要 0:不要 檢查 MAIL_NOREPLY */
{
  if ((noreply && (hdr->xmode & MAIL_NOREPLY)) ||
    HAS_PERM(PERM_DENYMAIL) ||
    !HAS_PERM(strchr(hdr->owner, '@') ? PERM_INTERNET : PERM_LOCAL))
  {
    return XO_FOOT;
  }

  strcpy(quote_user, hdr->owner);
  strcpy(quote_nick, hdr->nick);

#ifdef MULTI_MAIL
  /* itoc.001125: 常有人誤按群組寄信，確定一下 */
  if ((hdr->xmode & MAIL_MULTI) && vans(MSG_MULTIREPLY) == 'y')
    multi_reply(hdr);
  else
#endif
    mail_reply(hdr);

  return XO_HEAD;
}


/* ----------------------------------------------------- */
/* Mail Box call-back routines				 */
/* ----------------------------------------------------- */


static inline int
mbox_attr(type)
  int type;
{
#ifdef OVERDUE_MAILDEL
  if (type & MAIL_DELETE)
    return 'D';
#endif

  if (type & MAIL_REPLIED)
    return (type & MAIL_MARKED) ? 'R' : 'r';

  return "+ Mm"[type & 3];
}


static void
mbox_item(num, hdr)
  int num;			/* sequence number */
  HDR *hdr;
{
#ifdef OVERDUE_MAILDEL
  int xmode;

  xmode = hdr->xmode;
  prints(xmode & MAIL_DELETE ? "%6d%c\033[1;5;37;41m%c\033[m " : "%6d%c%c ",
    num, tag_char(hdr->chrono), mbox_attr(xmode));
#else
  prints("%6d%c%c ", num, tag_char(hdr->chrono), mbox_attr(hdr->xmode));
#endif

  hdr_outs(hdr, d_cols + 47);
}


static int
mbox_body(xo)
  XO *xo;
{
  HDR *hdr;
  int num, max, tail;

  max = xo->max;
  if (max <= 0)
  {
    vmsg("您沒有來信");
    return XO_QUIT;
  }

  num = xo->top;
  hdr = (HDR *) xo_pool;
  tail = num + XO_TALL;
  if (max > tail)
    max = tail;

  move(3, 0);
  do
  {
    mbox_item(++num, hdr++);
  } while (num < max);
  clrtobot();

  /* return XO_NONE; */
  return XO_FOOT;	/* itoc.010403: 把 b_lines 填上 feeter */
}


static int
mbox_head(xo)
  XO *xo;
{
  if (HAS_STATUS(STATUS_BIFF))	/* 一進入信箱就拿掉 STATUS_BIFF */
    cutmp->status ^= STATUS_BIFF;

  vs_head("郵件選單", str_site);
  prints(NECKER_MBOX, d_cols, "");
  return mbox_body(xo);
}


static int
mbox_load(xo)
  XO *xo;
{
  xo_load(xo, sizeof(HDR));
  return mbox_body(xo);
}


static int
mbox_init(xo)
  XO *xo;
{
  xo_load(xo, sizeof(HDR));
  return mbox_head(xo);
}


static int
mbox_delete(xo)
  XO *xo;
{
  int pos;
  HDR *hdr;
  char fpath[64], *dir;

  pos = xo->pos;
  hdr = (HDR *) xo_pool + (pos - xo->top);

#ifdef OVERDUE_MAILDEL
  /* Thor.980901: mark 後若被'D'起來，則一樣可以 delete，只有 MARK & no delete 才會無效 */
  if ((hdr->xmode & (MAIL_MARKED | MAIL_DELETE)) == MAIL_MARKED)
#else
  if (hdr->xmode & MAIL_MARKED)
#endif
    return XO_NONE;

  if (vans(msg_del_ny) == 'y')
  {
    dir = xo->dir;
    currchrono = hdr->chrono;
    if (!rec_del(dir, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono))
    {
      hdr_fpath(fpath, dir, hdr);
      unlink(fpath);
      return XO_LOAD;
    }
  }
  return XO_FOOT;
}


static int
chkmbox(hdr)
  HDR *hdr;
{
  return (hdr->xmode & MAIL_MARKED);
}


static int
vfymbox(hdr, pos)
  HDR *hdr;
  int pos;
{
  return (Tagger(hdr->chrono, pos, TAG_NIN) || chkmbox(hdr));
}


static void
delmbox(xo, hdr)
  XO *xo;
  HDR *hdr;
{
  char fpath[64];

  hdr_fpath(fpath, xo->dir, hdr);
  unlink(fpath);
}


static int
mbox_rangedel(xo)
  XO *xo;
{
  return xo_rangedel(xo, sizeof(HDR), chkmbox, delmbox);
}


static int
mbox_prune(xo)
  XO *xo;
{
  return xo_prune(xo, sizeof(HDR), vfymbox, delmbox);
}


static int
mbox_forward(xo)
  XO *xo;
{
  if (HAS_PERM(PERM_DENYMAIL))
    return XO_NONE;

  return post_forward(xo);	/* itoc.011230: 與 post_forward() 共用 */
}


static int
mbox_browse(xo)	/* itoc.000513: 讓信讀到一半也能 reply mark delete */
  XO *xo;
{
  HDR *hdr;
  int pos, xmode, nmode;
  char *dir, fpath[64];

  dir = xo->dir;

  for (;;)
  {
    pos = xo->pos;
    hdr = (HDR *) xo_pool + (pos - xo->top);
    xmode = hdr->xmode;

    hdr_fpath(fpath, dir, hdr);

    /* Thor.990204: 為考慮more 傳回值 */
    if ((nmode = more(fpath, FOOTER_MAILER)) < 0)
      break;

    /* itoc.010324: 秀完該篇全部以後要設為已讀，以免一直按空白往下讀，只把最後一篇設已讀 */
    if (nmode == 0 && !(xmode & MAIL_READ))
    {
      hdr->xmode = xmode | MAIL_READ;
      rec_put(dir, hdr, sizeof(HDR), pos, NULL);
    }

    strcpy(currtitle, str_ttl(hdr->title));

re_key:
    switch (xo_getch(xo, nmode))
    {
    case XO_BODY:
      continue;

    case 'y':
    case 'r':
      strcpy(quote_file, fpath);
      do_mreply(hdr, 1);
      break;

    case 'd':
      if (mbox_delete(xo) == XO_LOAD)
	return mbox_init(xo);
      break;

    case 'm':
      /* hdr->xmode ^= MAIL_MARKED; */
      /* 在 mbox_browse 時看不到 m 記號，所以限制只能 mark */
      hdr->xmode |= MAIL_MARKED;
      break;

    case 'x':	/* itoc.011231: mbox_browse 時可轉錄信去看板 */
      post_cross(xo);
      break;

    case 'X':
      mbox_forward(xo);
      break;

    case '/':
      if (vget(b_lines, 0, "搜尋：", hunt, sizeof(hunt), DOECHO))
      {
	more(fpath, FOOTER_MAILER);
	goto re_key;
      }
      continue;

    case 'E':
      return mbox_edit(xo);

    case 'C':	/* itoc.000515: mbox_browse 時可存入暫存檔 */
      {
	FILE *fp;
	if (fp = tbf_open())
	{ 
	  f_suck(fp, fpath); 
	  fclose(fp);
	}
      }
      break;

    case 'h':
      xo_help("mbox");
      break;
    }
    break;
  }

  nmode = hdr->xmode | MAIL_READ;
  if (xmode != nmode)
  {
    hdr->xmode = nmode;
    rec_put(dir, hdr, sizeof(HDR), pos, NULL);
  }

  return mbox_head(xo);
}


static int
mbox_reply(xo)
  XO *xo;
{
  int pos, xmode;
  HDR *hdr;

  pos = xo->pos;
  hdr = (HDR *) xo_pool + pos - xo->top;
  xmode = hdr->xmode;

  hdr_fpath(quote_file, xo->dir, hdr);
  do_mreply(hdr, 1);

  if (hdr->xmode != xmode)
    rec_put(xo->dir, hdr, sizeof(HDR), pos, NULL);

  return XO_HEAD;
}


int
mbox_edit(xo)		/* itoc.010301: 可以編輯自己信箱中的信 */
  XO *xo;
{
  char fpath[64];
  HDR *hdr;

  hdr = (HDR *) xo_pool + (xo->pos - xo->top);
  hdr_fpath(fpath, xo->dir, hdr);

  curredit = EDIT_MAIL;	/* Thor.981105: 直接指定寫信 */

  if (!strcmp(hdr->owner, cuser.userid) || HAS_PERM(PERM_ALLBOARD))	/* 自己寄給自己的信或是站長 */
    vedit(fpath, 0);
  else
    vedit(fpath, -1);	/* 只能編輯不可儲存 */

  /* return mbox_head(xo); */
  return XO_HEAD;	/* itoc.041023: XZ_MBOX 和 XZ_XPOST 共用 mbox_edit() */
}


static int
mbox_title(xo)		/* itoc.020113: 可以改自己信箱中的標題 */
  XO *xo;
{
  HDR *fhdr, mhdr;
  int pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  fhdr = (HDR *) xo_pool + cur;
  memcpy(&mhdr, fhdr, sizeof(HDR));

  if (!is_author(&mhdr) && !HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  vget(b_lines, 0, "標題：", mhdr.title, TTLEN + 1, GCARRY);

  if (HAS_PERM(PERM_ALLBOARD))	/* itoc.000213: 原作者只能改標題 */
  {
    vget(b_lines, 0, "作者：", mhdr.owner, 73 /* sizeof(mhdr.owner) */, GCARRY);
		/* Thor.980727: sizeof(mhdr.owner) = 80 會超過一行 */
    vget(b_lines, 0, "暱稱：", mhdr.nick, sizeof(mhdr.nick), GCARRY);
    vget(b_lines, 0, "日期：", mhdr.date, sizeof(mhdr.date), GCARRY);
  }

  if (memcmp(fhdr, &mhdr, sizeof(HDR)) && vans(msg_sure_ny) == 'y')
  {
    *fhdr = mhdr;
    rec_put(xo->dir, fhdr, sizeof(HDR), xo->key == XZ_XPOST ? fhdr->xid : pos, NULL);
    move(3 + cur, 0);
    mbox_item(++pos, fhdr);

    /* itoc.010709: 修改文章標題順便修改內文的標題 */
    header_replace(xo, fhdr);
  }
  return XO_FOOT;
}

 
static int
mbox_mark(xo)
  XO *xo;
{
  HDR *hdr;
  int cur, pos;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;
  move(3 + cur, 7);
  outc(mbox_attr(hdr->xmode ^= MAIL_MARKED));
  rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, NULL);
  return XO_NONE;
}


static int
mbox_tag(xo)
  XO *xo;
{
  HDR *hdr;
  int tag, pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

  if (xo->key == XZ_XPOST)
    pos = hdr->xid;

  if (tag = Tagger(hdr->chrono, pos, TAG_TOGGLE))
  {
    move(3 + cur, 6);
    outc(tag > 0 ? '*' : ' ');
  }

  /* return XO_NONE; */
  return xo->pos + 1 + XO_MOVE; /* lkchu.981201: 跳至下一項 */
}


static int
mbox_send(xo)
  XO *xo;
{
  m_send();
  return mbox_head(xo);
}


static int
mbox_visit(xo)
  XO *xo;
{
  int pos, fd;
  char *dir;
  HDR *hdr;

  pos = vans("設定所有信件 (V)已讀 (Q)取消？[Q] ");
  if (pos == 'v')
  {
    if (HAS_STATUS(STATUS_BIFF))
      cutmp->status ^= STATUS_BIFF;

    dir = xo->dir;
    if ((fd = open(dir, O_RDONLY)) < 0)
      return XO_FOOT;

    pos = 0;
    mgets(-1);
    while (hdr = mread(fd, sizeof(HDR)))
    {
      if (!(hdr->xmode & MAIL_READ))
      {
	hdr->xmode |= MAIL_READ;
	rec_put(dir, hdr, sizeof(HDR), pos, NULL);
      }
      pos++;
    }
    close(fd);

    return mbox_init(xo);
  }
  return XO_FOOT;
}


static int
mbox_sysop(xo)		/* itoc.001029.註解: 方便收 sysop/guest 的信 */
  XO *xo;
{
  if (xo == (XO *) &cmbox)	/* 若已經進入第二人信箱，則不能再進第三人信箱 */
  {
#ifdef SYSOP_CHECK_MAIL
    if (HAS_PERM(PERM_SYSOP))		/* itoc.001029: 限制只有 SYSOPs 可以看 user 信箱 */
    {
      ACCT acct;
      XO *xt;
      char fpath[64];

      if (acct_get(msg_uid, &acct) > 0)
      {
	alog("進入信箱", acct.userid);
	usr_fpath(fpath, acct.userid, fn_dir);
	xz[XZ_MBOX - XO_ZONE].xo = xt = xo_new(fpath);
	xover(XZ_MBOX);
	free(xt);
	xz[XZ_MBOX - XO_ZONE].xo = xo;
      }
      return mbox_init(xo);
    }
#else
    if (HAS_PERM(PERM_ALLADMIN))	/* itoc.030914: 讓所有 ADMINs 都可以看 sysop/guest 信箱 */
    {
      char *userid;
      XO *xt;
      char fpath[64];

      sprintf(fpath, "進入 (1)%s (2)%s 的信箱？[1] ", str_sysop, STR_GUEST);
      userid = (vans(fpath) == '2') ? STR_GUEST : str_sysop;
      alog("進入信箱", userid);
      usr_fpath(fpath, userid, fn_dir);
      xz[XZ_MBOX - XO_ZONE].xo = xt = xo_new(fpath);
      xover(XZ_MBOX);
      free(xt);
      xz[XZ_MBOX - XO_ZONE].xo = xo;
      return mbox_init(xo);
    }
#endif
  }

  return XO_NONE;
}


static int
mbox_gem(xo)		/* itoc.010727: 將個人精華區和看板精華區整合 */
  XO *xo;
{
  char fpath[64];

  usr_fpath(fpath, cuser.userid, "gem/"FN_DIR);
  /* 個人精華區不需要 GEM_X_BIT */
  XoGem(fpath, "個人精華區", GEM_W_BIT | GEM_M_BIT);
  return mbox_init(xo);
}


static int
mbox_copy(xo)		/* itoc.011025: 取代 gem_gather */
  XO *xo;
{
  int tag;

  tag = AskTag("信箱文章拷貝");

  if (tag < 0)
    return XO_FOOT;

  gem_buffer(xo->dir, tag ? NULL : (HDR *) xo_pool + (xo->pos - xo->top), NULL);

  zmsg("檔案標記完成。[注意] 拷貝後才能刪除原文！");
  return mbox_gem(xo);		/* 拷貝完直接進精華區 */
}


static int
mbox_help(xo)
  XO *xo;
{
  xo_help("mbox");
  return mbox_head(xo);
}


static KeyFunc mbox_cb[] =
{
  XO_INIT, mbox_init,
  XO_LOAD, mbox_load,
  XO_HEAD, mbox_head,
  XO_BODY, mbox_body,

  'r', mbox_browse,
  's', mbox_send,
  'd', mbox_delete,
  'D', mbox_rangedel,
  'x', post_cross,
  'X', mbox_forward,
  'E', mbox_edit,
  'T', mbox_title,
  'm', mbox_mark,
  'R', mbox_reply,
  'y', mbox_reply,
  'v', mbox_visit,
  'w', post_write,		/* itoc.010408: 借用 post_write 即可 */

  KEY_TAB, mbox_sysop,
  'z', mbox_gem,
  'c', mbox_copy,
  'g', gem_gather,

  't', mbox_tag,

  '~', XoXselect,		/* itoc.001220: 搜尋作者/標題 */
  'S', XoXsearch,		/* itoc.001220: 搜尋相同標題文章 */
  'a', XoXauthor,		/* itoc.001220: 搜尋作者 */
  '/', XoXtitle,		/* itoc.001220: 搜尋標題 */
  'f', XoXfull,			/* itoc.030608: 全文搜尋 */
  'G', XoXmark,			/* itoc.010325: 搜尋 mark 文章 */
  'L', XoXlocal,		/* itoc.010822: 搜尋本地文章 */

  Ctrl('D'), mbox_prune,
  Ctrl('Q'), xo_uquery,
  Ctrl('O'), xo_usetup,

  'h', mbox_help
};


void
mbox_main()
{
  cmbox.mail_xo.pos = XO_TAIL;
  cmbox.mail_xo.xyz = str_site;
  usr_fpath(cmbox.dir, cuser.userid, fn_dir);
  xz[XZ_MBOX - XO_ZONE].xo = (XO *) &cmbox;
  xz[XZ_MBOX - XO_ZONE].cb = mbox_cb;
}


KeyFunc xmbox_cb[] =
{
  XO_INIT, xpost_init,
  XO_LOAD, xpost_load,
  XO_HEAD, xpost_head,
  XO_BODY, mbox_body,		/* Thor.980911: 共用即可 */

  'r', xmbox_browse,
  'y', mbox_reply,
  's', mbox_send,
  'x', post_cross,
  'X', post_forward,
  'E', mbox_edit,
  'T', mbox_title,
  'm', mbox_mark,
  'd', mbox_delete,
  'w', post_write,		/* itoc.010408: 借用 post_write 即可 */

  'c', mbox_copy,
  'g', gem_gather,

  't', mbox_tag,

  '~', XoXselect,
  'S', XoXsearch,
  'a', XoXauthor,
  '/', XoXtitle,
  'f', XoXfull,
  'G', XoXmark,
  'L', XoXlocal,

  Ctrl('Q'), xo_uquery,
  Ctrl('O'), xo_usetup,

  'h', mbox_help
};
