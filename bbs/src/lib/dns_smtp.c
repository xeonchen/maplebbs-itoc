/*-------------------------------------------------------*/
/* lib/dns_smtp.c	( NTHU CS MapleBBS Ver 3.00 )	 */
/*-------------------------------------------------------*/
/* target : included C file for DNS routines		 */
/* create : 96/11/20					 */
/* update : 96/12/15					 */
/*-------------------------------------------------------*/


#include "dns.h"


/* static inline */ void /* Thor. 990811: for bmtad use to check from */
dns_mx(domain, mxlist)
  char *domain;
  char *mxlist;
{
  querybuf ans;
  int n, ancount, qdcount;
  unsigned char *cp, *eom;
  int type;

  *mxlist = 0;

  n = dns_query(domain, T_MX, &ans);

  if (n < 0)
    return;

  /* find first satisfactory answer */

  cp = (u_char *) & ans + sizeof(HEADER);
  eom = (u_char *) & ans + n;

  for (qdcount = ntohs(ans.hdr.qdcount); qdcount--; cp += n + QFIXEDSZ)
  {
    if ((n = dn_skipname(cp, eom)) < 0)
      return;
  }

  ancount = ntohs(ans.hdr.ancount);
  domain = mxlist + MAX_MXLIST - MAX_DNAME - 2;

  while (--ancount >= 0 && cp < eom)
  {
    if ((n = dn_expand((void *) &ans, eom, cp, mxlist, MAX_DNAME)) < 0)
      break;

    cp += n;

    type = getshort(cp);
    n = getshort(cp + 8);
    cp += 10;

    if (type == T_MX)
    {
      /* pref = getshort(cp); */
      *mxlist = '\0';
      if ((dn_expand((void *) &ans, eom, cp + 2, mxlist, MAX_DNAME)) < 0)
	break;

      if (!*mxlist)
	return;

      /* Thor.980820:註解: 將數個 MX entry 用 : 串起來以便一個一個試 */
      while (*mxlist)
	mxlist++;
      *mxlist++ = ':';

      if (mxlist >= domain)
	break;
    }

    cp += n;
  }

  *mxlist = '\0';
}


int
dns_smtp(host)
  char *host;
{
  int sock;
  char *str, *ptr, mxlist[MAX_MXLIST];

#ifdef HAVE_RELAY_SERVER
  /* 如果有自定的 relay server，先 try 它試試 */
  if ((sock = dns_open(RELAY_SERVER, 25)) >= 0)
    return sock;
#endif

  dns_mx(host, str = mxlist);
  if (!*str)
  {
    /* Thor.990716: 因呼叫時可能將host用ip放入，故作特別處理，
                    使不透過dns_open()寄信 */ 
    /* if(*host>='0' && *host<='9') return -1; */
    /* Thor.990811: 用dns_aton()較完整 */
    if (dns_aton(host) != INADDR_NONE)
      return -1;

    str = host;
  }

  for (;;)
  { /* Thor.980820: 註解: 萬一host格式為 xxx:yyy:zzz, 則先試 xxx,不行再試 yyy */
    ptr = str;
    while (sock = *ptr)
    {
      if (sock == ':')
      {
	*ptr++ = '\0';
	break;
      }
      ptr++;
    }

    if (!*str)
      return -1;

    sock = dns_open(str, 25);
    if (sock >= 0)
      return sock;

    str = ptr;
  }
}
