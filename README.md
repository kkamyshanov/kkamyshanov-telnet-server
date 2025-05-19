# kkamyshanov-telnet-server
Implementation of the test assignment for the position of C/C++ Software Engineer (Applied Software Development)

## Build
```
g++ -std=c++17 -Wall -Wextra -o telnet_server main.cpp tlnt.cpp parser.cpp gc.cpp
```
OR
```
mkdir build && cd build
cmake ..
make
```

## Server
```
./telnet_server
```
Ctrl + C - Close the Server

## Client
```
stty raw -echo
nc 127.0.0.1 2323
```
Ctrl + C/Ctrl + D - Close the Client
