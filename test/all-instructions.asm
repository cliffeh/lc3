.ORIG x3000
label1
  ADD R0, R1, R2
label2
  ADD R2, R3, #7
  BR label1
  BRn label2
  BRz label1
  BRzp label2
  BRnp label1
  BRnz label2
  BRnzp label1
  JMP R4
label3
  JSR label3
  JSRR R5
.END
