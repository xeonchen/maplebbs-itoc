/*-------------------------------------------------------*/
/* util/fix_uno.c	( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : 重建所有使用者的 userno			 */
/* create : 04/10/16					 */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "bbs.h"


#undef	VERBOSE		/* 是否顯示詳細訊息 */

#ifdef VERBOSE
#define DEBUG(arg)	printf arg
#else
#define DEBUG(arg)	;
#endif


#define FN_OLDACCT	"olduserno"	/* 記錄全站舊的 userno */


typedef struct
{
  int userno;
  char userid[IDLEN + 1];
}	UNO;


/*-------------------------------------------------------*/
/* 重建 .ACCT 及 .USR					 */
/*-------------------------------------------------------*/


static void
new_acct(userid)
  char *userid;
{
  static int userno = 1;	/* userno 從 1 開始 */
  static time_t now = 100000;	/* 隨便給一個時間 */

  char fpath[64];
  ACCT acct;
  SCHEMA slot;
  UNO uno;

  usr_fpath(fpath, userid, FN_ACCT);
  if (rec_get(fpath, &acct, sizeof(ACCT), 0) < 0)
  {
    /* 如果找不到 .ACCT，要刪除所有名單 */
    usr_fpath(fpath, userid, FN_PAL);
    unlink(fpath);
#ifdef HAVE_LIST
    usr_fpath(fpath, userid, FN_LIST);
    unlink(fpath);
#endif
#ifdef HAVE_ALOHA
    usr_fpath(fpath, userid, FN_ALOHA);
    unlink(fpath);
#endif

    DEBUG(("授予 %s 新的 userno 失敗 => 無法讀取該使用者的資料\n", userid));
    return;
  }

  /* 將原本的 userno 備份給水球列表轉換使用 */
  memset(&uno, 0, sizeof(UNO));
  uno.userno = acct.userno;
  str_ncpy(uno.userid, acct.userid, sizeof(uno.userid));
  rec_add(FN_OLDACCT, &uno, sizeof(UNO));

  acct.userno = userno++;
  unlink(fpath);
  rec_add(fpath, &acct, sizeof(ACCT));

  memset(&slot, 0, sizeof(SCHEMA));
  slot.uptime = now++;
  memcpy(slot.userid, acct.userid, IDLEN);
  rec_add(FN_SCHEMA, &slot, sizeof(SCHEMA));

  DEBUG(("授予 %s 新的 userno 成功\\n", userid));
}


/*-------------------------------------------------------*/
/* 查 userno						 */
/*-------------------------------------------------------*/


static int new_num;
static UNO *new_uno;


static int
uno_cmp_userid(a, b)
  UNO *a, *b;
{
  return strcmp(a->userid, b->userid);
}


static void
collect_new_uno()
{
  int fd, num;
  SCHEMA slot;

  if ((new_num = rec_num(FN_SCHEMA, sizeof(SCHEMA))) > 0)
  {
    new_uno = (UNO *) malloc(new_num * sizeof(UNO));

    if ((fd = open(FN_SCHEMA, O_RDONLY)) >= 0)
    {
      num = 0;
      while (read(fd, &slot, sizeof(SCHEMA)) == sizeof(SCHEMA) && num < new_num)
      {
	new_uno[num].userno = num + 1;
	str_ncpy(new_uno[num].userid, slot.userid, sizeof(new_uno[num].userid));	/* slot.userid 不含 '\0' */
	num++;
      }
      close(fd);
    }

    if (new_num > 1)
      qsort(new_uno, new_num, sizeof(UNO), uno_cmp_userid);
  }
}


static int
acct_uno(userid)	/* 用 ID 找新的 userno */
  char *userid;
{
  UNO uno, *find;

  str_ncpy(uno.userid, userid, sizeof(uno.userid));	/* 其實用 strcpy 即可，但以防萬一 */
  if (find = bsearch(&uno, new_uno, new_num, sizeof(UNO), uno_cmp_userid))
    return find->userno;
  return 0;
}


static int old_num;
static UNO *old_uno;


static int
uno_cmp_userno(a, b)
  UNO *a, *b;
{
  return a->userno - b->userno;
}


static void
collect_old_uno()
{
  int fsize;

  if (old_uno = (UNO *) f_img(FN_OLDACCT, &fsize))
  {
    old_num = fsize / sizeof(UNO);
    if (old_num > 1)
      qsort(old_uno, old_num, sizeof(UNO), uno_cmp_userno);
  }
}


static int
acct_uno2(olduno)	/* 用舊的 userno 找新的 userno */
  int olduno;
{
  UNO uno, *find;

  uno.userno = olduno;
  if (find = bsearch(&uno, old_uno, old_num, sizeof(UNO), uno_cmp_userno))
    return acct_uno(find->userid);
  return 0;
}


/*-------------------------------------------------------*/
/* 重建 pal/list.?/aloha/benz/bpal			 */
/*-------------------------------------------------------*/


#define BENZ_MAX	512	/* 假設每個人的系統協尋不超過 512 人 */

static int rec_max;
static char *rec_pool;


static void
new_pal(userid)
  char *userid;
{
  int fd, num;
  char folder[64];
  PAL pal;

  usr_fpath(folder, userid, FN_PAL);
  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    num = 0;
    while (read(fd, &pal, sizeof(PAL)) == sizeof(PAL) && num < rec_max)
    {
      if ((pal.userno = acct_uno(pal.userid)) > 0)
      {
	memcpy(rec_pool + num * sizeof(PAL), &pal, sizeof(PAL));
	num++;
      }
    }
    close(fd);

    unlink(folder);
    rec_add(folder, rec_pool, num * sizeof(PAL));
    DEBUG(("成功\重建 %s 的朋友名單，共 %d 人\n", userid, num));
  }
}


#ifdef HAVE_LIST
static void
new_list(userid)
  char *userid;
{
  int fd, ch, num;
  char folder[64], fname[16];
  PAL pal;

  for (ch = '1'; ch <= '5'; ch++)
  {
    sprintf(fname, "%s.%c", FN_LIST, ch);
    usr_fpath(folder, userid, fname);
    if ((fd = open(folder, O_RDONLY)) >= 0)
    {
      num = 0;
      while (read(fd, &pal, sizeof(PAL)) == sizeof(PAL) && num < rec_max)
      {
	if ((pal.userno = acct_uno(pal.userid)) > 0)
	{
	  memcpy(rec_pool + num * sizeof(PAL), &pal, sizeof(PAL));
	  num++;
	}
      }
      close(fd);

      unlink(folder);
      rec_add(folder, rec_pool, num * sizeof(PAL));
      DEBUG(("成功\重建 %s 的特殊名單，共 %d 人\n", userid, num));
    }
  }
}
#endif


#ifdef HAVE_ALOHA
static void
new_aloha(userid)
  char *userid;
{
  int fd, num;
  char folder[64];
  ALOHA aloha;

  FRIENZ frienz;
  char fpath[64];

  usr_fpath(folder, userid, FN_ALOHA);
  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    /* 準備好要加入對方的 frienz */
    memset(&frienz, 0, sizeof(FRIENZ));
    strcpy(frienz.userid, userid);
    num = 0;
    if ((frienz.userno = acct_uno(userid)) > 0)
    {
      while (read(fd, &aloha, sizeof(ALOHA)) == sizeof(ALOHA) && num < rec_max)
      {
	if ((aloha.userno = acct_uno(aloha.userid)) > 0)
	{
	  memcpy(rec_pool + num * sizeof(ALOHA), &aloha, sizeof(ALOHA));
	  num++;

	  /* 把自己加入對方的 frienz 中 */
	  usr_fpath(fpath, aloha.userid, FN_FRIENZ);
	  rec_add(fpath, &frienz, sizeof(FRIENZ));
	  DEBUG(("成功\重建 %s 的上站通知名單，共 %d 人\n", userid, num));
	}
      }
    }
    close(fd);

    unlink(folder);
    rec_add(folder, rec_pool, num * sizeof(ALOHA));
  }
}
#endif


#ifdef LOGIN_NOTIFY
static void
new_benz(userid)
  char *userid;
{
  int fd, num;
  char folder[64];
  BENZ benz;

  usr_fpath(folder, userid, FN_BENZ);
  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    num = 0;
    while (read(fd, &benz, sizeof(BENZ)) == sizeof(BENZ) && num < rec_max)
    {
      if ((benz.userno = acct_uno(benz.userid)) > 0)
      {
	memcpy(rec_pool + num * sizeof(BENZ), &benz, sizeof(BENZ));
	num++;
      }
    }
    close(fd);

    unlink(folder);
    rec_add(folder, rec_pool, num * sizeof(BENZ));
    DEBUG(("成功\重建 %s 的系統協尋名單，共 %d 人\n", userid, num));
  }
}
#endif


static void
new_bmw(userid)
  char *userid;
{
  int fsize;
  char folder[64];
  BMW *data, *head, *tail;

  usr_fpath(folder, userid, FN_BMW);
  if (data = (BMW *) f_img(folder, &fsize))
  {
    head = data;
    tail = data + (fsize / sizeof(BMW));
    do
    {
      head->sender = acct_uno2(head->sender);
      head->recver = acct_uno2(head->recver);
    } while (++head < tail);

    unlink(folder);
    rec_add(folder, data, fsize);
    free(data);

    DEBUG(("成功\重建 %s 的水球列表，共 %d 個\n", userid, fsize / sizeof(BMW)));
  }
}


#ifdef HAVE_MODERATED_BOARD
static void
new_bpal(brdname)
  char *brdname;
{
  int fd, num;
  char folder[64];
  PAL pal;

  brd_fpath(folder, brdname, FN_PAL);
  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    num = 0;
    while (read(fd, &pal, sizeof(PAL)) == sizeof(PAL) && num < rec_max)
    {
      if ((pal.userno = acct_uno(pal.userid)) > 0)
      {
	memcpy(rec_pool + num * sizeof(PAL), &pal, sizeof(PAL));
	num++;
      }
    }
    close(fd);

    unlink(folder);
    rec_add(folder, rec_pool, num * sizeof(PAL));
    DEBUG(("成功\重建 %s 的板友名單，共 %d 人\n", brdname, num));
  }
}
#endif


/*-------------------------------------------------------*/
/* 主函式						 */
/*-------------------------------------------------------*/


int
main()
{
  char c;
  char *userid, fpath[64];
  struct dirent *de;
  DIR *dirp;
#ifdef HAVE_MODERATED_BOARD
  FILE *fp;
#endif

  chdir(BBSHOME);

  /* itoc.050113: 先畫一塊記憶體來存，最後再一次寫回，節省 I/O */
  rec_max = PAL_MAX;
#ifdef HAVE_ALOHA
  if (rec_max < ALOHA_MAX)
    rec_max = ALOHA_MAX;
#endif
#ifdef LOGIN_NOTIFY
  if (rec_max < BENZ_MAX)
    rec_max = BENZ_MAX;
#endif
  rec_pool = (char *) malloc(REC_SIZ * rec_max);

  /*-----------------------------------------------------*/
  /* 第一圈: 授予新的 userno，並刪除 frienz		 */
  /*-----------------------------------------------------*/

  unlink(FN_SCHEMA);

  for (c = 'a'; c <= 'z'; c++)
  {
    printf("授予新的 userno: 開始處理 %c 開頭的 ID\n", c);
    sprintf(fpath, "usr/%c", c);

    if (!(dirp = opendir(fpath)))
      continue;

    while (de = readdir(dirp))
    {
      userid = de->d_name;
      if (*userid <= ' ' || *userid == '.')
	continue;

      /* 將 .ACCT 換新的 userno，並寫回 .USR */
      new_acct(userid);

#ifdef HAVE_ALOHA
      /* 刪除 frienz */
      usr_fpath(fpath, userid, FN_FRIENZ);
      unlink(fpath);
#endif
    }

    closedir(dirp);
  }

  collect_new_uno();
  collect_old_uno();
  printf("授予所有人新的 userno 完成，全站共 %d 人\n", new_num);


  /*-----------------------------------------------------*/
  /* 第二圈: 重建所有人的 pal/list.?/aloha/benz/bmw	 */
  /*-----------------------------------------------------*/

  for (c = 'a'; c <= 'z'; c++)
  {
    printf("重建新的 pal/list/aloha/benz: 開始處理 %c 開頭的 ID\n", c);
    sprintf(fpath, "usr/%c", c);

    if (!(dirp = opendir(fpath)))
      continue;

    while (de = readdir(dirp))
    {
      userid = de->d_name;
      if (*userid <= ' ' || *userid == '.')
	continue;

      new_pal(userid);
#ifdef HAVE_LIST
      new_list(userid);
#endif
#ifdef HAVE_ALOHA
      new_aloha(userid);
#endif
#ifdef LOGIN_NOTIFY
      new_benz(userid);
#endif
      new_bmw(userid);
    }

    closedir(dirp);
  }


#ifdef HAVE_MODERATED_BOARD
  /*-----------------------------------------------------*/
  /* 第三圈: 重建所有看板的 bpal			 */
  /*-----------------------------------------------------*/

  printf("重建新的 bpal\n");
  if (fp = fopen(FN_BRD, "r"))
  {
    BRD brd;
    while (fread(&brd, sizeof(BRD), 1, fp) == 1)
    {
      if (*brd.brdname)
	new_bpal(brd.brdname);
    }
  }
#endif

  free(rec_pool);
  free(new_uno);
  free(old_uno);
  unlink(FN_OLDACCT);

  return 0;
}
