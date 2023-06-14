#include <masterworkers.h>

void inserimento_ordinato(Nodo** testa, Risultato_worker* dato){
	Nodo* new;
	CHECKNULL(new, malloc(sizeof(Nodo)), "Errore malloc nodo inserimento");
	new->elem = dato;
	new->next = NULL;
	Nodo* prev = NULL;
	Nodo* curr = *testa;
	while(curr != NULL && dato->risultato >= ((Risultato_worker*)((curr)->elem))->risultato){
		prev = curr;
		curr = curr->next;
	}
	if(prev == NULL){
		new->next = *testa;
		*testa = new;
	}
	else{
		prev->next = new;
		new->next = curr;
	}
}

Risultato_worker* pop(Nodo** testa){
	Nodo* temp = *testa;
	*testa = (*testa)->next;
	Risultato_worker *res = (Risultato_worker*)(temp->elem);
	free(temp);
	return res;
}

//stampa risultato e nome dei file presenti nella lista
void stampa_da_lista(Nodo* testa){
	if(testa==NULL){
		printf("Nessun file presente nella lista\n");
		return;
	}
	while(testa!=NULL){
		printf("%lu %s\n", ((Risultato_worker*)((testa)->elem))->risultato, ((Risultato_worker*)((testa)->elem))->nome_file);
		testa = testa->next;
	}
}

//elimina elementi della lista
void elimina_lista(Nodo** testa){
	while(testa!=NULL){
		Risultato_worker* tmp = pop(testa);
		//free(tmp->nome_file);
		free(tmp);
	}
}

// Leggo 'n' bytes da un descrittore
ssize_t readn(int fd, void *ptr, size_t n) {  
   size_t   nleft;
   ssize_t  nread;
   nleft = n;
   while (nleft > 0) {
     if((nread = read(fd, ptr, nleft)) < 0){
        if (nleft == n) return -1; /* error, return -1 */
        else break; /* error, return amount read so far */
     }else if (nread == 0) break; /* EOF */
     nleft -= nread;
     ptr   += nread;
   }
   return(n - nleft); /* return >= 0 */
}

int main(int argc, char const *argv[]){
	Nodo* testa = NULL;
	int err;
	int fd_socket;
	SYSCALL_EXIT(fd_socket, socket(AF_UNIX, SOCK_STREAM, 0), "Errore creazione socket");
	//Creo struct per accept
	struct sockaddr_un sa;
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, SOCKNAME, strlen(SOCKNAME));
	while(connect(fd_socket, (struct sockaddr*)&sa, sizeof(sa))==-1){
		if(errno == ENOENT){ //se la socket non esiste
			sleep(1);
		}
		else{
			perror("Errore nella connect del collector");
			exit(EXIT_FAILURE);
		}
	}
	while(TRUE){
		Risultato_worker* res;
		CHECKNULL(res, malloc(sizeof(Risultato_worker)), "Errore malloc res");
		SYSCALL_EXIT(err, readn(fd_socket, res, sizeof(Risultato_worker)),"Errore nella readn con socket");
		if(strncmp("stampa", res->nome_file, strlen("stampa"))==0){ //controllo se devo stampare
			stampa_da_lista(testa);
		}
		else if(strncmp("Fine", res->nome_file, strlen("Fine"))==0){ //fine
			break;
		}
		else{ //in questo caso e' un file
			inserimento_ordinato(&testa, res);
		}
	}
	stampa_da_lista(testa);
	elimina_lista(&testa);
	return 0;
}