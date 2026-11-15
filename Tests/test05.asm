; Test Name: test05.asm - Sum 1 to 10 with stack locals
; This test sums the integers from 1 to 10 using stack-local variables and a loop.
; Name: Parth Tomar | Roll No: 2501CS09
; Declaration: I, Parth Tomar, declare that this test program is my own work.
; Sum the integers 1 through 10 using stack-local variables.
        ldc 0x1000
        a2sp
        adj -2
        ldc 10
        stl 0
        ldc 0
        stl 1
loop:   ldl 0
        brz done
        ldl 1
        ldl 0
        add
        stl 1
        ldl 0
        adc -1
        stl 0
        br loop
done:   ldl 1
        halt