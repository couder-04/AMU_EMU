; Test Name: test03.asm - SET label values and arithmetic
; This test uses SET directives to assign immediate values to labels and executes a short arithmetic sequence to confirm label resolution and data handling.
; Name: Parth Tomar | Roll No: 2501CS09
; Declaration: I, Parth Tomar, declare that this test program is my own work.
; Test SET label values and execute a short arithmetic sequence.
; test3.asm
; Test SET
val: SET 75
ldc     val
adc     val2
val2: SET 66
halt
