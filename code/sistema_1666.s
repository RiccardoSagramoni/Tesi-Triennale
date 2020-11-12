.global end_trampoline_text
end_trampoline_text:

////////////////////////////////////////////////////
// 		Sezione TRAMPOLINE_DATA: tabelle e dati			//
////////////////////////////////////////////////////
.section .trampoline_data, "aw"	
.balign 4096
.global start_trampoline_data
start_trampoline_data:

// puntatori alle tabelle GDT e IDT
// nel formato richiesto dalle istruzioni LGDT e LIDT
gdt_pointer:
	.word end_gdt-gdt		// limite della GDT
	.quad gdt				// base della GDT
idt_pointer:
	.word 0xFFF			// limite della IDT (256 entrate)
	.quad idt			// base della IDT
triple_fault_idt:
	.word 0
	.quad 0
param_err:
	.asciz "indirizzo non valido: %p"

.balign 8
.global gdt
gdt:
	.quad 0		//segmento nullo
code_sys_seg:
	.word 0b0           //limit[15:0]   not used
	.word 0b0           //base[15:0]    not used
	.byte 0b0           //base[23:16]   not used
	.byte 0b10011010    //P|DPL|1|1|C|R|A|  DPL=00=sistema
	.byte 0b00100000    //G|D|L|-|-------|  L=1 long mode
	.byte 0b0           //base[31:24]   not used
code_usr_seg:
	.word 0b0           //limit[15:0]   not used
	.word 0b0           //base[15:0]    not used
	.byte 0b0           //base[23:16]   not used
	.byte 0b11111010    //P|DPL|1|1|C|R|A|  DPL=11=utente
	.byte 0b00100000    //G|D|L|-|-------|  L=1 long mode
	.byte 0b0           //base[31:24]   not used
data_usr_seg:
	.word 0b0           //limit[15:0]   not used
	.word 0b0           //base[15:0]    not used
	.byte 0b0           //base[23:16]   not used
	.byte 0b11110010    //P|DPL|1|0|E|W|A|  DPL=11=utente
	.byte 0b00000000    //G|D|-|-|-------|
	.byte 0b0           //base[31:24]   not used
des_tss:
	.space 16,0	//segmento tss, riempito a runtime
end_gdt:

.global tss
tss:
	.fill DIM_DESP, 0

.global des_finestra_kernel
des_finestra_kernel:
	.quad 0

.global end_trampoline_data
end_trampoline_data:


.section .trampoline_bss, "aw", @nobits
.balign 4096
.global start_trampoline_bss
start_trampoline_bss:

// .balign 16
idt:
	// spazio per 256 gate
	// verra' riempita a tempo di esecuzione
	.space 16 * 256, 0
end_idt:

exc_error:
	.space 8, 0

.global end_trampoline_bss
end_trampoline_bss: