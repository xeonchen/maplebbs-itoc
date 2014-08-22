#include <string.h>


#define	PASSLEN	13


/* ----------------------------------------------------- */
/* password encryption					 */
/* ----------------------------------------------------- */


char *crypt();
static char pwbuf[PASSLEN + 1];


char *
genpasswd(pw)
  char *pw;
{
  char saltc[2];
  int i, c;

  if (!*pw)
    return pw;

  i = 9 * getpid();
  saltc[0] = i & 077;
  saltc[1] = (i >> 6) & 077;

  for (i = 0; i < 2; i++)
  {
    c = saltc[i] + '.';
    if (c > '9')
      c += 7;
    if (c > 'Z')
      c += 6;
    saltc[i] = c;
  }
  strcpy(pwbuf, pw);
  return crypt(pwbuf, saltc);
}


/* Thor.990214: 註解: 合密碼時, 傳回0 */
int
chkpasswd(passwd, test)
  char *passwd, *test;
{
  char *pw;
  
  /* if(!*passwd) return -1 */ /* Thor.990416: 怕有時passwd是空的 */
  str_ncpy(pwbuf, test, sizeof(pwbuf));
  pw = crypt(pwbuf, passwd);
  return (strncmp(pw, passwd, PASSLEN));
}
