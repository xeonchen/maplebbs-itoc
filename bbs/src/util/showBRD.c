/*-------------------------------------------------------*/
/* util/showBRD.c	( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : show board info				 */
/* create : 01/10/05                                     */
/* update :   /  /                                       */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/
/* syntax : showBRD [target_board]                       */
/*-------------------------------------------------------*/


#include "bbs.h"


static void
_bitmsg(msg, str, level)
  char *msg, *str;
  int level;
{
  int cc;

  printf("%s", msg);
  while (cc = *str)
  {
    printf("%c", (level & 1) ? cc : '-');
    level >>= 1;
    str++;
  }
  printf("\n");
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int show_allbrd;
  BRD brd;
  FILE *fp;

  if (argc < 2)
    show_allbrd = 1;
  else
    show_allbrd = 0;

  chdir(BBSHOME);

  if (!(fp = fopen(FN_BRD, "r")))
    return -1;

  while (fread(&brd, sizeof(BRD), 1, fp) == 1)
  {
    if (show_allbrd || !str_cmp(brd.brdname, argv[1]))
    {
      printf("看板名稱：%-13s     看板標題：[%s] %s\n", brd.brdname, brd.class, brd.title);
      printf("投票狀態：%-13d     看板板主：%s\n", brd.bvote, brd.BM);
      _bitmsg(MSG_READPERM, STR_PERM, brd.readlevel);
      _bitmsg(MSG_POSTPERM, STR_PERM, brd.postlevel);
      _bitmsg(MSG_BRDATTR, STR_BATTR, brd.battr);
      printf("文章篇數：%d\n", brd.bpost);
      printf("開板時間：%s\n", Btime(&brd.bstamp));
      printf(".DIR時間：%s\n", Btime(&brd.btime));
      printf("最後一篇：%s\n", Btime(&brd.blast));

      if (!show_allbrd)
	break;
    }
  }

  fclose(fp);

  return 0;
}
