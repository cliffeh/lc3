#include "lc3.h"

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
/* unix only */
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/termios.h>
#include <sys/types.h>
#include <unistd.h>

#define MEMORY_MAX (1 << 16)

struct termios original_tio;

void
disable_input_buffering ()
{
  tcgetattr (STDIN_FILENO, &original_tio);
  struct termios new_tio = original_tio;
  new_tio.c_lflag &= ~ICANON & ~ECHO;
  tcsetattr (STDIN_FILENO, TCSANOW, &new_tio);
}

void
restore_input_buffering ()
{
  tcsetattr (STDIN_FILENO, TCSANOW, &original_tio);
}

void
handle_interrupt (int signal)
{
  restore_input_buffering ();
  printf ("\n");
  exit (-2);
}

void
read_image_file (uint16_t memory[], FILE *file)
{
  /* the origin tells us where in memory to place the image */
  uint16_t origin;
  fread (&origin, sizeof (origin), 1, file);
  origin = SWAP16 (origin);

  /* we know the maximum file size so we only need one fread */
  uint16_t max_read = MEMORY_MAX - origin;
  uint16_t *p = memory + origin;
  size_t read = fread (p, sizeof (uint16_t), max_read, file);

  /* swap to little endian */
  while (read-- > 0)
    {
      *p = SWAP16 (*p);
      ++p;
    }
}

int
read_image (uint16_t memory[], const char *image_path)
{
  FILE *file = fopen (image_path, "rb");
  if (!file)
    {
      return 0;
    };
  read_image_file (memory, file);
  fclose (file);
  return 1;
}

// TODO put this in a header somewhere
int execute_program (uint16_t memory[], uint16_t reg[]);

int
main (int argc, const char *argv[])
{
  if (argc < 2)
    {
      /* show usage string */
      printf ("lc3 [image-file1] ...\n");
      exit (2);
    }

  uint16_t memory[MEMORY_MAX]; /* 65536 locations */
  uint16_t reg[R_COUNT];

  for (int j = 1; j < argc; ++j)
    {
      if (!read_image (memory, argv[j]))
        {
          printf ("failed to load image: %s\n", argv[j]);
          exit (1);
        }
    }

  signal (SIGINT, handle_interrupt);
  disable_input_buffering ();
  // TODO capture return code
  execute_program (memory, reg);
  restore_input_buffering ();

  exit (0);
}
