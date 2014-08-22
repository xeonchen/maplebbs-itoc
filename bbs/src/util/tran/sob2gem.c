/*-------------------------------------------------------*/
/* util/transman.c					 */
/*-------------------------------------------------------*/
/* target : Maple Sob 2.36 至 Maple 3.02 精華區轉換	 */
/* create : 98/06/15					 */
/* update : 02/10/26					 */
/* author : ernie@micro8.ee.nthu.edu.tw			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/
/* syntax : transman [target_board]			 */
/*-------------------------------------------------------*/


#if 0

   1. 程式不開目錄，使用前先確定 gem/target_board/? 目錄存在
      if not，先開新板 or transbrd
   2. 只轉 M.*.A 及 D.*.A，其他 link 不轉換
   3. 如有需要請先 chmod 644 `find PATH -perm 600`

   ps. User on ur own risk.

#endif


#include "sob.h"


/* ----------------------------------------------------- */
/* 轉換精華區						 */
/* ----------------------------------------------------- */


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
trans_man_stamp(folder, token, hdr, fpath, time)
  char *folder;
  int token;
  HDR *hdr;
  char *fpath;
  time_t time;
{
  char *fname, *family;
  int rc;

  fname = fpath;
  while (rc = *folder++)
  {
    *fname++ = rc;
    if (rc == '/')
      family = fname;
  }
  if (*family != '.')
  {
    fname = family;
    family -= 2;
  }
  else
  {
    fname = family + 1;
    *fname++ = '/';
  }

  *fname++ = token;

  *family = radix32[time & 31];
  archiv32(time, fname);

  if (rc = open(fpath, O_WRONLY | O_CREAT | O_EXCL, 0600))
  {
    memset(hdr, 0, sizeof(HDR));
    hdr->chrono = time;
    str_stamp(hdr->date, &hdr->chrono);
    strcpy(hdr->xname, --fname);
    close(rc);
  }
  return;
}


/* ----------------------------------------------------- */
/* 轉換主程式						 */
/* ----------------------------------------------------- */

 
static void
transman(index, folder)
  char *index, *folder;
{
  static int count = 100;

  int fd;
  char *ptr, buf[256], fpath[64];
  fileheader fh;
  HDR hdr;
  time_t chrono;

  if ((fd = open(index, O_RDONLY)) >= 0)
  {
    while (read(fd, &fh, sizeof(fh)) == sizeof(fh))
    {
      strcpy(buf, index);
      ptr = strrchr(buf, '/') + 1;
      strcpy(ptr, fh.filename);

      if (*fh.filename == 'M' && dashf(buf))	/* 只轉 M.xxxx.A 及 D.xxxx.a */
      {
	/* 轉換文章 .DIR */
	memset(&hdr, 0, sizeof(HDR));
	chrono = trans_hdr_chrono(fh.filename);
	trans_man_stamp(folder, 'A', &hdr, fpath, chrono);
	hdr.xmode = 0;
	str_ncpy(hdr.owner, fh.owner, sizeof(hdr.owner));
	str_ncpy(hdr.title, fh.title + 3, sizeof(hdr.title));
	rec_add(folder, &hdr, sizeof(HDR));

	/* 拷貝檔案 */
	f_cp(buf, fpath, O_TRUNC);
      }
      else if (*fh.filename == 'D' && dashd(buf))
      {
	char sub_index[256];

	/* 轉換文章 .DIR */
	memset(&hdr, 0, sizeof(HDR));
	chrono = ++count;		/* WD 的目錄命名比較奇怪，只好自己給數字 */
	trans_man_stamp(folder, 'F', &hdr, fpath, chrono);
	hdr.xmode = GEM_FOLDER;
	str_ncpy(hdr.owner, fh.owner, sizeof(hdr.owner));
	str_ncpy(hdr.title, fh.title + 3, sizeof(hdr.title));
	rec_add(folder, &hdr, sizeof(HDR));

	/* recursive 進去轉換子目錄 */
	strcpy(sub_index, buf);
	ptr = strrchr(sub_index, '/') + 1;
	sprintf(ptr, "%s/.DIR", fh.filename);
	transman(sub_index, fpath);
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
  char *brdname, index[64], folder[64];
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
  if (!dashd(OLD_BBSHOME "/man/boards"))
  {
    printf("ERROR! Can't open " OLD_BBSHOME "/man/boards\n");
    exit(-1);
  }

  if (argc == 1)
  {
    if ((fd = open(FN_BOARD, O_RDONLY)) >= 0)
    {
      while (read(fd, &bh, sizeof(bh)) == sizeof(bh))
      {
	brdname = bh.brdname;

	sprintf(folder, "gem/brd/%s", brdname);
	if (!dashd(folder))
	{
	  printf("ERROR! %s not exist. New it first.\n", folder);
	  continue;
	}

	sprintf(index, OLD_BBSHOME "/man/boards/%s/.DIR", brdname);
	sprintf(folder, "gem/brd/%s/%s", brdname, FN_DIR);

	printf("轉換 %s 精華區\n", brdname);
	transman(index, folder);
      }
      close(fd);
    }
  }
  else
  {
    brdname = argv[1];

    sprintf(folder, "gem/brd/%s", brdname);
    if (!dashd(folder))
    {
      printf("ERROR! %s not exist. New it first.\n", folder);
      exit(-1);
    }

    sprintf(index, OLD_BBSHOME "/man/boards/%s/.DIR", brdname);
    sprintf(folder, "gem/brd/%s/%s", brdname, FN_DIR);

    printf("轉換 %s 精華區\n", brdname);
    transman(index, folder);

    exit(1);
  }

  exit(0);
}
