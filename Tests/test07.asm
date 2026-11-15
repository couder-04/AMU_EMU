; Test Name: test07.asm - Invalid labels and operands
; This test checks more malformed labels, incorrect operands, broken comments, invalid DATA usage, and invalid SET values.
; Name: Parth Tomar | Roll No: 2501CS09
; Declaration: I, Parth Tomar, declare that this test program is my own work.
; Test additional malformed labels, operands, comments, DATA, and SET.
1badlabel:
ldc 5

start:
ldc

add 10

foo 5

ldc missing_label

// Wrong Comment format

dup:
ldc 1

dup:
ldc 2

data hello

SET 10

value: SET abc

halt