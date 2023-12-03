## lc3 (overall)
* consolidate shared code
  * move mask macros into lc3.h
    * replace bare masks with macros
* automated `make distcheck` on merge

## lc3as (assembler)
* more/better test cases
* handle all cases where the operand can be either a label or a number (e.g., BR)

## lc3vm (virtual machine)
* get the fucker working!
* interactive mode?
* disallow stdin/stdout? (or find a better way of handling it)
