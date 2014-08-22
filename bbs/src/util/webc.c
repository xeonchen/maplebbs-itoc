/*-------------------------------------------------------*/
/* util/webc.c		( NTHU CS MapleBBS Ver 3.00 )	 */
/*-------------------------------------------------------*/
/* target : WEB client (command-line mode)		 */
/* create : 95/03/29				 	 */
/* update : 97/03/29				 	 */
/*-------------------------------------------------------*/
/* syntax : webc file host URL [port]			 */
/*-------------------------------------------------------*/


#include "bbs.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>


static char pool[4096];


static int
webc(file, host, path, port)
  char *file;
  char *host;
  char *path;
  int port;
{
  int cc, sock, tlen;
  FILE *fp;
  char *xhead, *xtail, buf[120], tag[8];

  sock = dns_open(host, port);
  if (sock < 0)
    return -1;

  sprintf(buf, "GET %s\n\n", path);
  cc = strlen(buf);
  if (send(sock, buf, cc, 0) != cc)
  {
    close(sock);
    return -1;
  }

  xhead = pool;
  xtail = pool;
  tlen = 0;

  strcpy(buf, file);
  strcat(buf, "-");
  fp = fopen(buf, "w");

  for (;;)
  {
    if (xhead >= xtail)
    {
      xhead = pool;
      cc = recv(sock, xhead, sizeof(pool), 0);
      if (cc <= 0)
	break;
      xtail = xhead + cc;
    }

    cc = *xhead++;
    if (cc == '<')
    {
      tlen = 1;
      continue;
    }

    if (tlen)
    {
      /* support <br> and <P> */

      if (cc == '>')
      {
	if (tlen == 3 && !str_ncmp(tag, "br", 2))
	{
	  fputc('\n', fp);
	}
	else if (tlen == 2 && !str_ncmp(tag, "P", 1))
	{
	  fputc('\n', fp);
	  fputc('\n', fp);
	}

	tlen = 0;
	continue;
      }

      if (tlen <= 2)
      {
	tag[tlen - 1] = cc;
      }

      tlen++;
      continue;
    }

    if (cc != '\r')
      fputc(cc, fp);
  }

  close(sock);

  fputc('\n', fp);
  fclose(fp);
  rename(buf, file);
  return cc;
}


/* ----------------------------------------------------- */
/* main routines					 */
/* ----------------------------------------------------- */


int
main(argc, argv)
  int argc;
  char *argv[];
{
  if (argc < 4 || argc > 5)
  {
    printf("Usage: %s file host URL [port]\n", argv[0]);
    return -1;
  }

  close(0);
  close(1);
  close(2);

  dns_init();

  webc(argv[1], argv[2], argv[3], argc == 4 ? 80 : atoi(argv[4]));
  return 0;
}
