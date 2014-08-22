/*-------------------------------------------------------*/
/* util/brd2gem.c	( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : 看板分類到精華區			         */
/* create : 01/09/09					 */
/* update : 03/02/13                                     */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/
/* syntax : brd2gem 分類 Classname			 */
/*-------------------------------------------------------*/


#include "bbs.h"


#if 0	/* 使用方法 */

  這個程式是拿來給 bbs 版本轉換時，自動建立 @Class 用的。
  或是在誤砍分類時，也可以拿出來重建 @Class 用。

  假設要把所有分類為「系統」和「站內」的看板都放在「BBS」這個分類裡面，
  以及把所有分類為「個人」的看板都放在「Personal」這個分類裡面。
  
  1. 上 BBS 站，在 (A)nnounce/Class 裡面 Ctrl+P 選 (C)，建立檔名為 BBS 的分類 (標題任意)。
  2. 上 BBS 站，在 (A)nnounce/Class 裡面 Ctrl+P 選 (C)，建立檔名為 Personal 的分類 (標題任意)。
  3. 在工作站中以 bbs 身分執行
     % ~bbs/src/util/tran/brd2gem 系統 BBS
     % ~bbs/src/util/tran/brd2gem 站內 BBS
     % ~bbs/src/util/tran/brd2gem 個人 Personal

#endif


static void
brd_2_gem(brd, gem)
  BRD *brd;
  HDR *gem;
{
  memset(gem, 0, sizeof(HDR));
  time(&gem->chrono);
  strcpy(gem->xname, brd->brdname);
  sprintf(gem->title, "%-13s%-5s%s", brd->brdname, brd->class, brd->title);
  gem->xmode = GEM_BOARD | GEM_FOLDER;

#ifdef HAVE_MODERATED_BOARD
  /* 秘密板、好友板 */
  if (brd->readlevel == PERM_SYSOP || brd->readlevel == PERM_BOARD)
    gem->xmode |= GEM_RESTRICT;
#endif
}


static int
hdr_cmp(a, b)
  HDR *a;
  HDR *b;
{
  /* itoc.010413: 分類/板名交叉比對 */
  int k = strncmp(a->title + BNLEN + 1, b->title + BNLEN + 1, BCLEN);
  return k ? k : str_cmp(a->xname, b->xname);
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int fd;
  char folder[64];
  BRD brd;
  HDR hdr;

  chdir(BBSHOME);

  if (argc != 3)
  {
    printf("Usage: %s 分類 Classname\n", argv[0]);
    exit(-1);
  }

  if (strlen(argv[1]) > BCLEN || strlen(argv[2]) > BNLEN)
  {
    printf("「分類」要短於 %d，Classname 要短於 %d\n", BCLEN, BNLEN);
    exit(-1);
  }

  sprintf(folder, "gem/@/@%s", argv[2]);

  if ((fd = open(FN_BRD, O_RDONLY)) >= 0)
  {
    while (read(fd, &brd, sizeof(BRD)) == sizeof(BRD))
    {
      if (!strcmp(brd.class, argv[1]))
      {
	brd_2_gem(&brd, &hdr);
	rec_add(folder, &hdr, sizeof(HDR));
      }
    }
    close(fd);
  }

  rec_sync(folder, sizeof(HDR), hdr_cmp, NULL);
}
