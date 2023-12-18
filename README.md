# lc3

A set of tools for [LC-3](https://en.wikipedia.org/wiki/Little_Computer_3) programs, consisting of (at present):

* an assembler/assembly source debugger (`lc3as`)
* a virtual machine (`lc3vm`)
* an object code differ (`lc3diff`)

## Examples
```bash
# dump debug information about the code the assembler would generate
./lc3as -Fdebug < test/2048.asm

# generate object code from assembly
./lc3as < test/2048.asm > 2048.obj

# look at a diff of two different object code binaries
./lc3diff test/2048.obj 2048.obj | less -R

# execute object code
./lc3vm 2048.obj
```

## Building

You'll need `flex` and `bison` to build this project. (No, `lex` and `yacc` won't do.) Building it follows the familiar autotools pattern:

```bash
# only needed if you've cloned this directly
# ./autogen.sh

# configure and build the binaries
./configure
make

# run unit tests
make check
```

## Usage

### lc3as

```
Usage: lc3as [FILE]

If FILE is not provided this program will read from stdin.

Options:
  -D, --disassemble       disassemble object code to assembly (implies -Fp)
  -F, --format=FORMAT     output format (default: "object")
  -S, --symbols=FILE      also read/write symbols to/from FILE
  -o, --output=FILE       write output to FILE (default: "-")
      --version           show version information and exit

Help options:
  -?, --help              Show this help message
      --usage             Display brief usage message

Supported output formats:

  o[bject]   output assembled object code (default)
  a[ddress]  print the address of each instruction
  b[its]     print a binary representation
  h[ex]      print a hexadecimal representation
  p[retty]   pretty-print the assembly code itself
  l[ower]    print everything in lowercase
  u[pper]    print everything in uppercase (default)
  d[ebug]    shorthand for -Fa -Fh -Fp

Report bugs to <cliff.snyder@gmail.com>.
```

### lc3vm

```
Usage: lc3vm [FILE...]

Options:
  -i, --interactive     run in interactive mode
      --version         show version information and exit

Help options:
  -?, --help            Show this help message
      --usage           Display brief usage message

Report bugs to <cliff.snyder@gmail.com>.
```

### lc3vm (interactive mode):
```
Command             Arguments   Description
asm , a             file        assemble and load one or more assembly files
load, l             file        load one or more object files
run , r                         run the currently-loaded program
help, h, ?                      display this help message
exit, quit, q, x                exit the program
```

### lc3diff

```
Usage: lc3diff FILE1 FILE2

Either FILE1 or FILE2 may be specified as - for stdin.

Options:
  -c, --colorize            colorize output
  -m, --marker[=STRING]     set diff marker to STRING (default: "*")
  -o, --output=FILE         write output to FILE (default: "-")
  -q, --quiet               only output differences
  -s, --summary             output a summary of the differences at the end
      --version             show version information and exit

Help options:
  -?, --help                Show this help message
      --usage               Display brief usage message

Report bugs to <cliff.snyder@gmail.com>.
```

## [TODOs](TODO.md)
At time of writing, the `lc3as` assembler generates LC-3 object code that is executable using both `lc3vm` as well as the reference simulator found [here](https://highered.mheducation.com/sites/0072467509/student_view0/lc-3_simulator.html). `lc3vm` appears to be working correctly (on Linux) - I've played through several games of [2048](https://github.com/rpendleton/lc3-2048) - and features an interactive mode for assembling, loading, and running programs. I don't think there's much left to do in the assembler, but I'd like to continue to flesh out the interactive mode of the virtual machine. I _think_ getting it to run on Windows should be a relatively straightforward matter of swapping around some of the platform-specific bits w/rt terminal I/O using the code [here](https://www.jmeiners.com/lc3-vm/src/lc3-win.c) as a guide, but I haven't gotten around to it just yet.

There is also a binary diff tool `lc3diff` that I wrote mostly because I couldn't figure out what flags I wanted/needed in any of the available hex dump tools (hexdump/od/xxd). It works pretty okay - it could be "smarter", but it's functional.

...and, of course, there could always be more tests. I have a handful of tests I've built up as I've implemented this, but they're incomplete, don't check edge cases/failure modes, etc.

## References
* [Write your Own Virtual Machine](https://www.jmeiners.com/lc3-vm/) - The original inspiration for this project
* [LC-3 ISA specification](https://www.jmeiners.com/lc3-vm/supplies/lc3-isa.pdf)
* [LC3 Tools](https://highered.mheducation.com/sites/0072467509/student_view0/lc-3_simulator.html)
* [Little Computer 3](https://en.wikipedia.org/wiki/Little_Computer_3) - oblig Wikipedia article on LC-3
* [Hello World](https://github.com/rpendleton/lc3sim-c/tree/main/tests/hello-world)
* [lc3-2048](https://github.com/rpendleton/lc3-2048)
* [lc3-rogue](https://github.com/justinmeiners/lc3-rogue)
* [bison manual](https://www.gnu.org/software/bison/manual/bison.html#Rules-Syntax)
* [Practical parsing with Flex and Bison](https://begriffs.com/posts/2021-11-28-practical-parsing.html#using-a-parser-as-a-library)
* [Thread-safe / reentrant bison + flex](https://stackoverflow.com/questions/48850242/thread-safe-reentrant-bison-flex)
* https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
* http://www.lihaoyi.com/post/BuildyourownCommandLinewithANSIescapecodes.html
