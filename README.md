How to Run
Compile both server and client:

gcc tcp.c -o server
gcc client.c -o client $(pkg-config --cflags --libs libnotify)

Start the server:
./server

Run the client with host and watch path:
./client 127.0.0.1 /path/to/monitor
