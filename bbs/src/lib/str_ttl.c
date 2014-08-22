char *
str_ttl(title)
  char *title;
{
  if ((title[2] == ':') && 
    ((title[0] == 'R' && title[1] == 'e') || (title[0] == 'F' && title[1] == 'w')))
  {
    title += 3;
    if (*title == ' ')
      title++;
  }

  return title;
}
