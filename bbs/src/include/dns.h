/*-------------------------------------------------------*/
/* lib/dns.h		( NTHU CS MapleBBS Ver 3.00 )	 */
/*-------------------------------------------------------*/
/* target : header file for DNS routines		 */
/* create : 96/11/20					 */
/* update : 96/12/15					 */
/*-------------------------------------------------------*/


#ifndef	_DNS_H_
#define _DNS_H_


#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>


#undef	HAVE_RELAY_SERVER	/* 採用 relay server 來外寄信件 */

#ifdef HAVE_RELAY_SERVER
#define	RELAY_SERVER	"mail.tnfsh.tn.edu.tw"	/* outbound mail server */
#endif


#ifndef INADDR_NONE
#define	INADDR_NONE	0xffffffff
#endif

#define	INADDR_FMT	"%u.%u.%u.%u"


typedef union
{
  unsigned char d[4];
  unsigned long addr;
}     ip_addr;


#if 0
  The standard udp packet size PACKETSZ (512) is not sufficient for some
  nameserver answers containing very many resource records. The resolver may
  switch to tcp and retry if it detects udp packet overflow. Also note that
  the resolver routines res_query and res_search return the size of the
  un*truncated answer in case the supplied answer buffer it not big enough
  to accommodate the entire answer.
#endif


#if	PACKETSZ > 1024
#define MAXPACKET       PACKETSZ
#else
#define	MAXPACKET	1024	/* max packet size used internally by BIND */
#endif


/* MAX_MXLIST 要比 MAX_DNAME 大得多 */
#define MAX_DNAME	128	/* maximum domain name */
#define MAX_MXLIST      1024	/* maximum dx list */


typedef union
{
  HEADER hdr;
  u_char buf[MAXPACKET];
}     querybuf;			/* response of DNS query */


static inline unsigned short
getshort(c)
  unsigned char *c;
{
  unsigned short u;

  u = c[0];
  return (u << 8) + c[1];
}


#endif	/* _DNS_H_ */
