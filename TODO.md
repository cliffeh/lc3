## lc3 (overall)
* consolidate shared code
* is a basic disassembler possible?
* jettison autotools? (more trouble than it's worth)

## lc3as (assembler)
* more/better test cases
* better string literal handling
* move the parser code into a function
* for everything that can use a label, also allow it to use a NUMLIT
* get rid of exit() calls in functions
  * let the caller decide how to handle errors

## lc3vm (virtual machine)
* interactive mode?
* move execution into its own function
* support windows?
* encapsulate setup/teardown stuff elsewhere

## lc3diff
* make it moar prettier?
* options
  * turn on/off coloring
* include prinable chars as a column?
