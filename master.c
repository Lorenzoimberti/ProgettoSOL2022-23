#include <masterworkers.h>

int dim_coda_default = 8; //dimensione di default coda
int dim_coda;
int num_workers_default = 4;

long ritardo_default = 0; //valore di default del tempo
long ritardo;

int visitaCartella(const char* nome){
	DIR* d;
	struct dirent* dir;
	if((d = opendir(nome))==NULL){
		printf("Errore nell'apertura di %s\n", nome);
		return -1;
	}
	errno = 0;
	while((dir = readdir(d)) != NULL && !termina){
		if(strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0){ //ignoro directory "." e ".."
            errno = 0;
            continue;
        }
        //creo il nuovo path
        int lunghezza_nome = strlen(nome); //lunghezza cartella passata come parametro
        int nuova_lunghezza = lunghezza_nome + strlen(dir->d_name) + 1; //aggiungo 1 per contare il '/' finale
        char* percorso;
        CHECKNULL(percorso, malloc((nuova_lunghezza + 1)*sizeof(char)), "Errore allocazione stringa percorso");
        snprintf(percorso, nuova_lunghezza+1, "%s/%s", nome, dir->d_name); //ottengo il nuovo percorso
        struct stat info;
        if(stat(percorso, &info) == -1){
        	perror("Errore nella stat");
        	return -1;
        }
        if(S_ISDIR(info.st_mode)){
        	int res;
        	if((res = visitaCartella(percorso))<0){
        		perror("Errore nella visita della cartella");
        	}
        	free(percorso);
        }
        else if(S_ISREG(info.st_mode)){
        	Lock(&mtx_coda);
			while(file_presenti==dim_coda){
				pthread_cond_wait(&coda_piena, &mtx_coda); //mi metto in attesa
			}
			if(!termina){ //posso inserire il file in coda
				insertLista(&testa, &coda, percorso);
				pthread_cond_broadcast(&coda_vuota);
				file_presenti++;
			}
			else{
				pthread_cond_broadcast(&coda_vuota);
				Unlock(&mtx_coda);
				break;
			}
			Unlock(&mtx_coda);
			usleep(ritardo*1000); //sospendo in millisecondi
        }
        else printf("File %s non regolare, passo al successivo\n", dir->d_name);
        errno = 0;
	}
	if(errno != 0){
        printf("Errore nell'ottenere file dalla cartella\n");
       	closedir(d);
       	return -1;
    }
    closedir(d);
    return 0;
}

void* master(void* args){
	arg_Master* argomenti = (arg_Master*) args;
	int num_workers;
	char* cartella = NULL;
	if(argomenti->dir != NULL){
		CHECKNULL(cartella, calloc((strlen(argomenti->dir)+1), sizeof(char)), "Errore calloc cartella");
		strncpy(cartella, argomenti->dir, strlen(argomenti->dir));
	}
	if(argomenti->num_workers < 0) num_workers = num_workers_default;
	else num_workers = argomenti->num_workers;
	if(argomenti->dim_coda < 0) dim_coda = dim_coda_default;
	else dim_coda = argomenti->dim_coda;
	if(argomenti->ritardo < 0) ritardo = ritardo_default;
	else ritardo = argomenti->ritardo;
	int i = argomenti->i;
	int argc = argomenti->argc;
	char** argv = argomenti->argv;
	int fd_collector = argomenti->fd_collector;
	pthread_t workers[num_workers];
	for(int i=0; i<num_workers; i++){
		pthread_create(&workers[i], NULL, thread_job, &fd_collector);
	}
	struct stat info;
	while(i<argc && !termina){
		if(stat(argv[i], &info)==-1){
			perror("Errore nel trovare informazioni del file corrente, passo al successivo");
			i++;
			continue;
		}
		if(S_ISREG(info.st_mode)){
			Lock(&mtx_coda);
			while(file_presenti==dim_coda){
				pthread_cond_wait(&coda_piena, &mtx_coda); //mi metto in attesa
			}
			if(!termina){ //posso inserire il file in coda
				insertLista(&testa, &coda, argv[i]);
				pthread_cond_broadcast(&coda_vuota);
				file_presenti++;
			}
			else{
				pthread_cond_broadcast(&coda_vuota);
				Unlock(&mtx_coda);
				break;
			}
			Unlock(&mtx_coda);
		}
		else printf("File %s non regolare, passo al successivo\n", argv[i]);
		i++;
		usleep(ritardo*1000); //sospendo in millisecondi
	}
	if(cartella != NULL && !termina){
		if(stat(cartella, &info)==-1){
			perror("Errore nel trovare informazioni della directory corrente");
		}
		else if(S_ISDIR(info.st_mode)){
			int risultato = visitaCartella(cartella);
			if(risultato<0){
				perror("Visita cartella non riuscita");
			}
		}
		else printf("%s non e' una cartella", cartella);
	}
	//printf("post malone handler\n");
	termina = TRUE;
	pthread_cond_broadcast(&coda_vuota); //avviso che termina e' diventato TRUE
	for(int i=0; i<num_workers; i++){ //join sui thread
		pthread_join(workers[i], NULL);
	}
	if(cartella != NULL){ //libero memoria allocata
		free(cartella);
	}
	return 0;
}
