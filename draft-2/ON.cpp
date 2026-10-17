#include<bits/stdc++.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
using namespace std;

vector<vector<int>> configfile(string &file){
    ifstream fin(file);
    if(!fin){
        cerr<<"unable to open file\n";return{};
    }

    vector<vector<int>> dummy;
    string line;
    while(getline(fin,line)){
        if(!line.empty() && line.back()=='\r')line.pop_back();
        int st_ind=line.find_first_not_of(" \t");
        if(st_ind==string::npos)continue;
        if(line[st_ind]=='#')continue;
        istringstream ss(line.substr(st_ind));
        vector<int> row;
        int x;
        while(ss>>x){
            if( (x!=-1) && (x<=0) ){cerr<<"Invalid cost\n";return{};}
            row.push_back(x);
        }
        dummy.push_back(row);
    }
    int n=dummy.size();
    if(n==0){
        cerr<<"Empty config file\n";
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
    send(clfd,to_send.c_str(),to_send.size(),0);
}
int main(int argc, char* argv[]){
    if(argc<=1){
        cerr<<"give config file";
        return 1;
    }
    string file=argv[1];
    auto costmat=configfile(file);
    int n=costmat.size();
    for(auto v:costmat){
        for(auto x:v)cout<<x<<" ";
        cout<<endl;
    }

    vector<int> VNs(n,-1);
    vector<uint32_t> ip(n);
    vector<int> ports(n);

    int socfd,new_fd;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    socklen_t len;
    socfd=socket(AF_INET,SOCK_STREAM,0);
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(5000);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(socfd,(struct sockaddr*)&my_addr, sizeof(my_addr))<0){
        perror("bind");
        return 1;
    }
    if(listen(socfd, 10)<0){
        perror("listen");
        return 1;
    };

    for(int i=0;i<n;i++){
        int cl=accept(socfd,nullptr,nullptr);
        char buf[7];
        recv(cl,buf,7,0);
        memcpy(&ip[i],buf+1,4);
        uint16_t p;
        memcpy(&p,buf+5,2);
        ports[i]=ntohs(p);
        VNs[i]=cl;
    }
    for(int i=0; i<n;i++){
        send_link_state(VNs[i],i,costmat,ports,ip);
        close(VNs[i]);
    }
    close(socfd);
}