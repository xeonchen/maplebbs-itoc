/*-------------------------------------------------------*/
/* proto.h	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : prototype and macros			 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/


#ifndef	_PROTO_H_
#define	_PROTO_H_


/* ----------------------------------------------------- */
/* External function declarations			 */
/* ----------------------------------------------------- */

/* OS */

char *genpasswd();


/* ----------------------------------------------------- */
/* prototypes						 */
/* ----------------------------------------------------- */


#include "../maple/maple.p"


/* ----------------------------------------------------- */
/* macros						 */
/* ----------------------------------------------------- */


#define	dashd(fpath)	S_ISDIR(f_mode(fpath))
#define	dashf(fpath)	S_ISREG(f_mode(fpath))


#define	STR4(x)		((x[0] << 24) + (x[1] << 16) + (x[2] << 8) + x[3])
                     /* Thor.980913: «OÃÒprecedence */

#define	IS_ZHC_LO	is_zhc_low
#define	IS_ZHC_HI(x)	(x & 0x80)

#if 0
#define rnd(x)		(rand() % (x))		/* using lower-order bits */
#endif

#define rnd(x)		((int) (((x) + 0.0) * rand() / (RAND_MAX + 1.0)))	/* using high-order bits */

#endif				/* _PROTO_H_ */
