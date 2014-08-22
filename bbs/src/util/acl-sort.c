/*-------------------------------------------------------*/
/* util/acl-sort.c	( NTHU CS MapleBBS Ver 3.00 )	 */
/*-------------------------------------------------------*/
/* target : sort [Access Control List]			 */
/* create : 98/03/29				 	 */
/* update : 98/03/29				 	 */
/*-------------------------------------------------------*/
/* syntax : acl-sort <file>				 */
/*-------------------------------------------------------*/


#include "bbs.h"
#include "splay.h"


typedef struct
{
  int domain;
  char text[0];
} AclText;


static int
at_cmp(x, y)
  AclText *x;
  AclText *y;
{
  char *tail1, *tail2;
  int c1, c2, diff;

  tail1 = x->text + x->domain;
  tail2 = y->text + y->domain;

  for (;;)
  {
    c1 = *tail1--;
    if (c1 == '@')
      c1 = 0;
    else if (c1 >= 'A' && c1 <= 'Z')
      c1 |= 32;

    c2 = *tail2--;
    if (c2 == '@')
      c2 = 0;
    else if (c2 >= 'A' && c2 <= 'Z')
      c2 |= 32;

    if (diff = c1 - c2)
      return (diff);
  }
}


static void
at_out(top)
  SplayNode *top;
{
  AclText *at;

  if (top == NULL)
    return;

  at_out(top->left);

  at = (AclText *) top->data;
  fputs(at->text + 1, stdout);

  at_out(top->right);
}


static void
acl_sort(fpath)
  char *fpath;
{
  FILE *fp;
  int len, domain;
  AclText *at;
  SplayNode *top;
  char *str, buf[256];

  if (!(fp = fopen(fpath, "r")))
    return;

  top = NULL;

  while (fgets(buf, sizeof(buf) - 2, fp))
  {
    str = buf;
    if (*str <= '#')
      continue;

    while (*++str > ' ')
      ;

    domain = str - buf;


    while (*str)
      str++;

    len = str - buf;

    at = (AclText *) malloc(sizeof(AclText) + len + 2);
    at->domain = domain;
    at->text[0] = '\0';
    strcpy(at->text + 1, buf);

    top = splay_in(top, at, at_cmp);
  }

  at_out(top);
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  if (argc != 2)
  {
    printf("Usage:\t%s file\n", argv[0]);
  }
  else
  {
    acl_sort(argv[1]);
  }
  exit(0);
}
