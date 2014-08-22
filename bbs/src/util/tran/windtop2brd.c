/*-------------------------------------------------------*/
/* util/wintop2brd.c					 */
/*-------------------------------------------------------*/
/* target : WindTop .BRD 轉換				 */
/* create : 03/06/30					 */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "windtop.h"


static usint
trans_battr(oldbattr)
  usint oldbattr;
{
  usint battr;

  battr = 0;

  if (oldbattr & BATTR_NOZAP)
    battr |= BRD_NOZAP;

  if (oldbattr & BATTR_NOTRAN)
    battr |= BRD_NOTRAN;

  if (oldbattr & BATTR_NOCOUNT)
    battr |= BRD_NOCOUNT;

  if (oldbattr & BATTR_NOSTAT)
    battr |= BRD_NOSTAT;

  if (oldbattr & BATTR_NOVOTE)
    battr |= BRD_NOVOTE;

  if (oldbattr & BATTR_ANONYMOUS)
    battr |= BRD_ANONYMOUS;

  return battr;
}


int
main()
{
  int fd;
  BRD brd;
  boardheader bh;
  char buf[256];

  chdir(BBSHOME);

  unlink(FN_BRD);

  if ((fd = open(FN_BOARD, O_RDONLY)) >= 0)
  {
    while (read(fd, &bh, sizeof(bh)) == sizeof(bh))
    {
      if (*bh.brdname)
      {
	/* 轉換 .BRD */
	memset(&brd, 0, sizeof(BRD));

	str_ncpy(brd.brdname, bh.brdname, sizeof(brd.brdname));
	str_ncpy(brd.class, bh.class, sizeof(brd.class));
	str_ncpy(brd.title, bh.title, sizeof(brd.title));
	str_ncpy(brd.BM, bh.BM, sizeof(brd.BM));
	brd.bvote = bh.bvote ? 1 : 0;
	brd.bstamp = bh.bstamp;
	brd.readlevel = bh.readlevel;
	brd.postlevel = bh.postlevel;
	brd.battr = trans_battr(bh.battr);

	rec_add(FN_BRD, &brd, sizeof(BRD));

	/* 建立 expire.conf */
	if (bh.expireday && bh.expiremax && bh.expiremin)
	{
	  sprintf(buf, "%s\t%d\t%d\t%d\n", brd.brdname, bh.expireday, bh.expiremax, bh.expiremin);
	  f_cat("etc/expire.conf", buf);
	}
      }
    }
  }

  return 0;
}
