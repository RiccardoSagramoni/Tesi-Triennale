// Confini sezioni trampolino
extern "C" natq start_trampoline_text;
extern "C" natq end_trampoline_text;
extern "C" natq start_trampoline_data;
extern "C" natq end_trampoline_data;
extern "C" natq start_trampoline_bss;
extern "C" natq end_trampoline_bss;

// tab_entry che descrive la finestra di memoria shadow.
// Da sostituire con la finestra di memoria normale
// quando il programma passa a livello utente
tab_entry des_finestra_shadow = 0;

// tab_entry che descrive la finestra di memoria kernel completa.
extern "C" tab_entry des_finestra_kernel;

void mappa_pagina_shadow (vaddr ind_virt, faddr tab3, bool isText);

// Restituisce un riferimento al descrittore di livello liv 
// da cui passa la traduzione dell'indirizzo ind_virt nella memoria shadow
tab_entry& get_des_shadow (int livello, vaddr ind_virt, faddr tab3) 
{
	faddr tab = tab3;

	for (int i = 3; i > livello; i--) {
		// Preleva il descrittore della tabella di livello i-esimo
		tab_entry entry = get_entry(tab, i_tab(ind_virt, i));
		
		if (!extr_P(entry))
			panic("P=0 non ammesso");

		tab = extr_IND_FISICO(entry);
	}

	return get_entry(tab, i_tab(ind_virt, livello));
}

// Se assente, crea una nuova tabella di traduzione di livello 1-3
// o mappa la traduzione di una pagina virtuale al livello 1 
// nello spazio di traduzione shadow
void crea_shadow (vaddr ind_virt, int liv, faddr tab3, bool isText)
{
	if (liv < 0 || liv > 3) {
		panic("crea_shadow(...): valore liv non valido");
	}
	
	tab_entry& dt = get_des_shadow(liv + 1, ind_virt, tab3);
	bool bit_P = extr_P(dt);

	if (!bit_P && liv != 0) {
		des_frame* df = alloca_frame_libero();
		
		if (df == 0) {
			flog(LOG_ERR, "Impossibile allocare copia shadow di una pagina");
			panic("errore");
		}

		// Inizializza descrittore di pagina fisica
		df->livello = liv;
		df->residente = true;
		df->processo = esecuzione->id;
		df->ind_virtuale = ind_virt;
		
		// Inizializza nuovo descrittore di tabella o pagina virtuale
		faddr new_entry = indirizzo_frame(df);
		memset(reinterpret_cast<void*>(new_entry), 0, DIM_PAGINA);

		// Collega il nuovo descrittore al precedente
		set_IND_FISICO(dt, new_entry);
		set_P(dt, true);
		dt |= BIT_RW;
	}
	else if (!bit_P) {
		// Mappa una pagina virtuale
		set_IND_FISICO(dt, ind_virt);
		set_P(dt, true);
		if (!isText) dt = dt | BIT_RW;
	}
}

void mappa_pagina_shadow (vaddr ind_virt, faddr tab3, bool isText)
{
	// Crea le tabelle di traduzione dal livello 2 in poi
	for (int i = 2; i >= 0; i--) {
		crea_shadow(ind_virt, i, tab3, isText);
	}
}

// Crea la tabella di livello 3 "shadow", ovvero la FM costituita
// dalle pagine kernel strettamente necessario ai processi utenti
// e le funzioni trampolino per passare nella FM completa (ed uscirvi)
faddr crea_tab3_shadow ()
{
	des_frame* df = alloca_frame_libero();
	if (df == 0) {
		flog(LOG_ERR, "Impossibile allocare copia shadow della finestra di memoria");
		panic("errore");
	}

	// Inizializza il descrittore di frame
	df->livello = 3;
	df->residente = true;
	df->processo = esecuzione->id;
	df->ind_virtuale = 0;

	// Inizializza la tabella
	faddr tab3 = indirizzo_frame(df);
	memset(reinterpret_cast<void*>(tab3), 0, DIM_PAGINA);

	return tab3;
}

// Mappa nella memoria virtuale shadow
// la sezione text, data e bss della finestra di memoria trampolino
// che sono allineate a 4KiB
void mappa_modulo_sistema_trampolino_in_shadow (faddr tabFM)
{	
	natq dim_trampoline_text = (natq)&end_trampoline_text - (natq)&start_trampoline_text;
	natq num_pag_text = (dim_trampoline_text / DIM_PAGINA) + 1;

	for (natq i = 0; i < num_pag_text; i++) {
		mappa_pagina_shadow((natq)&start_trampoline_text + i*DIM_PAGINA, tabFM, true);
	}		

	natq dim_trampoline_data = (natq)&end_trampoline_data - (natq)&start_trampoline_data;
	natq num_pag_data = (dim_trampoline_data / DIM_PAGINA) + 1;

	for (natq i = 0; i < num_pag_data; i++) {
		mappa_pagina_shadow((natq)&start_trampoline_data + i*DIM_PAGINA, tabFM, false);
	}

	natq dim_trampoline_bss = (natq)&end_trampoline_bss - (natq)&start_trampoline_bss;
	natq num_pag_bss = (dim_trampoline_bss / DIM_PAGINA) + 1;
	
	for (natq i = 0; i < num_pag_bss; i++) {
		mappa_pagina_shadow((natq)&start_trampoline_bss + i*DIM_PAGINA, tabFM, false);
	}
}

void crea_finestra_FM_shadow ()
{
	faddr tab3_shadow = crea_tab3_shadow();
	
	set_IND_FISICO(des_finestra_shadow, tab3_shadow);
	set_P(des_finestra_shadow, true);
	des_finestra_shadow |= BIT_RW;
	
	// Mappa la sezione text, data e bss della finestra di memoria trampolino
	mappa_modulo_sistema_trampolino_in_shadow(tab3_shadow);
}

// mappa la memoria fisica in memoria virtuale, inclusa l'area PCI
// (copiamo la finestra gia' creata dal boot loader)
bool crea_finestra_FM(faddr tab4)
{
	faddr boot_dir = readCR3();
	copy_des(boot_dir, tab4, I_SIS_C, N_SIS_C);
	
	crea_finestra_FM_shadow();

	// Salva la tab_entry relativa alla finestra di memoria in
	// modalita' kernel, nella memoria trampolino
	des_finestra_kernel = *(reinterpret_cast<tab_entry*>(tab4));

	return true;
}
