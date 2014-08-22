/*-------------------------------------------------------*/
/* util/hdr-dump.c	( NTHU CS MapleBBS Ver 2.36 )	 */
/*-------------------------------------------------------*/
/* target : 看板標題表     				 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/
/* Usage:	hdr-dump .DIR   			 */
/*-------------------------------------------------------*/


#include "bbs.h"

int
main(argc, argv)
  int argc;
  char *argv[];
{
  int fd, count;
  HDR hdr;

  if (argc < 2)
  {
    printf("Usage:\t%s .DIR\n", argv[0]);
    exit(1);
  }

  if ((fd = open(argv[1], O_RDONLY)) < 0)
  {
    printf("error open file\n");
    exit(1);
  }

  count = 0;
  while (read(fd, &hdr, sizeof(HDR)) == sizeof(HDR))
  {
    count++;
    printf("%04d %c/%s %s (%s) %s %s\n", count, hdr.xname[7], hdr.xname, hdr.owner, hdr.nick, hdr.date, hdr.title);
  }
  close(fd);

  exit(0);
}
