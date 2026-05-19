.section .text.boot
.globl _start
.type _start, @function

_start:
    /*
     * OpenSBI transfers control to the supervisor kernel with:
     *   a0 = hart ID
     *   a1 = device tree blob physical address
     */

.option push
.option norelax
    la gp, __global_pointer$
.option pop

    la sp, boot_stack_top
    la t0, __bss_start
    la t1, __bss_end

.Lclear_bss:
    bgeu t0, t1, .Lenter_kernel
    sd zero, 0(t0)
    addi t0, t0, 8
    j .Lclear_bss

.Lenter_kernel:
    call kmain

.Lhang:
    wfi
    j .Lhang

.size _start, . - _start

.section .bss.stack, "aw", @nobits
.align 12
boot_stack:
    .space 16384
boot_stack_top:
