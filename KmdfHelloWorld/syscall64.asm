
EXTERN ContextTable:DQ

EXTERN KiArgumentTable:DQ
EXTERN NumSyscalls:DQ
EXTERN oldKiSystemCall64:DQ
EXTERN oldInterrupt:DQ
EXTERN	__imp_KeLowerIrql:PROC

; void* __cdecl memcpy(void* _Dst, void const* _Src, size_t _Size);
extrn memcpy:PROC

EXTERN hookSystemCallx64:PROC

USERMD_STACK_GS = 10h
KERNEL_STACK_GS = 1A8h

MAX_SYSCALL_INDEX = 1000h

.CODE


HookSyscallEntryPoint PROC
    ;cli                                    ; Disable interrupts    a оно тут надо?
    swapgs                                  ;  регистр GS указывает на структуру KPCR 
                                            ;(пользовательский уровень указывает на структуру TEB, которая используется 
                                            ;для идентификации информации о потоке, 
                                            ;а структура KPCR уровня ядра используется для идентификации информации о процессоре)

    mov         gs:[USERMD_STACK_GS], rsp   ; Сохраним указатель стека пользовательского слоя в элемент UserRsp KPCR

    cmp         rax, [NumSyscalls]      ; Is the index larger than the array size?
    jge         KiSystemCall64              ;
    
    jmp         SaveContextFun

HookSyscallEntryPoint ENDP

SaveContextFun PROC
     
    mov         rsp, gs:[KERNEL_STACK_GS]
    
    push rbp
    mov rbp, rsp
    
    push rbx
    push rcx
    mov rbx, rax 
    imul rbx, 104
    
    ;__________________________________
    ; save number and context of syscall
    
    mov         rcx, [ContextTable]
   
    mov qword ptr [rcx + rbx], rax
    mov qword ptr [rcx + rbx + 8], rax
    mov qword ptr [rcx + rbx + 8*2], r10
    mov qword ptr [rcx + rbx + 8*3], rdx
    mov qword ptr [rcx + rbx + 8*4], r8
    mov qword ptr [rcx + rbx + 8*5], r9
    

    push rsi
    push rdi
    push rdx
    push rax

    add rcx, rbx
    mov rbx, 48

    mov rsi, [KiArgumentTable]
    xor rdx, rdx
    mov dl, byte ptr [rsi + rax] ;num bytes from stack
    
    mov byte ptr [rcx+rbx], dl
    inc rbx

    mov rsi, gs:[USERMD_STACK_GS]
    add rsi, 8
   
   
   start_save:
    
        cmp rdx, 0
        je end_save

        mov al,  byte ptr [rsi]
        mov byte ptr [rcx+rbx], al
    
        inc rbx
        inc rsi
        dec rdx
        jmp start_save
    end_save:
    
    pop rax
    pop rdx
    pop rdi
    pop rsi

    pop rcx
    pop rbx

    mov rsp, rbp
    pop rbp
    ;__________________________________
    jmp         KiSystemCall64

SaveContextFun ENDP


KiSystemCall64 PROC
	mov         rsp, gs:[USERMD_STACK_GS]   ; Usermode RSP
	swapgs                                  ; Switch to usermode GS
	jmp         [oldKiSystemCall64]         ; Jump back to the old syscall handler
KiSystemCall64 ENDP




;Прерывание 0x4c
HookInterrupt PROC
    
    
    cmp rax, 0abcdh
    jne end_int_hook
    
    push rbp
    mov rbp, rsp

    PUSH RAX
	PUSH RCX
	PUSH RDX
    push FS
	push RBX
    
    MOV RBX, 30h
	MOV FS, RBX
	XOR  RCX, RCX
    
    CALL QWORD PTR __imp_KeLowerIrql
	
    cli
    
    ;lea rax, [hookSystemCallx64] ;система почему то полностью зависает при этом вызове
    ;MOV RBX, 30h
	;MOV FS, RBX
	;;XOR  RCX, RCX
    ;call rax
	
    ;work only for one processor
    
    lea rax, [HookSyscallEntryPoint]
	mov rdx, rax
	shr rdx, 32
	mov rcx, 0C0000082h
	wrmsr
    
    pop RBX
	pop FS
	
    POP RDX
	POP RCX
	POP RAX
	
    mov rsp, rbp
    pop rbp

	
    end_int_hook: 
    sti
    iretq
HookInterrupt ENDP

; 0x4d
UnHookInterrupt PROC
    
    
    cmp rax, 0abcdh
    jne end_int_uhook
    
    push rbp
    mov rbp, rsp

    PUSH RAX
	PUSH RCX
	PUSH RDX
    push FS
	push RBX
    
    MOV RBX, 30h
	MOV FS, RBX
	XOR  RCX, RCX
    
    ;CALL QWORD PTR __imp_KeLowerIrql
	
    cli
	
    ;work only for one processor
    
    mov rax, [oldKiSystemCall64]
	mov rdx, rax
	shr rdx, 32
	mov rcx, 0C0000082h
	wrmsr
    
    pop RBX
	pop FS
	
    POP RDX
	POP RCX
	POP RAX
	
    mov rsp, rbp
    pop rbp

	
    end_int_uhook: 
    sti
    iretq
UnHookInterrupt ENDP

;0x4f
GetSysNumInt PROC
    mov rax, [NumSyscalls]
    iretq
GetSysNumInt ENDP


;rax - size
;rdx - address
;int 0x4e
BufHookInterrupt PROC
    
   
    ;cmp rax, [NumSyscalls]
    ;jne end_int_bhook
    
    push rbp
    mov rbp, rsp

    PUSH RAX
	PUSH RCX
	PUSH RDX
    push FS
	push RBX
    push r8

    MOV RBX, 30h
	MOV FS, RBX
	XOR  RCX, RCX
    
    ;CALL QWORD PTR __imp_KeLowerIrql
	
    cli
    
    ;______
    ;Возвращаем информацию о системных вызовах в буфер адрес которого в RDX
    ;______
        
  ; rcx = * dst
  ; rdx = * src
  ; r8  = u64 size

  mov rcx, rdx ;dst
  mov rdx, [ContextTable] ;src
  imul rax, 104
  mov r8, rax   ;size
  lea rbx, [memcpy]
  call rbx   ; copy memory
    ;_____

    pop r8
    pop RBX
	pop FS
	
    POP RDX
	POP RCX
	POP RAX
	
    mov rsp, rbp
    pop rbp

	
    end_int_bhook: 
    sti
    iretq
BufHookInterrupt ENDP

END