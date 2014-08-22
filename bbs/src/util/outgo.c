/*-------------------------------------------------------*/
/* util/outgo.c		( YZU CSE WindTop BBS )		 */
/*-------------------------------------------------------*/
/* target : 自動送信程式				 */
/* create : 00/06/22					 */
/* update :   /  /  					 */
/*-------------------------------------------------------*/
/* syntax : outgo [board] [start] [end]			 */
/* NOTICE : 將看板文章重新送到 news			 */
/*-------------------------------------------------------*/


#include "bbs.h"


static inline void
outgo_post(hdr, board)
  HDR *hdr;
  char *board;
{
  bntp_t bntp;

  memset(&bntp, 0, sizeof(bntp_t));
  bntp.chrono = hdr->chrono;
  strcpy(bntp.board, board);
  strcpy(bntp.xname, hdr->xname);
  strcpy(bntp.owner, hdr->owner);
  strcpy(bntp.nick, hdr->nick);
  strcpy(bntp.title, hdr->title);
  rec_add("innd/out.bntp", &bntp, sizeof(bntp_t));
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  char fpath[128];
  int start, end, fd;
  char *board;
  HDR hdr;

  if (argc > 3)
  {
    chdir(BBSHOME);

    board = argv[1];
    start = atoi(argv[2]);
    end = atoi(argv[3]);
    brd_fpath(fpath, board, FN_DIR);
    if (fd = open(fpath, O_RDONLY))
    {
      lseek(fd, (off_t) ((start - 1) * sizeof(HDR)), SEEK_SET);
      while (read(fd, &hdr, sizeof(HDR)) == sizeof(HDR) && start <= end)
      {
	if (!(hdr.xmode & POST_INCOME))
	  outgo_post(&hdr, board);
	start++;
      }
      close(fd);
    }
  }
  else
  {
    printf("usage: %s 看板 起點 終點\n", argv[0]);
  }
  
  return 0;
}
