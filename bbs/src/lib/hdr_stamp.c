/* ----------------------------------------------------- */
/* hdr_stamp - create unique HDR based on timestamp	 */
/* ----------------------------------------------------- */
/* fpath - directory					 */
/* token - (A / F / 0) | [HDR_LINK / HDR_COPY]		 */
/* ----------------------------------------------------- */
/* return : open() fd (not close yet) or link() result	 */
/* ----------------------------------------------------- */


#include "dao.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#if 0	/* itoc.030303.註解: 簡易說明 */

  hdr_stamp() 會做出一個新的 HDR，依傳入的 token 不同而有差異：

   0 : 新增一篇信件(family 是 @)，回傳的 fpath 是 hdr 所指向
       和 hdr_fpath(fpath, folder, hdr); 所產生的 fpath 相同

  'A': 新增一篇文章(family 是 A)，回傳的 fpath 是 hdr 所指向
       和 hdr_fpath(fpath, folder, hdr); 所產生的 fpath 相同

  'F': 新增一個卷宗(family 是 F)，回傳的 fpath 是 hdr 所指向
       和 hdr_fpath(fpath, folder, hdr); 所產生的 fpath 相同

  HDR_LINK      : fpath 已有舊檔案時，要複製舊檔案到新信件(family 是 @) 去
                  並將 hdr 指向這篇新信件，回傳的 fpath 是原來舊檔案
                  舊檔案和新信件是 hard link，改了其中一篇，另一篇也會一起被改
                  刪除舊檔案，新信件並不會被刪除

  HDR_LINK | 'A': fpath 已有舊檔案時，要複製舊檔案到新文章(family 是 A) 去
                  並將 hdr 指向這篇新文章，回傳的 fpath 是原來舊檔案
                  舊檔案和新文章是 hard link，改了其中一篇，另一篇也會一起被改
                  刪除舊檔案，新文章並不會被刪除

  HDR_COPY      : fpath 已有舊檔案時，要複製舊檔案到新信件(family 是 @) 去
                  並將 hdr 指向這篇新信件，回傳的 fpath 是原來舊檔案
                  舊檔案和新信件是 copy，改了其中一篇，另一篇並不會被改
                  舊檔案與新信件是完全獨立不相關的二個檔案

  HDR_COPY | 'A': fpath 已有舊檔案時，要複製舊檔案到新文章(family 是 A) 去
                  並將 hdr 指向這篇新文章，回傳的 fpath 是原來舊檔案
                  舊檔案和新文章是 copy，改了其中一篇，另一篇並不會被改
                  舊檔案與新文章是完全獨立不相關的二個檔案

#endif

int
hdr_stamp(folder, token, hdr, fpath)
  char *folder;
  int token;
  HDR *hdr;
  char *fpath;
{
  char *fname, *family;
  int rc, chrono;
  char *flink, buf[128];

  flink = NULL;
  if (token & (HDR_LINK | HDR_COPY))
  {
    flink = fpath;
    fpath = buf;
  }

  fname = fpath;
  while (rc = *folder++)
  {
    *fname++ = rc;
    if (rc == '/')
      family = fname;
  }
  if (*family != '.')
  {
    fname = family;
    family -= 2;
  }
  else
  {
    fname = family + 1;
    *fname++ = '/';
  }

  if (rc = token & 0xdf)	/* 變大寫 */
  {
    *fname++ = rc;
  }
  else
  {
    *fname = *family = '@';
    family = ++fname;
  }

  chrono = time(0);

  for (;;)
  {
    *family = radix32[chrono & 31];
    archiv32(chrono, fname);

    if (flink)
    {
      if (token & HDR_LINK)
	rc = f_ln(flink, fpath);
      else
        rc = f_cp(flink, fpath, O_EXCL);
    }
    else
    {
      rc = open(fpath, O_WRONLY | O_CREAT | O_EXCL, 0600);
    }

    if (rc >= 0)
    {
      memset(hdr, 0, sizeof(HDR));
      hdr->chrono = chrono;
      str_stamp(hdr->date, &hdr->chrono);
      strcpy(hdr->xname, --fname);
      break;
    }

    if (errno != EEXIST)
      break;

    chrono++;
  }

  return rc;
}
