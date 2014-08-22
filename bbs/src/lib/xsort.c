#include <sys/types.h>
#include <stdlib.h>


#define min(a, b)	(a) < (b) ? a : b
#undef	TEST


/* Qsort routine from Bentley & McIlroy's "Engineering a Sort Function". */


#define swapcode(TYPE, parmi, parmj, n) { 		\
	long i = (n) / sizeof (TYPE); 			\
	register TYPE *pi = (TYPE *) (parmi); 		\
	register TYPE *pj = (TYPE *) (parmj); 		\
	do { 						\
		register TYPE	t = *pi;		\
		*pi++ = *pj;				\
		*pj++ = t;				\
        } while (--i > 0);				\
}


#define SWAPINIT(a, es) \
	swaptype = (((char *)a - (char *)0) % sizeof(long) || \
	es % sizeof(long)) ? 2 : (es == sizeof(long)? 0 : 1);


static inline void
swapfunc(a, b, n, swaptype)
  char *a, *b;
  int n, swaptype;
{
  if (swaptype <= 1)
    swapcode(long, a, b, n)
  else
    swapcode(char, a, b, n)
}


#define swap(a, b)					\
	if (swaptype == 0) {				\
		long t = *(long *)(a);			\
		*(long *)(a) = *(long *)(b);		\
		*(long *)(b) = t;			\
	} else						\
		swapfunc(a, b, es, swaptype)


#define vecswap(a, b, n) 	if ((n) > 0) swapfunc(a, b, n, swaptype)


static inline char *
med3(a, b, c, cmp)
  char *a, *b, *c;
  int (*cmp) ();
{
  return cmp(a, b) < 0 ?
    (cmp(b, c) < 0 ? b : (cmp(a, c) < 0 ? c : a))
    : (cmp(b, c) > 0 ? b : (cmp(a, c) < 0 ? a : c));
}


void
xsort(a, n, es, cmp)
  void *a;
  size_t n, es;
  int (*cmp) ();
{
  char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
  int d, r, swaptype, swap_cnt;

  SWAPINIT(a, es);

loop:

  swap_cnt = 0;
  if (n < 7)
  {
    for (pm = a + es; pm < (char *) a + n * es; pm += es)
      for (pl = pm; pl > (char *) a && cmp(pl - es, pl) > 0;
	pl -= es)
	swap(pl, pl - es);
    return;
  }

  pm = a + (n / 2) * es;

  if (n > 7)
  {
    pl = a;
    pn = a + (n - 1) * es;
    if (n > 40)
    {
      d = (n >> 3) * es;
      pl = med3(pl, pl + d, pl + d + d, cmp);
      pm = med3(pm - d, pm, pm + d, cmp);
      pn = med3(pn - 2 * d, pn - d, pn, cmp);
    }
    pm = med3(pl, pm, pn, cmp);
  }
  swap(a, pm);
  pa = pb = a + es;

  pc = pd = a + (n - 1) * es;
  for (;;)
  {
    while (pb <= pc && (r = cmp(pb, a)) <= 0)
    {
      if (r == 0)
      {
	swap_cnt = 1;
	swap(pa, pb);
	pa += es;
      }
      pb += es;
    }
    while (pb <= pc && (r = cmp(pc, a)) >= 0)
    {
      if (r == 0)
      {
	swap_cnt = 1;
	swap(pc, pd);
	pd -= es;
      }
      pc -= es;
    }
    if (pb > pc)
      break;
    swap(pb, pc);
    swap_cnt = 1;
    pb += es;
    pc -= es;
  }

  if (swap_cnt == 0)
  {				/* Switch to insertion sort */
    for (pm = a + es; pm < (char *) a + n * es; pm += es)
      for (pl = pm; pl > (char *) a && cmp(pl - es, pl) > 0; pl -= es)
	swap(pl, pl - es);
    return;
  }

  pn = a + n * es;
  r = min(pa - (char *) a, pb - pa);
  vecswap(a, pb - r, r);

  r = min(pd - pc, pn - pd - es);
  vecswap(pb, pn - r, r);

  if ((r = pb - pa) > es)
    xsort(a, r / es, es, cmp);

  if ((r = pd - pc) > es)
  {
    /* Iterate rather than recurse to save stack space */
    a = pn - r;
    n = r / es;
    goto loop;
  }
  /* xsort(pn - r, r / es, es, cmp); */
}


#ifdef	TEST

#define	MMM	(0x40000)

static int
int_cmp(a, b)
  int *a;
  int *b;
{
  return *a - *b;
}


main()
{
  int *x, *y, *z, n;

  x = malloc(MMM * sizeof(int));
  if (!x)
    return;

  y = x;
  z = x + MMM;

  n = time(0) & (0x40000 -1) /* 16387 */;

  do
  {
    *x = n = (n * 10001) & (0x100000 - 1);
  } while (++x < z);

  xsort(y, MMM, sizeof(int), int_cmp);
}
#endif
