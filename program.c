#include "program.h"
#include "parse.h"
#include <stdlib.h>
#include <string.h>

int
assemble_program (program *prog, FILE *in)
{
  yyscan_t scanner;
  yylex_init (&scanner);
  yyset_in (in, scanner);

  int rc = yyparse (prog, scanner);
  if (rc == 0)
    rc = resolve_symbols (prog);

  yylex_destroy (scanner);

  return rc;
}

int
load_program (program *prog, FILE *in)
{
  /* the origin tells us where in memory to place the image */
  size_t read = fread (&prog->orig, sizeof (prog->orig), 1, in);
  prog->orig = SWAP16 (prog->orig);

  /* we know the maximum file size so we only need one fread */
  uint16_t *p = prog->mem + prog->orig;
  read = fread (p, sizeof (uint16_t), (MEMORY_MAX - prog->orig), in);
  prog->len = read;

  /* swap to little endian */
  while (read-- > 0)
    {
      *p = SWAP16 (*p);
      ++p;
    }

  // TODO check for read errors?
  return 0;
}

int
disassemble_program (program *prog, FILE *symin, FILE *in)
{
  if (load_program (prog, in) != 0)
    {
      fprintf (stderr, "error: failed to load program\n");
      return 1;
    }

  if (symin)
    {
      if (load_symbols (prog, symin) != 0)
        {
          fprintf (stderr, "error: failed to load symbols\n");
          return 1;
        }
      else
        attach_symbols (prog);
    }

  return 0;
}

int
attach_symbols (program *prog)
{
}

// int
// attach_symbols (instruction *instructions, symbol *symbols)
// {
// for (symbol *sym = symbols; sym; sym = sym->next)
//   {
//     for (instruction *inst = instructions; inst; inst = inst->next)
//       {
//         if (sym->addr == inst->addr)
//           inst->hint = sym->hint;

//         switch (inst->word >> 12)
//           {
//           case OP_BR:
//           case OP_LD:
//           case OP_LDI:
//           case OP_LEA:
//           case OP_ST:
//           case OP_STI:
//             {
//               int16_t PCoffset9 = inst->word & 0x1FF;
//               int16_t reladdr = ((sym->addr - inst->addr) - 1) & 0x1FF;
//               if (PCoffset9 == reladdr)
//                 {
//                   inst->sym = sym;
//                 }
//             }
//             break;

//           case OP_JSR:
//             {
//               if (inst->word & (1 << 11))
//                 {
//                   int16_t PCoffset11 = inst->word & 0x7FF;
//                   int16_t reladdr = ((sym->addr - inst->addr) - 1) &
//                   0x7FF; if (PCoffset11 == reladdr)
//                     {
//                       inst->sym = sym;
//                     }
//                 }
//             }
//             break;

//           default:
//             {
//               // TODO handle FILL?
//             }
//           }
//       }
//   }
// }

int
resolve_symbols (program *prog)
{
  // for every address in memory
  for (uint16_t iaddr = prog->orig; iaddr < prog->orig + prog->len; iaddr++)
    {
      // ...if that instruction contains a label reference
      if (prog->ref[iaddr] && prog->ref[iaddr]->label)
        {
          int resolved = 0;
          // ...for every adress we know about
          for (uint16_t saddr = prog->orig; saddr < prog->orig + prog->len;
               saddr++)
            {
              // ...if there is a symbol at that address that matches the label
              if (prog->sym[saddr]
                  && strcmp (prog->ref[iaddr]->label, prog->sym[saddr]->label)
                         == 0)
                {
                  if ((prog->ref[iaddr]->flags >> 12) == HINT_FILL)
                    prog->mem[iaddr] = saddr;
                  else
                    prog->mem[iaddr]
                        |= ((saddr - iaddr - 1) & prog->ref[iaddr]->flags);
                  resolved = 1;
                  break;
                }
            }

          // if we get here, we've got an unresolved symbol
          if (!resolved)
            {
              fprintf (stderr, "error: unresolved symbol: %s\n",
                       prog->ref[iaddr]->label);
              return 1;
            }
        }
    }
  return 0;
}

int
load_symbols (program *prog, FILE *in)
{
  char buf[4096];

  while (fgets (buf, 4096, in))
    {
      char *p = strtok (buf, " \t\n");
      char *label = strtok (0, " \t\n");
      char *hint = strtok (0, " \t\n");
      // TODO this could be a little more sophisticated...
      if (*p && *p == 'x' && *label) // hint optional
        {
          // TODO capture endptr?
          uint16_t saddr = strtol (p + 1, 0, 16);
          prog->sym[saddr]->label = strdup (label);
          if (hint)
            prog->sym[saddr]->flags = atoi (hint) << 12;
        }
    }

  return 0;
}

const char *opnames[16][2] = {
  { "BR", "br" },   { "ADD", "add" }, { "LD", "ld" },   { "ST", "st" },
  { "JSR", "jsr" }, { "AND", "and" }, { "LDR", "ldr" }, { "STR", "str" },
  { "RTI", "rti" }, { "NOT", "not" }, { "LDI", "ldi" }, { "STI", "sti" },
  { "JMP", "jmp" }, { "RES", "res" }, { "LEA", "lea" }, { "TRAP", "trap" }
};

int
disassemble_addr (char *dest, int flags, uint16_t addr, program *prog)
{
  int n = 0, rc = 0, cas = (flags & FMT_LC) ? 1 : 0, op;

  // special cases supported by assembler hinting
  if (prog->ref[addr])
    {
      switch (prog->ref[addr]->flags >> 12)
        {
        case HINT_FILL:
          {
            n += sprintf (dest + n, cas ? ".fill" : ".FILL");
            if (prog->ref[addr]->label)
              n += sprintf (dest + n, " %s", prog->ref[addr]->label);
            else
              n += sprintf (dest + n, cas ? " x%0x" : " x%0X",
                            prog->mem[addr]);

            return rc; // 0
          }
          break;

        case HINT_STRINGZ:
          {
            n += sprintf (dest + n, cas ? ".stringz \"" : ".STRINGZ \"");
            while (prog->mem[addr + rc] != 0
                   && (addr + rc) < (prog->orig + prog->len))
              {
                char c = (char)prog->mem[addr + rc++];
                switch (c)
                  {
                  // clang-format off
                    case '\007': n += sprintf (dest + n, "\\a");  break;
                    case '\013': n += sprintf (dest + n, "\\v");  break;
                    case '\b':   n += sprintf (dest + n, "\\b");  break;
                    case '\e':   n += sprintf (dest + n, "\\e");  break;
                    case '\f':   n += sprintf (dest + n, "\\f");  break;
                    case '\n':   n += sprintf (dest + n, "\\n");  break;
                    case '\r':   n += sprintf (dest + n, "\\r");  break;
                    case '\t':   n += sprintf (dest + n, "\\t");  break;
                    case '\\':   n += sprintf (dest + n, "\\\\"); break;
                    case '"':    n += sprintf (dest + n, "\\\""); break;
                    default:     n += sprintf (dest + n, "%c", c);
                    // clang-format on
                  }
              }
            n += sprintf (dest + n, "\"");
            return rc;
          }
          break;
        }
    }

  switch (op = prog->mem[addr] >> 12)
    {
    case OP_ADD:
    case OP_AND:
      {
        n += sprintf (dest + n, "%s", opnames[op][cas]);
        n += sprintf (dest + n, " %c%d,", cas ? 'r' : 'R',
                      ((prog->mem[addr] >> 9) & 0x7));
        n += sprintf (dest + n, " %c%d,", cas ? 'r' : 'R',
                      ((prog->mem[addr] >> 6) & 0x7));

        if (prog->mem[addr] & (1 << 5))
          {
            int16_t imm5 = SIGN_EXTEND (prog->mem[addr] & 0x1F, 5);
            n += sprintf (dest + n, " #%d", imm5);
          }
        else
          n += sprintf (dest + n, " %c%d", cas ? 'r' : 'R',
                        ((prog->mem[addr] >> 0) & 0x7));
      }
      break;

    case OP_BR:
      {
        n += sprintf (dest + n, "%s", opnames[op][cas]);

        // nzp flags always lowercase
        n += sprintf (dest + n, "%s%s%s",
                      (prog->mem[addr] & (1 << 11)) ? "n" : "",
                      (prog->mem[addr] & (1 << 10)) ? "z" : "",
                      (prog->mem[addr] & (1 << 9)) ? "p" : "");

        if (prog->ref[addr])
          n += sprintf (dest + n, " %s", prog->ref[addr]->label);
        else // TODO sign extended int?
          n += sprintf (dest + n, " #%d", (prog->mem[addr] & 0x1FF));
      }
      break;

    case OP_JMP:
      {
        uint16_t BaseR = (prog->mem[addr] >> 6) & 0x7;
        if (BaseR == 7) // assume RET special case
          n += sprintf (dest + n, "%s", cas ? "ret" : "RET");
        else
          {
            n += sprintf (dest + n, "%s", opnames[op][cas]);
            n += sprintf (dest + n, " %c%d", cas ? 'r' : 'R', BaseR);
          }
      }
      break;

    case OP_JSR:
      {
        if (prog->mem[addr] & (1 << 11))
          {
            n += sprintf (dest + n, "%s", opnames[op][cas]);

            if (prog->ref[addr])
              n += sprintf (dest + n, " %s", prog->ref[addr]->label);
            else // TODO sign extended int?
              n += sprintf (dest + n, " #%d", (prog->mem[addr] & 0x7FF));
          }
        else
          {
            n += sprintf (dest + n, "%s", cas ? "jsrr" : "JSRR");
            n += sprintf (dest + n, " %c%d", cas ? 'r' : 'R',
                          ((prog->mem[addr] >> 6) & 0x7));
          }
      }
      break;

    case OP_LD:
    case OP_LDI:
    case OP_LEA:
    case OP_ST:
    case OP_STI:
      {
        n += sprintf (dest + n, "%s", opnames[op][cas]);
        n += sprintf (dest + n, " %c%d,", cas ? 'r' : 'R',
                      ((prog->mem[addr] >> 9) & 0x7));

        if (prog->ref[addr])
          n += sprintf (dest + n, " %s", prog->ref[addr]->label);
        else // TODO sign extended int?
          n += sprintf (dest + n, " #%d", (prog->mem[addr] & 0x1FF));
      }
      break;

    case OP_LDR:
    case OP_STR:
      {
        n += sprintf (dest + n, "%s", opnames[op][cas]);
        n += sprintf (dest + n, " %c%d,", cas ? 'r' : 'R',
                      ((prog->mem[addr] >> 9) & 0x7));
        n += sprintf (dest + n, " %c%d,", cas ? 'r' : 'R',
                      ((prog->mem[addr] >> 6) & 0x7));

        int16_t offset6 = SIGN_EXTEND (prog->mem[addr] & 0x3F, 6);
        n += sprintf (dest + n, " #%d", offset6);
      }
      break;

    case OP_NOT:
      { // TODO check that the lower 6 bits are all 1s?
        n += sprintf (dest + n, "%s", opnames[op][cas]);
        n += sprintf (dest + n, " %c%d,", cas ? 'r' : 'R',
                      ((prog->mem[addr] >> 9) & 0x7));
        n += sprintf (dest + n, " %c%d", cas ? 'r' : 'R',
                      ((prog->mem[addr] >> 6) & 0x7));
      }
      break;

    case OP_RTI:
      {
        n += sprintf (dest + n, "%s", opnames[op][cas]);
      }
      break;

    case OP_TRAP:
      { // TODO string table for trap names
        uint16_t trapvect8 = prog->mem[addr] & 0xFF;
        switch (trapvect8)
          {
          case TRAP_GETC:
            n += sprintf (dest + n, "%s", cas ? "getc" : "GETC");
            break;
          case TRAP_OUT:
            n += sprintf (dest + n, "%s", cas ? "out" : "OUT");
            break;
          case TRAP_PUTS:
            n += sprintf (dest + n, "%s", cas ? "puts" : "PUTS");
            break;
          case TRAP_IN:
            n += sprintf (dest + n, "%s", cas ? "%s", "in" : "IN");
            break;
          case TRAP_PUTSP:
            n += sprintf (dest + n, "%s", cas ? "%s", "putsp" : "PUTSP");
            break;
          case TRAP_HALT:
            n += sprintf (dest + n, "%s", cas ? "%s", "halt" : "HALT");
            break;
          default:
            n += sprintf (dest + n, "%s", cas ? "%s", "trap " : "TRAP ");
            n += sprintf (dest + n, cas ? " x%x" : "x%X", trapvect8);
          }
      }
      break;

    default: // TODO be silent? do something else?
      fprintf (stderr, "i don't grok this op: %x\n", op);
    }

  return rc;
}
