// Task State Segment, richiesto dall'hardware.
// Per proteggere il sistema da Meltdown, e' unico per tutti 
// i processi ed e' mappato nella memoria shadow del kernel
struct __attribute__ ((packed)) task_state_segment  {
	// parte richiesta dall'hardware
	natl riservato1;
	vaddr punt_nucleo;
	// due quad  a disposizione (puntatori alle pile ring 1 e 2)
	natq disp1[2];
	natq riservato2;
	//entry della IST, non usata
	natq disp2[7];
	natq riservato3;
	natw riservato4;
	natw iomap_base; // si veda init_tss()
};

extern "C" task_state_segment tss;

// Descrittore software di processo
struct des_proc {
	vaddr punt_nucleo; 	// puntatore alla pila sistema del processo
	faddr cr3;
	natq contesto[N_REG]; 	// array per salvare il contesto
	natl cpl;
};

// Array dei descrittori di processo
des_proc* descrittori_processi[NUM_TSS];
// Posizione immediatamente successiva all'ultimo id assegnato
natl next_id = 0;

// Dato un id, restituisce il puntatore al corrispondente des_proc
extern "C" des_proc* des_p(natl id) 
{
	return descrittori_processi[id];
}

// Cerca un'entrata libera nell'array descrittori_processi da assegnare
// al processo il cui descrittore e' passato come parametro.
// Restituisce l'offset dell'entrata assegnata (l'id del nuovo processo) 
// oppure NUM_TSS se non ci sono piu' id liberi 
natl seleziona_id_processo (des_proc* d)
{
	natl i = next_id;

	do {
		if (descrittori_processi[i] == 0) {
			// Trovato descrittore di processo libero
			descrittori_processi[i] = d;
			next_id = (i + 1) % NUM_TSS;
			return i;
		}
		
		i = (i + 1) % NUM_TSS;
	} while (i != next_id);

	// Non ci sono piu' id liberi
	return NUM_TSS;
}

// Rende libero l'id passato come parametro, azzerando la relativa
// entrata nell'array descrittori processi
void rilascia_id_processo (natl id)
{
	descrittori_processi[id] = 0;
}
