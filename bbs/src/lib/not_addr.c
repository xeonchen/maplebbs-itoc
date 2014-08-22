#define	STRICT_FQDN_EMAIL


int
not_addr(addr)
  char *addr;
{
  int ch, mode;

  mode = -1;

  while (ch = *addr)
  {
    if (ch == '@')
    {
      if (++mode)
	break;
    }

#ifdef	STRICT_FQDN_EMAIL
    else if ((ch != '.') && (ch != '-') && (ch != '_') && !is_alnum(ch))
#else
    else if (!is_alnum(ch) && !strchr(".-_[]%!:", ch))
#endif

      return 1;

    addr++;
  }

  return mode;
}
