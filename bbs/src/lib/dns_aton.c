/*-------------------------------------------------------*/
/* lib/dns_aton.c	( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : included C file for DNS routines		 */
/* create : 96/11/20					 */
/* update : 99/08/11					 */
/*-------------------------------------------------------*/


#include "dns.h"


/* ----------------------------------------------------- */
/* get IP address by address string 			 */
/* ----------------------------------------------------- */

/* Thor.990811: §ì¥Xip addr */

unsigned long
dns_aton(name)
  char *name;
{
  ip_addr addr; 
  u_char *cp; 
  int cc, cd;

  /* disallow names consisting only of digits/dots, unless they end in a dot. */

  cc = name[0];
  if (cc >= '0' && cc <= '9') 
  { 
    addr.addr = 0; cd = 0; 

    for (cp = name;; ++cp) 
    { 
      cc = *cp; 
      if (!cc) 
      { 
        /* 
         * All-numeric, no dot at the end. Fake up a hostent as if we'd 
         * actually done a lookup.  What if someone types 255.255.255.255? 
         * The test below will succeed spuriously... ??? 
         */ 
 
        if (*--cp == '.' || cd != 3) 
          return INADDR_NONE; 
 
        return addr.addr;
      } 
      else if(cc >= '0' && cc <= '9')
      {
        addr.d[cd] *= 10;
        addr.d[cd] += cc - '0';
      }
      else if(cc == '.')
      {
        if( ++cd >= 4)
          break;
      }
      else
        break;
    } 
  }              
  return INADDR_NONE;
}

#if 0
/* ----------------------------------------------------- */
/* get IP address by host name				 */
/* ----------------------------------------------------- */


unsigned long
dns_addr(name)
  char *name;
{
  ip_addr addr;
  u_char *cp, *eom;
  int cc, n, type, ancount, qdcount;
  querybuf ans;
  char hostbuf[MAXDNAME];

  /* disallow names consisting only of digits/dots, unless they end in a dot. */

  cc = name[0];
  if (cc >= '0' && cc <= '9')
  {
    for (cp = name;; ++cp)
    {
      cc = *cp;
      if (!cc)
      {
	/*
	 * All-numeric, no dot at the end. Fake up a hostent as if we'd
	 * actually done a lookup.  What if someone types 255.255.255.255?
	 * The test below will succeed spuriously... ???
	 */

	if (*--cp == '.')
	  break;

	return inet_addr(name);
      }
      if ((cc < '0' || cc > '9') && cc != '.')
	break;
    }
  }

  n = dns_query(name, T_A, &ans);
  if (n < 0)
    return INADDR_NONE;

  /* find first satisfactory answer */

  cp = (u_char *) & ans + sizeof(HEADER);
  eom = (u_char *) & ans + n;

  for (qdcount = ntohs(ans.hdr.qdcount); qdcount--; cp += n + QFIXEDSZ)
  {
    if ((n = dn_skipname(cp, eom)) < 0)
      return INADDR_NONE;
  }

  ancount = ntohs(ans.hdr.ancount);
  while (--ancount >= 0 && cp < eom)
  {
    if ((n = dn_expand((u_char *) &ans, eom, cp, hostbuf, MAXDNAME)) < 0)
      return INADDR_NONE;

    cp += n;
    type = getshort(cp);
    n = getshort(cp + 8);
    cp += 10;

    if (type == T_A)
    {

      addr.d[0] = cp[0];
      addr.d[1] = cp[1];
      addr.d[2] = cp[2];
      addr.d[3] = cp[3];
      return addr.addr;
    }

    cp += n;
  }
  return INADDR_NONE;
}
#endif
