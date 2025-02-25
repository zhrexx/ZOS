section .data
    global syscall_table
syscall_table:
    ; dd sys_mkdir
    ; dd sys_rmdir
    
    dd 0

section .text
    ; extern sys_mkdir, sys_rmdir


global syscall_dispatcher
syscall_dispatcher:
    cmp eax, 16
    ja .invalid_syscall
    
    push ebx
    push ecx
    push edx
    push esi
    push edi
    
    call [syscall_table + eax * 4]
    
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    
    ret
    
.invalid_syscall:
    mov eax, -1
    ret
