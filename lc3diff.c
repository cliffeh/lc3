#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COLOR_RED "\x1B[31m"
#define COLOR_GRN "\x1B[32m"
#define COLOR_RST "\x1B[0m"

// diff 2 lc3 object files

int
main (int argc, char *argv[])
{
  FILE *f1, *f2;
  uint16_t lineno = 0, r1, r2;
  int diff_count = 0, rc;

  if (argc != 3)
    {
      fprintf (stderr, "usage: %s file1 file2\n", argv[0]);
      exit (1);
    }

  if (!(f1 = fopen (argv[1], "r")))
    {
      fprintf (stderr, "error opening %s: %s\n", argv[1], strerror (errno));
      exit (1);
    }

  if (strcmp (argv[2], "-") == 0)
    {
      f2 = stdin;
    }
  else if (!(f2 = fopen (argv[2], "r")))
    {
      fprintf (stderr, "error opening %s: %s\n", argv[2], strerror (errno));
      exit (1);
    }

  while (!feof (f1) && !feof (f2))
    {
      // TODO check for rc == 1
      rc = fread (&r1, sizeof (uint16_t), 1, f1);
      rc = fread (&r2, sizeof (uint16_t), 1, f2);

      if (r1 == r2)
        {
          printf (COLOR_GRN "%04x %04x %04x" COLOR_RST "\n", lineno++, r1, r2);
        }
      else
        {
          printf (COLOR_RED "%04x %04x %04x" COLOR_RST "\n", lineno++, r1, r2);
          diff_count++;
        }
    }

  while (!feof (f1))
    {
      rc = fread (&r1, sizeof (uint16_t), 1, f1);
      printf (COLOR_RED "%04x %04x     " COLOR_RST "\n", lineno++, r1);
      diff_count++;
    }

  while (!feof (f2))
    {
      rc = fread (&r2, sizeof (uint16_t), 1, f2);
      printf (COLOR_RED "%04x      %04x" COLOR_RST "\n", lineno++, r2);
      diff_count++;
    }

  if (diff_count)
    {
      printf ("%d differences\n", diff_count);
      exit (1);
    }

  exit (0);
}