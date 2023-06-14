#include <masterworkers.h>

int termina;

void* thread_job(void* arg){
	int fd_collector = *((int*)arg);
	int lavora = TRUE; //variabile di supporto per la terminazione
	while(lavora){
		Lock(&mtx_coda);
		while(testa == NULL && !termina){
			pthread_cond_wait(&coda_vuota, &mtx_coda);
		}
		if(termina && testa == NULL){
			lavora = FALSE;
			Unlock(&mtx_coda);
			break;
		}
		char* path = (char*)(popLista(&testa, &coda)); //ho preso file dalla lista
		file_presenti--;
		pthread_cond_signal(&coda_piena);
		Unlock(&mtx_coda);
		struct stat info;
		if(stat(path, &info) == -1){
        	printf("Errore nel recuperare informazioni del file, passo al successivo\n");
        	free(path);
        	continue;
      }
      int count = (info.st_size)/8;
      long file[count];
      FILE* f;
      if((f=fopen(path, "rb"))==NULL){
      	printf("Errore apertura del file %s, passo al successivo\n", path);
      	free(path);
      	continue;
      }
      if(count != (fread(file, sizeof(long), count, f))){
      	printf("Errore lettura del file %s, passo al successivo\n", path);
      	free(path);
      	fclose(f);
      	continue;
      }
      long result = 0;
      for(int i=0; i<count; i++){
      	result += i*file[i]; //formula data dal testo
      }
      Risultato_worker res; //definisco il risultato
      memset(&res, 0, sizeof(Risultato_worker));
      strncpy(res.nome_file, path, strlen(path));
      res.risultato = result;
      int nwritten;
      Lock(&mtx_socket);
      WRITEN_AND_UNLOCK(nwritten, writen(fd_collector, &res, sizeof(Risultato_worker)), "Errore writen", mtx_socket);
      Unlock(&mtx_socket);
      fclose(f);
      if(strchr(path, '/')!=NULL){
        free(path);
      }
   }
   return NULL;
}
