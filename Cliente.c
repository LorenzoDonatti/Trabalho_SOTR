#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>		// inet_aton
#include <pthread.h>

int sockfd;

void *leitura(void *arg) {							//thread responsavel pela leitura
    char buffer[256];
    int n;
    while (1) {
        bzero(buffer,sizeof(buffer));				               
        n = recv(sockfd,buffer,50,0);						
        if (n <= 0) {							       //verifica possíveis erros
            printf("Erro lendo do socket!\n");
            exit(1);
        }
        printf("MSG: %s",buffer);						//printa no monitor
    }
}

int main(int argc, char *argv[]) {
    int portno, n;
    struct sockaddr_in serv_addr;
    pthread_t t;

    char buffer[256];								//define nome e porta
    if (argc < 3) {
       fprintf(stderr,"Uso: %s nomehost porta\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);							//função que transforma a porta em int
    sockfd = socket(AF_INET, SOCK_STREAM, 0);					//socket
    if (sockfd < 0) {								//verifica possiveis erros
        printf("Erro criando socket!\n");
        return -1;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));				//zera as variáveis
    serv_addr.sin_family = AF_INET;
//    serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    inet_aton(argv[1], &serv_addr.sin_addr);
    serv_addr.sin_port = htons(portno);					//criação da porta do servidor
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("Erro conectando!\n");
        return -1;
    }
    pthread_create(&t, NULL, leitura, NULL);					//criação da thread de leitura
    do {
        bzero(buffer,sizeof(buffer));				 		//limpa o buffer
        printf("Digite a mensagem (ou sair):");				
        fgets(buffer,50,stdin);						//escreve no terminal
        n = send(sockfd,buffer,50,0);						//envia para o servidor
        if (n == -1) {								//verifica erros
            printf("Erro escrevendo no socket!\n");
            return -1;
        }
        if (strcmp(buffer,"sair\n") == 0) {					//comando de saida
            break;
        }
    } while (1);
    close(sockfd);
    return 0;
}
