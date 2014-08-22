/*-------------------------------------------------------*/
/* history.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : innbbsd history				 */
/* create : 04/04/01					 */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "innbbsconf.h"


typedef struct
{
  time_t htime;			/* 加入 history 檔的時間 */
  int hash;			/* 為了快速搜尋 */
  char msgid[256];		/* message id (假設 256 已夠長) */
  char board[BNLEN + 1];
  char xname[9];
}	HIS;


void 
HISmaint()			/* 維護 history 檔，將過早的 history 刪除 */
{
  int i, fd, total;
  char fpath[64];
  time_t now;
  struct stat st;
  HIS *data, *hhead, *htail, *his;

  /* 只保留最近 EXPIREDAYS 天的 history */
  time(&now);
  now = time(NULL) - EXPIREDAYS * 86400;

  for (i = 0; i < 32; i++)
  {
    sprintf(fpath, "innd/history/%02d", i);

    if ((fd = open(fpath, O_RDONLY)) < 0)
      continue;

    fstat(fd, &st);
    data = (HIS *) malloc(total = st.st_size);
    total = read(fd, data, total);
    close(fd);

    hhead = data;
    htail = data + total / sizeof(HIS);
    total = 0;

    for (his = hhead; his < htail; his++)
    {
      if (his->htime > now)	/* 這筆 history 不被砍 */
      {
	memcpy(hhead, his, sizeof(HIS));
	hhead++;
	total += sizeof(HIS);
      }
    }

    if ((fd = open(fpath, O_WRONLY | O_CREAT | O_TRUNC, 0600)) >= 0)
    {
      write(fd, data, total);
      close(fd);
    }

    free(data);
  }
}


void 
HISadd(msgid, board, xname)	/* 將 (msgid, path, xname) 此配對記錄在 history 中 */
  char *msgid;
  char *board;
  char *xname;
{
  HIS his;
  char fpath[64];

  memset(&his, 0, sizeof(HIS));

  time(&(his.htime));
  his.hash = str_hash(msgid, 1);
  str_ncpy(his.msgid, msgid, sizeof(his.msgid));
  str_ncpy(his.board, board, sizeof(his.board));
  str_ncpy(his.xname, xname, sizeof(his.xname));

  /* 依 msgid 將 history 打散至 32 個檔案 */
  sprintf(fpath, "innd/history/%02d", his.hash & 31);
  rec_add(fpath, &his, sizeof(HIS));
}


int				/* 1:在history中 0:不在history中 */
HISfetch(msgid, board, xname)	/* 查詢 history 中，msgid 發表去了哪裡 */
  char *msgid;
  char *board;			/* 傳出在 history 中的記錄的看板及檔名 */
  char *xname;
{
  HIS his;
  char fpath[64];
  int fd, hash;
  int rc = 0;

  /* 如果同一 msgid 發表去很多個看板，那麼目前只會回傳第一個看板及檔名 */

  /* 依 msgid 找出在哪一份 history 檔案中 */
  hash = str_hash(msgid, 1);
  sprintf(fpath, "innd/history/%02d", hash & 31);

  /* 去該份 history 檔案中找看看有沒有 */
  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    lseek(fd, 0, SEEK_SET);
    while (read(fd, &his, sizeof(HIS)) == sizeof(HIS))
    {
      /* 用 hash 先粗略比對，若相同再用 msgid 完整比對 */
      if ((hash == his.hash) && !strcmp(msgid, his.msgid))
      {
	if (board)
	  strcpy(board, his.board);
	if (xname)
	  strcpy(xname, his.xname);
	rc = 1;
	break;
      }
    }
    close(fd);
  }

  return rc;
}
