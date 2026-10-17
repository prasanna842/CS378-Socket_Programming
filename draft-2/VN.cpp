#include<bits/stdc++.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <unistd.h>
#include <cstring>
using namespace std;

char myId=0;
uint32_t myIp=0;
int myPort=0;
vector<char> neighId;
vector<uint32_t> neighIp;
vector<int> neighPort;
vector<int> neighCost;

int main(int argc, char* argv[]){
    if(argc!=4){
        cerr<<"insufficient args\n";
        return 1;
    }
    string on_ip_addr=argv[1],myudp=argv[2],my_ip=argv[3];
    int socfd_tcp=socket(PF_INET,SOCK_STREAM,0);
    int socfd_udp=socket(PF_INET,SOCK_DGRAM,0);

    sockaddr_in addr_udp{};
    addr_udp.sin_family = AF_INET;
    addr_udp.sin_port = htons(stoi(myudp));
    inet_aton(my_ip.c_str(),&addr_udp.sin_addr);
    if(bind(socfd_udp,(struct sockaddr*)&addr_udp, sizeof(addr_udp))<0){
        perror("bind udp");
        return 1;
    };

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    inet_aton(on_ip_addr.c_str(),&(addr.sin_addr));
    connect(socfd_tcp,(struct sockaddr*)&addr, sizeof(addr));

    string s;
    s+=char(1); // CONNECT MESSAGE
    s.append((char*)&addr_udp.sin_addr.s_addr,4);
    s.append((char*)&addr_udp.sin_port,2);
    send(socfd_tcp,s.c_str(),s.size(),0);

    while(true){
        char header[2];
        if(recv(socfd_tcp,header,2,MSG_WAITALL)!=2) break;
        int count = (unsigned char) header[1];
        string msg(count*11,'\0');
        recv(socfd_tcp,&msg[0],count*11,MSG_WAITALL);

        for(int i=0;i<count;i++){
            const char* t=msg.data()+i*11;
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
                neighId.push_back(t[0]);
                neighIp.push_back(ip);
                neighPort.push_back(ntohs(port));
                neighCost.push_back(ntohl(cost));

            }
        }
        cout<<myId<<"=0\n";
        for(int k=0;k<neighId.size();k++){
            cout<<neighId[k]<<"="<<neighCost[k]<<endl;
        }
    }
    close(socfd_udp);
    close(socfd_tcp);
}