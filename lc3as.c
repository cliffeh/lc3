#ifdef HAVE_CONFIG_H
#include "config.h"
#define VERSION_STRING "lc3as " PACKAGE_VERSION
#endif

#ifndef VERSION_STRING
#define VERSION_STRING "lc3as unknown"
#endif

#include "lc3as.h"
#include "popt/popt.h"
#include "util.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERR_EXIT(args...)                                                     \
  do                                                                          \
    {                                                                         \
      fprintf (stderr, "error: ");                                            \
      fprintf (stderr, args);                                                 \
      fprintf (stderr, "\n");                                                 \
      poptPrintHelp (optCon, stderr, 0);                                      \
      poptFreeContext (optCon);                                               \
      exit (1);                                                               \
    }                                                                         \
  while (0)

extern FILE *yyin;
extern int yyparse (program *prog);

int
main (int argc, const char *argv[])
{
  int rc, flags = FORMAT_OBJECT;
  char *infile = "-", *outfile = "-", *format = "object";
  FILE *out = 0;
  yyin = 0;

  poptContext optCon;

  struct poptOption options[] = {
    /* longName, shortName, argInfo, arg, val, descrip, argDescript */
    { "format", 'F', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &format, 'F',
      "output format; can be one of a[ddress], b[its], d[ebug], h[ex], "
      "o[bject], p[retty] (note: debug is shorthand for -Fa -Fb -Fh -Fp)",
      "FORMAT" },
    { "input", 'i', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &infile, 'i',
      "read input from FILE", "FILE" },
    { "output", 'o', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &outfile,
      'o', "write output to FILE", "FILE" },
    { "version", 0, POPT_ARG_NONE, 0, 'Z', "show version information and exit",
      0 },
    POPT_AUTOHELP POPT_TABLEEND
  };

  optCon = poptGetContext (0, argc, argv, options, 0);

  while ((rc = poptGetNextOpt (optCon)) > 0)
    {
      switch (rc)
        {
        case 'F':
          {
            if (strcmp (format, "a") == 0 || strcmp (format, "address") == 0)
              {
                flags |= FORMAT_ADDR;
              }
            else if (strcmp (format, "b") == 0 || strcmp (format, "bits") == 0)
              {
                flags |= FORMAT_BITS;
              }
            else if (strcmp (format, "d") == 0
                     || strcmp (format, "debug") == 0)
              {
                flags = FORMAT_DEBUG;
              }
            else if (strcmp (format, "h") == 0 || strcmp (format, "hex") == 0)
              {
                flags |= FORMAT_HEX;
              }
            else if (strcmp (format, "o") == 0
                     || strcmp (format, "object") == 0)
              {
                fprintf (stderr, "warning: object format overridden by other "
                                 "format flags\n");
              }
            else if (strcmp (format, "p") == 0
                     || strcmp (format, "pretty") == 0)
              {
                flags |= FORMAT_PRETTY;
              }
            else
              {
                ERR_EXIT ("unknown format specified '%s'", format);
              }
          }
          break;

        case 'i':
          {
            // TODO support >1 input file?
            if (yyin)
              {
                ERR_EXIT ("more than one input file specified");
              }
            else if (strcmp (infile, "-") == 0)
              {
                yyin = stdin;
              }
            else
              {
                if (!(yyin = fopen (infile, "r")))
                  {
                    ERR_EXIT ("couldn't open input file '%s': %s", infile,
                              strerror (errno));
                  }
              }
          }
          break;

        case 'o':
          {
            if (out)
              {
                ERR_EXIT ("more than one output file specified");
              }
            else if (strcmp (outfile, "-") == 0)
              {
                out = stdout;
              }
            else
              {
                if (!(out = fopen (outfile, "w")))
                  {
                    ERR_EXIT ("couldn't open output file '%s': %s", outfile,
                              strerror (errno));
                  }
              }
          }
          break;

        case 'Z':
          {
            printf (VERSION_STRING);
            poptFreeContext (optCon);
            exit (0);
          }
          break;
        }
    }

  if (rc != -1)
    {
      ERR_EXIT ("%s: %s\n", poptBadOption (optCon, POPT_BADOPTION_NOALIAS),
                poptStrerror (rc));
    }

  if (!yyin)
    yyin = stdin;
  if (!out)
    out = stdout;

  program *prog = calloc (1, sizeof (program));

  rc = yyparse (prog);

  if (rc == 0)
    {
      if ((rc = generate_code (out, prog, flags)) != 0)
        {
          fprintf (stderr, "%i errors found\n", rc);
        }
      // TODO free_program()
    }
  else
    {
      // TODO better error handling/messages
      fprintf (stderr, "there was an error\n");
    }

  return rc;
}

// TODO a more efficient way of looking up label addresses
int
find_address_by_label (const instruction_list *instructions, const char *label)
{
  for (const instruction_list *l = instructions; l; l = l->tail)
    {
      const instruction *inst = l->head;
      if (inst->op == -1 && strcmp (label, inst->label) == 0)
        return inst->addr;
    }
  return -1;
}

int
char_to_reg (char c)
{
  // clang-format off
  switch(c)
    {
    case '0': return R_R0;
    case '1': return R_R1;
    case '2': return R_R2;
    case '3': return R_R3;
    case '4': return R_R4;
    case '5': return R_R5;
    case '6': return R_R6;
    case '7': return R_R7;
    default:  return R_COUNT;
    }
  // clang-format on
}

// TODO move to util?
static const char *
trapvec8_to_str (int trapvec8)
{
  // clang-format off
  switch (trapvec8)
    {
      case 0x20: return "GETC";
      case 0x21: return "OUT";
      case 0x22: return "PUTS";
      case 0x23: return "IN";
      case 0x24: return "PUTSP";
      case 0x25: return "HALT";
    }
  // clang-format on
}

#define PPRINT(out, cp, args...)                                              \
  do                                                                          \
    {                                                                         \
      if (cp)                                                                 \
        cp += fprintf (out, "  ");                                            \
      cp += fprintf (out, args);                                              \
    }                                                                         \
  while (0)

int
generate_code (FILE *out, program *prog, int flags)
{
  int err_count = 0, cp = 0;

  if (flags & FORMAT_PRETTY)
    PPRINT (out, cp, ".ORIG x%04X\n", prog->orig);

  for (instruction_list *l = prog->instructions; l; l = l->tail)
    {
      char buf[32] = "", pbuf[4096] = "";
      cp = 0;

      instruction *inst = l->head;

      if (inst->op >= 0)
        {
          SET_OP (inst->inst, inst->op);
          // TODO use orig+addr?
          if (flags & FORMAT_ADDR)
            PPRINT (out, cp, "x%04X", inst->addr);
        }

      // TODO need to actually print out the object code!
      switch (inst->op)
        {
        case -2:
          {
            if (flags & FORMAT_PRETTY)
              {
                sprintf (pbuf, ".STRINGZ \"%s\"", inst->label);
              }
          }
          break;

        case -1:
          {
            if (flags & FORMAT_PRETTY)
              {
                PPRINT (out, cp, "%s\n", inst->label);
                continue;
              }
          }
          break;

        case OP_ADD:
          {
            inst->inst |= (inst->reg[0] << 9);
            inst->inst |= (inst->reg[1] << 6);
            if (inst->immediate)
              {
                inst->inst |= (1 << 5);
                // bottom 5 bits
                inst->inst |= (inst->imm5 & 0x0000001F);
                sprintf (pbuf, "ADD R%d, R%d, #%d", inst->reg[0], inst->reg[1],
                         inst->imm5);
              }
            else
              {
                inst->inst |= inst->reg[2];

                sprintf (pbuf, "ADD R%d, R%d, R%d", inst->reg[0], inst->reg[1],
                         inst->reg[2]);
              }
          }
          break;

        case OP_AND:
          {
            inst->inst |= (inst->reg[0] << 9);
            inst->inst |= (inst->reg[1] << 6);
            if (inst->immediate)
              {
                inst->inst |= (1 << 5);
                // bottom 5 bits
                inst->inst |= (inst->imm5 & 0x0000001F);

                sprintf (pbuf, "AND R%d, R%d, #%d", inst->reg[0], inst->reg[1],
                         inst->imm5);
              }
            else
              {
                inst->inst |= inst->reg[2];

                sprintf (pbuf, "AND R%d, R%d, R%d", inst->reg[0], inst->reg[1],
                         inst->reg[2]);
              }
          }
          break;

        case OP_BR:
          {
            inst->inst |= (inst->cond << 9);
            int addr = find_address_by_label (prog->instructions, inst->label);
            if (addr < 0)
              {
                fprintf (stderr,
                         "error: could not find address for label '%s'\n",
                         inst->label);
                err_count++;
              }
            else
              {
                // bottom 9 bits
                inst->inst |= (addr & 0x000001FF);
              }

            sprintf (pbuf, "BR");
            char *p = pbuf + 2;
            if (inst->cond & FL_NEG)
              *p++ = 'n';

            if (inst->cond & FL_ZRO)
              *p++ = 'z';

            if (inst->cond & FL_POS)
              *p++ = 'p';

            sprintf (p, " %s", inst->label);
          }
          break;

        case OP_JMP:
          {

            if (inst->immediate)
              {
                inst->inst |= (inst->reg[0] << 6);
                sprintf (pbuf, "JMP R%d", inst->reg[0]);
              }
            else
              { // RET special case
                inst->inst |= (7 << 6);
                sprintf (pbuf, "RET");
              }
          }
          break;

        case OP_JSR:
          {
            if (inst->immediate)
              {
                inst->inst |= (1 << 11);
                int addr
                    = find_address_by_label (prog->instructions, inst->label);
                if (addr < 0)
                  {
                    fprintf (stderr,
                             "error: could not find address for label '%s'\n",
                             inst->label);
                    err_count++;
                  }
                else
                  {
                    // bottom 11 bits
                    inst->inst |= (addr & 0x000007FF);
                  }
                sprintf (pbuf, "JSR %s", inst->label);
              }
            else
              {
                inst->inst |= (inst->reg[0] << 6);
                sprintf (pbuf, "JSRR R%d", inst->reg[0]);
              }
          }
          break;

        case OP_LD:
          {
            inst->inst |= (inst->reg[0] << 9);
            int addr = find_address_by_label (prog->instructions, inst->label);
            if (addr < 0)
              {
                fprintf (stderr,
                         "error: could not find address for label '%s'\n",
                         inst->label);
                err_count++;
              }
            else
              {
                // bottom 9 bits
                inst->inst |= (addr & 0x000001FF);
              }
            sprintf (pbuf, "LD R%d, %s", inst->reg[0], inst->label);
          }
          break;

        case OP_LDI:
          {
            inst->inst |= (inst->reg[0] << 9);
            int addr = find_address_by_label (prog->instructions, inst->label);
            if (addr < 0)
              {
                fprintf (stderr,
                         "error: could not find address for label '%s'\n",
                         inst->label);
                err_count++;
              }
            else
              {
                // bottom 9 bits
                inst->inst |= (addr & 0x000001FF);
              }
            sprintf (pbuf, "LDI R%d, %s", inst->reg[0], inst->label);
          }
          break;

        case OP_LDR:
          {
            inst->inst |= (inst->reg[0] << 9);
            inst->inst |= (inst->reg[1] << 6);
            // bottom 6 bits
            inst->inst |= (inst->offset6 & 0x0000003F);
            sprintf (pbuf, "LDR R%d, R%d, #%d", inst->reg[0], inst->reg[1],
                     inst->offset6);
          }
          break;

        case OP_LEA:
          {
            inst->inst |= (inst->reg[0] << 9);
            int addr = find_address_by_label (prog->instructions, inst->label);
            if (addr < 0)
              {
                fprintf (stderr,
                         "error: could not find address for label '%s'\n",
                         inst->label);
                err_count++;
              }
            else
              {
                // bottom 9 bits
                inst->inst |= (addr & 0x000001FF);
              }
            sprintf (pbuf, "LEA R%d, %s", inst->reg[0], inst->label);
          }
          break;

        case OP_NOT:
          {
            inst->inst |= (inst->reg[0] << 9);
            inst->inst |= (inst->reg[1] << 6);
            inst->inst |= (0x3F); // bottom 6 bits
            sprintf (pbuf, "NOT R%d, R%d", inst->reg[0], inst->reg[1]);
          }
          break;

        case OP_RTI:
          {
            sprintf (pbuf, "RTI");
          }
          break;

        case OP_ST:
          {
            inst->inst |= (inst->reg[0] << 9);
            int addr = find_address_by_label (prog->instructions, inst->label);
            if (addr < 0)
              {
                fprintf (stderr,
                         "error: could not find address for label '%s'\n",
                         inst->label);
                err_count++;
              }
            else
              {
                // bottom 9 bits
                inst->inst |= (addr & 0x000001FF);
              }
            sprintf (pbuf, "ST R%d, %s", inst->reg[0], inst->label);
          }
          break;

        case OP_STI:
          {
            inst->inst |= (inst->reg[0] << 9);
            int addr = find_address_by_label (prog->instructions, inst->label);
            if (addr < 0)
              {
                fprintf (stderr,
                         "error: could not find address for label '%s'\n",
                         inst->label);
                err_count++;
              }
            else
              {
                // bottom 9 bits
                inst->inst |= (addr & 0x000001FF);
              }
            sprintf (pbuf, "STI R%d, %s", inst->reg[0], inst->label);
          }
          break;

        case OP_STR:
          {
            inst->inst |= (inst->reg[0] << 9);
            inst->inst |= (inst->reg[1] << 6);
            // bottom 6 bits
            inst->inst |= (inst->offset6 & 0x0000003F);
            sprintf (pbuf, "STR R%d, R%d, #%d", inst->reg[0], inst->reg[1],
                     inst->offset6);
          }
          break;

        case OP_TRAP:
          {
            inst->inst |= inst->trapvect8;
            if (inst->immediate)
              sprintf (pbuf, "TRAP x%x", inst->trapvect8);
            else
              sprintf (pbuf, "%s", trapvec8_to_str (inst->trapvect8));
          }
          break;
        }

      if (inst->op >= 0)
        {
          if (flags & FORMAT_HEX)
            PPRINT (out, cp, "x%04X", inst->inst);

          if (flags & FORMAT_BITS)
            {
              inst_to_bits (buf, inst->inst);
              PPRINT (out, cp, "%s", buf);
            }

          if (!flags)
            {
              fwrite (&inst->inst, sizeof (uint16_t), 1, out);
            }
        }

      if (flags & FORMAT_PRETTY)
        {
          if (!cp)
            fprintf (out, "  ");
          PPRINT (out, cp, "%s", pbuf);
        }

      if (flags)
        {
          fprintf (out, "\n");
        }
    }

  if (flags & FORMAT_PRETTY)
    fprintf (out, ".END\n");

  return err_count;
}
