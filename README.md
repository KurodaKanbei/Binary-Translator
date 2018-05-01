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

$6-9 add & sub register (%eax) 

$10-13 base register (%ebx) 

$14-17 counter register (%ecx) 

$18-21 modula register (%edx) 

$22-23 (%esi) 

$24-25 (%edi) 

$28 manual stack point (use for push & pop) 

$29 stack point (%esp) 

$30 frame point (%ebp) 

$31 return address register 

## Development Log
