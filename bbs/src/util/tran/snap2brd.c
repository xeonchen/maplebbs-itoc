/*-------------------------------------------------------*/
/* util/snap2brd.c	( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : M3 BRD 轉換程式				 */
/* create : 03/07/09					 */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "snap.h"


#define FN_BRD_TMP	".BRD.tmp"

int
main()
{
  boardheader bh;
  BRD brd;
  FILE *fp;

  chdir(BBSHOME);

  if (!(fp = fopen(FN_BRD, "r")))
    return -1;

  while (fread(&bh, sizeof(bh), 1, fp) == 1)
  {
    if (!*bh.brdname)	/* 此板已被刪除 */
      continue;

    memset(&brd, 0, sizeof(BRD));

    /* 轉換的動作在此 */
    str_ncpy(brd.brdname, bh.brdname, sizeof(brd.brdname));
    str_ncpy(brd.class, "？？", sizeof(brd.class));		/* 分類要重設 */
    str_ncpy(brd.title, bh.title + 3, sizeof(brd.title));	/* 跳過 "□ " */
    str_ncpy(brd.BM, bh.BM, sizeof(brd.BM));
    brd.bvote = bh.bvote ? 1 : 0;
    brd.bstamp = bh.bstamp;
    brd.readlevel = bh.readlevel;
    brd.postlevel = bh.postlevel;
    brd.battr = bh.battr;
    brd.btime = 0;
    brd.bpost = 0;
    brd.blast = 0;

    rec_add(FN_BRD_TMP, &brd, sizeof(BRD));
  }

  fclose(fp);

  /* 刪除舊的，把新的更名 */
  unlink(FN_BRD);
  rename(FN_BRD_TMP, FN_BRD);

  return 0;
}
