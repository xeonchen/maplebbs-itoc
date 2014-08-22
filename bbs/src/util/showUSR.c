/*-------------------------------------------------------*/
/* util/showUSR.c       ( NTHU CS MapleBBS Ver 3.00 )    */
/*-------------------------------------------------------*/
/* target : 秀出 FN_SCHEMA                               */
/* create :   /  /                                       */
/* update : 02/11/03                                     */
/*-------------------------------------------------------*/
/* syntax : showUSR                                      */
/*-------------------------------------------------------*/


#include "bbs.h"


int 
main()
{
  int fd, n;
  SCHEMA *usr;
  struct stat st;

  chdir(BBSHOME);

  if ((fd = open(FN_SCHEMA, O_RDONLY)) < 0)
  {
    printf("ERROR at open file\n");
    exit(1);
  }

  fstat(fd, &st);
  usr = (SCHEMA *) malloc(st.st_size);
  read(fd, usr, st.st_size);
  close(fd);

  printf("\n%s  ==> %ld bytes\n", FN_SCHEMA, st.st_size);

  fd = st.st_size / sizeof(SCHEMA);
  for (n = 0; n < fd; n++)
  {
    /* userno: 在 .USR 中是第幾個 slot */
    /* uptime: 註冊的時間 (若 ID 是空白則是被 reaper 掉的時間) */
    /* userid: ID (若是空白表示此人被 reaper 了) */

    printf("userno:%d  uptime:%s  userid:%-12.12s\n",
      n + 1, Btime(&usr[n].uptime), usr[n].userid);

    if (n % 23 == 22)	/* 每 23 筆按任意鍵繼續 */
    {
      printf("-== Press ENTER to continue and 'q + ENTER' to quit ==-\n");
      if (getchar() == 'q')
	break;
    }
  }

  free(usr);
  return 0;
}
