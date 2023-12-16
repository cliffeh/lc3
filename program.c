#include "program.h"
#include "machine.h"
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

// int
// disassemble_program (program *prog, FILE *symin, FILE *in)
// {
//   uint16_t memory[MEMORY_MAX];

//   // duplicates load_image, but we need to know both the origin and the
//   length uint16_t orig; size_t read = fread (&orig, sizeof (orig), 1, in);

//   if (read != 1)
//     {
//       fprintf (stderr, "origin could not be read\n");
//       return -1;
//     }

//   orig = SWAP16 (orig);

//   /* we know the maximum file size so we only need one fread */
//   uint16_t *p = memory + orig;
//   read = fread (p, sizeof (uint16_t), (MEMORY_MAX - orig), in);

//   if (read == 0)
//     {
//       fprintf (stderr, "no bytes read\n");
//       return -1;
//     }

//   prog->orig = orig;
//   prog->len = read;

//   instruction handle, *tail = &handle;

//   uint16_t addr = 0;
//   /* swap to little endian */
//   while ((read - addr) > 0)
//     {
//       tail->next = calloc (1, sizeof (instruction));
//       tail->next->addr = orig + addr++;
//       tail->next->word = SWAP16 (*p);
//       tail = tail->next;
//       tail->last = tail;
//       ++p;
//     }
//   prog->instructions = handle.next;

//   if (symin)
//     {
//       prog->symbols = load_symbols (symin);
//       attach_symbols (prog->instructions, prog->symbols);
//     }

//   return 0;
// }

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
              if (prog->symbols[saddr]
                  && strcmp (prog->ref[iaddr]->label,
                             prog->symbols[saddr]->label)
                         == 0)
                {
                  if ((prog->ref[iaddr]->flags >> 12) == HINT_FILL)
                    prog->memory[iaddr] = saddr;
                  else
                    prog->memory[iaddr]
                        |= ((saddr - iaddr - 1) & prog->ref[iaddr]->flags);
                  resolved = 1;
                  break;
                }
            }
          if (!resolved)
            {
              // if we get here, we've got an unresolved symbol
              fprintf (stderr, "error: unresolved symbol: %s\n",
                       prog->ref[iaddr]->label);
              return 1;
            }
        }
    }
  return 0;
}

// void
// free_symbols (symbol *symbols)
// {
//   // TODO figure out why this hangs!
//   symbol *sym = symbols;

//   while (sym)
//     {
//       if (sym->label)
//         free (sym->label);

//       symbol *tmp = sym->next;
//       free (sym);
//       sym = tmp;
//     }
// }

// int
// dump_symbols (FILE *out, symbol *symbols)
// {
//   for (symbol *sym = symbols; sym; sym = sym->next)
//     {
//       fprintf (out, "x%04x %s %d\n", sym->addr, sym->label, sym->hint);
//     }
//   return 0;
// }

// // TODO finish implementing!
// symbol *
// load_symbols (FILE *in)
// {
//   symbol handle, *tail = &handle;
//   char buf[1024], *p;

//   while (fgets (buf, 1024, in))
//     {
//       char *addr = strtok (buf, " \t\n");
//       char *label = strtok (0, " \t\n");
//       char *hint = strtok (0, " \t\n");
//       // TODO this could be a little more sophisticated...
//       if (*addr && *addr == 'x' && *label) // hint optional
//         {
//           tail->next = calloc (1, sizeof (symbol));
//           // TODO check return ptr
//           tail->next->addr = strtol (addr + 1, 0, 16);
//           tail->next->label = strdup (label);
//           if (hint)
//             tail->next->hint = atoi (hint);
//           tail->next->is_set = 1;
//           tail = tail->next;
//         }
//     }

//   return handle.next;
// }

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
                            prog->memory[addr]);

            return rc; // 0
          }
          break;

        case HINT_STRINGZ:
          {
            n += sprintf (dest + n, cas ? ".stringz \"" : ".STRINGZ \"");
            while (prog->memory[addr + rc] != 0
                   && (addr + rc) < (prog->orig + prog->len))
              {
                char c = (char)prog->memory[addr + rc++];
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

  switch (op = prog->memory[addr] >> 12)
    {
    case OP_ADD:
    case OP_AND:
      {
        n += sprintf (dest + n, "%s", opnames[op][cas]);
        n += sprintf (dest + n, " %c%d,", cas ? 'r' : 'R',
                      ((prog->memory[addr] >> 9) & 0x7));
        n += sprintf (dest + n, " %c%d,", cas ? 'r' : 'R',
                      ((prog->memory[addr] >> 6) & 0x7));

        if (prog->memory[addr] & (1 << 5))
          {
            int16_t imm5 = SIGN_EXTEND (prog->memory[addr] & 0x1F, 5);
            n += sprintf (dest + n, " #%d", imm5);
          }
        else
          n += sprintf (dest + n, " %c%d", cas ? 'r' : 'R',
                        ((prog->memory[addr] >> 0) & 0x7));
      }
      break;

    case OP_BR:
      {
        n += sprintf (dest + n, "%s", opnames[op][cas]);

        // nzp flags always lowercase
        n += sprintf (dest + n, "%s%s%s",
                      (prog->memory[addr] & (1 << 11)) ? "n" : "",
                      (prog->memory[addr] & (1 << 10)) ? "z" : "",
                      (prog->memory[addr] & (1 << 9)) ? "p" : "");

        if (prog->ref[addr])
          n += sprintf (dest + n, " %s", prog->ref[addr]->label);
        else // TODO sign extended int?
          n += sprintf (dest + n, " #%d", (prog->memory[addr] & 0x1FF));
      }
      break;

    case OP_JMP:
      {
        uint16_t BaseR = (prog->memory[addr] >> 6) & 0x7;
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
        if (prog->memory[addr] & (1 << 11))
          {
            n += sprintf (dest + n, "%s", opnames[op][cas]);

            if (prog->ref[addr])
              n += sprintf (dest + n, " %s", prog->ref[addr]->label);
            else // TODO sign extended int?
              n += sprintf (dest + n, " #%d", (prog->memory[addr] & 0x7FF));
          }
        else
          {
            n += sprintf (dest + n, "%s", cas ? "jsrr" : "JSRR");
            n += sprintf (dest + n, " %c%d", cas ? 'r' : 'R',
                          ((prog->memory[addr] >> 6) & 0x7));
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
                      ((prog->memory[addr] >> 9) & 0x7));

        if (prog->ref[addr])
          n += sprintf (dest + n, " %s", prog->ref[addr]->label);
        else // TODO sign extended int?
          n += sprintf (dest + n, " #%d", (prog->memory[addr] & 0x1FF));
      }
      break;

    case OP_LDR:
    case OP_STR:
      {
        n += sprintf (dest + n, "%s", opnames[op][cas]);
        n += sprintf (dest + n, " %c%d,", cas ? 'r' : 'R',
                      ((prog->memory[addr] >> 9) & 0x7));
        n += sprintf (dest + n, " %c%d,", cas ? 'r' : 'R',
                      ((prog->memory[addr] >> 6) & 0x7));

        int16_t offset6 = SIGN_EXTEND (prog->memory[addr] & 0x3F, 6);
        n += sprintf (dest + n, " #%d", offset6);
      }
      break;

    case OP_NOT:
      { // TODO check that the lower 6 bits are all 1s?
        n += sprintf (dest + n, "%s", opnames[op][cas]);
        n += sprintf (dest + n, " %c%d,", cas ? 'r' : 'R',
                      ((prog->memory[addr] >> 9) & 0x7));
        n += sprintf (dest + n, " %c%d", cas ? 'r' : 'R',
                      ((prog->memory[addr] >> 6) & 0x7));
      }
      break;

    case OP_RTI:
      {
        n += sprintf (dest + n, "%s", opnames[op][cas]);
      }
      break;

    case OP_TRAP:
      { // TODO string table for trap names
        uint16_t trapvect8 = prog->memory[addr] & 0xFF;
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
