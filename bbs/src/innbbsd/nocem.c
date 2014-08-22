/*-------------------------------------------------------*/
/* nocem.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : NoCeM-INNBBSD				 */
/* create : 99/02/25					 */
/* update :   /  /  					 */
/* author : leeym@cae.ce.ntu.edu.tw			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#if 0	/* itoc.030109.註解: nocem.c 的流程 */
  從 rec_article.c 收到 receive_nocem() 以後
  receive_nocem() → NCMparse() 把 notice parse 出來 → NCMverify() 驗證是不是真的
  → NCMcancel() 再送回 rec_article.c 的 cancel_article() 處理
#endif
                          

#include "innbbsconf.h"

#ifdef _NoCeM_

#include "bbslib.h"
#include "inntobbs.h"
#include "nocem.h"


/* 驗證簽名：以下二者至多只能選一者 #define (可以二者都 #undef) */
#undef	PGP	/* 必須裝有 pgp5 才可 define，並請檢查 pgpv 的路徑 */
#undef	GPG	/* 必須裝有 gpg 才可 define，並請檢查 gpg 的路徑 */

static int num_spammid = 0;
static char NCMVER[20];
static char ISSUER[80];
static char TYPE[40];
static char ACTION[20];
static char SPAMMID_NOW[80];
static char SPAMMID[MAXSPAMMID][80];
static char errmsg[512] = "nothing";


/* ----------------------------------------------------- */
/* NCM maintain						 */
/* ----------------------------------------------------- */


ncmperm_t *
search_issuer(issuer, type)
  char *issuer;
  char *type;		/* 若 type == NULL 表示只比對 issuer */
{
  ncmperm_t *find;
  int i;

  for (i = 0; i < NCMCOUNT; i++)
  {
    find = NCMPERM + i;
    if (strstr(issuer, find->issuer) && 
      (!type || !strcmp(find->type, "*") || !str_cmp(find->type, type)))
      return find;
  }
  return NULL;
}


static void
NCMupdate(issuer, type)
  char *issuer, *type;
{
  ncmperm_t ncm;

  memset(&ncm, 0, sizeof(ncmperm_t));
  str_ncpy(ncm.issuer, issuer, sizeof(ncm.issuer));
  str_ncpy(ncm.type, type, sizeof(ncm.type));
  ncm.perm = 0;
  rec_add("innd/ncmperm.bbs", &ncm, sizeof(ncmperm_t));
  read_ncmperm();
}


/* ----------------------------------------------------- */
/* PGP verify						 */
/* ----------------------------------------------------- */


#ifdef PGP
static int
run_pgp(cmd, in, out)
  char *cmd;
  FILE **in, **out;
{
  int pin[2], pout[2], child_pid;
  char fpath[64];

  strcpy(fpath, BBSHOME "/.pgp");
  setenv("PGPPATH", fpath, 1);

  *in = *out = NULL;

  pipe(pin);
  pipe(pout);

  if (!(child_pid = fork()))
  {
    /* We're the child. */
    close(pin[1]);
    dup2(pin[0], 0);
    close(pin[0]);

    close(pout[0]);
    dup2(pout[1], 1);
    close(pout[1]);

    execl("/bin/sh", "sh", "-c", cmd, NULL);
    _exit(127);
  }
  /* Only get here if we're the parent. */
  close(pout[1]);
  *out = fdopen(pout[0], "r");

  close(pin[0]);
  *in = fdopen(pin[1], "w");

  return child_pid;
}


static int
verify_buffer(buf, passphrase)
  char *buf, *passphrase;
{
  FILE *pgpin, *pgpout;
  char tmpbuf[1024] = " ";
  int ans = NOPGP;

  setenv("PGPPASSFD", "0", 1);
  run_pgp("/usr/local/bin/pgpv -f +batchmode=1 +OutputInformationFD=1", &pgpin, &pgpout);
  if (pgpin && pgpout)
  {
    fprintf(pgpin, "%s\n", passphrase);		/* Send the passphrase in, first */
    memset(passphrase, 0, strlen(passphrase));	/* Burn the passphrase */
    fprintf(pgpin, "%s", buf);
    fclose(pgpin);

    *buf = '\0';
    fgets(tmpbuf, sizeof(tmpbuf), pgpout);
    while (!feof(pgpout))
    {
      strcat(buf, tmpbuf);
      fgets(tmpbuf, sizeof(tmpbuf), pgpout);
    }

    wait(NULL);

    fclose(pgpout);
  }

  if (strstr(buf, "BAD signature made"))
  {
    strcpy(errmsg, "BAD signature");
    ans = PGPBAD;
  }
  else if (strstr(buf, "Good signature made"))
  {
    strcpy(errmsg, "Good signature");
    ans = PGPGOOD;
  }
  else if (strcpy(tmpbuf, strstr(buf, "Signature by unknown keyid:")))
  {
    sprintf(errmsg, "%s ", strtok(tmpbuf, "\r\n"));
    ans = PGPUN;
  }

  unsetenv("PGPPASSFD");
  return ans;
}


static int	/* return 0 success, otherwise fail */
NCMverify()
{
  char passphrase[80] = "Haha, I am Leeym..";
  return verify_buffer(BODY, passphrase);
}
#endif	/* PGP */


/* ----------------------------------------------------- */
/* GPG verify						 */
/* ----------------------------------------------------- */


#ifdef GPG
static int
run_gpg(cmd, in, out)
  char *cmd;
  FILE **in, **out;
{
  int pin[2], pout[2], child_pid;
  char fpath[64];

  strcpy(fpath, BBSHOME "/.gnupg");
  setenv("GPGPATH", fpath, 1);

  *in = *out = NULL;

  pipe(pin);
  pipe(pout);

  if (!(child_pid = fork()))
  {
    /* We're the child. */
    close(pin[1]);
    dup2(pin[0], 0);
    close(pin[0]);

    close(pout[0]);
    dup2(pout[1], 1);
    close(pout[1]);

    execl("/bin/sh", "sh", "-c", cmd, NULL);
    _exit(127);
  }
  /* Only get here if we're the parent. */
  close(pout[1]);
  *out = fdopen(pout[0], "r");

  close(pin[0]);
  *in = fdopen(pin[1], "w");

  return child_pid;
}


static int
verify_buffer(buf)
  char *buf;
{
  FILE *pgpin, *pgpout;
  char tmpbuf[1024] = " ";
  int ans = NOPGP;

  setenv("PGPPASSFD", "0", 1);
  run_gpg("/usr/local/bin/gpg --no-secmem-warning --verify", &pgpin, &pgpout);
  if (pgpin && pgpout)
  {
    fprintf(pgpin, "%s", buf);
    fclose(pgpin);

    *buf = '\0';
    fgets(tmpbuf, sizeof(tmpbuf), pgpout);
    while (!feof(pgpout))
    {
      strcat(buf, tmpbuf);
      fgets(tmpbuf, sizeof(tmpbuf), pgpout);
    }

    wait(NULL);

    fclose(pgpout);
  }

  if (strstr(buf, "BAD signature made"))
  {
    strcpy(errmsg, "BAD signature");
    ans = PGPBAD;
  }
  else if (strstr(buf, "Good signature made"))
  {
    strcpy(errmsg, "Good signature");
    ans = PGPGOOD;
  }
  else if (strcpy(tmpbuf, strstr(buf, "Signature by unknown keyid:")))
  {
    sprintf(errmsg, "%s ", strtok(tmpbuf, "\r\n"));
    strcpy(KEYID, strrchr(tmpbuf, ' ') + 1);
    ans = PGPUN;
  }

  unsetenv("PGPPASSFD");
  return ans;
}


static int
NCMverify()
{
  return verify_buffer(BODY);
}
#endif	/* GPG */


/* ----------------------------------------------------- */
/* parse NoCeM Notice Headers/Body			 */
/* ----------------------------------------------------- */


static int
readNCMheader(line)
  char *line;
{
  if (!str_ncmp(line, "Version", strlen("Version")))
  {
    str_ncpy(NCMVER, line + strlen("Version") + 2, sizeof(NCMVER));
    if (strcmp(NCMVER, "0.9"))
    {
      sprintf(errmsg, "unknown version: %s", NCMVER);
      return P_FAIL;
    }
  }
  else if (!str_ncmp(line, "Issuer", strlen("Issuer")))
  {
    str_ncpy(ISSUER, line + strlen("Issuer") + 2, sizeof(ISSUER));
    FROM = ISSUER;
  }
  else if (!str_ncmp(line, "Type", strlen("Type")))
  {
    str_ncpy(TYPE, line + strlen("Type") + 2, sizeof(TYPE));
  }
  else if (!str_ncmp(line, "Action", strlen("Action")))
  {
    str_ncpy(ACTION, line + strlen("Action") + 2, sizeof(ACTION));
    if (strcmp(ACTION, "hide"))
    {
      sprintf(errmsg, "unsupported action: %s", ACTION);
      return P_FAIL;
    }
  }

  return P_OKAY;
}


static int
readNCMbody(line)
  char *line;
{
  char buf[LINELEN], *group;

  strcpy(buf, line);

  if (!strstr(buf, "\t"))
    return P_FAIL;

  group = strrchr(line, '\t') + 1;

  if (buf[0] == '<' && strstr(buf, ">"))
  {
    strtok(buf, "\t");
    strcpy(SPAMMID_NOW, buf);
  }

  if (num_spammid && !strcmp(SPAMMID[num_spammid - 1], SPAMMID_NOW))
    return 0;

  if (search_newsfeeds_bygroup(group))
    strcpy(SPAMMID[num_spammid++], SPAMMID_NOW);

  return 0;
}


static int	/* return 0 success, otherwise fail */
NCMparse()
{
  char *fptr, *ptr;
  int type = TEXT;

  if (!(fptr = strstr(BODY, "-----BEGIN PGP SIGNED MESSAGE-----")))
  {
    strcpy(errmsg, "notice isn't signed");
    return P_FAIL;
  }

  for (ptr = strchr(fptr, '\n'); ptr != NULL && *ptr != '\0'; fptr = ptr + 1, ptr = strchr(fptr, '\n'))
  {
    int ch = *ptr;
    int ch2 = *(ptr - 1);

    *ptr = '\0';
    if (*(ptr - 1) == '\r')
      *(ptr - 1) = '\0';

    if (num_spammid > MAXSPAMMID)
      return P_OKAY;

    if (!strncmp(fptr, "@@", 2))
    {
      if (strstr(fptr, "BEGIN NCM HEADERS"))
      {
	type = NCMHDR;
      }
      else if (strstr(fptr, "BEGIN NCM BODY"))
      {
	if (NCMVER && ISSUER && TYPE && ACTION)
	{
	  ncmperm_t *ncmt;
	  ncmt = (ncmperm_t *) search_issuer(ISSUER, TYPE);
	  if (ncmt == NULL)
	  {
	    NCMupdate(ISSUER, TYPE);
	    sprintf(errmsg, "unknown issuer: %s, %s", ISSUER, MSGID);
	    return P_UNKNOWN;
	  }
	  if (!ncmt->perm)
	  {
	    sprintf(errmsg, "disallow issuer: %s, %s", ISSUER, MSGID);
	    return P_DISALLOW;
	  }
	}
	else
	{
	  strcpy(errmsg, "HEADERS syntax not correct");
	  return P_FAIL;
	}
	type = NCMBDY;
      }
      else if (strstr(fptr, "END NCM BODY"))
      {
        *ptr = ch;
        *(ptr - 1) = ch2;
        break;
      }
      else
      {
	strcpy(errmsg, "NCM Notice syntax not correct");
	return P_FAIL;
      }
      *ptr = ch;
      *(ptr - 1) = ch2;
      continue;
    }

    if (type == NCMHDR && readNCMheader(fptr) == P_FAIL)
      return P_FAIL;
    if (type == NCMBDY)
      readNCMbody(fptr);
    *ptr = ch;
    *(ptr - 1) = ch2;
  }

  if (NCMVER && ISSUER && TYPE && ACTION)
    return P_OKAY;

  strcpy(errmsg, "HEADERS syntax not correct");
  return P_FAIL;
}


extern int cancel_article();


static void
NCMcancel()
{
  int i;

  for (i = 0; i < num_spammid; i++)
    cancel_article(SPAMMID[i]);
}


/* ----------------------------------------------------- */
/* NoCeM-innbbsd					 */
/* ----------------------------------------------------- */


static void
initial_nocem()
{
  memset(SPAMMID[0], 0, strlen(SPAMMID[0]) * num_spammid);
  num_spammid = 0;
  memset(SPAMMID_NOW, 0, strlen(SPAMMID_NOW));
}


int			/* 0:success  -1:fail */
receive_nocem()
{
  int cc;

  initial_nocem();

  cc = NCMparse();

  if (cc != P_OKAY)
  {
    if (cc != P_DISALLOW)
      bbslog("<nocem> :Warn: %s\n", errmsg);
    return 0;
  }

  if (!num_spammid)	/* nothing to cancel */
    return 0;

#if (defined(PGP) || defined(GPG))
  cc = NCMverify();

  if (cc != PGPGOOD)
  {
    bbslog("<nocem> :Warn: %s, %s, %s\n", errmsg, MSGID, ISSUER);
    return -1;
  }
#endif

  NCMcancel();

  return 0;
}
#endif	/* _NoCeM_ */
