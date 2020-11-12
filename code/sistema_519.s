//////////////////////////////////////////////////
// 	  Sezione TRAMPOLINE_TEXT: codice kernel    //
//////////////////////////////////////////////////
.section .trampoline_text, "ax"
.balign 4096
.global start_trampoline_text
start_trampoline_text:

// Parte in comune tra trampoline_in (chiamata dal modulo sistema)
// e a_trampoline_in (chiamata dal modulo I/O)
trampoline_in_2:
	.cfi_startproc
	.cfi_def_cfa_offset 8

	// Modifica la tab_entry della tabella di livello 4
	// relativa alla finestra di memoria fisica,
	// sostituendo il sottoalbero di traduzione shadow 
	// (usato per proteggere il sistema da Meltdown)
	// con quello kernel, cosi' da poter accedere alle funzionalita'
	// del nucleo

	movq des_finestra_kernel, %rbx
	movq %cr3, %rax
	movq %rbx, (%rax)

	retq
	.cfi_endproc

// Parte in comune tra trampoline_out (chiamata dal modulo sistema)
// e a_trampoline_out (chiamata dal modulo I/O)
trampoline_out_2:
	.cfi_startproc
	.cfi_def_cfa_offset 8

	// Modifica la tab_entry della tabella di livello 4
	// relativa alla finestra di memoria fisica,
	// sostituendo il sottoalbero di traduzione kernel
	// con quello shadow, per proteggere il sistema da Meltdown.
	// In questo modo, il programma utente non potra' accedere
	// in maniera speculativa alle zone di memoria del kernel

	movq des_finestra_shadow, %rbx
	movq %cr3, %rax
	movq %rbx, (%rax)
	
	// Invalidiamo il TLB per togliere le traduzioni della finestra di memoria fisica
	call 	invalida_TLB
	
	retq
	.cfi_endproc


trampoline_in:
	.cfi_startproc
	.cfi_def_cfa_offset 8

	pushq %rbx
	.cfi_adjust_cfa_offset 8
	.cfi_offset rbx, -16
	pushq %rax
	.cfi_adjust_cfa_offset 8
	.cfi_offset rax, -24

	// Controlliamo se stiamo entrando dentro il kernel dallo spazio shadow,
	// controllando a quale sottospazio appartiene il %RIP salvato dalla
	// chiamata dell'interruzione
	movq 24(%rsp), %rbx 	// %RIP salvato nella pila da INT
	movabs $(1 << 47), %rax
	addq %rbx, %rax
	jnc 1f 			// salta se non dobbiamo entrare nel trampolino

	call trampoline_in_2

1:	popq %rax
	.cfi_adjust_cfa_offset -8
	.cfi_restore rax
	popq %rbx
	.cfi_adjust_cfa_offset -8
	.cfi_restore rbx
	retq
	.cfi_endproc

// Effettua la fase di trampolino in uscita dal kernel, dopo
// aver controllato che il programma tornera' a livello utente
trampoline_out:
	.cfi_startproc
	.cfi_def_cfa_offset 8
	
	pushq %rbx
	.cfi_adjust_cfa_offset 8
	.cfi_offset rbx, -16
	pushq %rax
	.cfi_adjust_cfa_offset 8
	.cfi_offset rax, -24

	// Controlliamo se stiamo passando da sistema a utente
	movq 24(%rsp), %rbx		// %RIP salvato
	movabs $(1 << 47), %rax
	addq %rbx, %rax
	jnc 1f				// salta se non dobbiamo entrare nel trampolino

	// STIAMO USCENDO DAL KERNEL
	// E' necessario il trampolino in uscita
	call 	trampoline_out_2

1:	popq %rax
	.cfi_adjust_cfa_offset -8
	.cfi_restore rax
	popq %rbx
	.cfi_adjust_cfa_offset -8
	.cfi_restore rbx
	retq
	.cfi_endproc

// Routine int $TIPO_TRAMP_IN, accessibile solo dal modulo I/O
a_trampoline_in:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	pushq %rbx
	.cfi_adjust_cfa_offset 8
	.cfi_offset rbx, -16
	pushq %rax
	.cfi_adjust_cfa_offset 8
	.cfi_offset rax, -24
	
	call trampoline_in_2
	
	popq %rax
	.cfi_adjust_cfa_offset -8
	.cfi_restore rax
	popq %rbx
	.cfi_adjust_cfa_offset -8
	.cfi_restore rbx
	iretq
	.cfi_endproc

// Routine int $TIPO_TRAMP_OUT, accessibile solo dal modulo I/O
a_trampoline_out:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	pushq %rbx
	.cfi_adjust_cfa_offset 8
	.cfi_offset rbx, -16
	pushq %rax
	.cfi_adjust_cfa_offset 8
	.cfi_offset rax, -24
	
	call trampoline_out_2
	
	popq %rax
	.cfi_adjust_cfa_offset -8
	.cfi_restore rax
	popq %rbx
	.cfi_adjust_cfa_offset -8
	.cfi_restore rbx
	iretq
	.cfi_endproc
