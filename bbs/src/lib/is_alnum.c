int
is_alnum(ch)
  int ch;
{
  return ((ch >= '0' && ch <= '9') ||
    (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'));
}
