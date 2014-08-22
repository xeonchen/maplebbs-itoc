/*-------------------------------------------------------*/
/* bbslink.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : innbbsd NNTP and NNRP			 */
/* create : 95/04/27					 */
/* update : 04/10/23					 */
/* author : skhuang@csie.nctu.edu.tw			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "innbbsconf.h"
#include "bbslib.h"
#include "inntobbs.h"
#include "nntp.h"
#include <stdarg.h>


#if 0	/* itoc.030122.註解: 程式流程 */

  0. bbsd 會把新文章的檔頭記錄在 out.bntp

  1. 執行本程式以後，在 main() 處理一下參數

  2. 在 main():initial_bbs() 讀出設定檔，然後進入 bbslink()

  3. 在 bbslink():deal_bntp() 中首先處理 out.bntp
     由於 out.bntp 是把所有板的新文章都放在一起，所以在這裡把這 out.bntp 檔依站台分去 *.link

  4. 在 bbslink():visit_site() 中依以下步驟，一一拜訪各站

     4.1. open_connect() 開啟連線
     4.2. send_outgoing() 把本站台對應的 link 檔一筆一筆讀出來，把信送去對方站
     4.3. readnews() 依序讀取每個想要的 newsgroup，並取對方站的信
     4.4. close_connect() 關閉連線

  [註] 即使沒有啟動 innbbsd，也可以使用 bbslink

#endif


static int SERVERfd = -1;
static FILE *SERVERrfp = NULL;
static FILE *SERVERwfp = NULL;
static char SERVERbuffer[1024];


/* itoc.030122.註解: 以下這幾個在指定參數時才有用 */
static int Verbose = 0;			/* 1: 顯示詳細訊息 */
static int KillFormerProc = 0;		/* 1: 刪除上次執行失敗的 bbslink */
static int ResetActive = 0;		/* 1: 將 high-number 更新到與 news server 上相同 */
static int MaxArts = MAX_ARTS;		/* 對 news server 每個群組最多只抓幾封文章 */
static char *DefaultProcSite = NULL;	/* !=NULL: 只處理某特定站台 */


#define DEBUG(arg)	if (Verbose) printf arg


/*-------------------------------------------------------*/
/* 處理 bntp 檔						 */
/*-------------------------------------------------------*/


static nodelist_t *
search_nodelist_bynode(name)
  char *name;
{
  nodelist_t nl;

  str_ncpy(nl.name, name, sizeof(nl.name));
  return bsearch(&nl, NODELIST, NLCOUNT, sizeof(nodelist_t), nl_bynamecmp);
}


static newsfeeds_t *
search_newsfeeds_byboard(board)
  char *board;
{
  newsfeeds_t nf;

  str_ncpy(nf.board, board, sizeof(nf.board));
  return bsearch(&nf, NEWSFEEDS_B, NFCOUNT, sizeof(newsfeeds_t), nf_byboardcmp);
}


typedef struct
{
  char board[BNLEN + 1];
  char filename[9];
  char group[80];
  char from[80];
  char title[80];
  char date[40];
  char msgid[80];
  char control[80];
  char charset[20];
}	soverview_t;


static void
queuefeed(node, sover)
  nodelist_t *node;
  soverview_t *sover;
{
  int fd;

  /* itoc.030122.註解: *.link 檔是依站台分好 待送(或送不成)的 batch */

  if (node->feedfd < 0)
  {
    char linkfile[64];

    sprintf(linkfile, "innd/%s.link", node->name);
    if ((fd = open(linkfile, O_WRONLY | O_CREAT | O_APPEND, 0600)) < 0)
      return;
    node->feedfd = fd;
  }
  else
  {
    fd = node->feedfd;
  }

  /* flock(fd, LOCK_EX); */
  /* Thor.981205: 用 fcntl 取代flock, POSIX標準用法 */
  f_exlock(fd);

  write(fd, sover, sizeof(soverview_t));

  /* flock(fd, LOCK_UN); */
  /* Thor.981205: 用 fcntl 取代flock, POSIX標準用法 */
  f_unlock(fd);
}


static char *
Gtime(now)
  time_t now;
{
  static char datemsg[40];

  strftime(datemsg, sizeof(datemsg), "%d %b %Y %X GMT", gmtime(&now));
  return datemsg;
}


static void
deal_sover(bntp)
  bntp_t *bntp;
{
  newsfeeds_t *nf;
  nodelist_t *nl;
  soverview_t sover;
  time_t mtime;
  char buf[80];
  char *board, *filename;

  board = bntp->board;

  if (!(nf = search_newsfeeds_byboard(board)))
  {
    bbslog("<bbslink> :Warn: %s 此板不在 newsfeeds.bbs 中\n", board);
    DEBUG(("─→:Warn: %s 此板不在 newsfeeds.bbs 中\n", board));
    return;
  }

  if (!(nl = search_nodelist_bynode(nf->path)))
    return;

  filename = bntp->xname;

  memset(&sover, 0, sizeof(soverview_t));

  if (bntp->chrono > 0)		/* 新信 */
  {
    mtime = bntp->chrono;
    str_ncpy(sover.title, bntp->title, sizeof(sover.title));
    sprintf(sover.msgid, "%s$%s@" MYHOSTNAME, board, filename);
  }
  else				/* cancel */
  {
    time(&mtime);
    sprintf(buf, "%s$%s@" MYHOSTNAME, board, filename);		/* 欲砍文章的 Message-ID */
    sprintf(sover.title, "cmsg cancel <%s>", buf);
    sprintf(sover.msgid, "C%s$%s@" MYHOSTNAME, board, filename);/* LHD.030628: 在原 msgid 加任意字串當作 cmsg 的 Message-ID */
    sprintf(sover.control, "cancel <%s>", buf);
  }

  str_ncpy(sover.board, board, sizeof(sover.board));
  str_ncpy(sover.filename, filename, sizeof(sover.filename));
  sprintf(sover.from, "%s.bbs@" MYHOSTNAME " (%s)", bntp->owner, bntp->nick);
  str_ncpy(sover.date, Gtime(mtime), sizeof(sover.date));
  str_ncpy(sover.group, nf->newsgroup, sizeof(sover.group));
  str_ncpy(sover.charset, nf->charset, sizeof(sover.charset));

  queuefeed(nl, &sover);
}


static void
deal_bntp()
{
  char *OUTING = "innd/.outing";		/* 處理時暫存的檔 */
  int fd, i;
  nodelist_t *node;
  bntp_t bntp;

  if (rename("innd/out.bntp", OUTING))	/* 沒有新文章 */
    return;

  /* initail 各 node 的 feedfd */
  for (i = 0; i < NLCOUNT; i++)
  {
    node = NODELIST + i;
    node->feedfd = -1;
  }

  /* 貼到各個站台所屬的 *.link */
  if ((fd = open(OUTING, O_RDONLY)) >= 0)
  {
    while (read(fd, &bntp, sizeof(bntp_t)) == sizeof(bntp_t))
      deal_sover(&bntp);
    close(fd);
  }

  /* close 各 node 的 feedfd */
  for (i = 0; i < NLCOUNT; i++)
  {
    node = NODELIST + i;
    if (node->feedfd >= 0)
      close(node->feedfd);
  }

  unlink(OUTING);
}


/*-------------------------------------------------------*/
/* 連去某個站						 */
/*-------------------------------------------------------*/


static int
inetclient(server, port)
  char *server;
  int port;
{
  struct hostent *host;		/* host information entry */
  struct sockaddr_in sin;	/* Internet endpoint address */
  int fd;

  if (!*server || !port)
    return -1;

  memset(&sin, 0, sizeof(sin));

  if (!(host = gethostbyname(server)))
    sin.sin_addr.s_addr = inet_addr(server);
  else
    memcpy(&sin.sin_addr.s_addr, host->h_addr, host->h_length);

  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);

  /* Allocate a socket */
  fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd < 0)
    return -1;

  /* Connect the socket to the server */
  if (connect(fd, (struct sockaddr *) & sin, sizeof(sin)) < 0)
  {
    close(fd);
    return -1;
  }

  return fd;
}


static int
tcpcommand(char *fmt, ...)
{
  va_list args;
  char *ptr;

  va_start(args, fmt);
  vfprintf(SERVERwfp, fmt, args);
  va_end(args);
  fprintf(SERVERwfp, "\r\n");
  fflush(SERVERwfp);

  if (!fgets(SERVERbuffer, sizeof(SERVERbuffer), SERVERrfp))
    return 0;

  if (ptr = strchr(SERVERbuffer, '\r'))
    *ptr = '\0';
  if (ptr = strchr(SERVERbuffer, '\n'))
    *ptr = '\0';

  return atoi(SERVERbuffer);
}


static int			/* 200~202:成功 0:失敗 */
open_connect(node)		/* 連去這個站 */
  nodelist_t *node;
{
  char *host = node->host;
  int port = node->port;

  DEBUG(("╭<open_connect> 正在開啟連線\n"));

  if ((SERVERfd = inetclient(host, port)) < 0)
  {
    bbslog("<bbslink> :Err: 伺服器連線失敗：%s %d\n", host, port);
    DEBUG(("╰<open_connect> 伺服器連線失敗\n"));
    return 0;
  }

  if (!(SERVERrfp = fdopen(SERVERfd, "r")) || !(SERVERwfp = fdopen(SERVERfd, "w")))
  {
    bbslog("<bbslink> :Err: fdopen 發生錯誤\n");
    DEBUG(("╰<open_connect> fdopen 發生錯誤\n"));
    return 0;
  }

  if (!fgets(SERVERbuffer, sizeof(SERVERbuffer), SERVERrfp) || SERVERbuffer[0] != '2')	/* 200 201 202 都能取信 */
  {
    bbslog("<bbslink> :Err: 伺服器拒絕連線：%s %d\n", host, port);
    DEBUG(("╰<open_connect> 伺服器拒絕連線\n"));
    return 0;
  }

  /* itoc.040512: MODE READER 只要講一次就夠了 */
  if (node->xmode & INN_USEPOST)
  {
    tcpcommand("MODE READER");
    if (SERVERbuffer[0] != '2')	/* 200 201 202 都能取信 */
    {
      bbslog("<bbslink> :Err: 伺服器拒絕連線：%s %d\n", host, port);
      DEBUG(("╰<open_connect> 伺服器拒絕連線\n"));
      return 0;
    }
  }

  DEBUG(("│<open_connect> 伺服器連線成功\\n"));
  return atoi(SERVERbuffer);
}


static void
close_connect()		/* 結束連去這個站 */
{
  int status;

  status = tcpcommand("QUIT");
  if (status != NNTP_GOODBYE_ACK_VAL && status != 221)
  {
    bbslog("<bbslink> :Warn: 無法正常斷線\n");
    DEBUG(("│<close_connect> 無法正常斷線\n"));
  }

  DEBUG(("╰<close_connect> 已關閉連線\n"));

  if (SERVERrfp)
    fclose(SERVERrfp);
  if (SERVERwfp)
    fclose(SERVERwfp);
  if (SERVERfd >= 0)
    close(SERVERfd);
}


/*-------------------------------------------------------*/
/* 送出文章						 */
/*-------------------------------------------------------*/


static int			/* -1:失敗 */
sover_post(sover)
  soverview_t *sover;
{
  if (sover->control[0])	/* 送出 cancel message */
  {
    static char BODY_BUF[128];

    sprintf(BODY_BUF, "%s\r\n", sover->title);
    BODY = BODY_BUF;	/* cancel message 時，BODY 指向 BODY_BUF */
  }
  else				/* 送出新文章 */
  {
    static char *BODY_BUF;
    char *ptr, *str, fpath[64];
    int fd, size;
    struct stat st;

    /* 檢查文章還在不在 */
    sprintf(fpath, "brd/%s/%c/%s", sover->board, sover->filename[7], sover->filename);
    if ((fd = open(fpath, O_RDONLY)) < 0)
      return -1;
    fstat(fd, &st);
    size = st.st_size;
    if (size <= 0)
    {
      close(fd);
      return -1;
    }

    /* 一般文章時，BODY 指向 malloc 生出來的區塊 */

    BODY_BUF = !BODY_BUF ? (char *) malloc(size + 1) : (char *) realloc(BODY_BUF, size + 1);
    read(fd, BODY_BUF, size);
    close(fd);
    ptr = BODY_BUF + size;
    *ptr = '\0';

    /* 跳過文章的前幾行檔頭不要 */
    for (str = BODY_BUF;;str = ptr + 1)
    {
      ptr = strchr(str, '\n');
      if (!ptr)			/* 找到文章最後了還找不到空行，那麼整個檔案都當做內文 */
      {
	BODY = BODY_BUF;
	break;
      }

      if (ptr == str)		/* 找到一行空行，那麼以下就都是內文了 */
      {
	BODY = str + 1;
	break;
      }
    }
  }

  if (sover->charset[0] == 'g')
  {
    b52gb(BODY);
    b52gb(sover->from);
    b52gb(sover->title);
  }

  return 0;
}


static void
fail_post(msgid)
  char *msgid;
{
  bbslog("<bbslink> :Warn: %s <%s>\n", SERVERbuffer, msgid);
  DEBUG(("│→:Warn: %s <%s>\n", SERVERbuffer, msgid));
}


static void
send_outgoing(node, sover)
  nodelist_t *node;
  soverview_t *sover;
{
  int cc, status;
  char *msgid, *str;

  msgid = sover->msgid;

  DEBUG(("│┌ MSGID: %s\n", msgid));
  DEBUG(("││ GROUP: %s\n", sover->group));
  DEBUG(("││ FROM : %s\n", sover->from));
  DEBUG(("│└ SUBJ : %s\n", sover->title));

  /* 先把文章準備好 */
  if (sover_post(sover) < 0)
  {
    DEBUG(("│→ 本篇文章已遭刪除或檔案遺失，取消送出\n"));
    return;
  }

  /* 向 server 送出 IHAVE/POST 要求 */
  if (node->xmode & INN_USEIHAVE)
  {
    status = tcpcommand("IHAVE <%s>", msgid);
    if (status != NNTP_SENDIT_VAL)
    {
      fail_post(msgid);
      return;
    }
  }
  else /* if (node->xmode & INN_USEPOST) */
  {
    status = tcpcommand("POST");
    if (status != NNTP_START_POST_VAL)
    {
      fail_post(msgid);
      return;
    }
  }

  /* 寫入文章的檔頭 */
  fprintf(SERVERwfp, "Path: %s\r\n", MYBBSID);
  fprintf(SERVERwfp, "From: %s\r\n", sover->from);
  fprintf(SERVERwfp, "Newsgroups: %s\r\n", sover->group);
  /* fprintf(SERVERwfp, "Subject: %s\r\n", sover->title); */
  output_rfc2047_qp(SERVERwfp, "Subject: ", sover->title, sover->charset, "\r\n");
  fprintf(SERVERwfp, "Date: %s\r\n", sover->date);
  fprintf(SERVERwfp, "Organization: %s\r\n", *sover->charset == 'b' ? BBSNAME : BBSNAME2);	/* itoc.040425: 若不是 big5 就用英文站名 */
  fprintf(SERVERwfp, "Message-ID: <%s>\r\n", msgid);
  fprintf(SERVERwfp, "Mime-Version: 1.0\r\n");
  fprintf(SERVERwfp, "Content-Type: text/plain; charset=\"%s\"\r\n", sover->charset);
  fprintf(SERVERwfp, "Content-Transfer-Encoding: 8bit\r\n");
  if (sover->control[0])
    fprintf(SERVERwfp, "Control: %s\r\n", sover->control);
  fputs("\r\n", SERVERwfp);	/* 檔頭和內文空一行 */

  /* 寫入文章的內容 */
  for (str = BODY; cc = *str; str++)
  {
    if (cc == '\n')
    {
      /* itoc.030127.註解: 把 "\n" 換成 "\r\n" */
      fputc('\r', SERVERwfp);
    }
    else if (cc == '.')
    {
      /* If the text contained a period as the first character of the text 
         line in the original, that first period is doubled. */
      if (str == BODY || str[-1] == '\n')
        fputc('.', SERVERwfp);
    }
      
    fputc(cc, SERVERwfp);
  }

  /* IHAVE/POST 結束 */
  status = tcpcommand(".");
  if (node->xmode & INN_USEIHAVE)
  {
    if (status != NNTP_TOOKIT_VAL)
      fail_post(msgid);
  }
  else /* if (node->xmode & INN_USEPOST) */
  {
    if (status != NNTP_POSTEDOK_VAL)
      fail_post(msgid);
  }
}


/*-------------------------------------------------------*/
/* 對 news server 下指令				 */
/*-------------------------------------------------------*/


static int		/* 1:成功 0:失敗 */
NNRPgroup(newsgroup, low, high)	/* 切換 group，並傳回 low-number 及 high-number */
  char *newsgroup;
  int *low, *high;
{
  int i;
  char *ptr;

  if (tcpcommand("GROUP %s", newsgroup) != NNTP_GROUPOK_VAL)
    return 0;

  ptr = SERVERbuffer;

  /* 找 SERVERbuffer 的第二個 ' ' */
  for (i = 0; i < 2; i++)
  {
    ptr++;
    if (!*ptr || !(ptr = strchr(ptr, ' ')))
      return 0;
  }
  if ((i = atoi(ptr + 1)) >= 0)
    *low = i;

  /* 找 SERVERbuffer 的第三個 ' ' */
  ptr++;
  if (!*ptr || !(ptr = strchr(ptr, ' ')))
    return 0;
  if ((i = atoi(ptr + 1)) >= 0)
    *high = i;

  return 1;
}


static char *tempfile = "innd/bbslinktmp";

static int			/* 1:成功 0:失敗 */
NNRParticle(artno)		/* 取回第 artno 篇的全文 */
  int artno;
{
  FILE *fp;
  char *ptr;

  if (tcpcommand("ARTICLE %d", artno) != NNTP_ARTICLE_FOLLOWS_VAL)
    return 0;

  if (!(fp = fopen(tempfile, "w")))
    return 0;

  while (fgets(SERVERbuffer, sizeof(SERVERbuffer), SERVERrfp))
  {
    if (ptr = strchr(SERVERbuffer, '\r'))
      *ptr = '\0';
    if (ptr = strchr(SERVERbuffer, '\n'))
      *ptr = '\0';

    if (!strcmp(SERVERbuffer, "."))	/* 文章結束 */
      break;

    fprintf(fp, "%s\n", SERVERbuffer);
  }

  fclose(fp);
  return 1;
}



#if 0	/* itoc.030109.註解: my_post 的流程 */
            ┌→ receive_article() → bbspost_add()
  my_post() ├→ receive_nocem()   → 送去 nocem.c 處理
            └→ cancel_article()  → bbspost_cancel()
#endif


static void
my_post()
{
  int rel, size;
  char *ptr, *data;
  struct stat st;

  if ((rel = open(tempfile, O_RDONLY)) >= 0)
  {
    fstat(rel, &st);
    size = st.st_size;
    data = (char *) malloc(size + 1);	/* 保留 1 byte 給 '\0' */
    size = read(rel, data, size);
    close(rel);

    if (size >= 2)
    {
      if (data[size - 2] == '\n')	/* 把最後重覆的 '\n' 換成 '\0' */
        size--;
    }
    data[size] = '\0';		/* 補上 '\0' */

    rel = readlines(data - 1);

    if (rel > 0)
    {
      if (ptr = CONTROL)
      {
	if (!str_ncmp(ptr, "cancel ", 7))
	  rel = cancel_article(ptr + 7);
      }
      else
      {
#ifdef _NoCeM_
	if (strstr(SUBJECT, "@@") && strstr(BODY, "NCM") && strstr(BODY, "PGP"))
	  rel = receive_nocem();
	else
#endif
	  rel = receive_article();
      }

      if (rel < 0)
      {
	DEBUG(("│→<my_post> 接收文章失敗\n"));
      }
    }
    else if (rel == 0)		/* PATH包括自己 */
    {
      DEBUG(("│→<my_post> PATH 包括自己\n"));
    }
    else /* if (rel < 0) */	/* 檔頭欄位不完整 */
    {
      DEBUG(("│→<my_post> 檔頭欄位不完整\n"));
    }

    free(data);
  }

  unlink(tempfile);
}


/*-------------------------------------------------------*/
/* 更新 high-number					 */
/*-------------------------------------------------------*/


static int
nf_samegroup(nf)
  newsfeeds_t *nf;
{
  return !strcmp(nf->newsgroup, GROUP) && !strcmp(nf->path, NODENAME);
}


static void
changehigh(hdd, ram)
  newsfeeds_t *hdd, *ram;
{
  if (ram->high >= 0)
  {
    hdd->high = ram->high;
    hdd->xmode &= ~INN_ERROR;
  }
  else
  {
    hdd->xmode |= INN_ERROR;
  }
}


static void
updaterc(nf, pos, high)
  newsfeeds_t *nf;
  int pos;			/* 於 newsfeeds.bbs 裡面的位置 */
  int high;			/* >=0:目前抓到哪一篇 <0:error */
{
  nf->high = high;
  GROUP = nf->newsgroup;
  rec_ref("innd/newsfeeds.bbs", nf, sizeof(newsfeeds_t), pos, nf_samegroup, changehigh);
}


/*-------------------------------------------------------*/
/* 抓取文章						 */
/*-------------------------------------------------------*/


static void
readnews(node)
  nodelist_t *node;
{
  int i, low, high, artcount, artno;
  char *name, *newsgroup;
  newsfeeds_t *nf;

  name = node->name;

  for (i = 0; i < NFCOUNT; i++)	/* 依序讀取每個 newsgroup */
  {
    nf = NEWSFEEDS + i;

    if (strcmp(name, nf->path))	/* 如果不是這個站台就跳過 */
      continue;

    newsgroup = nf->newsgroup;

    DEBUG(("│┌<readnews> 進入 %s\n", newsgroup));

    /* 取得 news server 上的 low-number 及 high-number */
    if (!NNRPgroup(newsgroup, &low, &high))
    {
      updaterc(nf, i, -1);
      DEBUG(("│└<readnews> 無法取得此群組的 low-number 及 high-number 或此群組不存在\n"));
      continue;		/* 此群組不存在，輪下一個群組 */
    }

    if (ResetActive)
    {
      if (nf->high != high || nf->xmode & INN_ERROR)
        updaterc(nf, i, high);
      DEBUG(("│└<readnews> 結束 %s，此群組之 high-number 已與伺服器同步\n", newsgroup));
      continue;		/* 若 ResetActive 則不取信，輪下一個群組 */
    }

    if (nf->high >= high)
    {
      if (nf->high > high || nf->xmode & INN_ERROR)	/* server re-number */
	updaterc(nf, i, high);

      DEBUG(("│└<readnews> 結束 %s，此群組已沒有新文章\n", newsgroup));
      continue;		/* 這群組已沒有新文章，輪下一個群組 */
    }

    if (nf->high < low - 1)				/* server re-number */
    {
      updaterc(nf, i, high);
      DEBUG(("│└<readnews> 結束 %s，此群組之 high-number 因伺服器異動而更新\n", newsgroup));
      continue;		/* 這群組變更過 low-number，輪下一個群組 */
    }

    /* 取回群組上第 nf->high + 1 開始的 MaxArts 篇的文章 */

    artcount = 0;
    for (artno = nf->high + 1;; artno++)
    {
      if (NNRParticle(artno))
      {
	DEBUG(("││<readnews> [%d] 正取回群組上第 %d 篇文章\n", artcount, artno));
	my_post();
	if (++artcount >= MaxArts)
	  break;
      }
      if (artno >= high)
	break;
    }

    updaterc(nf, i, artno);

    DEBUG(("│└<readnews> 結束 %s，一共取回 %d 篇新文章\n", newsgroup, artcount));
  }			/* end for() */
}


/*-------------------------------------------------------*/
/* lock/unlock 程式，同時只能有一個 bbslink 在跑	 */
/*-------------------------------------------------------*/


static char *lockfile = "innd/bbslinking";

static void
bbslink_un_lock()
{
  unlink(lockfile);
}


static int
bbslink_get_lock()
{
  int fd;
  char buf[10];

  if ((fd = open(lockfile, O_RDONLY)) >= 0)
  {
    int pid;
    struct stat st;

    /* lockfile 已存在，代表有 bbslink 正在跑 */

    if (read(fd, buf, sizeof(buf)) > 0 && (pid = atoi(buf)) > 0 && kill(pid, 0) == 0)
    {
      /* 如果卡太久，就自動 kill 掉 */
      if (KillFormerProc || (!fstat(fd, &st) && st.st_mtime > time(NULL) + BBSLINK_EXPIRE))
      {
	kill(pid, SIGTERM);
      }
      else
      {
	DEBUG(("有另外一個 bbslink 的 process [%d] 正在運作中\n", pid));
	return 0;
      }
    }

    close(fd);

    bbslink_un_lock();
  }

  sprintf(buf, "%d\n", getpid());
  f_cat(lockfile, buf);

  return 1;
}


/*-------------------------------------------------------*/
/* 主程式						 */
/*-------------------------------------------------------*/


static void
visit_site(node)
  nodelist_t *node;
{
  int status, response, fd, num;
  char linkfile[64];
  soverview_t sover;

  NODENAME = node->name;

  /* 若有指定只處理某特定站，那麼就只處理該站台 */
  if (DefaultProcSite && strcmp(NODENAME, DefaultProcSite))
  {
    DEBUG(("→ 這並非所指定要處理的站台，直接跳過\n"));
    return;
  }

  status = 0;
  sprintf(linkfile, "innd/%s.link", NODENAME);
  if (dashf(linkfile))
    status ^= 0x01;
  if (!(node->xmode & INN_FEEDED))
    status ^= 0x02;

  if (!status)		/* 不需要去拜訪對方 */
  {
    DEBUG(("→ 此站台沒有新信待送且被餵信，不需要去拜訪\n"));
    return;
  }

  if (!(response = open_connect(node)))		/* 連線失敗 */
    return;

  if (status & 0x01)	/* 有新信待送 */
  {
    if (response == NNTP_POSTOK_VAL)
    {
      /* 把 linkfile 裡面所記錄要送的信一一送出 */
      num = 0;
      if ((fd = open(linkfile, O_RDONLY)) >= 0)
      {
	while (read(fd, &sover, sizeof(soverview_t)) == sizeof(soverview_t))
	{
	  send_outgoing(node, &sover);
	  num++;
	}
	close(fd);
	unlink(linkfile);
      }
      DEBUG(("│→ 總共送出 %d 篇文章\n", num));
    }
    else
    {
      DEBUG(("│→ 沒有在此站台發表文章的權限\n"));
    }
  }
  else
  {
    DEBUG(("│→ 沒有新信待送\n"));
  }

  if (status & 0x02)	/* 需要連去取信 */
  {
    readnews(node);
  }
  else
  {
    DEBUG(("│→ 此站台設定被餵信，不需要去取信\n"));
  }

  close_connect();
}


static void
bbslink()
{
  int i;
  nodelist_t *node;

  /* 訊息顯示 */
  DEBUG(("─────────────────────────────────────\n"));
  DEBUG(("※ nodelist.bbs 裡面一共有 %d 個站台，接下來將一一去拜訪\n", NLCOUNT));
  DEBUG(("※ 參數設定：\n"));
  DEBUG(("   (1) 刪除上次執行失敗的 bbslink：%s\n", KillFormerProc ? "是" : "否"));
  DEBUG(("   (2) 將 high-number 更新到與 news server 上相同：%s\n", ResetActive ? "是" : "否"));
  DEBUG(("   (3) 對 news server 每個群組最多只抓 %d 封文章\n", MaxArts));
  DEBUG(("   (4) 只處理某特定站台或是處理所有站台：%s\n", DefaultProcSite ? DefaultProcSite : "處理所有站台"));

  DEBUG(("─────────────────────────────────────\n"));
  DEBUG(("◎ 開始處理 out.bntp，整理要送出去的文章\n"));
  deal_bntp();
  DEBUG(("◎ out.bntp 整理完成\n"));
  DEBUG(("─────────────────────────────────────\n"));

  /* 把 nodelist.bbs 中的所有站台都去拜訪一遍 */
  for (i = 0; i < NLCOUNT; i++)
  {
    node = NODELIST + i;
    DEBUG(("◎ [%d] 開始拜訪 <%s> %s (%d)\n", i + 1, node->name, node->host, node->port));
    visit_site(node);
    DEBUG(("─────────────────────────────────────\n"));
  }
}


static void
usage(argv)
  char *argv;
{
  printf("Usage: %s [options]\n", argv);
  printf("       -c  將 high-number 與伺服器上同步(不取信)\n");
  printf("       -k  砍掉目前正在跑的 bbslink，並重新啟動 bbslink\n");
  printf("       -v  顯示詳細的連線過程\n");
  printf("       -a ######  指定每個群組最多取幾封信(預設 %d 封)\n", MAX_ARTS);
  printf("       -s site    只取這個站台的文章\n");
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int c, errflag = 0;

  chdir(BBSHOME);
  umask(077);

  while ((c = getopt(argc, argv, "a:s:ckv")) != -1)
  {
    switch (c)
    {
    case 'a':
      if ((c = atoi(optarg)) > 0)
	MaxArts = c;
      break;

    case 's':
      DefaultProcSite = optarg;
      break;

    case 'c':
      ResetActive = 1;
      break;

    case 'k':
      KillFormerProc = 1;
      break;

    case 'v':
      Verbose = 1;
      break;

    default:
      errflag++;
      break;
    }
  }

  if (errflag > 0)
  {
    usage(argv[0]);
    return -1;
  }

  /* 開始 bbslink，將 bbslink 鎖住 */
  if (!bbslink_get_lock())
    return -1;

  init_bshm();

  if (initial_bbs())
    bbslink();

  /* 結束 bbslink，將 bbslink 解開 */
  bbslink_un_lock();

  return 0;
}
