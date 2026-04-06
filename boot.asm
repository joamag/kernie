; boot.asm - boots from real mode into 64-bit long mode
; then jumps to kernel_main

[bits 16]
[org 0x7C00]

KERNEL_OFFSET equ 0x100000

boot:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; --- load kernel sectors from disk into 0x8000 temporarily ---
    ; we'll copy to 0x100000 after entering long mode
    mov bx, 0x8000          ; destination buffer
    mov al, 30              ; number of sectors to read (15KB should be plenty)
    mov ch, 0               ; cylinder 0
    mov cl, 2               ; start from sector 2 (sector 1 is boot sector)
    mov dh, 0               ; head 0
    mov dl, 0x80            ; first hard disk (use 0x00 for floppy)
    mov ah, 0x02            ; BIOS read sectors
    int 0x13
    jc disk_error

    ; --- enable A20 ---
    in al, 0x92
    or al, 2
    out 0x92, al

    ; --- set up page tables at 0x1000 ---
    mov edi, 0x1000
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
    rep stosd

    mov edi, 0x1000
    mov dword [edi],         0x2003   ; PML4[0] -> PDPT
    mov dword [edi + 0x1000], 0x3003  ; PDPT[0] -> PD
    ; identity map first 4MB (2 x 2MB huge pages)
    mov dword [edi + 0x2000], 0x0083  ; PD[0] -> 0x000000 (2MB, present+rw+huge)
    mov dword [edi + 0x2008], 0x200083; PD[1] -> 0x200000 (2MB)

    ; --- enable PAE ---
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; --- set long mode in EFER ---
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; --- enable paging + protected mode ---
    mov eax, cr0
    or eax, (1 << 31) | 1
    mov cr0, eax

    lgdt [gdt_ptr]
    jmp 0x08:long_mode_entry

disk_error:
    mov si, .msg
.print:
    lodsb
    or al, al
    jz .hang
    mov ah, 0x0E
    int 0x10
    jmp .print
.hang:
    cli
    hlt
.msg: db "Disk read error!", 0

; --- GDT ---
align 8
gdt:
    dq 0                          ; null
.code: equ $ - gdt
    dq 0x00AF9A000000FFFF         ; 64-bit code
.data: equ $ - gdt
    dq 0x00CF92000000FFFF         ; data
gdt_ptr:
    dw $ - gdt - 1
    dd gdt

; --- 64-bit entry ---
[bits 64]
long_mode_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov rsp, 0x90000

    ; copy kernel from 0x8000 to 0x100000
    mov rsi, 0x8000
    mov rdi, KERNEL_OFFSET
    mov rcx, 30 * 512 / 8         ; 30 sectors, copy as qwords
    rep movsq

    ; jump to kernel at 0x100000 (where linker puts it)
    mov rax, KERNEL_OFFSET
    call rax

    cli
    hlt

; --- pad to 512 bytes ---
times 510 - ($ - $$) db 0
dw 0xAA55
