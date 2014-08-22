/*-------------------------------------------------------*/
/* util/transbrd.c					 */
/*-------------------------------------------------------*/
/* target : WD 至 Maple 3.02 看板轉換			 */
/*          .BOARDS => .BRD				 */
/* create : 98/06/15					 */
/* update : 02/01/05					 */
/* author : ernie@micro8.ee.nthu.edu.tw			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/

#if 0

   1. 修改 struct boardheader 及 transbrd()
      (boardheader 兩版定義的字串長度不一，請自行換成數字)      
   2. WD 看板分類(boardheader.title 前 4 bytes)捨棄
   3. 投票不轉換
   4. 進板畫面直接 copy
   5. 如有需要請 chmod 644 `find PATH -perm 600`
   6. 開 gem 目錄 gem/target_board/? 但不轉換 gem
   7. 不會更新 bshm，使用後請自行更新

   ps. Use on ur own risk.

#endif


#include "wd.h"


static inline usint
trans_brd_battr(brdattr)		/* itoc.010426: 轉換看板屬性 */
  usint brdattr;
{
  usint battr;

  battr = 0;

  if (brdattr & 00001)
    battr |= BRD_NOZAP;

  if (brdattr & 00002)
    battr |= BRD_NOCOUNT;

  if (brdattr & 00004)
    battr |= BRD_NOTRAN;

#ifdef HAVE_ANONYMOUS
  if (brdattr & 00100)
    battr |= BRD_ANONYMOUS;
#endif

#ifdef HAVE_MODERATED_BOARD
  if (brdattr & (00020 | 01000))	/* 隱藏板投票不公告 */
    battr |= BRD_NOVOTE;
#endif

  return battr;
}


static inline usint
trans_brd_rlevel(brdattr)		/* itoc.010426: 轉換看板閱讀權限 */
  usint brdattr;
{
#ifdef HAVE_MODERATED_BOARD
  if (brdattr & 00020)			/* 隱藏板 */
    return PERM_SYSOP;
  else if (brdattr & 01000)		/* 好友板 */
    return PERM_BOARD;
  else					/* 一般看板 */
#endif
    return 0;
}


static inline usint
trans_brd_plevel(brdattr)		/* itoc.010426: 轉換看板發表權限 */
  usint brdattr;
{
#ifdef HAVE_MODERATED_BOARD
  if (brdattr & 00020)			/* 隱藏板 */
    return 0;
  else if (brdattr & 01000)		/* 好友板 */
    return 0;
  else					/* 一般看板 */
#endif
    return PERM_POST;	/* 一般看板預設為 POST_POST */
}


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
/* 轉換主程式						 */
/* ----------------------------------------------------- */


static void
transbrd(bh)
  boardheader *bh;
{
  static time_t stamp = 0;

  int fd;
  char index[64], folder[64], buf[64], fpath[64];
  fileheader fh;
  HDR hdr;
  BRD newboard;
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
  str_ncpy(newboard.class, bh->title, sizeof(newboard.class));  
  str_ncpy(newboard.title, bh->title + 7, sizeof(newboard.title));
  str_ncpy(newboard.BM, bh->BM, sizeof(newboard.BM));
  newboard.bstamp = stamp++;
  newboard.battr = trans_brd_battr(bh->brdattr);
  newboard.readlevel = trans_brd_rlevel(bh->brdattr);
  newboard.postlevel = trans_brd_plevel(bh->brdattr);

  rec_add(FN_BRD, &newboard, sizeof(newboard));		/* 別忘了用 brd2gem.c 來轉換 Class */

  /* 開新目錄 */

  sprintf(fpath, "gem/brd/%s", newboard.brdname);
  mak_dirs(fpath);
  mak_dirs(fpath + 4);

  /* 轉換進板畫面 */

  sprintf(buf, OLD_BBSHOME "/boards/%s/notes", bh->brdname);
  
  if (dashf(buf))
  {
    brd_fpath(fpath, newboard.brdname, FN_NOTE);
    f_cp(buf, fpath, O_TRUNC);
  }

  /* 轉換文章 */

  sprintf(index, OLD_BBSHOME "/boards/%s/.DIR", bh->brdname);	/* 舊的 .DIR */
  brd_fpath(folder, newboard.brdname, ".DIR");			/* 新的 .DIR */

  if ((fd = open(index, O_RDONLY)) >= 0)
  {
    while (read(fd, &fh, sizeof(fh)) == sizeof(fh))
    {
      sprintf(buf, OLD_BBSHOME "/boards/%s/%s", bh->brdname, fh.filename);
      if (dashf(buf))	/* 文章檔案在才做轉換 */
      {
	/* 轉換文章 .DIR */
	memset(&hdr, 0, sizeof(HDR));
	chrono = trans_hdr_chrono(fh.filename);
	trans_hdr_stamp(folder, chrono, &hdr, fpath);
	str_ncpy(hdr.owner, fh.owner, sizeof(hdr.owner));
	str_ansi(hdr.title, fh.title, sizeof(hdr.title));
	hdr.xmode = (fh.filemode & 0x2) ? POST_MARKED : 0;
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
    exit(-1);
  }

  chdir(BBSHOME);

  if (!dashf(FN_BOARD))
  {
    printf("ERROR! Can't open " FN_BOARD "\n");
    exit(-1);
  }
  if (!dashd(OLD_BBSHOME "/boards"))
  {
    printf("ERROR! Can't open " OLD_BBSHOME "/boards\n");
    exit(-1);
  }

  if ((fd = open(FN_BOARD, O_RDONLY)) >= 0)
  {
    while (read(fd, &bh, sizeof(bh)) == sizeof(bh))
    {
      if (argc == 1)
      {
	transbrd(&bh);
      }
      else if (!strcmp(bh.brdname, argv[1]))
      {
	transbrd(&bh);
	exit(1);
      }
    }
    close(fd);
  }



  exit(0);
}
