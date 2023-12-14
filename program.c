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
resolve_symbols (program *prog)
{
  for (instruction *inst = prog->instructions; inst; inst = inst->next)
    {
      if (inst->sym)
        {
          if (!inst->sym->is_set)
            {
              fprintf (stderr, "error: unresolved symbol: %s\n",
                       inst->sym->label);
              return 1;
            }

          if (inst->flags)
            inst->word |= (((inst->sym->addr - inst->addr) - 1) & inst->flags);
          else
            inst->word = inst->sym->addr;
        }
    }

  return 0;
}

symbol *
find_symbol_by_addr (symbol *symbols, uint16_t addr)
{
  for (symbol *sym = symbols; sym; sym = sym->next)
    {
      if (sym->addr == addr)
        return sym;
    }
  return 0;
}

symbol *
find_symbol_by_label (symbol *symbols, const char *label)
{
  for (symbol *sym = symbols; sym; sym = sym->next)
    {
      if (strcmp (label, sym->label) == 0)
        return sym;
    }
  return 0;
}

symbol *
find_or_create_symbol (program *prog, const char *label)
{
  symbol *sym = find_symbol_by_label (prog->symbols, label);

  if (!sym)
    {
      sym = calloc (1, sizeof (symbol));
      sym->label = strdup (label);
      sym->next = prog->symbols;
      prog->symbols = sym;
    }

  return sym;
}

void
free_instructions (instruction *instructions)
{
  instruction *inst = instructions;

  while (inst)
    {
      instruction *tmp = inst->next;
      free (inst);
      inst = tmp;
    }
}

void
free_symbols (symbol *symbols)
{
  // TODO figure out why this hangs!
  symbol *sym = symbols;

  while (sym)
    {
      if (sym->label)
        free (sym->label);

      symbol *tmp = sym->next;
      free (sym);
      sym = tmp;
    }
}

#define PPRINT(dest, flags, fmt, lc, UC, ...)                                 \
  sprintf (dest, fmt,                                                         \
           (flags & FORMAT_LC) ? lc : UC __VA_OPT__ (, ) __VA_ARGS__)

int
disassemble_word (char *dest, int flags, symbol *symbols, uint16_t addr,
                  uint16_t word)
{
  instruction inst;
  memset (&inst, 1, sizeof (instruction));
  inst.addr = addr;
  inst.word = word;
  return disassemble_instruction (dest, flags, symbols, &inst);
}

int
disassemble_instruction (char *dest, int flags, symbol *symbols,
                         instruction *inst)
{
  int n = 0;

  switch (inst->word >> 12)
    {
    case OP_ADD:
      {
        n += PPRINT (dest + n, flags, "%s ", "add", "ADD");
        n += PPRINT (dest + n, flags, "%c%d, ", 'r', 'R',
                     ((inst->word >> 9) & 0x7));
        n += PPRINT (dest + n, flags, "%c%d, ", 'r', 'R',
                     ((inst->word >> 6) & 0x7));

        if (inst->word & (1 << 5))
          {
            int16_t imm5 = SIGN_EXTEND (inst->word & 0x1F, 5);
            n += sprintf (dest + n, "#%d", imm5);
          }
        else
          n += PPRINT (dest + n, flags, "%c%d", 'r', 'R',
                       ((inst->word >> 0) & 0x7));
      }
      break;

    case OP_AND:
      {
        n += PPRINT (dest + n, flags, "%s ", "and", "AND");
        n += PPRINT (dest + n, flags, "%c%d, ", 'r', 'R',
                     ((inst->word >> 9) & 0x7));
        n += PPRINT (dest + n, flags, "%c%d, ", 'r', 'R',
                     ((inst->word >> 6) & 0x7));

        if (inst->word & (1 << 5))
          {
            int16_t imm5 = SIGN_EXTEND (inst->word & 0x1F, 5);
            n += sprintf (dest + n, "#%d", imm5);
          }
        else
          n += PPRINT (dest + n, flags, "%c%d", 'r', 'R',
                       ((inst->word >> 0) & 0x7));
      }
      break;

    case OP_BR:
      {
        n += PPRINT (dest + n, flags, "%s", "br", "BR");

        // nzp flags always lowercase
        n += sprintf (dest + n, "%s%s%s ", (inst->word & (1 << 11)) ? "n" : "",
                      (inst->word & (1 << 10)) ? "z" : "",
                      (inst->word & (1 << 9)) ? "p" : "");

        int16_t PCoffset9 = SIGN_EXTEND (inst->word & 0x1FF, 9);
        symbol *sym
            = find_symbol_by_addr (symbols, PCoffset9 + inst->addr + 1);
        if (sym)
          n += sprintf (dest + n, "%s", sym->label);
        else // TODO sign extended int?
          n += sprintf (dest + n, "#%d", PCoffset9);
      }
      break;

    case OP_JMP:
      {
        uint16_t BaseR = (inst->word >> 6) & 0x7;
        if (BaseR == 7) // assume RET special case
          n += PPRINT (dest + n, flags, "%s", "ret", "RET");
        else
          {
            n += PPRINT (dest + n, flags, "%s ", "jmp", "JMP");
            n += PPRINT (dest + n, flags, "%c%d", 'r', 'R', BaseR);
          }
      }
      break;

    case OP_JSR:
      {
        if (inst->word & (1 << 11))
          {
            n += PPRINT (dest + n, flags, "%s ", "jsr", "JSR");

            int16_t PCoffset11 = SIGN_EXTEND (inst->word & 0x7FF, 11);
            symbol *sym
                = find_symbol_by_addr (symbols, PCoffset11 + inst->addr + 1);

            if (sym)
              n += sprintf (dest + n, "%s", sym->label);
            else // TODO sign extended int?
              n += sprintf (dest + n, "#%d", PCoffset11);
          }
        else
          {
            n += PPRINT (dest + n, flags, "%s ", "jsrr", "JSRR");
            n += PPRINT (dest + n, flags, "%c%d", 'r', 'R',
                         ((inst->word >> 6) & 0x7));
          }
      }
      break;

    case OP_LD:
      {
        n += PPRINT (dest + n, flags, "%s ", "ld", "LD");
        n += PPRINT (dest + n, flags, "%c%d, ", 'r', 'R',
                     ((inst->word >> 9) & 0x7));

        int16_t PCoffset9 = SIGN_EXTEND (inst->word & 0x1FF, 9);
        symbol *sym
            = find_symbol_by_addr (symbols, PCoffset9 + inst->addr + 1);
        if (sym)
          n += sprintf (dest + n, "%s", sym->label);
        else // TODO sign extended int?
          n += sprintf (dest + n, "#%d", PCoffset9);
      }
      break;

    case OP_LDI:
      {
        n += PPRINT (dest + n, flags, "%s ", "ldi", "LDI");
        n += PPRINT (dest + n, flags, "%c%d, ", 'r', 'R',
                     ((inst->word >> 9) & 0x7));

        int16_t PCoffset9 = SIGN_EXTEND (inst->word & 0x1FF, 9);
        symbol *sym
            = find_symbol_by_addr (symbols, PCoffset9 + inst->addr + 1);
        if (sym)
          n += sprintf (dest + n, "%s", sym->label);
        else // TODO sign extended int?
          n += sprintf (dest + n, "#%d", PCoffset9);
      }
      break;

    case OP_LDR:
      {
        n += PPRINT (dest + n, flags, "%s ", "ldr", "LDR");
        n += PPRINT (dest + n, flags, "%c%d, ", 'r', 'R',
                     ((inst->word >> 9) & 0x7));
        n += PPRINT (dest + n, flags, "%c%d, ", 'r', 'R',
                     ((inst->word >> 6) & 0x7));

        int16_t offset6 = SIGN_EXTEND (inst->word & 0x3F, 6);
        n += sprintf (dest + n, "#%d", offset6);
      }
      break;

    case OP_LEA:
      {
        n += PPRINT (dest + n, flags, "%s ", "lea", "LEA");
        n += PPRINT (dest + n, flags, "%c%d, ", 'r', 'R',
                     ((inst->word >> 9) & 0x7));

        int16_t PCoffset9 = SIGN_EXTEND (inst->word & 0x1FF, 9);
        symbol *sym
            = find_symbol_by_addr (symbols, PCoffset9 + inst->addr + 1);
        if (sym)
          n += sprintf (dest + n, "%s", sym->label);
        else // TODO sign extended int?
          n += sprintf (dest + n, "#%d", PCoffset9);
      }
      break;

    case OP_NOT:
      { // TODO check that the lower 6 bits are all 1s?
        n += PPRINT (dest + n, flags, "%s ", "not", "NOT");
        n += PPRINT (dest + n, flags, "%c%d, ", 'r', 'R',
                     ((inst->word >> 9) & 0x7));
        n += PPRINT (dest + n, flags, "%c%d", 'r', 'R',
                     ((inst->word >> 6) & 0x7));
      }
      break;

    case OP_RTI:
      {
        n += PPRINT (dest + n, flags, "%s", "rti", "RTI");
      }
      break;

    case OP_ST:
      {
        n += PPRINT (dest + n, flags, "%s ", "st", "ST");
        n += PPRINT (dest + n, flags, "%c%d, ", 'r', 'R',
                     ((inst->word >> 9) & 0x7));

        int16_t PCoffset9 = SIGN_EXTEND (inst->word & 0x1FF, 9);
        symbol *sym
            = find_symbol_by_addr (symbols, PCoffset9 + inst->addr + 1);
        if (sym)
          n += sprintf (dest + n, "%s", sym->label);
        else // TODO sign extended int?
          n += sprintf (dest + n, "#%d", PCoffset9);
      }
      break;

    case OP_STI:
      {
        n += PPRINT (dest + n, flags, "%s ", "sti", "STI");
        n += PPRINT (dest + n, flags, "%c%d, ", 'r', 'R',
                     ((inst->word >> 9) & 0x7));

        int16_t PCoffset9 = SIGN_EXTEND (inst->word & 0x1FF, 9);
        symbol *sym
            = find_symbol_by_addr (symbols, PCoffset9 + inst->addr + 1);
        if (sym)
          n += sprintf (dest + n, "%s", sym->label);
        else // TODO sign extended int?
          n += sprintf (dest + n, "#%d", PCoffset9);
      }
      break;

    case OP_STR:
      {
        n += PPRINT (dest + n, flags, "%s ", "str", "STR");
        n += PPRINT (dest + n, flags, "%c%d, ", 'r', 'R',
                     ((inst->word >> 9) & 0x7));
        n += PPRINT (dest + n, flags, "%c%d, ", 'r', 'R',
                     ((inst->word >> 6) & 0x7));

        int16_t offset6 = SIGN_EXTEND (inst->word & 0x3F, 6);
        n += sprintf (dest + n, "#%d", offset6);
      }
      break;

    case OP_TRAP:
      {
        uint16_t trapvect8 = inst->word & 0xFF;
        switch (trapvect8)
          {
          case TRAP_GETC:
            n += PPRINT (dest + n, flags, "%s", "getc", "GETC");
            break;
          case TRAP_OUT:
            n += PPRINT (dest + n, flags, "%s", "out", "OUT");
            break;
          case TRAP_PUTS:
            n += PPRINT (dest + n, flags, "%s", "puts", "PUTS");
            break;
          case TRAP_IN:
            n += PPRINT (dest + n, flags, "%s", "in", "IN");
            break;
          case TRAP_PUTSP:
            n += PPRINT (dest + n, flags, "%s", "putsp", "PUTSP");
            break;
          case TRAP_HALT:
            n += PPRINT (dest + n, flags, "%s", "halt", "HALT");
            break;
          default:
            n += PPRINT (dest + n, flags, "%s", "trap ", "TRAP ");
            if (flags & FORMAT_LC)
              n += sprintf (dest + n, "x%x", trapvect8);
            else
              n += sprintf (dest + n, "x%X", trapvect8);
          }
      }
      break;

    default: // TODO be silent? do something else?
      fprintf (stderr, "i don't grok this op: %x\n", (inst->word >> 12));
    }

  return n;
}
