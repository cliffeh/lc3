## lc3 (overall)
* better documentation
  * doxygen?

## lc3as (assembler)
* better string literal handling
* for everything that can use a label, also allow it to use a NUMLIT

## lc3vm (virtual machine)
* support windows?
* interactive mode improvements
  * maintain command history
  * implement unimplemented commands
  * command aliases/shortcuts
  * smarter tab completion
  * make ^C drop the user back into the shell
* better prompt?
  * include currently-loaded program?
  * include indicator of last return code?
* better instrumented
  * dump regs/memory
  * maintain symbol table
  * step-wise execution

## lc3diff
* make it moar prettier?
* options
  * turn on/off coloring
* include prinable chars as a column?
