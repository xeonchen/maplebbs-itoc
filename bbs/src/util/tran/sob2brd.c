/*-------------------------------------------------------*/
/* util/transbrd.c					 */
/*-------------------------------------------------------*/
/* target : Maple Sob 2.36 至 Maple 3.02 看板轉換	 */
/*          .BOARDS => .BRD				 */
/* create :   /  /  					 */
/* update : 98/06/14					 */
/* author : ernie@micro8.ee.nthu.edu.tw			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/
/* syntax : transbrd [target_board]			 */
/*-------------------------------------------------------*/

#if 0

   1. 修改 struct boardheader 及 transbrd()
       (boardheader 兩版定義的字串長度不一，請自行換成數字)
   2. 投票不轉換
   3. 進板畫面 copy
   4. 如有需要請 chmod 644 `find PATH -perm 600`
   5. 開 gem 目錄 gem/target_board/? 但不轉換 gem
   6. 不會更新 bshm，使用後請自行更新
   7. 轉換後請手動設看板權限

   ps. Use on ur own risk.

#endif


#include "sob.h"


static time_t
trans_hdr_chrono(filename)
  char *filename;
{
  char time_str[11];

  /* M.1087654321.A 或 M.987654321.A */
  str_ncpy(time_str, filename + 2, filename[2] == '1' ? 11 : 10);

  return (time_t) atoi(time_str);
}


static void
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
  str_ncpy(newboard.title, bh->title + 4, sizeof(newboard.title));
  str_ncpy(newboard.BM, bh->BM, sizeof(newboard.BM));
  newboard.bstamp = stamp++;
  newboard.battr = BRD_NOTRAN;				/* 預設不轉信 */
  newboard.readlevel = 0;
  newboard.postlevel = PERM_POST;

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
