#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

const int MAX_IP_ADDR_LEN=16;
const int MAX_IP_CIDR_LEN=19;
const int MAX_SEND_COM_LEN=12;
const int MAX_RECV_COM_LEN=23;

struct my_ip{
    int parts[4];//bytewise ip devision
};

int is_equal_my_ip_struct(struct my_ip s, struct my_ip e){
    for(int i=0;i<4;i++){
        if(s.parts[i]!=e.parts[i]){
            return 0;
        }
    }
    return 1;
}

void* rcv_arp_rep(){

}

void main(int argc, char *argv[]){

    char opt;
    char* hostmin,*hostmax;
    struct my_ip min_ip,max_ip;
    char* timeout;

    if(argc<3 || argc>5){
        printf("Not valid command.\n use  <exec name> <option> <arg...> <int timeout sec>\n<options>:\n-p receive hostmin and hostmax as args\n-c receive CIDR notation.\n");
        exit(EXIT_FAILURE);        
    }

    if(argv[1]=="-p"){
        hostmin=argv[2];
        hostmax=argv[3];
        timeout=argv[4];

        int j=0;
        char* temp=malloc(3*sizeof(char));
        memset(temp,0,sizeof(temp));
        for(int i=0;hostmin[i];i++){
            if(hostmin[i]=='.'){
                // strncat(temp,"\0",1);
                min_ip.parts[j]=atoi(temp);
                memset(temp,0,sizeof(temp));
                j++;
                continue;
            }
            strncat(temp,hostmin[i],1);
        }
        
        j=0;
        memset(temp,0,sizeof(temp));
        for(int i=0;hostmax[i];i++){
            if(hostmax[i]=='.'){
                // strncat(temp,"\0",1);
                max_ip.parts[j]=atoi(temp);
                memset(temp,0,sizeof(temp));
                j++;
                continue;
            }
            strncat(temp,hostmax[i],1);
        }

        for(int i=0;i<4;i++){
            if(min_ip.parts[i]>max_ip.parts[i]){
                printf("min and max ip addresses are not correct.\nEnter them in the following order: Min host addr Max host addr ");
                exit(EXIT_FAILURE);
            }
        }
    }else if(argv[1]=="-c"){
        // char* cidr_form=malloc(MAX_IP_CIDR_LEN*sizeof(char));
        // cidr_form=argv[3];
        printf("not yet developed.\n");
        exit(EXIT_FAILURE);
    }else{
        printf("no such an option.\n");
        exit(EXIT_FAILURE);
    }

    while(1){
        char* ip_str=malloc(MAX_IP_ADDR_LEN*sizeof(char));
        memset(ip_str,0,sizeof(ip_str));

        for(int i=0;i<4;i++){
            char temp[3];
            sprintf(temp,"%d",min_ip.parts[i]);
            strcat(ip_str,temp);
        }
        
        /*sending arp req*/
        char* send_command=malloc(sizeof(char)*(MAX_SEND_COM_LEN+MAX_IP_ADDR_LEN-1));
        
        strcat(send_command,"sudo ./send ");
        strcat(send_command, ip_str);
        strcat(send_command," &");

        system(send_command);

        /*waiting for arp rep*/
        
        char* recv_command=malloc(sizeof(char)*MAX_RECV_COM_LEN);

        strcat(recv_command,"sudo timeout ");
        strcat(recv_command,timeout);
        strcat(recv_command," ./recv");

        system(recv_command);

        for(int i=3;i<0;i--){
            if(min_ip.parts[i]!=255){
                min_ip.parts[i]++;
                break;
            }
            min_ip.parts[i]=0;
        }

        if(is_equal_my_ip_struct(min_ip,max_ip)){
            break;
        }        
    }

}
