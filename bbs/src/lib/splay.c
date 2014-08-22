/*-------------------------------------------------------*/
/* lib/splay.c		( NTHU CS MapleBBS Ver 3.00 )	 */
/*-------------------------------------------------------*/
/* author : opus.bbs@bbs.cs.nthu.edu.tw		 	 */
/* target : splay-tree sort routines		 	 */
/* create : 97/03/29				 	 */
/* update : 97/03/29				 	 */
/*-------------------------------------------------------*/


#include "splay.h"
#include <stdio.h>
#include <stdlib.h>


SplayNode *
splay_in(top, data, compare)
  SplayNode *top;
  void *data;
  int (*compare)();
{
  int splay_cmp;
  SplayNode *node, *l, *r, *x, N;

  node = (SplayNode *) malloc(sizeof(SplayNode));
  node->data = data;

  if (top == NULL)
  {
    node->left = node->right = NULL;
    return node;
  }

  /* --------------------------------------------------- */
  /* splay this splay tree				 */
  /* --------------------------------------------------- */

  N.left = N.right = NULL;
  l = r = &N;

  for (;;)
  {
    splay_cmp = compare(data, top->data);
    if (splay_cmp < 0)
    {
      if (!(x = top->left))
	break;
      if ((splay_cmp = compare(data, x->data)) < 0)
      {
	/* rotate right */

	top->left = x->right;
	x->right = top;
	top = x;
	if (top->left == NULL)
	  break;
      }
      r->left = top;		/* link right */
      r = top;
      top = top->left;
    }
    else if (splay_cmp > 0)
    {
      if (!(x = top->right))
	break;
      if ((splay_cmp = compare(data, x->data)) > 0)
      {
	/* rotate left */

	top->right = x->left;
	x->left = top;
	top = x;
	if (top->right == NULL)
	  break;
      }
      l->right = top;		/* link left */
      l = top;
      top = top->right;
    }
    else
    {
      break;
    }
  }

  l->right = top->left;		/* assemble */
  r->left = top->right;
  top->left = N.right;
  top->right = N.left;

  /* --------------------------------------------------- */
  /* construct this splay tree				 */
  /* --------------------------------------------------- */

  if (splay_cmp < 0)
  {
    node->left = top->left;
    node->right = top;
    top->left = NULL;
    return node;
  }

  if (splay_cmp > 0)
  {
    node->right = top->right;
    node->left = top;
    top->right = NULL;
    return node;
  }

  /* duplicate entry */

  free(node);
  return top;
}


#ifdef TEST

typedef struct
{
  int i;
}      intnode;


static void
printint(a)
  intnode *a;
{
  printf("%d\n", a->i);
}


static int
compareint(a, b)
  intnode *a, *b;
{
  return a->i - b->i;
}


static void
splay_out(top)
  SplayNode *top;
{
  if (top == NULL)
    return;

  splay_out(top->left);
  printint(top->data);
  splay_out(top->right);
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int i;
  intnode *I;
  SplayNode *top = NULL;

  srand(time(NULL));
  for (i = 0; i < 100; i++)
  {
    I = (intnode *) malloc(sizeof(intnode));
    I->i = rand() % 1000;
    top = splay_in(top, I, compareint);
  }
  splay_out(top);
  return 0;
}

#endif		/* TEST */
