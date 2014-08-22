/*-------------------------------------------------------*/
/* help.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : help 說明文件				 */
/* create : 03/05/10				 	 */
/* update :   /  /  				 	 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "bbs.h"


static void
do_help(path)	/* itoc.021122: 說明文件 */
  char *path;
{
  char *str;
  char fpath[64];
  int num, pageno, pagemax, redraw, reload;
  int ch, cur, i;
  struct stat st;
  PAL *pal;

  /* 說明文件都放在 etc/help/ 下 */
  sprintf(fpath, "etc/help/%s/%s", path, fn_dir);
  str = strchr(fpath, '.');

  reload = 1;
  pageno = 0;
  cur = 0;
  pal = NULL;

  do
  {
    if (reload)
    {
      if (stat(fpath, &st) == -1)
	return;

      i = st.st_size;
      num = (i / sizeof(PAL)) - 1;
      if (num < 0)
	return;

      if ((ch = open(fpath, O_RDONLY)) >= 0)
      {
	pal = pal ? (PAL *) realloc(pal, i) : (PAL *) malloc(i);
	read(ch, pal, i);
	close(ch);
      }

      pagemax = num / XO_TALL;
      reload = 0;
      redraw = 1;
    }

    if (redraw)
    {
      /* itoc.註解: 盡量做得像 xover 格式 */
      vs_head("說明文件", str_site);
      prints(NECKER_HELP, d_cols, "");

      i = pageno * XO_TALL;
      ch = BMIN(num, i + b_lines - 4);
      move(3, 0);
      do
      {
	prints("%6d    %-14s%s\n", i + 1, pal[i].userid, pal[i].ship);
	i++;
      } while (i <= ch);

      outf(FEETER_HELP);
      move(3 + cur, 0);
      outc('>');
      redraw = 0;
    }

    ch = vkey();
    switch (ch)
    {
    case KEY_RIGHT:
    case '\n':
    case ' ':
    case 'r':
      i = cur + pageno * XO_TALL;
      strcpy(str, pal[i].userid);
      more(fpath, NULL);
      strcpy(str, fn_dir);
      redraw = 1;
      break;
    
    case Ctrl('P'):
      if (HAS_PERM(PERM_ALLADMIN))
      {
        PAL new;

	memset(&new, 0, sizeof(PAL));

	if (vget(b_lines, 0, "標題：", new.ship, sizeof(new.ship), DOECHO) &&
	  vget(b_lines, 0, "檔案：", new.userid, IDLEN + 1, DOECHO))
	{
	  strcpy(str, new.userid);
	  i = vedit(fpath, 0);
	  strcpy(str, fn_dir);
	  if (!i)
	  {
	    rec_add(fpath, &new, sizeof(PAL));
	    num++;
	    cur = num % XO_TALL;	/* 游標放在新加入的這篇 */
	    pageno = num / XO_TALL;
	    reload = 1;
	  }
	}
	redraw = 1;
      }
      break;

    case 'd':
      if (HAS_PERM(PERM_ALLADMIN))
      {
	if (vans(msg_del_ny) == 'y')
	{
	  i = cur + pageno * XO_TALL;
	  strcpy(str, pal[i].userid);
	  unlink(fpath);
	  strcpy(str, fn_dir);
	  rec_del(fpath, sizeof(PAL), i, NULL);
	  cur = i ? ((i - 1) % XO_TALL) : 0;	/* 游標放在砍掉的前一篇 */
	  reload = 1;
	}
	redraw = 1;
      }
      break;

    case 'T':
      if (HAS_PERM(PERM_ALLADMIN))
      {
	i = cur + pageno * XO_TALL;
	if (vget(b_lines, 0, "標題：", pal[i].ship, sizeof(pal[0].ship), GCARRY))
	  rec_put(fpath, &pal[i], sizeof(PAL), i, NULL);
	redraw = 1;
      }
      break;

    case 'E':
      if (!HAS_STATUS(STATUS_EDITHELP))	/* 若是從 vedit 時進來 help 則不能再 vedit */
      {
	i = cur + pageno * XO_TALL;
	strcpy(str, pal[i].userid);
	vedit(fpath, HAS_PERM(PERM_ALLADMIN) ? 0 : -1);
	strcpy(str, fn_dir);
	redraw = 1;
      }
      break;

    case 'm':
      if (HAS_PERM(PERM_ALLADMIN))
      {
	char buf[40], ans[5];

	i = cur + pageno * XO_TALL;
	sprintf(buf, "請輸入第 %d 選項的新位置：", i + 1);
	if (vget(b_lines, 0, buf, ans, 5, DOECHO))
	{
	  redraw = atoi(ans) - 1;	/* 借用 redraw */
	  if (redraw < 0)
	    redraw = 0;
	  else if (redraw > num)
	    redraw = num;

	  if (redraw != i)
	  {
	    if (!rec_del(fpath, sizeof(PAL), i, NULL))
	    {
	      rec_ins(fpath, &pal[i], sizeof(PAL), redraw, 1);
	      cur = redraw % XO_TALL;
	      pageno = redraw / XO_TALL;
	    }
	    reload = 1;
	  }
	}
	redraw = 1;
      }
      break;

    default:
      ch = xo_cursor(ch, pagemax, num, &pageno, &cur, &redraw);
      break;
    }
  } while (ch != 'q');

  free(pal);
}


#include <stdarg.h>

int
vaHelp(pvar)
  va_list pvar;
{
  char *path;
  path = va_arg(pvar, char *);
  do_help(path);
  return 0;
}
