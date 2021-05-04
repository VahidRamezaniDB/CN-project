#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <pthread.h>
#include <math.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>

/*
getting host ip or host addr from user

retrieving ip addr

validation

make and configure sockets

connection
*/
#define PING_PKT_S 64
const int MAX_IP_STR_LEN=16;
const int MAX_THREAD_COUNT=10;
const int MAX_ADDRESS_STR_LEN=30;
const int CHECK_ALL_PORTS=1;
const int CHECK_RESERVED_PORTS=2;
const int CHECK_SERVICES=3;
const int GET_PING=4;
int pingloop=1;
// const int PING_PKT_S=64;
int PING_SLEEP_RATE=1000000;
int RECV_TIMEOUT=1;

struct connect_port_args{
    int sock_id;
    struct sockaddr_in *server_address;
    int port_range_start;
    int port_range_end;
    struct timeval *timeout;
};

struct ping_pkt{
    struct icmphdr hdr;
    char msg[PING_PKT_S-sizeof(struct icmphdr)];
};

void interrupt_handler(int dummy){
    pingloop=0;
}

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

int connect_time_limit(int sockno,struct sockaddr *addr,int addrlen,struct timeval *timeout){

    /*
    returns 0 if connection was successful
    returns 1 if timeout reached
    returns -1 if an error occured
    */

    int res, opt;

	// get socket flags
	if ((opt = fcntl (sockno, F_GETFL, NULL)) < 0) {
        fputs("get falgs error.\n",stderr);
		return -1;
	}

	// set socket non-blocking
	if (fcntl (sockno, F_SETFL, opt | O_NONBLOCK) < 0) {
        fputs("set falgs error.\n",stderr);
		return -1;
	}

	// try to connect
	if ((res = connect (sockno, addr, addrlen)) < 0) {
		if (errno == EINPROGRESS) {
			fd_set wait_set;

			// make file descriptor set with socket
			FD_ZERO (&wait_set);
			FD_SET (sockno, &wait_set);

			// wait for socket to be writable; return after given timeout
			res = select (sockno + 1, NULL, &wait_set, NULL, timeout);
		}
	}
	// connection was successful immediately
	else {
		res = 1;
	}

	// reset socket flags
	if (fcntl (sockno, F_SETFL, opt) < 0) {
        fputs("reset falgs error.\n",stderr);
		return -1;
	}

	// an error occured in connect or select
	if (res < 0) {
		return -1;
	}
	// select timed out
	else if (res == 0) {
		errno = ETIMEDOUT;
		return 1;
	}
	// almost finished...
	else {
		socklen_t len = sizeof (opt);

		// check for errors in socket layer
		if (getsockopt (sockno, SOL_SOCKET, SO_ERROR, &opt, &len) < 0) {
            fputs("socket layer checking error.\n",stderr);
			return -1;
		}

		// there was an error
		if (opt) {
			errno = opt;
            fputs("socket layer checking error.\n",stderr);
			return -1;
		}
	}

	return 0;
}

void *connect_port(void *args){
    struct connect_port_args arguments=*(struct connect_port_args *)args;
    int rv;

    printf("%d-%d\n",arguments.port_range_start,arguments.port_range_end);

    int server_port=arguments.port_range_start;
    for(server_port;server_port<arguments.port_range_end;server_port++){   
        arguments.server_address->sin_port=htons(server_port);
        rv=connect_time_limit(arguments.sock_id,
        (struct sockaddr *)arguments.server_address,
        sizeof(*arguments.server_address),
        arguments.timeout);
        if(rv==0){
            printf("port %d is active.\n",server_port);
        }else if(rv==1){
            printf("port %d is close. (timout reached)\n",server_port);
        }else{
            printf("port %d is close. (error connecting)\n",server_port);
        }
    }
}

void scan_ports(struct sockaddr_in *server_address,int action){

    int port_range_start,port_range_end;
    int temp=0;
    int thread_no=0;
    int port_count=0;
    struct timeval *timeout=malloc(sizeof(struct timeval));

    if(timeout==NULL){
        fputs("memmory allocation failed. (timeout)\n",stderr);
        exit(EXIT_FAILURE);
    }

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

    memset(timeout,0,sizeof(timeout));
    
    double time_in=0;
    
    printf("how long do you want to wait for each port: (s)\n");
    scanf("%lf",&time_in);
    
    if(time_in<0 || time_in>600){
        fputs("invalid or too large time limit.\n",stderr);
        exit(EXIT_FAILURE);
    }

    timeout->tv_sec=floor(time_in);
    timeout->tv_usec=(double) (time_in-floor(time_in));


    fputs("How many threads you would like to create?\n",stdout);
    scanf("%d",&thread_no);

    if(thread_no<0 || thread_no>MAX_THREAD_COUNT){
        fputs("invalid or too high number of threads.\n",stderr);
        exit(EXIT_FAILURE);
    }


    struct connect_port_args temp_arg[thread_no];

    pthread_t tid[thread_no];
    for(int i=0;i<thread_no;i++){

        // temp_arg[i].sock__id=sock_id;
        temp_arg[i].sock_id=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        if(temp_arg[i].sock_id<0){
            perror("failed to create socket.");
            i--;
            continue;
        }
        temp_arg[i].server_address=server_address;
        temp_arg[i].timeout=timeout;
        temp_arg[i].port_range_start=port_range_start +(i*(port_count/thread_no));
        temp_arg[i].port_range_end=port_range_start +((i+1)*(port_count/thread_no));
        if(i==thread_no-1){
            temp_arg[i].port_range_end=port_range_end+1;
        }
        pthread_create(&tid[i],NULL,(void *)connect_port,(void *)&temp_arg[i]);
    }
    for(int i=0;i<thread_no;i++){
        pthread_join(tid[i],NULL);
        close(temp_arg[i].sock_id);
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
        if(strcmp(arg[2],"-p")==0){
            return GET_PING;
        }
    }else{
        return 0;
    }
}

unsigned short checksum(void *pckt, int len){
    unsigned short *buffer = pckt;
    unsigned int sum=0;
    unsigned short result;
  
    for ( sum = 0; len > 1; len -= 2 ){
        sum += *buffer++;
    }
    if ( len == 1 ){
        sum += *(unsigned char*)buffer;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

void send_ping(int ping_sockfd, struct sockaddr_in *ping_addr,char *ping_ip, char *host_name){

    int ttl_val=64, msg_count=0, i, addr_len, flag=1,msg_received_count=0;
      
    struct ping_pkt pckt;
    struct sockaddr_in r_addr;
    struct timespec time_start, time_end, tfs, tfe;
    long double rtt_msec=0, total_msec=0;
    struct timeval tv_out;
    tv_out.tv_sec = RECV_TIMEOUT;
    tv_out.tv_usec = 0;
  
    clock_gettime(CLOCK_MONOTONIC, &tfs);
  
      

    if (setsockopt(ping_sockfd, SOL_IP, IP_TTL,&ttl_val, sizeof(ttl_val)) != 0){
        printf("\nSetting socket options to TTL failed!\n");
        perror("ttl setting.\n");
        return;
    }
    else{
        printf("\nSocket set to TTL..\n");
    }
  
    setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO,(const char*)&tv_out, sizeof tv_out);
  
    while(pingloop){
        flag=1;
       
        bzero(&pckt, sizeof(pckt));
          
        pckt.hdr.type = ICMP_ECHO;
        pckt.hdr.un.echo.id = getpid();
          
        for ( i = 0; i < sizeof(pckt.msg)-1; i++ ){
            pckt.msg[i] = i+'0';
        }
        pckt.msg[i] = 0;
        pckt.hdr.un.echo.sequence = msg_count++;
        pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
  
  
        usleep(PING_SLEEP_RATE);
  
        clock_gettime(CLOCK_MONOTONIC, &time_start);
        if ( sendto(ping_sockfd, &pckt, sizeof(pckt), 0, 
           (struct sockaddr*) ping_addr, 
            sizeof(*ping_addr)) <= 0){
            printf("\nPacket Sending Failed!\n");
            flag=0;
        }
  
        addr_len=sizeof(r_addr);
  
        if ( recvfrom(ping_sockfd, &pckt, sizeof(pckt), 0, 
             (struct sockaddr*)&r_addr, &addr_len) <= 0 && msg_count>1){
            printf("\nPacket receive failed!\n");
        }
        else{
            clock_gettime(CLOCK_MONOTONIC, &time_end);
              
            double time_elapsed = ((double)(time_end.tv_nsec - time_start.tv_nsec))/1000000.0;
            rtt_msec = (time_end.tv_sec-time_start.tv_sec)*1000.0 + time_elapsed;

            if(flag){
                if(!(pckt.hdr.type ==69 && pckt.hdr.code==0)){
                    printf("Error..Packet received with ICMP type %d code %d\n", pckt.hdr.type, pckt.hdr.code);
                }
                else{
                    printf("\n%d bytes from %s (%s) msg_seq=%d ttl=%d rtt = %Lf ms.\n",
                     PING_PKT_S, host_name, ping_ip, msg_count,ttl_val, rtt_msec);
                    msg_received_count++;
                }
            }
        }    
    }
    clock_gettime(CLOCK_MONOTONIC, &tfe);
    double time_elapsed = ((double)(tfe.tv_nsec - tfs.tv_nsec))/1000000.0;
      
    total_msec = (tfe.tv_sec-tfs.tv_sec)*1000.0+ time_elapsed;
                     
    printf("\n===%s (%s) ping statistics===\n", ping_ip,host_name);
    printf("\n%d packets sent, %d packets received, %f percent packet loss. Total time: %Lf ms.\n\n",
    msg_count, msg_received_count,((msg_count - msg_received_count)/msg_count) * 100.0,total_msec); 
}

void* ping_driver(void *h_address_str){
    char *host_address_str=(char *) h_address_str;
    int sock;
    char *ip_addr=malloc(sizeof(char)*MAX_IP_STR_LEN);
    struct sockaddr_in *server_address=malloc(sizeof(struct sockaddr_in));

    int rv;
    if((rv=hname_to_ip(host_address_str,ip_addr,server_address))!=0){
        fprintf(stderr, "host name resolving failed\n more info: %s\n", gai_strerror(rv));
        exit(EXIT_FAILURE);
    }
    printf("trying to connect to '%s'\nIP: '%s'\n",host_address_str,ip_addr);

    sock= socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(sock<0){
        printf("Socket file descriptor not received!!\n");
        perror("socket creation.");
        exit(EXIT_FAILURE);
    }
    printf("Socket file descriptor %d received\n", sock);

    signal(SIGINT,interrupt_handler);
 
    send_ping(sock, server_address,ip_addr,host_address_str);
      
}

void ping(char input[]){

    int i=0;
    int counter=0;
    while(input[i]!='\0'){
        if(input[i]==' '){
            i++;
            continue;
        }
        for(i;;i++){
            if(input[i]==' '|| input[i]=='\0'){
                break;
            }
        }
        counter++;
    }
    printf("%d web addresses found.\n",counter);

    char addr[counter][MAX_ADDRESS_STR_LEN];
    i=0;
    int j=0;
    while(input[i]!='\0'){
        if(input[i]==' '){
            i++;
            continue;
        }
        int k=0;
        for(i;;i++){
            if(input[i]==' '|| input[i]=='\0'){
                addr[j][k]='\0';    
                break;
            }
            addr[j][k]=input[i];
            k++;
            if(k>=MAX_ADDRESS_STR_LEN){
                fputs("too large address.\n",stderr);
                exit(EXIT_FAILURE);
            }
        }
        j++;
    }
    for(int h=0;h<counter;h++){
        printf("%s\n",addr[h]);
    }

    pthread_t tid[counter];
    for(int h=0;h<counter;h++){
        pthread_create(&tid[h],NULL,(void *)ping_driver,(void *)&addr[h]);
    }
    for(int h=0;h<counter;h++){
        pthread_join(tid[h],NULL);
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

    if(action==GET_PING){
        fputs("ping processing...\n",stdout);
        ping(user_in);
        exit(EXIT_SUCCESS);
    }

    if((rv=hname_to_ip(user_in,host_ip_str,server_address))!=0){
        fprintf(stderr, "host name resolving failed\n more info: %s\n", gai_strerror(rv));
        exit(EXIT_FAILURE);
    }

    printf("%s ip resolved as %s\n",user_in,host_ip_str);

    if(action==CHECK_SERVICES){
        fputs("checking services.\n",stdout);
        exit(EXIT_SUCCESS);
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
    
    scan_ports(server_address,action);

    return 0;
}