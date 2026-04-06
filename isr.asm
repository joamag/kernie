; isr.asm - interrupt service routine stubs
; saves registers, calls C handler, restores, iretq

[bits 64]
[extern isr_handler]

; macro for exceptions that DON'T push an error code
%macro ISR_NOERR 1
[global isr%1]
isr%1:
    push 0              ; dummy error code
    push %1             ; interrupt number
    jmp isr_common
%endmacro

; macro for exceptions that DO push an error code
%macro ISR_ERR 1
[global isr%1]
isr%1:
    push %1             ; interrupt number (error code already on stack)
    jmp isr_common
%endmacro

; CPU exceptions 0-31
ISR_NOERR 0   ; divide by zero
ISR_NOERR 1   ; debug
ISR_NOERR 2   ; NMI
ISR_NOERR 3   ; breakpoint
ISR_NOERR 4   ; overflow
ISR_NOERR 5   ; bound range exceeded
ISR_NOERR 6   ; invalid opcode
ISR_NOERR 7   ; device not available
ISR_ERR   8   ; double fault
ISR_NOERR 9   ; coprocessor segment overrun
ISR_ERR   10  ; invalid TSS
ISR_ERR   11  ; segment not present
ISR_ERR   12  ; stack-segment fault
ISR_ERR   13  ; general protection fault
ISR_ERR   14  ; page fault
ISR_NOERR 15  ; reserved
ISR_NOERR 16  ; x87 floating-point
ISR_ERR   17  ; alignment check
ISR_NOERR 18  ; machine check
ISR_NOERR 19  ; SIMD floating-point
ISR_NOERR 20  ; virtualization
ISR_ERR   21  ; control protection
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

; IRQs 0-15 (mapped to interrupts 32-47)
ISR_NOERR 32  ; timer (IRQ 0)
ISR_NOERR 33  ; keyboard (IRQ 1)
ISR_NOERR 34
ISR_NOERR 35
ISR_NOERR 36
ISR_NOERR 37
ISR_NOERR 38
ISR_NOERR 39
ISR_NOERR 40
ISR_NOERR 41
ISR_NOERR 42
ISR_NOERR 43
ISR_NOERR 44
ISR_NOERR 45
ISR_NOERR 46
ISR_NOERR 47

; common handler: save all regs, call C, restore, iretq
isr_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp        ; pass pointer to saved regs as arg1
    call isr_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16         ; pop interrupt number + error code
    iretq
