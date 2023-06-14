#include <coda.h>
#include <utils.h>
#include <stdlib.h>

//inserisco in coda
void insertLista(Nodo** testa, Nodo** coda, void* arg){
	Nodo* nuovo;
	CHECKNULL(nuovo, malloc(sizeof(Nodo)), "Errore malloc nodo"); //controllo risultato malloc
	nuovo->elem = arg;
	nuovo->next = NULL;
	if(*testa==NULL){
		*testa = nuovo;
		*coda = *testa;
	}
	else{
		(*coda)->next = nuovo;
		*coda = nuovo;
	}
}

//tolgo dalla testa
void* popLista(Nodo** testa, Nodo** coda){
	Nodo* temp = *testa;
    *testa = (*testa)->next;
    if(testa == NULL){    //FORSE PUNTATORI
    	coda = NULL;
    }
    void* ret = (temp)->elem;
    free(temp);
    return ret;
}



