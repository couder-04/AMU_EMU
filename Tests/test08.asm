; Test Name: test08.asm - Arithmetic with multiply and divide
; This test evaluates n*(n+1)/2 for n=6 and checks that multiply/divide instructions leave 21 in the accumulator.
; Name: Parth Tomar | Roll No: 2501CS09
; Declaration: I, Parth Tomar, declare that this test program is my own work.
; Evaluate n*(n+1)/2 for n=6; mul and div should leave 21 in A.
        ldc 6
        ldc 7
        mul
        ldc 2
        div
        halt
