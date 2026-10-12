#include<bits/stdc++.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <unistd.h>
#include <cstring>
using namespace std;

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
    bind(socfd_udp,(struct sockaddr*)&addr_udp, sizeof(addr_udp));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    inet_aton(on_ip_addr.c_str(),&(addr.sin_addr));
    connect(socfd_tcp,(struct sockaddr*)&addr, sizeof(addr));
    string s=my_ip+","+myudp;
    send(socfd_tcp,s.c_str(),strlen(s.c_str()),0);
    char buf[1024];
    int sz=recv(socfd_tcp,buf,sizeof(buf),0);
    cout.write(buf,sz);
    cout<<endl;
    close(socfd_udp);
    close(socfd_tcp);
}