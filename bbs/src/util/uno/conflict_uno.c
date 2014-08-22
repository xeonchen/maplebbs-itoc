/*-------------------------------------------------------*/
/* util/conflict_uno.c	( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : 分析 all_user_uno 看看是否有重覆的 userno	 */
/* create : 04/10/16					 */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "bbs.h"


typedef struct
{
  int userno;
  char userid[IDLEN + 1];
}    COLLECTION;


static int
cmp_collection(i, j)
  COLLECTION *i, *j;
{
  return i->userno - j->userno;
}


int
main()
{
  int fd, n;
  COLLECTION *usr;
  struct stat st;

  chdir(BBSHOME);

  if ((fd = open("tmp/all_user_uno", O_RDONLY)) < 0)
  {
    printf("您必須先執行 collect_uno\n");
    return -1;
  }

  fstat(fd, &st);
  n = st.st_size;
  usr = (COLLECTION *) malloc(n);
  read(fd, usr, n);
  close(fd);

  fd = n / sizeof(COLLECTION);
  if (fd > 1)
  {
    qsort(usr, fd, sizeof(COLLECTION), cmp_collection);

    for (n = 1; n < fd; n++)
    {
      if (usr[n].userno == usr[n - 1].userno)
        printf("%d %s <====> %s\n", usr[n].userno, usr[n - 1].userid, usr[n].userid);
    }
  }

  free(usr);

  return 0;
}
