; Test Name: test02.asm - Assembler error handling
; This test intentionally contains malformed assembly, duplicate labels, invalid operands, missing arguments, and unknown mnemonics to verify assembler diagnostics.
; Name: Parth Tomar | Roll No: 2501CS09
; Declaration: I, Parth Tomar, declare that this test program is my own work.
; Test assembler diagnostics for malformed source.
; test2.asm
; Test error handling
label:
label: ; duplicate label definiton
br nonesuch ; no such label
ldc 08ge ; not a number
ldc ; missing operand
add 5 ; unexpected operand
ldc 5, 6; extra on end of line
0def: ; bogus label name
fibble; bogus mnemonic
0def ; bogus mnemonic
