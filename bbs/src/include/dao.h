/*-------------------------------------------------------*/
/* lib/dao.h	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : data abstract object			 */
/* create : 96/11/20					 */
/* update : 96/12/15					 */
/*-------------------------------------------------------*/


#ifndef	_DAO_H_
#define	_DAO_H_


#ifndef	NULL
#define	NULL	0			/* ((char *) 0) */
#endif


#ifndef	BLK_SIZ
#define BLK_SIZ         4096		/* disk I/O block size */
#endif


#ifndef	REC_SIZ
#define REC_SIZ         512		/* disk I/O record size */
#endif


/* Thor.981206: lkchu patch */
extern char radix32[];


#include "hdr.h"			/* prototype */
#include "dns.h"			/* dns type */
#include "splay.h"			/* splay type */
#include "../lib/dao.p"			/* prototype */


#endif	/* _DAO_H_ */
