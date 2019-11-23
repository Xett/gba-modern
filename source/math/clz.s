@--------------------------------------------------------------------------------
@ clz.s
@--------------------------------------------------------------------------------
@ Provides an implementation of the count-leading-zeros routine utilized
@ in the std::sort algorithm
@--------------------------------------------------------------------------------

@ Source code taken from http://bit.ly/2rmLz0w
@ r0: the number we want to clz
@ returns the number of leading zeros 
    .section .iwram, "ax", %progbits
    .align 2
    .arm
    .global __clzsi2
    .type __clzsi2 STT_FUNC
__clzsi2:
    @ Our initial guess (why 24? Well, it's better to check the comment in that same page)
    movs r1, #24

    @ Skip if no zeros in high 16 bits
    lsrs r2, r0, #16
    beq .skip16
    subs r1, r1, #16
    mov r0, r2

.skip16:
    lsrs r2, r0, #8
    beq .skip8
    subs r1, r1, #8
    mov r0, r2

.skip8:
    adr r2, .clztable
    ldrb r0, [r2, r0]
    add r0, r0, r1
    bx lr

    @ Generate the lookup table
    .macro reptb byte,count
    .rept \count
    .byte \byte
    .endr
    .endm

    .pool
.clztable:
    reptb 8,1
    reptb 7,1
    reptb 6,2
    reptb 5,4
    reptb 4,8
    reptb 3,16
    reptb 2,32
    reptb 1,64
    reptb 0,128
