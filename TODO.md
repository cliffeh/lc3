## lc3 (overall)
* consolidate shared code
  * move mask macros into lc3.h
    * replace bare masks with macros (or something)

## lc3as (assembler)
* more/better test cases
* push more of the instruction building logic into the parser
  * pretty much everything but labels should be doable in the parser
  * ...but it will mean changing the way pretty-printing works
* enable debug output *after* symbol resolution?

## lc3vm (virtual machine)
* get the fucker working!
* interactive mode?
* disallow stdin/stdout? (or find a better way of handling it)
