#include<bits/stdc++.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

using namespace std;

// // Take IPAddress (and perhaps port) of ON in command-line argument
// void take_ipaddr(){ 

// }

// // Also take own UDP port as argument can take own IP address
// void take_ownarg(){

// }

// // Open and bind socket to own UDP port – exit on failure with appropriate error message
// void setup_open_and_bind(){

// }

// //Make TCP connection to ON at port 5000, soon after start
// void make_tcp_connection(){

// }

// //Send CONNECT message to ON
// void connect(){

// }

// //Listen (constantly/periodically) for LINK-STATE message
// void listen(){

// }

// // On receiving LINK-STATE message, print to STDOUT
// void receive(){

// }

int main(int argc, char* argv[]){
    string myudp_port=argv[1];
    int socfd=socket(PF_INET,SOCK_STREAM,0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = INADDR_ANY;

    connect(socfd,(struct sockaddr*)&addr, sizeof(addr));
    char* msg="cs378...";
    send(socfd,msg,strlen(msg),0);
    close(socfd);
}