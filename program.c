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

int
disassemble_program (program *prog, FILE *symin, FILE *in)
{
  uint16_t memory[MEMORY_MAX];

  // duplicates load_image, but we need to know both the origin and the length
  uint16_t orig;
  size_t read = fread (&orig, sizeof (orig), 1, in);

  if (read != 1)
    {
      fprintf (stderr, "origin could not be read\n");
      return -1;
    }

  orig = SWAP16 (orig);

  /* we know the maximum file size so we only need one fread */
  uint16_t *p = memory + orig;
  read = fread (p, sizeof (uint16_t), (MEMORY_MAX - orig), in);

  if (read == 0)
    {
      fprintf (stderr, "no bytes read\n");
      return -1;
    }

  prog->orig = orig;
  prog->len = read;

  instruction handle, *tail = &handle;

  uint16_t addr = 0;
  /* swap to little endian */
  while ((read - addr) > 0)
    {
      tail->next = calloc (1, sizeof (instruction));
      tail->next->addr = orig + addr++;
      tail->next->word = SWAP16 (*p);
      tail = tail->next;
      tail->last = tail;
      ++p;
    }
  prog->instructions = handle.next;

  if (symin)
    {
      prog->symbols = load_symbols (symin);
      attach_symbols (prog->instructions, prog->symbols);
    }

  return 0;
}

int
attach_symbols (instruction *instructions, symbol *symbols)
{
  for (instruction *inst = instructions; inst; inst = inst->next)
    {
      switch (inst->word >> 12)
        {
        case OP_BR:
        case OP_LD:
        case OP_LDI:
        case OP_LEA:
        case OP_ST:
        case OP_STI:
          {
            uint16_t PCoffset9 = inst->word & 0x1FF;
            for (symbol *sym = symbols; sym; sym = sym->next)
              {
                if (PCoffset9 == ((sym->addr - inst->addr) - 1))
                  inst->sym = sym;
              }
          }
          break;
        // handle separately
        case OP_JSR:
          {
            if (inst->word & (1 << 11))
              {
                uint16_t PCoffset11 = inst->word & 0x7FF;
                for (symbol *sym = symbols; sym; sym = sym->next)
                  {
                    if (PCoffset11 == ((inst->sym->addr - inst->addr) - 1))
                      inst->sym = sym;
                  }
              }
          }
          break;
        }
    }
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

int
dump_symbols (FILE *out, symbol *symbols)
{
  for (symbol *sym = symbols; sym; sym = sym->next)
    {
      fprintf (out, "x%04x %s %d\n", sym->addr, sym->label, sym->hint);
    }
  return 0;
}

// TODO finish implementing!
symbol *
load_symbols (FILE *in)
{
  symbol handle, *tail = &handle;
  char buf[1024], *p;

  while (fgets (buf, 1024, in))
    {
      char *addr = strtok (buf, " \t\n");
      char *label = strtok (0, " \t\n");
      char *hint = strtok (0, " \t\n");
      // TODO this could be a little more sophisticated...
      if (*addr && *addr == 'x' && *label) // hint optional
        {
          tail->next = calloc (1, sizeof (symbol));
          // TODO check return ptr
          tail->next->addr = strtol (addr + 1, 0, 16);
          tail->next->label = strdup (label);
          if (hint)
            tail->next->hint = atoi (hint);
          tail->next->is_set = 1;
          tail = tail->next;
        }
    }

  return handle.next;
}

const char *opnames[16][2] = {
  { "BR", "br" },   { "ADD", "add" }, { "LD", "ld" },   { "ST", "st" },
  { "JSR", "jsr" }, { "AND", "and" }, { "LDR", "ldr" }, { "STR", "str" },
  { "RTI", "rti" }, { "NOT", "not" }, { "LDI", "ldi" }, { "STI", "sti" },
  { "JMP", "jmp" }, { "RES", "res" }, { "LEA", "lea" }, { "TRAP", "trap" }
};

int
disassemble_word (char *dest, int flags, symbol *symbols, uint16_t addr,
                  uint16_t word)
{
  instruction inst;
  memset (&inst, 0, sizeof (instruction));
  inst.addr = addr;
  inst.word = word;
  return disassemble_instruction (dest, flags, symbols, &inst);
}

int
disassemble_instruction (char *dest, int flags, symbol *symbols,
                         instruction *inst)
{
  int n = 0, cas = (flags & FMT_LC) ? 1 : 0, op;

  switch (op = inst->word >> 12)
    {
    case OP_ADD:
    case OP_AND:
      {
        n += sprintf (dest + n, "%s", opnames[op][cas]);
        n += sprintf (dest + n, " %c%d,", cas ? 'r' : 'R',
                      ((inst->word >> 9) & 0x7));
        n += sprintf (dest + n, " %c%d,", cas ? 'r' : 'R',
                      ((inst->word >> 6) & 0x7));

        if (inst->word & (1 << 5))
          {
            int16_t imm5 = SIGN_EXTEND (inst->word & 0x1F, 5);
            n += sprintf (dest + n, " #%d", imm5);
          }
        else
          n += sprintf (dest + n, " %c%d", cas ? 'r' : 'R',
                        ((inst->word >> 0) & 0x7));
      }
      break;

    case OP_BR:
      {
        n += sprintf (dest + n, "%s", opnames[op][cas]);

        // nzp flags always lowercase
        n += sprintf (dest + n, "%s%s%s", (inst->word & (1 << 11)) ? "n" : "",
                      (inst->word & (1 << 10)) ? "z" : "",
                      (inst->word & (1 << 9)) ? "p" : "");

        if (inst->sym)
          n += sprintf (dest + n, " %s", inst->sym->label);
        else // TODO sign extended int?
          n += sprintf (dest + n, " #%d", (inst->word & 0x1FF));
      }
      break;

    case OP_JMP:
      {
        uint16_t BaseR = (inst->word >> 6) & 0x7;
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
        if (inst->word & (1 << 11))
          {
            n += sprintf (dest + n, "%s", opnames[op][cas]);

            if (inst->sym)
              n += sprintf (dest + n, " %s", inst->sym->label);
            else // TODO sign extended int?
              n += sprintf (dest + n, " #%d", (inst->word & 0x7FF));
          }
        else
          {
            n += sprintf (dest + n, "%s", cas ? "jsrr" : "JSRR");
            n += sprintf (dest + n, " %c%d", cas ? 'r' : 'R',
                          ((inst->word >> 6) & 0x7));
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
                      ((inst->word >> 9) & 0x7));

        if (inst->sym)
          n += sprintf (dest + n, " %s", inst->sym->label);
        else // TODO sign extended int?
          n += sprintf (dest + n, " #%d", (inst->word & 0x1FF));
      }
      break;

    case OP_LDR:
    case OP_STR:
      {
        n += sprintf (dest + n, "%s", opnames[op][cas]);
        n += sprintf (dest + n, " %c%d,", cas ? 'r' : 'R',
                      ((inst->word >> 9) & 0x7));
        n += sprintf (dest + n, " %c%d,", cas ? 'r' : 'R',
                      ((inst->word >> 6) & 0x7));

        int16_t offset6 = SIGN_EXTEND (inst->word & 0x3F, 6);
        n += sprintf (dest + n, " #%d", offset6);
      }
      break;

    case OP_NOT:
      { // TODO check that the lower 6 bits are all 1s?
        n += sprintf (dest + n, "%s", opnames[op][cas]);
        n += sprintf (dest + n, " %c%d,", cas ? 'r' : 'R',
                      ((inst->word >> 9) & 0x7));
        n += sprintf (dest + n, " %c%d", cas ? 'r' : 'R',
                      ((inst->word >> 6) & 0x7));
      }
      break;

    case OP_RTI:
      {
        n += sprintf (dest + n, "%s", opnames[op][cas]);
      }
      break;

    case OP_TRAP:
      {
        uint16_t trapvect8 = inst->word & 0xFF;
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
      fprintf (stderr, "i don't grok this op: %x\n", (inst->word >> 12));
    }

  return n;
}
