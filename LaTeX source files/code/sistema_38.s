// offset dei vari registri all'interno di des_proc
.set punt_nucleo, 0
.set CR3, 8
.set RAX,CR3+8
.set RCX,CR3+16
.set RDX,CR3+24
.set RBX,CR3+32
.set RSP,CR3+40
.set RBP,CR3+48
.set RSI,CR3+56
.set RDI,CR3+64
.set R8, CR3+72
.set R9, CR3+80
.set R10,CR3+88
.set R11,CR3+96
.set R12,CR3+104
.set R13,CR3+112
.set R14,CR3+120
.set R15,CR3+128


//////////////////////////////////
//		Cambio contesto			//
//////////////////////////////////

// copia lo stato dei registri generali nel des_proc del
// processo puntato da esecuzione.
// Nessun registro viene sporcato.
salva_stato:
	// salviamo lo stato di un paio di registri
	// in modo da poterli temporaneamente riutilizzare
	// In particolare, useremo %rax come registro di lavoro
	// e %rbx come puntatore al des_proc.
	.cfi_startproc
	.cfi_def_cfa_offset 8
	pushq %rbx
	.cfi_adjust_cfa_offset 8
	.cfi_offset rbx, -16
	pushq %rax
	.cfi_adjust_cfa_offset 8
	.cfi_offset rax, -24

	// ricaviamo il puntatore al des_proc
	movq esecuzione, %rax
	movq $0, %rbx
	movw (%rax), %bx	// Campo id del proc_elem
	leaq descrittori_processi, %rax 
	movq (%rax, %rbx, 8), %rbx

	// copiamo per primo il vecchio valore di %rax
	movq (%rsp), %rax
	movq %rax, RAX(%rbx)
	// usiamo %rax come appoggio per copiare il vecchio %rbx
	movq 8(%rsp), %rax
	movq %rax, RBX(%rbx)
	// copiamo gli altri registri
	movq %rcx, RCX(%rbx)
	movq %rdx, RDX(%rbx)
	// salviamo il valore che %rsp aveva prima della chiamata
	// a salva stato (valore corrente meno gli 8 byte che
	// contengono l'indirizzo di ritorno e i 16 byte dovuti
	// alle due push che abbiamo fatto all'inizio)
	movq %rsp, %rax
	addq $24, %rax
	movq %rax, RSP(%rbx)
	movq %rbp, RBP(%rbx)
	movq %rsi, RSI(%rbx)
	movq %rdi, RDI(%rbx)
	movq %r8,  R8 (%rbx)
	movq %r9,  R9 (%rbx)
	movq %r10, R10(%rbx)
	movq %r11, R11(%rbx)
	movq %r12, R12(%rbx)
	movq %r13, R13(%rbx)
	movq %r14, R14(%rbx)
	movq %r15, R15(%rbx)

	popq %rax
	.cfi_adjust_cfa_offset -8
	.cfi_restore rax
	popq %rbx
	.cfi_adjust_cfa_offset -8
	.cfi_restore rbx

	retq
	.cfi_endproc


// carica nei registri del processore lo stato contenuto nel des_proc del
// processo puntato da esecuzione.
// Questa funzione sporca tutti i registri.
carica_stato:
	.cfi_startproc
	.cfi_def_cfa_offset 8
	
	// Carichiamo nel TSS il puntatore alla base della pila sistema
	// del processo in esecuzione
	movq esecuzione, %rcx
	movq $0, %rax
	movw (%rcx), %ax 		// id processo
	leaq descrittori_processi, %rbx
	movq (%rbx, %rax, 8), %rbx	// des_proc
	movq punt_nucleo(%rbx), %rcx
	leaq tss, %rax
	movq %rcx, 4(%rax)

	popq %rcx   //ind di ritorno, va messo nella nuova pila
	.cfi_adjust_cfa_offset -8
	.cfi_register rip, rcx

	// nuovo valore per cr3
	movq CR3(%rbx), %r10
	movq %cr3, %rax
	cmpq %rax, %r10
	je 1f			// evitiamo di invalidare il TLB
				// se cr3 non cambia
	movq %r10, %rax
	movq %rax, %cr3		// il TLB viene invalidato
1:

	// anche se abbiamo cambiato cr3 siamo sicuri che
	// l'esecuzione prosegue da qui, perche' ci troviamo dentro
	// la finestra FM che e' comune a tutti i processi
	movq RSP(%rbx), %rsp  //cambiamo pila
	pushq %rcx  //rimettiamo l'indirizzo di ritorno
	.cfi_adjust_cfa_offset 8
	.cfi_offset rip, -8

	// se il processo precedente era terminato o abortito la sua pila sistema
	// non era stata distrutta, in modo da permettere a noi di continuare
	// ad usarla. Ora che abbiamo cambiato pila possiamo disfarci della
	// precedente.
	cmpq $0, ultimo_terminato
	je 1f
	call distruggi_pila_precedente
1:

	movq RCX(%rbx), %rcx
	movq RDI(%rbx), %rdi
	movq RSI(%rbx), %rsi
	movq RBP(%rbx), %rbp
	movq RDX(%rbx), %rdx
	movq RAX(%rbx), %rax
	movq R8(%rbx), %r8
	movq R9(%rbx), %r9
	movq R10(%rbx), %r10
	movq R11(%rbx), %r11
	movq R12(%rbx), %r12
	movq R13(%rbx), %r13
	movq R14(%rbx), %r14
	movq R15(%rbx), %r15
	movq RBX(%rbx), %rbx

	retq
	.cfi_endproc
