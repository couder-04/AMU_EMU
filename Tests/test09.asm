; Test Name: test09.asm - Division by zero runtime error
; This test deliberately divides by zero to confirm the emulator raises and reports a runtime error.
; Name: Parth Tomar | Roll No: 2501CS09
; Declaration: I, Parth Tomar, declare that this test program is my own work.
; Verify that the emulator reports a division-by-zero runtime error.
        ldc 1
        ldc 0
        div
        halt
