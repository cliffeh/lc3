# lc3

A set of tools for [LC-3](https://en.wikipedia.org/wiki/Little_Computer_3) programs, consisting of (at present):

* an assembler/assembly source debugger (`lc3as`)
* a virtual machine (`lc3vm`)
* an object code differ (`lc3diff`)

## Usage Examples
```bash
# generate object code from assembly
./lc3as < test/2048.asm > 2048.obj

# look at a diff of two different object code binaries
./lc3diff test/2048.obj 2048.obj | less -R

# execute object code
./lc3vm 2048.obj
```

## Building

This project uses autotools, so building it follows the familiar pattern:

```bash
# only needed if you've cloned this directly from github
# ./autogen.sh

# configure and build the binaries
./configure
make

# run unit tests
make check

# I wouldn't necessarily recommend this
# make install
```

## [TODOs](TODO.md)

## References
* [Write your Own Virtual Machine](https://www.jmeiners.com/lc3-vm/) - original inspiration for this project
* https://www.jmeiners.com/lc3-vm/supplies/lc3-isa.pdf - Specs
* [LC3 Tools](https://highered.mheducation.com/sites/0072467509/student_view0/lc-3_simulator.html)
* [Little Computer 3](https://en.wikipedia.org/wiki/Little_Computer_3) - oblig Wikipedia article on LC-3
* [The LC-3](https://www.cs.utexas.edu/users/fussell/courses/cs310h/lectures/Lecture_10-310h.pdf) - University of Texas at Austin lecture on LC-3
* [Hello World example code](https://github.com/rpendleton/lc3sim-c/tree/main/tests/hello-world)
* [lc3-2048](https://github.com/rpendleton/lc3-2048)
* [lc3-rogue](https://github.com/justinmeiners/lc3-rogue)
* [bison manual](https://www.gnu.org/software/bison/manual/bison.html#Rules-Syntax)
* [Practical parsing with Flex and Bison](https://begriffs.com/posts/2021-11-28-practical-parsing.html#using-a-parser-as-a-library)
