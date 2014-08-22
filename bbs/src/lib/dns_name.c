/*-------------------------------------------------------*/
/* lib/dns_name.c	( NTHU CS MapleBBS Ver 3.00 )	 */
/*-------------------------------------------------------*/
/* target : included C file for DNS routines		 */
/* create : 96/11/20					 */
/* update : 96/12/15					 */
/*-------------------------------------------------------*/


#include "dns.h"
#include <string.h>


/* ----------------------------------------------------- */
/* get host name by IP address				 */
/* ----------------------------------------------------- */


int
dns_name(addr, name)
  unsigned char *addr;
  char *name;
{
  querybuf ans;
  char qbuf[MAXDNAME];
  char hostbuf[MAXDNAME];
  int n, type, ancount, qdcount;
  u_char *cp, *eom;

  sprintf(name, INADDR_FMT, addr[0], addr[1], addr[2], addr[3]);

  sprintf(qbuf, INADDR_FMT ".in-addr.arpa",
    addr[3], addr[2], addr[1], addr[0]);

  n = dns_query(qbuf, T_PTR, &ans);
  if (n < 0)
    return n;

  /* find first satisfactory answer */

  cp = (u_char *) & ans + sizeof(HEADER);
  eom = (u_char *) & ans + n;

  for (qdcount = ntohs(ans.hdr.qdcount); qdcount--; cp += n + QFIXEDSZ)
  {
    if ((n = dn_skipname(cp, eom)) < 0)
      return n;
  }

  for (ancount = ntohs(ans.hdr.ancount); --ancount >= 0 && cp < eom; cp += n)
  {
    if ((n = dn_expand((u_char *) &ans, eom, cp, hostbuf, MAXDNAME)) < 0)
      return n;

    cp += n;
    type = getshort(cp);
    n = getshort(cp + 8);
    cp += 10;

    if (type == T_PTR)
    {
      if ((n = dn_expand((u_char *) &ans, eom, cp, hostbuf, MAXDNAME)) >= 0)
      {
	strcpy(name, hostbuf);
	return 0;
      }
    }

#if 0
    if (type == T_CNAME)
    {
      strcpy(name, hostbuf);
      return 0;
    }
#endif
  }

  return -1;
}
