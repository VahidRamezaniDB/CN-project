#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>

/*
getting host ip or host addr from user

retrieving ip addr

validation

make and configure sockets

connection
*/
const int MAX_IP_STR_LEN=16;

int hname_to_ip(char *hname,char *ip, sockaddr_in *host){
    
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM;

    if ( (rv= getaddrinfo( hname , NULL , &hints , &servinfo)) != 0){
		return rv;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
		host = (struct sockaddr_in *) p->ai_addr;
		strcpy( ip, inet_ntoa( host->sin_addr ) );
        freeaddrinfo(servinfo);
        return 0;
    }
}


int main(int argc, char *argv[]){

    if(argc<2){
        fputs("Not enough arguments.\n",stderr);
        exit(EXIT_FAILURE);
    }

    char *host_ip_str=malloc(sizeof(char)*MAX_IP_STR_LEN);
    char *user_in;
    struct sockaddr_in serve_address;
    int rv;

    if(host_ip_str==NULL){
        fputs("memmory alloocation failed.(host_name)\n",stderr);
        exit(EXIT_FAILURE);
    }

    memset(host_ip_str,'\0',MAX_IP_STR_LEN);
    memset(&serve_address,0,sizeof(serve_address));
    user_in=argv[1];

    if((rv=hname_to_ip(user_in,host_ip_str,&serve_address))!=0){
        fprintf(stderr, "host name resolving failed\n more info: %s\n", gai_strerror(rv));
        exit(EXIT_FAILURE);
    }

    printf("%s ip resolved as %s\n",user_in,host_ip_str);

    int sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    if(sock<0){
        perror("failed to create socket.");
        exit(EXIT_FAILURE);
    }

    serve_address.sin_family=AF_INET;
    int pton_addr=inet_pton(AF_INET,host_ip_str,serve_address.sin_addr.s_addr);

    if(pton_addr==0){
        fputs("inavlid ip address. (pton)");
        exit(EXIT_FAILURE);
    }

    if(pton_addr<0){
        fputs("printable to numerical failed.");
        exit(EXIT_FAILURE);
    }

    

    return 0;
}










// int portno     = 80;
// char *hostname = "google.com";

// int sockfd;
// struct sockaddr_in serv_addr;
// struct hostent *server;

// sockfd = socket(AF_INET, SOCK_STREAM, 0);
// if (sockfd < 0) {
//     puts("ERROR opening socket");
// }

// server = gethostbyname(hostname);

// if (server == NULL) {
//     fprintf(stderr,"ERROR, no such host\n");
//     exit(0);
// }

// bzero((char *) &serv_addr, sizeof(serv_addr));
// serv_addr.sin_family = AF_INET;
// bcopy((char *)server->h_addr, 
//      (char *)&serv_addr.sin_addr.s_addr,
//      server->h_length);

// serv_addr.sin_port = htons(portno);
// if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
//     printf("Port is closed");
// } else {
//     printf("Port is active");
// }

// close(sockfd);
// return 0;
// }