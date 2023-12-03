## lc3 (overall)
* consolidate shared code
  * move mask macros into lc3.h
    * replace bare masks with macros
* add'l assembler directives (.FILL?)

## lc3as (assembler)
* more/better test cases
* handle all cases where the operand can be either a label or a number (e.g., BR)

## lc3vm (virtual machine)
* implement it!
* finish adding docs from https://www.jmeiners.com/lc3-vm/supplies/lc3-isa.pdf as comments
* interactive mode?
