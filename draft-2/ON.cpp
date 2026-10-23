#include<bits/stdc++.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<unistd.h>
using namespace std;

// Returns full matrix from cofig file
vector<vector<int>> configfile(string &file){  
    ifstream fin(file);
    if(!fin){
        cerr<<"unable to open file\n";return{};
    }
    vector<vector<int>> dummy;
    string line;
    while(getline(fin,line)){
        if(!line.empty() && line.back()=='\r')line.pop_back();
        size_t st_ind=line.find_first_not_of(" \t");
        if(st_ind==string::npos)continue;
        if(line[st_ind]=='#')continue;
        istringstream ss(line.substr(st_ind));
        vector<int> row;
        int x;
        while(ss>>x){
            if( (x!=-1) && (x<=0) ){cerr<<"Invalid cost\n";return{};}
            row.push_back(x);
        }
        if(!ss.eof()){
            cerr<<"Invalid token in config line \n";
            return {};
        }
        dummy.push_back(row);
    }
    int n=dummy.size();
    if(n==0){
        cerr<<"Empty config file\n";
        return {};
    }
    if(n+1>26){
        cerr<<"Nodes exceeding 26\n";
        return {};
    }
    for(int i=0;i<n;i++){
        if(dummy[i].size()!=(n-i)){
            cerr<<"Invalid config entries\n";
            return{};
        }
    }
    n++;
    vector<vector<int>> cost(n,vector<int>(n,-1));
    for(int i=0;i<n;i++)cost[i][i]=0;
    for(int i=0;i<(n-1);i++){
        for(int j=0;j<(dummy[i].size());j++){
            int k=(i+j+1);
            cost[i][k]=dummy[i][j];
            cost[k][i]=cost[i][k];
        }
    }
    return cost;
}

// Sending LINK STATE MESSAGE to VN
void send_link_state(int clfd,int ind,const vector<vector<int>>& cost_matrix, vector<int>& ports, vector<uint32_t>& ip){ 
    int n= cost_matrix.size();
    string tup;
    int num_neigh=0;
    for(int i=0;i<n;i++){
        if(cost_matrix[ind][i]>=0){
            tup+=char('A'+i);
            tup.append((char*)&ip[i],4);
            uint16_t port=htons(ports[i]);
            tup.append((char*)&port,2);
            uint32_t cost = htonl(cost_matrix[ind][i]);
            tup.append((char*)&cost,4);
            num_neigh++;
        }
    }
    string to_send;
    to_send+=char(2); // 2- LINK STATE messages
    to_send+=char(num_neigh);
    to_send+=tup;
    send(clfd,to_send.c_str(),to_send.size(),MSG_NOSIGNAL); 
}

int main(int argc, char* argv[]){
    if(argc<=1){
        cerr<<"give config file";
        return 1;
    }
    string file=argv[1];
    auto costmat=configfile(file);
    int n=costmat.size();
    if(n==0){
        cerr<<"ON: Invalid initial config file \n";
        return 1;
    }

    for(auto v:costmat){
        for(auto x:v)cout<<x<<" ";
        cout<<endl;
    }

    //Gives the last modification time
    long long last_time;
    struct stat st;
    if(stat(file.c_str(),&st)<0) last_time=0;
    else last_time= (long long)st.st_mtim.tv_sec*1000000000LL + st.st_mtim.tv_nsec;

    bool first_file=true;
    vector<int>  VN_fd(n,-1);
    vector<uint32_t> ip(n);
    vector<int> ports(n);
    int connections_cnt=0;
    vector<int> slot_of_fd(FD_SETSIZE,-1);
    fd_set master;
    fd_set read_fds;
    int fdmax;
    int yes=1;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    int socfd,new_fd;
    struct sockaddr_in my_addr={0};
    struct sockaddr_in their_addr={0};
    socklen_t len;

    if((socfd=socket(AF_INET,SOCK_STREAM,0))==-1){
        perror("socket");
        return 1;
    }

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(5000);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(socfd,(struct sockaddr*)&my_addr, sizeof(my_addr))<0){
        perror("bind");
        return 1;
    }
    if(listen(socfd, 26)<0){
        perror("listen");
        return 1;
    };

    FD_SET(socfd,&master);
    fdmax=socfd;

    auto first_free_ind=[&]()->int{
        for(int i=0;i<n;i++){
            if( VN_fd[i]==-1) return i;
        }
        return -1;
    };

    auto assign=[&](int fd,int slot)->void{
        struct timeval t={2,0};
        setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&t,sizeof(t));
        unsigned char b[7];
        int r=recv(fd,b,7,MSG_WAITALL);
        if(r!=7 || b[0]!=1){
            cerr<<"Dropping COnnection, bad info \n";
            close(fd);
            return;
        }
        FD_SET(fd,&master);
        if(fd>fdmax) fdmax=fd;
        slot_of_fd[fd]=slot;
         VN_fd[slot]=fd;
        memcpy(&ip[slot],b+1,4);
        uint16_t p;
        memcpy(&p,b+5,2);
        ports[slot]=ntohs(p);
        connections_cnt++;
        struct in_addr a;
        a.s_addr=ip[slot];
        cout<<"ON : node"<<char('A'+slot)<<" = "<<inet_ntoa(a)<<":"<<ports[slot]<<" connected \n";
        return ;
    };

    while(true){
        read_fds=master;
        struct timeval tv;
        tv.tv_sec=1;
        tv.tv_usec=0;
        if(select(fdmax+1,&read_fds,NULL,NULL,&tv)==-1){
            if(errno==EINTR) continue;
            perror("select");
            return 1;
        }

        for(int i=0;i<=fdmax;i++){
            if(!FD_ISSET(i,&read_fds)) continue;
            if(i==socfd){ // new connection 
                len=sizeof(their_addr);
                if((new_fd=accept(socfd,(struct sockaddr*)&their_addr,&len))==-1){
                    perror("accept");
                }else if(new_fd>=FD_SETSIZE){
                    cerr<<"too many connections\n";
                    close(new_fd);
                }else{
                    int slot=first_free_ind();
                    if(slot>=0){
                        assign(new_fd,slot);
                    }else{
                        close(new_fd); 
                        cout<<"Extra Node detected \n";
                    }
                }
            }else{
                char some[256];
                int nbytes=recv(i,some,sizeof(some),0);
                if(nbytes<=0){
                    int slot=slot_of_fd[i];
                    close(i);
                    FD_CLR(i,&master);
                    slot_of_fd[i]=-1;
                    if(slot>=0){
                        cout<<"ON : node "<<char('A'+slot)<<" disconnected \n";
                        VN_fd[slot]=-1;
                    }
                }
            }
        }
        long long prev_time;
        struct stat st;
        if(stat(file.c_str(),&st)<0) prev_time=0;
        else prev_time= (long long)st.st_mtim.tv_sec*1000000000LL + st.st_mtim.tv_nsec;
        bool is_new=false;
        if(prev_time!=0 && prev_time != last_time){
            last_time = prev_time;
            vector<vector<int>> newmat=configfile(file);
            if(newmat.empty() || (int)newmat.size()!=n ){
                cerr<<"ON : invalid file\n";
            }else{
                is_new=true;
                costmat=newmat;
                cout<<"ON: Configuration changed : "<<endl;
                for(auto v:costmat){
                    for(auto x:v)cout<<x<<" ";
                    cout<<endl;
                }
            }
        }
        if((first_file ||is_new) && (connections_cnt==n)){
            for(int i=0;i<n;i++){
                send_link_state( VN_fd[i],i,costmat,ports,ip);
            }
            first_file=false;
            cout<<"LINK STATE sent to all \n";
        }
    } 
}