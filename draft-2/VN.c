#include<stdio.h>
#include<string.h>
#include<stdint.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <unistd.h>
#include <time.h>

char myId=0;
uint32_t myIp=0;
int myPort=0;
char neighId[26];
uint32_t neighIp[26];
int neighPort[26];
int neighCost[26];
int neigh[26]={-1};
int neighcnt=0;
int seq_num=0;
int graph[26][26] ={-1};

char* send_link_state(){
    //  (3-LSP msg)-1, neighcnt -4, seq - 4, (org-id-1, ls_org_id -1, cost - 4)*neighcnt
    char msg[9+(6*neighcnt)];
    msg[0]=3;
    memcpy(msg+1,&neighcnt,4);
    memcpy(msg+5,&seq_num,4);
    int ind=9;
    for(int i=0;i<26;i++){
        if(neigh[i]==1){
            memcpy(msg+ind,&myId,1);
            memcpy(msg+ind+1,&neighId[i],1);
            memcpy(msg+ind+2,&neighCost[i],4);
            ind+=6;
        }
    }
    return msg;
}

int main(int argc, char* argv[]){
    if(argc!=4){
        fprintf(stderr,"insufficient args\n");
        return 1;
    }
    char *on_ip_addr=argv[1],*myudp=argv[2],*my_ip=argv[3];
    int socfd_tcp=socket(PF_INET,SOCK_STREAM,0);
    int socfd_udp=socket(PF_INET,SOCK_DGRAM,0);

    struct sockaddr_in addr_udp={0};
    addr_udp.sin_family = AF_INET;
    addr_udp.sin_port = htons(atoi(myudp));
    inet_aton(my_ip,&addr_udp.sin_addr);
    if(bind(socfd_udp,(struct sockaddr*)&addr_udp, sizeof(addr_udp))<0){
        perror("bind udp");
        return 1;
    };

    struct sockaddr_in addr={0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    inet_aton(on_ip_addr,&(addr.sin_addr));
    connect(socfd_tcp,(struct sockaddr*)&addr, sizeof(addr));

    char s[7];
    s[0]=1; // CONNECT MESSAGE
    memcpy(s+1,(char*)&addr_udp.sin_addr.s_addr,4);
    memcpy(s+5,(char*)&addr_udp.sin_port,2);
    send(socfd_tcp,s,7,0);

    while(1){
        char header[2];
        if(recv(socfd_tcp,header,2,MSG_WAITALL)!=2) break;
        int count = (unsigned char) header[1];
        char msg[count*11];
        recv(socfd_tcp,msg,count*11,MSG_WAITALL);

        for(int i=0;i<count;i++){
            const char* t=(msg+(i*11));
            uint32_t ip,cost;
            uint16_t port;
            memcpy(&ip,t+1,4);
            memcpy(&port,t+5,2);
            memcpy(&cost,t+7,4);
            if(cost==0){
                myId=t[0];
                myIp=ip;
                myPort=ntohs(port);
            }else{
                neigh[t[0]-'A']=1;
                neighId[t[0]-'A'] =t[0];
                neighIp[t[0]-'A']=ip;
                neighPort[t[0]-'A']=ntohs(port);
                neighCost[t[0]-'A']=ntohl(cost);
                graph[myId-'A'][t[0]-'A']=ntohl(cost);
                graph[t[0]-'A'][myId-'A']=ntohl(cost);
                neighcnt++;
            }
        }
        printf("%c=0\n",myId);
        for(int k=0;k<26;k++){
            if(neigh[k]==1)printf("%c=%d\n",neighId[k],neighCost[k]);
        }
    }
    while(1){
        clock_t st=clock();
        //flood
        for(int i=0;i<26;i++){
            if(neigh[i]==1){
                char* msg=send_link_state()
                struct sockaddr_in dst;
                dst.sin_family =AF_INET;
                dst.sin_port=htons(neighPort[i]);
                dst.sin_addr.s_addr=htonl(neighIp[i]);
                sendto(socfd_udp,msg,strlen(msg),0,(struct socaddr*)&dst,sizeof(dst));
            }
        }

    }
    close(socfd_udp);
    close(socfd_tcp);
}