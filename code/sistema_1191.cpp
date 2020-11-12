void distruggi_processo(proc_elem* p)
{
	des_proc* pdes_proc = des_p(p->id);

	faddr tab4 = pdes_proc->cr3;
	riassegna_tutto(p->id, tab4, I_MIO_C, N_MIO_C);
	riassegna_tutto(p->id, tab4, I_UTN_C, N_UTN_C);
	rilascia_tutto(tab4, I_UTN_P, N_UTN_P);
	ultimo_terminato = tab4;
	if (p != esecuzione) {
		distruggi_pila_precedente();
	}
	rilascia_id_processo(p->id);
	dealloca(pdes_proc);
}