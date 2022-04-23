proc_elem* crea_processo(void f(int), int a, int prio, char liv, bool IF)
{
	proc_elem*	p; // proc_elem per il nuovo processo
	natl identifier; // identificatore del processo
	des_proc*	pdes_proc; // descrittore di processo
	des_frame*	dpf_tab4; // tab4 del processo
	faddr pila_sistema;

	// ( allocazione (e azzeramento preventivo) di un des_proc
	pdes_proc = static_cast<des_proc*>(alloca(sizeof(des_proc)));
	if (pdes_proc == 0) goto errore1;
	memset(pdes_proc, 0, sizeof(des_proc));
	// )

	// ( selezione di un identificatore
	identifier = seleziona_id_processo(pdes_proc);
	if (identifier == NUM_TSS) goto errore2;
	// )

	// ( allocazione e inizializzazione di un proc_elem
	p = static_cast<proc_elem*>(alloca(sizeof(proc_elem)));
	if (p == 0) goto errore3;
	p->id = identifier;
	p->precedenza = prio;
	p->puntatore = 0;
	// )

	// ( creazione della tab4 del processo
	dpf_tab4 = alloca_frame(p->id, 4, 0);
	if (dpf_tab4 == 0) goto errore4;
	dpf_tab4->livello = 4;
	dpf_tab4->residente = true;
	dpf_tab4->processo = identifier;
	pdes_proc->cr3 = indirizzo_frame(dpf_tab4);
	crea_tab4(pdes_proc->cr3);
	mappa_pagina_shadow(pdes_proc->cr3, extr_IND_FISICO(des_finestra_shadow), false);
	// )

	// ( creazione della pila sistema .
	if (!crea_pila(p->id, fin_sis_p, DIM_SYS_STACK, LIV_SISTEMA))
		goto errore5;
	pila_sistema = carica_pila_sistema(p->id, fin_sis_p, DIM_SYS_STACK);
	if (pila_sistema == 0)
		goto errore6;
	// )

	if (liv == LIV_UTENTE) {
		// ( inizializziamo la pila sistema.
		natq* pl = reinterpret_cast<natq*>(pila_sistema);

		pl[-5] = reinterpret_cast<natq>(f); // RIP (codice utente)
		pl[-4] = SEL_CODICE_UTENTE; // CS (codice utente)
		pl[-3] = IF ? BIT_IF : 0; // RFLAGS
		pl[-2] = fin_utn_p - sizeof(natq); // RSP
		pl[-1] = SEL_DATI_UTENTE; // SS (pila utente)
		//   eseguendo una IRET da questa situazione, il processo
		//   passera' ad eseguire la prima istruzione della funzione f,
		//   usando come pila la pila utente (al suo indirizzo virtuale)
		// )

		// ( creazione della pila utente
		if (!crea_pila(p->id, fin_utn_p, DIM_USR_STACK, LIV_UTENTE)) {
			flog(LOG_WARN, "creazione pila utente fallita");
			goto errore6;
		}
		// )

		// ( infine, inizializziamo il descrittore di processo
		//   indirizzo del bottom della pila sistema, che verra' usato
		//   dal meccanismo delle interruzioni
		pdes_proc->punt_nucleo = fin_sis_p;

		//   inizialmente, il processo si trova a livello sistema, come
		//   se avesse eseguito una istruzione INT, con la pila sistema
		//   che contiene le 5 parole lunghe preparate precedentemente
		pdes_proc->contesto[I_RSP] = fin_sis_p - 5 * sizeof(natq);

		//   il registro RDI deve contenere il parametro da passare
		//   alla funzione f
		pdes_proc->contesto[I_RDI] = a;
		//pdes_proc->contesto[I_FPU_CR] = 0x037f;
		//pdes_proc->contesto[I_FPU_TR] = 0xffff;
		pdes_proc->cpl = LIV_UTENTE;

		//   tutti gli altri campi valgono 0
		// )
	} else {
		// ( inizializzazione della pila sistema
		natq* pl = reinterpret_cast<natq*>(pila_sistema);
		pl[-6] = reinterpret_cast<natq>(f);  // RIP (codice sistema)
		pl[-5] = SEL_CODICE_SISTEMA;  // CS (codice sistema)
		pl[-4] = IF ? BIT_IF : 0;  // RFLAGS
		pl[-3] = fin_sis_p - sizeof(natq);  // RSP
		pl[-2] = 0;  // SS
		pl[-1] = 0;  // ind. rit.
		             //(non significativo)
		//   i processi esterni lavorano esclusivamente a livello
		//   sistema. Per questo motivo, prepariamo una sola pila (la
		//   pila sistema)
		// )

		// ( inizializziamo il descrittore di processo
		pdes_proc->contesto[I_RSP] = fin_sis_p - 6 * sizeof(natq);
		pdes_proc->contesto[I_RDI] = a;

		//pdes_proc->contesto[I_FPU_CR] = 0x037f;
		//pdes_proc->contesto[I_FPU_TR] = 0xffff;
		pdes_proc->cpl = LIV_SISTEMA;

		//   tutti gli altri campi valgono 0
		// )
	}

	return p;

errore6:	rilascia_tutto(indirizzo_frame(dpf_tab4), I_SIS_P, N_SIS_P);
errore5:	rilascia_frame(dpf_tab4);
errore4:	dealloca(p);
errore3:	rilascia_id_processo(identifier);
errore2:	dealloca(pdes_proc);
errore1:	return 0;
}