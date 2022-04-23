a_readse_n:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	cavallo_di_troia %rsi
	cavallo_di_troia2 %rsi %rdx
	cavallo_di_troia %rcx
	int $TIPO_TRAMP_IN
	call c_readse_n
	int $TIPO_TRAMP_OUT
	iretq
	.cfi_endproc