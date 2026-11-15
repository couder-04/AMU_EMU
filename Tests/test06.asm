; Test Name: test06.asm - Unused labels and warnings
; This test includes unused labels and a backward branch to check warning diagnostics without invalid execution.
; Name: Parth Tomar | Roll No: 2501CS09
; Declaration: I, Parth Tomar, declare that this test program is my own work.
; Test unused-label and backward-branch warnings.
unused1:
unused2:

ldc 5
ldc 10
add

loop:
br loop

afterLoop:
ldc 1
ldc 2
sub

neverUsedLabel:
ldc 3
halt