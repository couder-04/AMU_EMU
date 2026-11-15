; Test Name: test01.asm - Valid syntax and labels
; This test checks valid assembly syntax, labels, forward/backward references, data declarations, and a bounded infinite loop.
; Name: Parth Tomar | Roll No: 2501CS09
; Declaration: I, Parth Tomar, declare that this test program is my own work.
; Test valid syntax, labels, branches, data, and a bounded infinite loop.
; test1.asm

label:	; an unused label
	ldc 0
	ldc -5
	ldc +5
loop: br loop ; an infinite loop
br next	;offset should be zero
next:
    ldc loop ; load code address
	ldc var1 ; forward ref
var1: data 0 ; a variable
halt
