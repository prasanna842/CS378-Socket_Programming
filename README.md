# CS378-Socket_Programming

## Team
- Vidith H (24B1095)
- Sai Prasanna J (24B1044)

## Languages
- Oracle Node (ON) in C++
- Virtual Node (VN) in C

## Build
```
g++ ON.cpp -o ON
gcc VN.c -o VN
```

## Run
Oracle Node (listens on TCP port 5000):
```
./ON config.txt
```

Virtual Nodes, one per terminal, started in order (A, B, C, ...):
```
./VN <ON_IP> <MY_UDP_PORT> <MY_IP>
```
