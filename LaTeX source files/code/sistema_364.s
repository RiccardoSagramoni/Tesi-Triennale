.set p_dpl_type, 0b10001001 //p=1,dpl=00,type=1001=tss ready
.set pres_bit,   0b10000000

	.global init_gdt
init_gdt:
	lgdt gdt_pointer

	// Inizilizza descrittore di TSS
	leaq des_tss, %rdx
	movw $DIM_DESP, (%rdx) 	//[15:0] = limit[15:0]
	decw (%rdx)
	leaq tss, %rax
	movw %ax, 2(%rdx)	//[31:16] = base[15:0]
	shrq $16, %rax
	movb %al, 4(%rdx)	//[39:32] = base[24:16]
	movb $p_dpl_type, 5(%rdx)	//[47:40] = p_dpl_type
	movb $0, 6(%rdx)	//[55:48] = 0
	movb %ah, 7(%rdx)	//[63:56] = base[31:24]
	shrq $16, %rax
	movl %eax, 8(%rdx) 	//[95:64] = base[63:32]
	movl $0, 12(%rdx)	//[127:96] = 0

	// carichiamo TR con l'offset dell'unico descrittore di tss
	// (in modo che il meccanismo delle interruzioni usi la
	// pila sistema del processo in esecuzione)
	leaq des_tss, %rax
	andb $0b11111101, 5(%rax)	// reset del bit BUSY
					// (richiesto dal processore
					// per compatibilita' con il modo
					// a 32 bit)
	movq $(des_tss - gdt), %rax

	ltr %ax

	retq