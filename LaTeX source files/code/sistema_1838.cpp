// Azzerra tutte le entrate dell'array descrittori_processi,
// rendendo liberi tutti gli id disponibili
void init_descrittori_processi ()
{
	memset(descrittori_processi, 0, sizeof(des_proc*) * NUM_TSS);
}

void init_tss ()
{
	//   Il campo iomap_base contiene l'offset (nel TSS) dell'inizio 
	//   della "I/O bitmap". Questa bitmap contiene un bit per ogni
	//   possibile indirizzo di I/O. Le istruzioni in e out eseguite
	//   da livello utente verranno permesse se il bit corrispondente
	//   all'indirizzo di I/O a cui si riferiscono vale 1.
	//   Per disattivare questo meccanismo dobbiamo inizializzare
	//   il campo iomap_base con un offset maggiore o uguale
	//   della dimensione del segmento TSS (come scritta nel
	//   descrittore di segmento TSS nella GDT, vedere 'set_entry_tss'
	//   in sistema.S)
	tss.iomap_base = DIM_DESP;

	// Gli altri campi del TSS sono nulli
}

extern "C" void salta_a_main();
extern "C" void cmain()
{
	natl mid;

	// (* anche se il primo processo non e' completamente inizializzato,
	//    gli diamo un identificatore, in modo che compaia nei log
	init.id = 0xFFFFFFFF;
	init.precedenza = MAX_PRIORITY;
	esecuzione = &init;
	// *)

	flog(LOG_INFO, "Nucleo di Calcolatori Elettronici, v5.12.6 con patch contro Meltdown");
	init_tss();
	flog(LOG_INFO, "tss inizializzato");
	init_gdt();
	flog(LOG_INFO, "gdt inizializzata");

	// (* Assegna allo heap di sistema HEAP_SIZE byte nel secondo MiB
	heap_init((addr)HEAP_START, HEAP_SIZE);
	flog(LOG_INFO, "Heap di sistema: %x B @%x", HEAP_SIZE, HEAP_START);
	// *)

	// ( il resto della memoria e' per i frame (parte M2)
	init_des_frame();
	flog(LOG_INFO, "Pagine fisiche: %d", N_DF);
	// )

	flog(LOG_INFO, "sis/cond [%p, %p)", ini_sis_c, fin_sis_c);
	flog(LOG_INFO, "sis/priv [%p, %p)", ini_sis_p, fin_sis_p);
	flog(LOG_INFO, "io /cond [%p, %p)", ini_mio_c, fin_mio_c);
	flog(LOG_INFO, "usr/cond [%p, %p)", ini_utn_c, fin_utn_c);
	flog(LOG_INFO, "usr/priv [%p, %p)", ini_utn_p, fin_utn_p);

	faddr inittab4 = crea_tab4();

	if(!crea_finestra_FM(inittab4))
			goto error;
	loadCR3(inittab4);
	flog(LOG_INFO, "Caricato CR3");

	apic_init(); // in libce
	apic_reset(); // in libce
	apic_fill();
	flog(LOG_INFO, "APIC inizializzato");

	// ( inizializzazione dello swap, che comprende la lettura
	//   degli entry point di start_io e start_utente
	if (!swap_init())
			goto error;
	flog(LOG_INFO, "sb: blocks = %d", swap_dev.sb.blocks);
	flog(LOG_INFO, "sb: user   = %p/%p",
			swap_dev.sb.user_entry,
			swap_dev.sb.user_end);
	flog(LOG_INFO, "sb: io     = %p/%p",
			swap_dev.sb.io_entry,
			swap_dev.sb.io_end);
	// )
	
	// ( inizializza l'array che contiene i puntatori ai descrittori di processo
	init_descrittori_processi();
	// )

	// ( creazione del processo main_sistema
	mid = crea_main_sistema();
	if (mid == 0xFFFFFFFF)
		goto error;
	flog(LOG_INFO, "Creato il processo main_sistema (id = %d)", mid);
	// )

	// ( creazione del processo dummy
	dummy_proc = crea_dummy();
	if (dummy_proc == 0xFFFFFFFF)
		goto error;
	flog(LOG_INFO, "Creato il processo dummy (id = %d)", dummy_proc);
	// )

	// (* selezioniamo main_sistema
	schedulatore();
	// *)
	// ( esegue CALL carica_stato; IRETQ (vedi "sistema.S")
	salta_a_main();
	// )

error:
	c_panic("Errore di inizializzazione");
}