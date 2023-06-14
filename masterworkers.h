#ifndef _MASTERWORKERS_H
#define _MASTERWORKERS_H

#include <coda.h>
#include <utils.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>

#define WRITEN(r,c,s) if((r=c)==-1){perror(s); exit(EXIT_FAILURE);}
#define WRITEN_AND_UNLOCK(r,c,s,m) if((r=c)==-1){perror(s); Unlock(&m); exit(EXIT_FAILURE);}

//nome socket
#define SOCKNAME "./farm_sock" 

//dichiaro testa, coda e dimensione
extern Nodo* testa;
extern Nodo* coda;
extern int dim_coda;
extern int file_presenti;

//pipe per comunicazione tra processi
//extern int my_pipe[2];

//dichiaro mutex e variabili di condizione
extern pthread_mutex_t mtx_coda;
extern pthread_cond_t coda_vuota;
extern pthread_cond_t coda_piena;

extern pthread_mutex_t mtx_socket;

//variabile per concludere l'esecuzione
extern int termina;

//struttura per il risultato elaborato dal worker
typedef struct {
	char nome_file[MAX_PATH_LEN];
	long risultato;
}Risultato_worker;

//struttura per descrivere i parametri del thread handler
typedef struct{
	sigset_t *set;
	int descrittore_collector;
}handlerArgs;

typedef struct{
	int num_workers;
	int dim_coda;
	long ritardo;
	char* dir;
	int i;
	int argc;
	int fd_collector;
	char** argv;
}arg_Master;

//funzione dei thread workers
void* thread_job(void* arg);

//funzione handler dei segnali
void* handler(void* args);

//funzione visita ricorsiva cartella
int visitaCartella(const char* nome);

void* master(void* args);

ssize_t writen(int fd, void *ptr, size_t n);

void Lock(pthread_mutex_t *mutex);

void Unlock(pthread_mutex_t *mutex);

#endif
