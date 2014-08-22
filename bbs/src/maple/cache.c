/*-------------------------------------------------------*/
/* cache.c	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : cache up data by shared memory		 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"
#include <sys/ipc.h>
#include <sys/shm.h>


#ifdef	HAVE_SEM
#include <sys/sem.h>
#endif


#ifdef MODE_STAT 
UMODELOG modelog; 
time_t mode_lastchange; 
#endif


#ifdef	HAVE_SEM

/* ----------------------------------------------------- */
/* semaphore : for critical section			 */
/* ----------------------------------------------------- */


static int ap_semid;


static void
attach_err(shmkey)
  int shmkey;
{
  char buf[20];

  sprintf(buf, "key = %x", shmkey);
  blog("shmget", buf);
  exit(1);
}


void
sem_init()
{
  int semid;

  union semun
  {
    int val;
    struct semid_ds *buf;
    ushort *array;
  }     arg =
  {
    1
  };

  semid = semget(BSEM_KEY, 1, 0);
  if (semid == -1)
  {
    semid = semget(BSEM_KEY, 1, IPC_CREAT | BSEM_FLG);
    if (semid == -1)
      attach_err(BSEM_KEY);
    semctl(semid, 0, SETVAL, arg);
  }
  ap_semid = semid;
}


void
sem_lock(op)
  int op;			/* op is BSEM_ENTER or BSEM_LEAVE */
{
  struct sembuf sops;

  sops.sem_num = 0;
  sops.sem_flg = SEM_UNDO;
  sops.sem_op = op;
  semop(ap_semid, &sops, 1);
}


#endif	/* HAVE_SEM */


/*-------------------------------------------------------*/
/* .UTMP cache						 */
/*-------------------------------------------------------*/


UCACHE *ushm;


void
ushm_init()
{
  ushm = shm_new(UTMPSHM_KEY, sizeof(UCACHE));
}


void
utmp_mode(mode)
  int mode;
{
  if (bbsmode != mode)
  {
#ifdef MODE_STAT
    time_t now;

    time(&now);
    modelog.used_time[bbsmode] += (now - mode_lastchange);
    mode_lastchange = now;
#endif

    cutmp->mode = bbsmode = mode;
  }
}


int
utmp_new(up)
  UTMP *up;
{
  UCACHE *xshm;
  UTMP *uentp, *utail;

  /* --------------------------------------------------- */
  /* semaphore : critical section			 */
  /* --------------------------------------------------- */

#ifdef	HAVE_SEM
  sem_lock(BSEM_ENTER);
#endif

  xshm = ushm;
  uentp = xshm->uslot;
  utail = uentp + MAXACTIVE;

  /* uentp += (up->pid % xshm->count); */	/* hashing */

  do
  {
    if (!uentp->pid)
    {
      usint offset;

      offset = (void *) uentp - (void *) xshm->uslot;
      memcpy(uentp, up, sizeof(UTMP));
      xshm->count++;
      if (xshm->offset < offset)
	xshm->offset = offset;
      cutmp = uentp;

#ifdef	HAVE_SEM
      sem_lock(BSEM_LEAVE);
#endif

      return 1;
    }
  } while (++uentp < utail);

  /* Thor:告訴user有人登先一步了 */

#ifdef	HAVE_SEM
  sem_lock(BSEM_LEAVE);
#endif

  return 0;
}


void
utmp_free(up)
  UTMP *up;
{
  if (!up || !up->pid)
    return;

#ifdef	HAVE_SEM
  sem_lock(BSEM_ENTER);
#endif

  up->pid = up->userno = 0;
  ushm->count--;

#ifdef	HAVE_SEM
  sem_lock(BSEM_LEAVE);
#endif
}


UTMP *
utmp_find(userno)
  int userno;
{
  UTMP *uentp, *uceil;

  uentp = ushm->uslot;
  uceil = (void *) uentp + ushm->offset;
  do
  {
    if (uentp->userno == userno)
      return uentp;
  } while (++uentp <= uceil);

  return NULL;
}


UTMP *
utmp_get(userno, userid)	/* itoc.010306: 檢查使用者是否在站上 */
  int userno;
  char *userid;
{
  UTMP *uentp, *uceil;
  int seecloak;
#ifdef HAVE_SUPERCLOAK
  int seesupercloak;
#endif

  /* itoc.020718.註解: 由於同一頁的同樣作者的機率實在太高，考慮是否記下查詢結果，
     然後先查先前的記錄，若找不到再去使用者名單找 */

  seecloak = HAS_PERM(PERM_SEECLOAK);
#ifdef HAVE_SUPERCLOAK
  seesupercloak = cuser.ufo & UFO_SUPERCLOAK;
#endif
  uentp = ushm->uslot;
  uceil = (void *) uentp + ushm->offset;
  do
  {
    if (uentp->pid && 		/* 已經離站的不檢查 */
      ((userno && uentp->userno == userno) || (userid && !strcmp(userid, uentp->userid))))
    {
      if (!seecloak && (uentp->ufo & UFO_CLOAK))	/* 隱形看不見 */
	continue;

#ifdef HAVE_SUPERCLOAK
      if (!seesupercloak && (uentp->ufo & UFO_SUPERCLOAK))	/* 紫隱看不見 */
	continue;
#endif

#ifdef HAVE_BADPAL
      if (!seecloak && is_obad(uentp))		/* 被設壞人，連別個 multi-login 也看不見 */
	break;
#endif

      return uentp;
    }
  } while (++uentp <= uceil);

  return NULL;
}


UTMP *
utmp_seek(hdr)		/* itoc.010306: 檢查使用者是否在站上 */
  HDR *hdr;
{
  if (hdr->xmode & POST_INCOME)	/* POST_INCOME 和 MAIL_INCOME 是相同的 */
    return NULL;
  return utmp_get(0, hdr->owner);
}


void  
utmp_admset(userno, status)	/* itoc.010811: 動態設定線上使用者 */
  int userno;
  usint status;
{
  UTMP *uentp, *uceil;
  extern int ulist_userno[];

  uentp = ushm->uslot;
  uceil = (void *) uentp + ushm->offset;
  do
  {
    if (uentp->userno == userno)
      uentp->status |= status;	/* 加上資料被變動過的旗標 */

    /* itoc.041211: 當我把對方新增/移除/變動朋友時，
       除了要幫他加上 STATUS_PALDIRTY (亦即叫他重新判斷他自己的 ulist_ftype[全部])，
       還要把我 ulist_userno[對方] 變成 0 (亦即叫我自己更新 ulist_ftype[對方]) */
    if (status == STATUS_PALDIRTY)
      ulist_userno[uentp - ushm->uslot] = 0;
  } while (++uentp <= uceil);
}


int
utmp_count(userno, show)
  int userno;
  int show;
{
  UTMP *uentp, *uceil;
  int count;

  count = 0;
  uentp = ushm->uslot;
  uceil = (void *) uentp + ushm->offset;
  do
  {
    if (uentp->userno == userno)
    {
      count++;
      if (show)
      {
	prints("(%d) 目前狀態為: %-17.16s(來自 %s)\n",
	  count, bmode(uentp, 0), uentp->from);
      }
    }
  } while (++uentp <= uceil);
  return count;
}


UTMP *
utmp_search(userno, order)
  int userno;
  int order;			/* 第幾個 */
{
  UTMP *uentp, *uceil;

  uentp = ushm->uslot;
  uceil = (void *) uentp + ushm->offset;
  do
  {
    if (uentp->userno == userno)
    {
      if (--order <= 0)
	return uentp;
    }
  } while (++uentp <= uceil);
  return NULL;
}


#if 0
int
apply_ulist(fptr)
  int (*fptr) ();
{
  UTMP *uentp;
  int i, state;

  uentp = ushm->uslot;
  for (i = 0; i < USHM_SIZE; i++, uentp++)
  {
    if (uentp->pid)
      if (state = (*fptr) (uentp))
	return state;
  }
  return 0;
}
#endif


/*-------------------------------------------------------*/
/* .BRD cache						 */
/*-------------------------------------------------------*/


BCACHE *bshm;


void
bshm_init()
{
  int i;

  /* itoc.030727: 在開啟 bbsd 之前，應該就要執行過 account，
     所以 bshm 應該已設定好 */

  bshm = shm_new(BRDSHM_KEY, sizeof(BCACHE));

  i = 0;
  while (bshm->uptime <= 0)	/* bshm 未設定完成，也許是未跑 account，也許是站長正好在開板 */
  {
    sleep(5);
    if (++i >= 6)		/* 若 30 秒以後還沒好，斷線離開 */
      abort_bbs();
  }
}


void
bshm_reload()		/* 開板以後，重新載入 bshm */
{
  time_t *uptime;
  int fd;
  BRD *head, *tail;

  uptime = &(bshm->uptime);

  while (*uptime <= 0)
  {
    /* 其他站長也剛好在開板，等待 30 秒 */
    sleep(30);
  }

  *uptime = -1;		/* 開始設定 */

  if ((fd = open(FN_BRD, O_RDONLY)) >= 0)
  {
    bshm->number = read(fd, bshm->bcache, MAXBOARD * sizeof(BRD)) / sizeof(BRD);
    close(fd);
  }

  /* 等所有 boards 資料更新後再設定 uptime */
  time(uptime);

  /* itoc.040314: 板主更改看板敘述或是站長更改看板時才會把 bpost/blast 寫進 .BRD 中
     所以 .BRD 裡的 bpost/blast 未必是對的，要重新 initial。
     initial 的方法是將 btime 設成 -1，讓 class_item() 去更新 */
  head = bshm->bcache;
  tail = head + bshm->number;
  do
  {
    head->btime = -1;
  } while (++head < tail);

  blog("CACHE", "reload bcache");
}


#if 0
int
apply_boards(func)
  int (*func) ();
{
  extern char brd_bits[];
  BRD *bhdr;
  int i;

  for (i = 0, bhdr = bshm->bcache; i < bshm->number; i++, bhdr++)
  {
    if (brd_bits[i])
    {
      if ((*func) (bhdr) == -1)
	return -1;
    }
  }
  return 0;
}
#endif


static int
brdname_cmp(a, b)
  BRD *a, *b;
{
  return str_cmp(a->brdname, b->brdname);
}


int
brd_bno(bname)
  char *bname;
{
  BRD xbrd, *bcache, *brdp, *bend;

  bcache = bshm->bcache;

  /* 先在舊看板 binary serach */

  /* str_ncpy(xbrd.brdname, bname, sizeof(xbrd.brdname)); */
  str_lower(xbrd.brdname, bname);	/* 直接換小寫，這樣在 brdname_cmp() 時會快一些 */
  if (bend = bsearch(&xbrd, bcache, bshm->numberOld, sizeof(BRD), brdname_cmp))
    return bend - bcache;

  /* 若找不到，再去新看板 sequential search */

  brdp = bcache + bshm->numberOld;
  bend = bcache + bshm->number;

  while (brdp < bend)
  {
    if (!str_cmp(bname, brdp->brdname))
      return brdp - bcache;

    brdp++;
  }

  return -1;
}


/*-------------------------------------------------------*/
/* movie cache						 */
/*-------------------------------------------------------*/


FCACHE *fshm;


void
fshm_init()
{
  fshm = shm_new(FILMSHM_KEY, sizeof(FCACHE));
}


/* ----------------------------------------------------- */
/* itoc.020822.註解:					 */
/* ----------------------------------------------------- */
/* 第 0 ～ FILM_MOVIE-1 張是系統畫面及說明畫面		 */
/* 第 FILM_MOVIE ～ fmax-1 張是動態看板			 */
/* ----------------------------------------------------- */
/* tag:							 */
/* < FILM_MOVIE  → 播放該張畫面			 */
/* >= FILM_MOVIE → 亂數播放 FILM_MOVIE~fmax-1 其中一張	 */
/* ----------------------------------------------------- */
/* row:							 */
/* >=0 → 系統畫面，從 (row, 0) 開始印			 */
/* <0  → 說明畫面，從 (0, 0) 開始印，最後會 vmsg(NULL)	 */
/* ----------------------------------------------------- */


void
film_out(tag, row)
  int tag;
  int row;			/* -1 : help */
{
  int fmax, len, *shot;
  char *film, buf[FILM_SIZ];

  len = 0;
  shot = fshm->shot;

  while (!(fmax = *shot))	/* util/camera.c 正在換片 */
  {
    sleep(5);
    if (++len >= 6)		/* 若 30 秒以後還沒換好片，可能是沒跑 camera，直接離開 */
      return;
  }

  if (row <= 0)
    clear();
  else
    move(row, 0);

  if (tag >= FILM_MOVIE)	/* 動態看板 */
    tag += time(0) % (fmax - FILM_MOVIE);

  film = fshm->film;

  if (tag)
  {
    len = shot[tag];
    film += len;
    len = shot[tag + 1] - len;
  }
  else
  {
    len = shot[1];
  }

  if (len >= FILM_SIZ - MOVIE_LINES)
    return;

  memcpy(buf, film, len);
  buf[len] = '\0';

  if (d_cols)	/* waynesan.040831: 依寬螢幕置中 */
  {
    char *ptr;
    for (film = buf; *film;)
    {
      if (ptr = strchr(film, '\n'))
	*ptr = '\0';
      move(row++, (d_cols >> 1));
      outx(film);
      if (ptr)
	film = ptr + 1;
    }
  }
  else
    outx(buf);

  if (row < 0)			/* help screen */
    vmsg(NULL);

  return;
}
