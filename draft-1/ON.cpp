#include<bits/stdc++.h>
using namespace std;
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// //Take one command-line argument, the configuration file, it is always a good programming habit to put sanity checks
// void read_config_file(){

// }

// //Listen for incoming connections from VNs on TCP port 5000
// void listen(){

// }

// //Also constantly (periodically) monitor changes to config file
// void modify_config(){

// }

// //Listen for (app layer) messages in each ON-VN connection
// void vn_to_on_messaege(){

// }

// void on_to_vn_message(){

// }

int main(int argc, char* argv[]){
    int socfd=socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(socfd,(struct sockaddr*)&addr, sizeof(addr));
    listen(socfd, 10);
    int cl=accept(socfd,nullptr,nullptr);
    char buf[128]={0};
    recv(cl,buf,sizeof(buf),0);
    cout<<buf<<endl;
    close(cl);
    close(socfd);
}