#include "util.h"
#include "lc3.h"

void
inst_to_bits (char *dest, uint16_t inst)
{
  char *p = dest;
  for (int i = 15; i >= 0; i--)
    {
      if (inst & (1 << i))
        *p++ = '1';
      else
        *p++ = '0';

      if (i && i % 4 == 0)
        *p++ = ' ';
    }

  // null-terminate like a good citizen
  *p = 0;
}

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
                return *p;
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
