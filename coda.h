#ifndef _CODA_H
#define _CODA_H 


//struttura per il nodo della lista
typedef struct nodo{
	void* elem; //elemento definito nel client
	struct nodo* next;
}Nodo;

//funzione che inserisce un argomento in coda alla lista, oppure in testa se vuota
void insertLista(Nodo** testa, Nodo** coda, void* arg);

//funzione che restituisce l'elemento in testa lista
void* popLista(Nodo** testa, Nodo** coda);

#endif
