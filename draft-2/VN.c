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
int neigh[26];
uint32_t prev_seq[26]={0};
int seq_num=0;
int graph[26][26] ={-1};

void flood(int udp_fd,char* msg,int l,struct sockaddr_in *from){
    for(int i=0;i<26;i++){
        if(neigh[i]!=1)continue;
        if((from!=NULL) && (from->sin_addr.s_addr == neighIp[i]) && ((ntohs(from->sin_port))==neighPort[i]))continue;
        struct sockaddr_in dst={0};
        dst.sin_family=AF_INET;
        dst.sin_port=htons(neighPort[i]);
        dst.sin_addr.s_addr=neighIp[i];
        sendto(udp_fd,msg,l,0,(struct sockaddr *)&dst,sizeof(dst));
        sendto(udp_fd,msg,l,0,(struct sockaddr *)&dst,sizeof(dst));
    }
}

void send_link_state(int fd_udp){
    //  (3-LSP msg)-1,org-id-1,neighcnt -1,seq - 4, (ls_org_id -1, cost - 4)*neighcnt
    if(myId==0)return;
    uint8_t neighcnt=0;
    for(int i=0;i<26;i++)if(neigh[i]==1)neighcnt++;
    char msg[7+(5*neighcnt)];
    seq_num++;
    msg[0]=3;
    msg[1]=myId;
    memcpy(msg+2,&neighcnt,1);
    uint32_t s=htonl(seq_num);
    memcpy(msg+3,&s,4);
    int ind=7;
    for(int i=0;i<26;i++){
        if(neigh[i]==1){
            memcpy(msg+ind,&neighId[i],1);
            uint32_t c=htonl(neighCost[i]);
            memcpy(msg+ind+1,&c,4);
            ind+=5;
        }
    }
    flood(fd_udp,msg,(7+(5*neighcnt)),NULL);
}

int read_ls_msg(char* msg, int len){ //return 1-new , 0-old, -1-error
    if(msg[0]!=3 || (len<7))return -1;
    char org=msg[1];
    if(org==myId)return 0;
    if((org<'A') || (org>'Z'))return -1;
    uint8_t neighcnt;
    memcpy(&neighcnt,msg+2,1);

    
    if(len<(7+(neighcnt*5)))return -1;

    for(int i=0;i<neighcnt;i++){
        char zz=msg[7+(5*i)];
        if((zz<'A') || (zz>'Z'))return -1;
    }

    uint32_t seq;
    memcpy(&seq,msg+3,4);
    seq=ntohl(seq);

    if(seq<=prev_seq[org-'A'])return 0 ;
    prev_seq[org-'A']=seq;

    for(int i=0;i<26;i++){
        graph[org-'A'][i]=-1;
        graph[i][org-'A']=-1;
    }
    graph[org-'A'][org-'A']=0;
    int ind=7;
    for(int i=0;i<neighcnt;i++){
        char y=msg[ind];
        int c;
        memcpy(&c,msg+ind+1,4);
        ind+=5;
        graph[y-'A'][org-'A']=(int)ntohl(c);
        graph[org-'A'][y-'A']=(int)ntohl(c);
    }
    return 1;
}

void show_graph(){
    printf("  ---  Current Known Graph  ---  \n");
    for(int i=0;i<26;i++){
        for(int j=0;j<26;j++){
            if(graph[i][j]==-1 || i>=j)continue;
            else printf("%d ",graph[i][j]);
        }
        printf("\n");
    }
}

int read_on(int socfd_tcp){ //0 - bad
    char header[2];
    if(recv(socfd_tcp,header,2,MSG_WAITALL)!=2) return 0;
    int count =(unsigned char) header[1];
    if(header[0]!=2 || count<1 || count>26)return 0;
    char msg[count*11];
    if(recv(socfd_tcp,msg,count*11,MSG_WAITALL)!=(count*11))return 0;

    int me=-1;
    for(int i=0;i<count;i++){
        uint32_t c;
        memcpy(&c,msg+(7+(i*11)),4);
        if(ntohl(c)==0){
            me=i;
            break;
        }
    }
    if(me<0)return 1;
    myId=msg[me*11];
    if((myId<'A') || (myId>'Z')){
        myId=0;
        return 1;
    }

    memset(neigh,0,sizeof(neigh));
    for(int i=0;i<26;i++){
        graph[myId-'A'][i]=-1;
        graph[i][myId-'A']=-1;
    }
    graph[myId-'A'][myId-'A']=0;

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
        }
    }
    printf("%c=0\n",myId);
    for(int k=0;k<26;k++){
        if(neigh[k]==1)printf("%c=%d\n",neighId[k],neighCost[k]);
    }
    printf("\n");
    return 1;
}

int main(int argc, char* argv[]){
    if(argc!=4){
        fprintf(stderr,"insufficient args\n");
        return 1;
    }
    for(int i=0;i<26;i++){
        for(int j=0;j<26;j++){
            if(i==j)graph[i][j]=0;
            else graph[i][j]=-1;
        }
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
    if(!inet_aton(my_ip,&addr_udp.sin_addr)){
        fprintf(stderr,"incorrect ip\n");
        return 1;
    }
    if(bind(socfd_udp,(struct sockaddr*)&addr_udp, sizeof(addr_udp))<0){
        perror("bind udp");
        return 1;
    };

    struct sockaddr_in addr={0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    if(!inet_aton(on_ip_addr,&(addr.sin_addr))){
        fprintf(stderr,"incorrect ip\n");
        return 1;
    }
    if(connect(socfd_tcp,(struct sockaddr*)&addr, sizeof(addr))<0){
        perror("connect");
        return 1;
    }

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
        FD_SET(socfd_udp,&rfs);
        mxfd=socfd_udp;
        if(tcp_present){
            FD_SET(socfd_tcp,&rfs);
            if(socfd_tcp>mxfd)mxfd=socfd_tcp;
        }
        struct timeval t_v={1,0};
        int rr=select(mxfd+1,&rfs,NULL,NULL,&t_v);
        
        if(rr<0){
            if(errno==EINTR)continue;
            perror("select");
            break;
        }

        if((rr>0) && tcp_present && FD_ISSET(socfd_tcp,&rfs)){
            if(read_on(socfd_tcp)) last=0;
            else{
                close(socfd_tcp);
                tcp_present=0;
            }
        }

        if((rr>0) && FD_ISSET(socfd_udp,&rfs)){
            char buf[256];
            struct sockaddr_in from;
            socklen_t z=sizeof(from);
            int len=recvfrom(socfd_udp,buf,sizeof(buf),0,(struct sockaddr *)&from,&z);
            if(len>0 && (read_ls_msg(buf,len)==1))flood(socfd_udp,buf,len,&from);
        }

        time_t pres=time(NULL);
        if((pres-last)>=20){
            send_link_state(socfd_udp);
            last=pres;
            show_graph();
        }
    }
    close(socfd_udp);
    if(tcp_present)close(socfd_tcp);
}