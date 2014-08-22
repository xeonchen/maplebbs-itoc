/*-------------------------------------------------------*/
/* util/gem-expire.c	( NTHU CS MapleBBS Ver 3.10 )    */
/*-------------------------------------------------------*/
/* target : 計算未編精華區 & 未編天數精華區排行榜	 */
/* create : 99/11/26                                     */
/* update : 01/08/27					 */
/* author : Jimmy.bbs@whshs.twbbs.org			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/
/* syntax : gem-expire					 */
/*-------------------------------------------------------*/


#include "bbs.h"

#define OUTFILE_GEMEMPTY	"gem/@/@-gem_empty"
#define OUTFILE_GEMOVERDUE	"gem/@/@-gem_overdue"


/*-------------------------------------------------------*/
/* BRD shm 部分須與 cache.c 相容			 */
/*-------------------------------------------------------*/


static BCACHE *bshm;


static void
init_bshm()
{
  /* itoc.030727: 在開啟 bbsd 之前，應該就要執行過 account，
     所以 bshm 應該已設定好 */

  bshm = shm_new(BRDSHM_KEY, sizeof(BCACHE));

  if (bshm->uptime <= 0)	/* bshm 未設定完成 */
    exit(0);
}


/*-------------------------------------------------------*/
/* 主程式						 */
/*-------------------------------------------------------*/


typedef struct
{
  int day;			/* 幾天沒整理精華區 */
  char brdname[BNLEN + 1];	/* 板名 */
  char BM[BMLEN + 1];		/* 板主 */
}	BRDDATA;


static int
int_cmp(a, b)
  BRDDATA *a, *b;
{
  return (b->day - a->day);	/* 由大排到小 */
}


static BRDDATA board[MAXBOARD];
static int locus = 0;			/* 總共記錄了幾個板 */


static void
topgem()
{
  time_t now;
  struct stat st;
  BRD *bhdr, *tail;
  char fpath[64], *brdname;

  time(&now);

  bhdr = bshm->bcache;
  tail = bhdr + bshm->number;
  do
  {
    /* 跳過不列入排行榜的看板 */
    if ((bhdr->readlevel | bhdr->postlevel) >= (PERM_VALID << 1))	/* (BASIC + ... + VALID) < (VALID << 1) */
      continue;

    brdname = bhdr->brdname;
    sprintf(fpath, "gem/brd/%s/@/@log", brdname);

    if (stat(fpath, &st) != -1)	/* 有精華區者檢查幾天未編 */
      board[locus].day = (now - st.st_mtime) / 86400;
    else			/* 無精華區者 */
      board[locus].day = 999;
    strcpy(board[locus].brdname, brdname);
    strcpy(board[locus].BM, bhdr->BM);

    locus++;
  } while (++bhdr < tail);

  qsort(board, locus, sizeof(BRDDATA), int_cmp);
}


static void
write_data()
{
  time_t now;
  struct tm *ptime;
  FILE *fpe, *fpo;
  int i, m, n;  

  time(&now);
  ptime = localtime(&now);

  fpe = fopen(OUTFILE_GEMEMPTY, "w");
  fpo = fopen(OUTFILE_GEMOVERDUE, "w");

  fprintf(fpe,
    "         \033[1;34m-----\033[37m=====\033[41m 看板精華區未編之看板 (至 %d 月 %d 日止) \033[;1;37m=====\033[34m-----\033[m\n"
    "           \033[1;42m 名次 \033[44m   看板名稱   \033[42m      精華區未編      \033[44m   板   主    \033[m\n",
    ptime->tm_mon + 1, ptime->tm_mday);

  fprintf(fpo,
    "        \033[1;34m-----\033[37m=====\033[41m 看板精華區未編天數之看板 (至 %d 月 %d 日止) \033[;1;37m=====\033[34m-----\033[m\n"
    "              \033[1;42m 名次 \033[44m    看板名稱    \033[42m 精華區未編天數 \033[44m   板   主    \033[m\n",
    ptime->tm_mon + 1, ptime->tm_mday);

  m = 1;
  n = 1;

  for (i = 0; i < locus; i++)
  {
    if (board[i].day == 999)
    {
      fprintf(fpe, "            %3d   %12s     %s      %.20s\n",
	m, board[i].brdname, "尚未編輯精華區", board[i].BM);
      m++;
    }
    else
    {
      fprintf(fpo, "                %s%3d    %12s        %4d       %.20s\033[m\n",
	n <= 3 ? "\033[1m" : (n <= 10 ? "\033[1;31m" : "\033[m"),
	n, board[i].brdname, board[i].day, board[i].BM);
      n++;
    }
  }

  fclose(fpe);
  fclose(fpo);
}


int 
main()
{
  chdir(BBSHOME);

  init_bshm();

  topgem();
  write_data();

  return 0;
}
