#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define TOTAL 5
#define TRUE 1
#define TAM 10         //define o tamanho do resfriador


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;				//inicializa o mutex
pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;			       //inicializa a variável de condição

struct nodo {
    int newsockfd;
};									     //cria um socket específico para cada cliente
struct nodo nodo[5];
//declaração de variáveis
double quant = 0, num;
char buffer[256], str[256];

char *comando;


void *cliente(void *arg) {
    long cid = (long)arg;
    int i, n,x;
    double c = (TAM - quant);
    printf("O reservatorio tem %lf L de bebida, faltando %lf L para estar completo\n", quant, c);
    while (TRUE) {
        bzero(buffer,sizeof(buffer));					   //zera o valor do buffer	
        n = read(nodo[cid].newsockfd,buffer,50);			  //lê o valor digitado pelo cliente
        if(strcmp(buffer, "status")==10){                              //verifica se o usuario digitou o comando correto
        	double c = (TAM - quant);
        	printf("O reservatorio tem %lf L de bebida, faltando %lf L para estar completo\n", quant, c);
        }	
	num = strtod(buffer, &comando);      				 //buffer = comando + num ---> num deve receber a parte numerica
	strcpy(str,comando);						 //passa os caracteres para uma string			
        if (n <= 0) {						 	 //se n<=0 causa erro na conexao
            printf("Erro lendo do socket id %lu!\n", cid);
            close(nodo[cid].newsockfd);
            nodo[cid].newsockfd = -1;

            pthread_exit(NULL);
        }
        // MUTEX LOCK - GERAL
        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&cond);					//libera as threads em espera
       
        for (i = 0;i < TOTAL; i++) {
            if ((i != cid) && (nodo[i].newsockfd != -1)) {
                n = write(nodo[i].newsockfd,buffer,50);
                if (n <= 0) {							//se n<=0 causa erro
                    printf("Erro escrevendo no socket id %i!\n", i);
//                    close(nodo[i].newsockfd);
//                    nodo[i].newsockfd = -1;
                }
            }
        }
        pthread_mutex_unlock(&mutex);
        // MUTEX UNLOCK - GERAL
    }
}

void *Retira(){
    while(TRUE){
    	if(strcmp(str, " retirar" )==10){				// verifica se str é compativel com o comando
	    	pthread_mutex_lock(&mutex);				//sinaliza o acesso exclusivo protegendo a região critica
	    	if(num>quant){						//verifica se o valor digitado não vai retirar demais
	    		double c = (TAM - quant);			// atualiza c com o valor restante do resfriador
	    		printf("Erro: O valor a ser retirado eh maior do que o presente no resfriador\n");	  //printa na tela do sistema	
			pthread_cond_wait(&cond, &mutex);		//coloca a thread em espera
		    	pthread_mutex_unlock(&mutex);			//libera o mutex lock
			}
			else{								
		    	quant = quant - num;						// quant recebe o valor atualizado de liquido
		    	double c = (TAM - quant);				       // atualiza c com o valor de espaço livre do resfriador
			printf("Valor removido com sucesso\n");
			printf("O reservatorio tem %lf L de bebida, faltando %lf L para estar completo\n", quant, c);
			
		    	pthread_cond_wait(&cond, &mutex);                    	//coloca a thread em espera
		    	pthread_mutex_unlock(&mutex);					//libera o mutex lock
	}	}
}
}

void *Adiciona() {
    while (TRUE){
    	if (strcmp(str, " adicionar")==10){                                   //verifica se str é compativel com o comando
	    	pthread_mutex_lock(&mutex);					//sinaliza o acesso exclusivo protegendo a região critica
	    	if(quant+num>TAM){					       //verifica se o valor digitado não supera o valor limite do resfriador
	    		double c = (TAM - quant);			      //atualiza c com o valor de espaço livre do resfriador
	    		printf("O valor a ser adicionado supera a capacidade maxima\n");
			pthread_cond_wait(&cond, &mutex);			//coloca a thread em espera
		    	pthread_mutex_unlock(&mutex);				//libera o mutex lock

			}
		else{
			quant = quant + num;						// quant recebe o valor atualizado de liquido
			double c = (TAM - quant);					//atualiza c com o valor de espaço livre do resfriador
			printf("Valor adicionado com sucesso\n");
			printf("O reservatorio tem %lf L de bebida, faltando %lf L para estar completo\n", quant, c);	
			pthread_cond_wait(&cond, &mutex);					 //coloca a thread em espera
	       	pthread_mutex_unlock(&mutex);						//libera o mutex lock
    }	}
    }
}

int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    int sockfd, portno;
    pthread_t t, adiciona, retira;

    long i;

    if (argc < 2) {										//verifica possíveis erros
        printf("Erro, porta nao definida!\n");
        printf("./SOFTWARE PORTA");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Erro abrindo o socket!\n");
        exit(1);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));                                            //parte responsável pelo
    portno = atoi(argv[1]);                                                                  //endereço do servidor   
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {		//verifica possíveis erros
        printf("Erro fazendo bind!\n");
        exit(1);
    }
    listen(sockfd,5);

    for (i = 0; i < TOTAL; i++) {
      nodo[i].newsockfd = -1;
    }
    while (1) {
        for (i = 0; i < TOTAL; i++) {
          if (nodo[i].newsockfd == -1) break;
        }
        nodo[i].newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
        // MUTEX LOCK - GERAL
        pthread_mutex_lock(&mutex);					
        pthread_create(&adiciona, NULL, Adiciona, NULL);		//cria as threads
    	pthread_create(&retira, NULL, Retira, NULL); 
        if (nodo[i].newsockfd < 0) {
            printf("Erro no accept!\n");
            exit(1);
        }
        pthread_create(&t, NULL, cliente, (void *)i);
        pthread_mutex_unlock(&mutex);
        // MUTEX UNLOCK - GERAL
    }
    pthread_join(adiciona,NULL);
    pthread_join(retira,NULL);                         //join
    pthread_join(t,NULL);
    
    //    close(sockfd);
    return 0; 
}
