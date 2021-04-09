#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <pthread.h>
#include<math.h>

/*
getting host ip or host addr from user

retrieving ip addr

validation

make and configure sockets

connection
*/
const int MAX_IP_STR_LEN=16;
const int MAX_THREAD_COUNT=10;
const int CHECK_ALL_PORTS=1;
const int CHECK_RESERVED_PORTS=2;
const int CHECK_SERVICES=3;

struct connect_port_args{
    int sock__id;
    struct sockaddr_in *server_address;
    int port_range_start;
    int port_range_end;
    int time_limit;
};

int hname_to_ip(char *hname,char *ip,struct sockaddr_in *host){
    
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

void *connect_port(void *args){
    struct connect_port_args arguments=*(struct connect_port_args *)args;
    int temp;

    printf("%d-%d\n",arguments.port_range_start,arguments.port_range_end);

    int server_port=arguments.port_range_start;
    for(server_port;server_port<=arguments.port_range_end;server_port++){   
        arguments.server_address->sin_port=htons(server_port);
        if((temp=connect(arguments.sock__id,(struct sockaddr *)arguments.server_address,sizeof(*arguments.server_address)))<0){
            printf("port %d is close.\n",server_port);
        }else{
            printf("port %d is active.\n",server_port);
        }
    }
}


void scan_ports(int sock_id,struct sockaddr_in *server_address,int action){

    int port_range_start,port_range_end;
    int temp=0;
    int time_limit=0;
    int thread_no=0;
    int port_count=0;

    puts("port scaning range.");
    puts("start from:");
    scanf("%d",&port_range_start);
    puts("to:");
    scanf("%d",&port_range_end);
    if(port_range_end>65535 || port_range_end<0 || port_range_start>65535 
        || port_range_start<0 || port_range_end<port_range_start){
        
        fputs("invalid port range.",stderr);
        exit(EXIT_FAILURE);
    }
    puts("OK");
    port_count= port_range_end - port_range_start +1;
    //printf("how long do you want to wait for each port: (s)\n");
    // scanf("%d",&time_limit);

    fputs("How many threads you would like to create?\n",stdout);
    scanf("%d",&thread_no);

    if(thread_no<0 || thread_no>MAX_THREAD_COUNT){
        fputs("invalid or too high number of threads.\n",stderr);
        exit(EXIT_FAILURE);
    }

    if(time_limit<0 || time_limit>600){
        fputs("invalid or too large time limit.\n",stderr);
        exit(EXIT_FAILURE);
    }


    // args->sock__id=sock_id;
    // args->server_address=server_address;
    // args->port_range_start=port_range_start;
    // args->port_range_end=port_range_end;
    // args->time_limit=time_limit;
    struct connect_port_args temp_arg[thread_no];

    pthread_t tid[thread_no];
    for(int i=0;i<thread_no;i++){

        temp_arg[i].sock__id=sock_id;
        temp_arg[i].server_address=server_address;
        temp_arg[i].time_limit=time_limit;
        temp_arg[i].port_range_start=port_range_start +(i*(port_count/thread_no));
        temp_arg[i].port_range_end=port_range_start +((i+1)*(port_count/thread_no));
        if(i==thread_no-1){
            temp_arg[i].port_range_end=port_range_end;
        }
        pthread_create(&tid[i],NULL,(void *)connect_port,(void *)&temp_arg[i]);
    }
    for(int i=0;i<thread_no;i++){
        pthread_join(tid[i],NULL);
    }
}


int extract_action(int n,char *arg[]){

    if(n==2){
        return CHECK_ALL_PORTS;
    }else if(n>2 && n<5){
        if(strcmp(arg[2],"-q")==0){
            return CHECK_SERVICES;
        }
        if(strcmp(arg[2],"-r")==0){
            return CHECK_RESERVED_PORTS;
        }
    }else{
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
    struct sockaddr_in *server_address=malloc(sizeof(struct sockaddr_in));
    int rv;
    int action=0;

    if(host_ip_str==NULL){
        fputs("memmory alloocation failed.(host_name)\n",stderr);
        exit(EXIT_FAILURE);
    }

    if(server_address==NULL){
        fputs("memmory alloocation failed.(server address)\n",stderr);
        exit(EXIT_FAILURE);
    }

    action=extract_action(argc, argv);
    if(action==0){
        fputs("invalid arguments or flags.\n",stderr);
        exit(EXIT_FAILURE);
    }
    
    memset(host_ip_str,'\0',MAX_IP_STR_LEN);
    memset(server_address,0,sizeof(*server_address));
    user_in=argv[1];

    if((rv=hname_to_ip(user_in,host_ip_str,server_address))!=0){
        fprintf(stderr, "host name resolving failed\n more info: %s\n", gai_strerror(rv));
        exit(EXIT_FAILURE);
    }

    printf("%s ip resolved as %s\n",user_in,host_ip_str);

    if(action==CHECK_SERVICES){
        fputs("checking services.\n",stdout);
        exit(EXIT_SUCCESS);
    }
    
    int sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    if(sock<0){
        perror("failed to create socket.");
        exit(EXIT_FAILURE);
    }

    server_address->sin_family=AF_INET;
    int pton_addr=inet_pton(AF_INET,host_ip_str,&server_address->sin_addr.s_addr);

    if(pton_addr==0){
        fputs("inavlid ip address. (pton)\n",stderr);
        exit(EXIT_FAILURE);
    }

    if(pton_addr<0){
        fputs("printable to numerical failed.\n",stderr);
        exit(EXIT_FAILURE);
    }
    
    scan_ports(sock,server_address,action);

    close(sock);
    return 0;
}


// // Set non-blocking 
    // if( (arg = fcntl(soc, F_GETFL, NULL)) < 0) { 
    //     fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
    //     exit(0); 
    // } 
    // arg |= O_NONBLOCK; 
    // if( fcntl(soc, F_SETFL, arg) < 0) { 
    //     fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
    //     exit(0); 
    // } 