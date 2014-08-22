/*-------------------------------------------------------*/
/* util/topsong.c        ( NTHU CS MapleBBS Ver 3.10 )   */
/*-------------------------------------------------------*/
/* target : 歌本使用排名                                 */
/* create : 01/09/28                                     */
/* update :   /  /                                       */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/
/* syntax : topsong                                      */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef LOG_SONG_USIES


#define OUTFILE_TOPSONG	"gem/@/@-topsong"


static void
write_data(songs, num)
  SONGDATA *songs;
  int num;
{
  int n;
  FILE *fp;

  if (!(fp = fopen(OUTFILE_TOPSONG, "w")))
    return;

  fprintf(fp, "    \033[36m──\033[37m名次\033[36m──────\033[37m歌  名"
    "\033[36m───────────\033[37m次數\033[36m──\033[m\n");

  for (n = 0; n < 50 && n < num; n++)		/* 只取前 50 名 */
  {
    fprintf(fp, "      %5d. %-38.38s %4d 次\033[m\n", 
      n + 1, songs[n].title, songs[n].count);
  }

  fclose(fp);
}


static int
count_cmp(b, a)
  SONGDATA *a, *b;
{
  return (a->count - b->count); 
}


int
main()
{
  int fd, size;
  struct stat st;
  SONGDATA *songs;

  chdir(BBSHOME);
  
  if ((fd = open(FN_RUN_SONGUSIES, O_RDWR)) < 0)
    return 0;

  if (!fstat(fd, &st) && (size = st.st_size) >= sizeof(SONGDATA))
  {
    songs = (SONGDATA *) malloc(size);
    size = read(fd, songs, size);

    qsort(songs, size / sizeof(SONGDATA), sizeof(SONGDATA), count_cmp);

    lseek(fd, 0, SEEK_SET);
    write(fd, songs, size);
    ftruncate(fd, size);

    write_data(songs, size / sizeof(SONGDATA));
    free(songs);
  }

  close(fd);
  return 0;
}

#else
int
main()
{
  printf("You shoule define LOG_SONG_USIES first.\n");
  return 0;
}
#endif		/* LOG_SONG_USIES */
