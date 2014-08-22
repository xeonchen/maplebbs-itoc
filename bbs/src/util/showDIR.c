/*-------------------------------------------------------*/
/* util/showDIR.c	( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : 顯示 .DIR 資料				 */
/* create : 03/05/24					 */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"


static char *
_bitmsg(str, level)
  char *str;
  int level;
{
  int cc;
  int num;
  static char msg[33];

  num = 0;
  while (cc = *str)
  {
    msg[num] = (level & 1) ? cc : '-';
    level >>= 1;
    str++;
    num++;
  }
  msg[num] = 0;

  return msg;
}


static inline void
showHDR(hdr)
  HDR *hdr;
{
  char msg1[40], msg2[40];

  strcpy(msg1, Btime(&(hdr->chrono)));
  strcpy(msg2, _bitmsg("0123456789ABCDEFGHIJKLMNOPQRSTUV", hdr->xmode));
  printf("> ------------------------------------------------------------------------------------------ \n"
    "時間: %s\nmode: %s\n檔案: %s\n作者: %s\n暱稱: %s\n日期: %s\n主題: %s\n", 
    msg1, msg2, hdr->xname, hdr->owner, hdr->nick, hdr->date, hdr->title);
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int pos;
  char *fname;
  HDR hdr;

  if (argc < 2)
  {
    printf("Usage: %s .DIR_path\n", argv[0]);
    exit(1);
  }

  fname = argv[1];
  if (strcmp(fname + strlen(fname) - 4, FN_DIR))
  {
    printf("This is not a .DIR file.\n");
    exit(1);
  }

  pos = 0;
  while (!rec_get(fname, &hdr, sizeof(HDR), pos))
  {
    showHDR(&hdr);
    pos++;
  }

  printf("> ------------------------------------------------------------------------------------------ \n");
}
