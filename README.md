# Binary-Translator
The course work of Operation System

## Related Documents
*[MIPS-Simulator Maunual](Reference/mips-simulator.pdf)

## Requirment
### mips-linux-gnu-gcc cross compiler
### mips-instruction-simulator

## Code Structure & Design

### register utilization ###
$0 always zero
$1 $at assembly reserved register
$2-3 $v0-1 syscall id & return value
$4 $a0 syscall argument
$5 void
$6-13 add & sub register (%eax)
$14-17 base register (%ebx)
$18-21 counter register (%ecx)
$22-25 modula register (%edx)
$26-27 reserve for future use
$28 manual stack point (use for push & pop)
$29 stack point (%esp)
$30 frame point (%ebp)
$31 return address register
## Development Log
