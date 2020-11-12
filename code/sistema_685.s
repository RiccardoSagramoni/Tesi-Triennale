a_activate_p:	// routine int $tipo_a
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call trampoline_in
	call salva_stato
	cavallo_di_troia %rdi
	call c_activate_p
	call carica_stato
	call trampoline_out
	iretq
	.cfi_endproc