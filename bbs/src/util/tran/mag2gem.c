/*-------------------------------------------------------*/
/* util/transman.c					 */
/*-------------------------------------------------------*/
/* target : Magic 至 Maple 3.02 精華區轉換		 */
/* create : 01/10/03					 */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#if 0

  0. 適用 Magic/Napoleon 轉 Maple 精華區
  1. 程式不開目錄，使用前先確定 gem/target_board/? 目錄存在
    if not，先開新板 or transbrd

  ps. User on ur own risk.

#endif


#include "mag.h"


/* ----------------------------------------------------- */
/* basic functions					 */
/* ----------------------------------------------------- */


static void
merge_msg(msg)		/* fget() 字串最後有 '\n' 要處理掉 */
  char *msg;
{
  int end;

  end = 0;
  while (end < 80)
  {
    if (msg[end] == '\n')
    {
      msg[end] = '\0';
      return;
    }

    end++;
  }
  msg[end] = '\0';	/* 強制斷在 80 字 */
}


static int			/* 'A':文章  'F':目錄 */
get_record(src, src_folder, title, path, num)	/* 讀出 .Name */
  char *src;
  char *src_folder;
  char *title, *path;
  int num;
{
  FILE *fp;
  char Name[128], Path[128], buf[128];
  int i, j;

  if (fp = fopen(src_folder, "r"))
  {
    /* 前三行沒用 */
    j = num * 4 + 3;
    for (i = 0; i < j; i++)
    {
      fgets(buf, 80, fp);
    }

    /* 每個索引有四行 */
    fgets(Name, 80, fp);
    fgets(Path, 80, fp);
    fgets(buf, 80, fp);
    fgets(buf, 80, fp);

    fclose(fp);

    if (Name[0] != '#' && Path[0] != '#')
    {
      merge_msg(Name);
      merge_msg(Path);

      strcpy(title, Name + 5);
      strcpy(path, Path + 7);

      sprintf(buf, "%s/%s", src, path);

      if (dashf(buf))
	return 'A';
      else if (dashd(buf))
	return 'F';
    }
  }

  return 0;
}


/* ----------------------------------------------------- */
/* 轉換程式						 */
/* ----------------------------------------------------- */


static void
transman(brdname, src, dst_folder)
  char *brdname;	/* 看板板名 */
  char *src;		/* 舊站的目錄 */
  char *dst_folder;	/* 新站的 .DIR 或  FXXXXXXX */
{
  int num;
  int type;
  char src_folder[80];		/* 舊站的 .Names */
  char sub_src[80];		
  char sub_dst_folder[80];
  char title[80], path[80];
  char cmd[256];
  HDR hdr;

  num = 0;
  sprintf(src_folder, "%s/.Names", src);

  while ((type = get_record(src, src_folder, title, path, num)))
  {
    if (type == 'A')		/* 文件 */
    {
      close(hdr_stamp(dst_folder, 'A', &hdr, cmd));
      str_ncpy(hdr.title, title, sizeof(hdr.title));
      rec_add(dst_folder, &hdr, sizeof(HDR));

      sprintf(cmd, "cp %s/%s gem/brd/%s/%c/%s", src, path, brdname, hdr.xname[7], hdr.xname);
      system(cmd);
    }
    else if (type == 'F')	/* 目錄 */
    {
      close(hdr_stamp(dst_folder, 'F', &hdr, cmd));
      hdr.xmode = GEM_FOLDER;
      str_ncpy(hdr.title, title, sizeof(hdr.title));
      rec_add(dst_folder, &hdr, sizeof(HDR));

      sprintf(sub_src, "%s/%s", src, path);
      sprintf(sub_dst_folder, "gem/brd/%s/%c/%s", brdname, hdr.xname[7], hdr.xname);
      transman(brdname, sub_src, sub_dst_folder);
    }
    num++;
  }
}


int
main()
{
  int fd;
  char src[64], dst_folder[64];
  BRD brd;

  chdir(BBSHOME);

  if ((fd = open(FN_BRD, O_RDONLY)) >= 0)
  {
    while (read(fd, &brd, sizeof(BRD)) == sizeof(BRD))
    {
      sprintf(src, OLD_MANPATH "/%s", brd.brdname);

      if (dashd(src))
      {
	sprintf(dst_folder, "gem/brd/%s", brd.brdname);

	if (dashd(dst_folder))
	{
	  sprintf(dst_folder, "gem/brd/%s/.DIR", brd.brdname);
	  transman(brd.brdname, src, dst_folder);	/* 新舊站都要有這個看板才轉 */
	}
      }
    }
    close(fd);
  }

  exit(0);
}
