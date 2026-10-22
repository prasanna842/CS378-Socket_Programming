#include<stdio.h>
#include<string.h>
#include<stdint.h>
#include<stdlib.h>
#include<sys/types.h>
#include<errno.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include<sys/select.h>

char myId=0;
uint32_t myIp=0;
int myPort=0;
char neighId[26];
uint32_t neighIp[26];
int neighPort[26];
int neighCost[26];
int neigh[26]={-1};
uint32_t seq[26]={0};
int neighcnt=0;
int seq_num=0;
int graph[26][26] ={-1};


int send_link_state(char *buf){
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
    buf=msg;
    return 9+(6*neighcnt);
}

void flood(int udp_fd,char* msg,int l){
    for(int i=0;i<26;i++){
        if(!neigh[i])continue;
        //
        struct sockaddr_in dst={0};
        dst.sin_family=AF_INET;
        dst.sin_port=htons(neighPort[i]);
        dst.sin_addr.s_addr=htonl(neighIP[i]);
        sendto(socfd_udp,msg,l,0,(struct socaddr*)&dst,sizeof(dst));
    }
}



void read_ls_msg(char* msg, int& changed){
    int ind =9;
    int ncnt;
    memcpy(&ncnt,msg[1],4);
    for(int i=0;i<ncnt;i++){
        char x,y;
        int c;
        memcpy(&x,msg[ind],1);
        memcpy(&y,msg[ind+1],1);
        memcpy(&c,msg[ind+2],4);
        ind+=6;
        if(graph[x][y]!=c){
            changed=1;
            graph[x][y]=c;
        }
    }
}

void graph(){
    printf("     Current Known Graph     \n");
    for(int i=0;i<26;i++){
        for(int j=0;j<26;j++){
            if(i==j)fprintf("0 ");
            else fprintf("%d ",graph[i][j]);
        }
        fprintf("\n");
    }
}

int read_on(){
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

int main(int argc, char* argv[]){
    if(argc!=4){
        fprintf(stderr,"insufficient args\n");
        return 1;
    }
    for(int i=0;i<26;i++){
        for(int j=0;j<26;j++)if(i==j)graph[i][j]=0;
    }
    char *on_ip_addr=argv[1],*myudp=argv[2],*my_ip=argv[3];
    int socfd_tcp=socket(AF_INET,SOCK_STREAM,0);
    int socfd_udp=socket(AF_INET,SOCK_DGRAM,0);
    if((socfd_tcp<0) || (socfd_udp<0)){
        perror("socket creation failes");
        return 1;
    }

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

    int tcp_present=1;
    int mxfd;
    time_t last=0;
    while(1){
        fd_set rfs;
        FD_ZERO(&rfs);
        FD_SET(socfd_udp,&rf);
        mxfd=socfd_udp;
        if(tcp_present){
            FD_SET(socfd_tcp,&rfs);
            if(socfd_tcp>mxfd)mxfd=socfd_tcp;
        }
        struct timeval t_v={20,0};
        int rr=select(mxfd+1,&rf,NULL,NULL,&tv);
        
        if((r>0) && tcp_present && FD_ISSET(socfd_tcp,&rf)){
            read_on();
            last=0;
        }

        if((r>0) && FD_ISSET){

        }
    }
    close(socfd_udp);
    if(tcp_present)close(socfd_tcp);
}