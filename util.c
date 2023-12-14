#include "util.h"

const char *
unescape_string (char *dest, const char *str)
{
  char *d = dest;
  for (const char *p = str; *p; p++)
    {
      if (*p == '\\')
        {
          switch (*(++p))
            {
              // clang-format off
            case 'a':  *d++ = '\007';  break;
            case 'b':  *d++ = '\b';    break;
            case 'e':  *d++ = '\e';    break;
            case 'f':  *d++ = '\f';    break;
            case 'n':  *d++ = '\n';    break;
            case 'r':  *d++ = '\r';    break;
            case 't':  *d++ = '\t';    break;
            case 'v':  *d++ = '\013';  break;
            case '\\': *d++ = '\\';    break;
            case '?':  *d++ = '?';     break;
            case '\'': *d++ = '\'';    break;
            case '"':  *d++ = '\"';    break;
            default:
              {
                return p;
              }
              // clang-format on
            }
        }
      else
        {
          *d++ = *p;
        }
    }
  // null terminate
  *d = 0;
  return 0;
}

const char *
escape_string (char *dest, const char *str)
{
  char *d = dest;
  for (const char *p = str; *p; p++)
    {
      switch (*p)
        {
          // clang-format off
            case '\007': *d++ = '\\'; *d++ = 'a';   break;
            case '\b':   *d++ = '\\'; *d++ = 'b';   break;
            case '\e':   *d++ = '\\'; *d++ = 'e';   break;
            case '\f':   *d++ = '\\'; *d++ = 'f';   break;
            case '\n':   *d++ = '\\'; *d++ = 'n';   break;
            case '\r':   *d++ = '\\'; *d++ = 'r';   break;
            case '\t':   *d++ = '\\'; *d++ = 't';   break;
            case '\013': *d++ = '\\'; *d++ = 'v';   break;
            case '\\':   *d++ = '\\'; *d++ = '\\';  break;
            case '"':    *d++ = '\\'; *d++ = '"';   break;
            default: *d++ = *p;
          // clang-format on
        }
    }

  // null terminate
  *d = 0;
  return 0;
}
