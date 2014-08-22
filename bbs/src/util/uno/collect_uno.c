/*-------------------------------------------------------*/
/* util/collect_uno.c	( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : 收集全站所有人的 userno			 */
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


static void
collect_uno(userid)
  char *userid;
{
  int fd;
  char fpath[64];
  ACCT acct;
  COLLECTION collection;

  usr_fpath(fpath, userid, FN_ACCT);
  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    read(fd, &acct, sizeof(ACCT));
    close(fd);

    memset(&collection, 0, sizeof(COLLECTION));
    collection.userno = acct.userno;
    strcpy(collection.userid, acct.userid);
    rec_add("tmp/all_user_uno", &collection, sizeof(COLLECTION));
  }
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  char c;
  char *userid, fpath[64];
  struct dirent *de;
  DIR *dirp;

  chdir(BBSHOME);

  unlink("tmp/all_user_uno");

  for (c = 'a'; c <= 'z'; c++)
  {
    sprintf(fpath, "usr/%c", c);

    if (!(dirp = opendir(fpath)))
      continue;

    while (de = readdir(dirp))
    {
      userid = de->d_name;
      if (*userid <= ' ' || *userid == '.')
	continue;
      collect_uno(userid);
    }

    closedir(dirp);
  }

  return 0;
}
