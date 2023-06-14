#include <masterworkers.h>

#define STAMPA_COLLECTOR "stampa"

#define OPZIONI "n:q:d:t:"


Nodo* testa = NULL;
Nodo* coda = NULL;
int file_presenti = 0; //numero di file presenti nella coda

int termina = FALSE;

pthread_mutex_t mtx_coda = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t coda_vuota = PTHREAD_COND_INITIALIZER;
pthread_cond_t coda_piena = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mtx_socket = PTHREAD_MUTEX_INITIALIZER;

// Scrivo "n" bytes in un descrittore
ssize_t writen(int fd, void *ptr, size_t n){  
   size_t   nleft;
   ssize_t  nwritten;
   nleft = n;
   while (nleft > 0) {
     if((nwritten = write(fd, ptr, nleft)) < 0) {
        if (nleft == n) return -1; /* error, return -1 */
        else break; /* error, return amount written so far */
     }else if (nwritten == 0) break; 
     nleft -= nwritten;
     ptr   += nwritten;
   }
   return(n - nleft); /* return >= 0 */
}

void Lock(pthread_mutex_t *mutex){
	int err;
	if((err = pthread_mutex_lock(mutex)) != 0){
		printf("Errore lock\n");
		pthread_exit((void*)err);
	}
	//printf("Locked\n");
}

void Unlock(pthread_mutex_t *mutex){
	int err;
	if((err = pthread_mutex_unlock(mutex)) != 0){
		printf("Errore unlock\n");
		pthread_exit((void*)err);
	}
	//printf("Unlocked\n");
}

void* handler(void* args){
	handlerArgs *arg = (handlerArgs*) args;
	while(TRUE){
		int sig, nwritten;
		int r = sigwait(arg->set, &sig);
		if(r != 0){
			errno = r;
			perror("FATAL ERROR 'sigwait'");
			return NULL;
		}
		switch(sig){
			case SIGHUP:
			case SIGINT:
			case SIGQUIT:
			case SIGTERM: {
				//printf("sono nell'handler\n");
				termina = TRUE;
				pthread_cond_signal(&coda_piena); //notifico che termina e' TRUE
				return NULL;
			}
			case SIGUSR1: {
				Risultato_worker* stampa;
				CHECKNULL(stampa, malloc(sizeof(Risultato_worker)), "Errore malloc handler");
				strncpy(stampa->nome_file, STAMPA_COLLECTOR, strlen(STAMPA_COLLECTOR));
				stampa->risultato = -1; //metto -1 per indicare che devo procedere alla stampa
				Lock(&mtx_socket);
				if((nwritten = writen(arg->descrittore_collector, &stampa, sizeof(Risultato_worker)))==-1){
					perror("Errore comunicazione con collector");
					Unlock(&mtx_socket);
					exit(EXIT_FAILURE);
				}
				Unlock(&mtx_socket); 
				break; 
			}
			default:
				break;
		}
	}
	return NULL;
}

void unlink_socket(){
	unlink(SOCKNAME);
}


int main(int argc, char* const argv[]){
	if(argc == 1){
		printf("Utilizzare il programma inserendo il nome del file\n");
		return 0;
	}
	int num_workers = -1;
	int dim_coda = -1;
	long ritardo = -1;
	char* cartella = NULL;
	int opt;
	while((opt = getopt(argc, argv, OPZIONI))!=-1){
		switch(opt){
			case 'n':{
				num_workers = strtol(optarg, NULL, 10);
				break;
			}
			case 'q':{
				dim_coda = strtol(optarg, NULL, 10);
				break;
			}
			case 'd':{
				CHECKNULL(cartella, calloc((strlen(optarg)+1), sizeof(char)), "Errore calloc cartella");
				strncpy(cartella, optarg, strlen(optarg));
				break;
			}
			case 't':{
				ritardo = strtol(optarg, NULL, 10);
				break;
			}
			case '?':{
				printf("Operazione non riconosciuta\n");
				break;
			}
			default:
				break;
		}
	}
	unlink_socket();
	atexit(unlink_socket);
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);
	sigaddset(&mask, SIGHUP);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGUSR1);
	if(pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0){ //blocco dei segnali
		fprintf(stderr, "FATAL ERROR\n");
		exit(EXIT_FAILURE);
	}
	struct sigaction s;
	memset(&s, 0, sizeof(s));
	s.sa_handler = SIG_IGN;
	if((sigaction(SIGPIPE, &s, NULL)) == -1){
		perror("sigaction");
		exit(EXIT_FAILURE);
	}
	int fd_ascolto, fd_collector;
	int errore;
	struct sockaddr_un sa;
	strncpy(sa.sun_path, SOCKNAME, strlen(SOCKNAME)+1);
	sa.sun_family = AF_UNIX;
	SYSCALL_EXIT(fd_ascolto, socket(AF_UNIX, SOCK_STREAM, 0), "Creazione socket");
	SYSCALL_EXIT(errore, bind(fd_ascolto, (struct sockaddr*)&sa, sizeof(sa)), "bind");
	SYSCALL_EXIT(errore, listen(fd_ascolto, 1), "listen"); //solo il collector in coda
	int pid;
	if((pid = fork())<0){
		perror("Errore fork nella creazione del processo Collector");
		exit(EXIT_FAILURE);
	}
	if(pid==0){ //faccio partire il collector
		execl("./collector", "collector", NULL);
		perror("Errore execl");
		exit(EXIT_FAILURE);
	}
	fd_collector = accept(fd_ascolto, NULL, 0);
	handlerArgs *args;
	CHECKNULL(args, malloc(sizeof(handlerArgs)), "Errore malloc args\n");
	args->set = &mask;
	args->descrittore_collector = fd_collector;
	pthread_t sighandler;
	if(pthread_create(&sighandler, NULL, handler, args) != 0){
		printf("Errore pthread_create\n");
		return -1;
	}
	arg_Master* argomenti;
	CHECKNULL(argomenti, malloc(sizeof(arg_Master)), "Errore malloc argomenti");
	argomenti->num_workers = num_workers;
	argomenti->dim_coda = dim_coda;
	argomenti->ritardo = ritardo;
	argomenti->dir = cartella;
	argomenti->i = optind;
	argomenti->argc = argc;
	argomenti->argv = argv;
	argomenti->fd_collector = fd_collector;
	pthread_t thread_master;
	if(pthread_create(&thread_master, NULL, master, argomenti)!=0){
		perror("Errore creazione del master thread");
		return -1;
	}
	pthread_join(thread_master, NULL);
	Risultato_worker fine;
	memset(&fine, 0, sizeof(Risultato_worker));
	strncpy(fine.nome_file, "Fine", strlen("Fine"));
	fine.risultato = -1;
	Lock(&mtx_socket);
	if((errore = writen(fd_collector, &fine, sizeof(Risultato_worker)))==-1){
		perror("Errore comunicazione con il collector");
		Unlock(&mtx_socket);
		exit(EXIT_FAILURE);
	}
	Unlock(&mtx_socket);
	wait(NULL);
	pthread_kill(sighandler, SIGTERM);
	pthread_join(sighandler, NULL);
	if(cartella != NULL){
		free(cartella);
	}
	free(args);
	free(argomenti);
	return 0;
}