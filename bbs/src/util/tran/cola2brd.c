/*-------------------------------------------------------*/
/* util/cola2brd.c  ( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : Cola 至 Maple 3.02 看板轉換			 */
/* create : 03/02/11					 */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "cola.h"


static inline time_t
trans_hdr_chrono(filename)
  char *filename;
{
  char time_str[11];

  /* M.1087654321.A 或 M.987654321.A */
  str_ncpy(time_str, filename + 2, filename[2] == '1' ? 11 : 10);

  return (time_t) atoi(time_str);
}


static inline void
trans_hdr_stamp(folder, t, hdr, fpath)
  char *folder;
  time_t t;
  HDR *hdr;
  char *fpath;
{
  FILE *fp;
  char *fname, *family;
  int rc;

  fname = fpath;
  while (rc = *folder++)
  {
    *fname++ = rc;
    if (rc == '/')
      family = fname;
  }
  fname = family + 1;
  *fname++ = '/';
  *fname++ = 'A';

  for (;;)
  {
    *family = radix32[t & 31];
    archiv32(t, fname);

    if (fp = fopen(fpath, "r"))
    {
      fclose(fp);
      t++;
    }
    else
    {
      memset(hdr, 0, sizeof(HDR));
      hdr->chrono = t;
      str_stamp(hdr->date, &hdr->chrono);
      strcpy(hdr->xname, --fname);
      break;
    }
  }
}


/* ----------------------------------------------------- */
/* 轉換主程式                                            */
/* ----------------------------------------------------- */


static void
transbrd(bh)
  boardheader *bh;
{
  static time_t stamp = 0;

  int fd;
  char fpath[64], folder[64], index[64], buf[64];
  fileheader fh;
  BRD newboard;
  HDR hdr;
  time_t chrono;

  printf("轉換 %s 看板\n", bh->brdname);

  brd_fpath(buf, bh->brdname, NULL);
  if (dashd(buf))
  {
    printf("%s 已經有此看板\n", bh->brdname);
    return;
  }

  if (!stamp)
    time(&stamp);

  /* 轉換 .BRD */

  memset(&newboard, 0, sizeof(newboard));
  str_ncpy(newboard.brdname, bh->brdname, sizeof(newboard.brdname));
  str_ncpy(newboard.class, bh->title + 2, sizeof(newboard.class));
  str_ncpy(newboard.title, bh->title + 12, sizeof(newboard.title));
  /* newboard.BM[0] = '\0'; */				/* 板主不轉換 */
  newboard.bstamp = stamp++;
  newboard.battr = BRD_NOTRAN;
  newboard.readlevel = 0;
  newboard.postlevel = PERM_POST;

  rec_add(FN_BRD, &newboard, sizeof(newboard));		/* 別忘了用 brd2gem.c 來轉換 Class */

  /* 開新目錄 */

  sprintf(fpath, "gem/brd/%s", newboard.brdname);
  mak_dirs(fpath);
  mak_dirs(fpath + 4);

  /* 轉換進板畫面 */

  sprintf(buf, COLABBS_BOARDS "/%s/.Welcome", bh->brdname);
  
  if (dashf(buf))
  {
    brd_fpath(fpath, newboard.brdname, FN_NOTE);
    f_cp(buf, fpath, O_TRUNC);
  }

  /* 轉換文章 */

  sprintf(index, COLABBS_BOARDS "/%s/.DIR", bh->brdname);	/* 舊的 .DIR */
  brd_fpath(folder, newboard.brdname, ".DIR");			/* 新的 .DIR */

  if ((fd = open(index, O_RDONLY)) >= 0)
  {
    while (read(fd, &fh, sizeof(fh)) == sizeof(fh))
    {
      sprintf(buf, COLABBS_BOARDS "/%s/%s", bh->brdname, fh.filename);

      if (dashf(buf))	/* 文章檔案在才做轉換 */
      {
	/* 轉換文章 .DIR */
	memset(&hdr, 0, sizeof(HDR));
	chrono = trans_hdr_chrono(fh.filename);
	trans_hdr_stamp(folder, chrono, &hdr, fpath);
	str_ncpy(hdr.owner, fh.owner, sizeof(hdr.owner));
	str_ansi(hdr.title, fh.title + 3, sizeof(hdr.title));
	hdr.xmode = 0;
	rec_add(folder, &hdr, sizeof(HDR));

	/* 拷貝檔案 */
	f_cp(buf, fpath, O_TRUNC);
      }
    }
    close(fd);
  }
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int fd;
  boardheader bh;

  /* argc == 1 轉全部板 */
  /* argc == 2 轉某特定板 */

  if (argc > 2)
  {
    printf("Usage: %s [target_board]\n", argv[0]);
    return -1;
  }

  chdir(BBSHOME);

  if (!dashf(FN_BOARD))
  {
    printf("ERROR! Can't open " FN_BOARD "\n");
    return -1;
  }
  if (!dashd(COLABBS_BOARDS))
  {
    printf("ERROR! Can't open " COLABBS_BOARDS "\n");
    return -1;
  }

  if ((fd = open(FN_BOARD, O_RDONLY)) >= 0)
  {
    while (read(fd, &bh, sizeof(bh)) == sizeof(bh))
    {
      bh.brdname[BNLEN] = '\0';	/* itoc.030211: Cola 的看板名可能超過 BNLEN */

      if (argc == 1)
      {
	transbrd(&bh);
      }
      else if (!strcmp(bh.brdname, argv[1]))
      {
	transbrd(&bh);
	return 1;
      }
    }
    close(fd);
  }

  return 0;
}
