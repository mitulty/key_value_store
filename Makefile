CFLAGS?=-g -Wall -Wno-unused-value -Wno-unused-result
all: client_file_read client_terminal_read server
server:  lfucache.o server.o kvstore.o myqueue.o
		$(CC) $(CFLAGS) lfucache.o server.o myqueue.o kvstore.o -o server -lpthread
client_terminal_read: kvclientlibrary.o client_terminal_read.o
		$(CC) $(CFLAGS) kvclientlibrary.o client_terminal_read.o -o client_terminal_read
client_file_read: kvclientlibrary.o client_file_read.o
		$(CC) $(CFLAGS) kvclientlibrary.o client_file_read.o -o client_file_read
lfucache.o: lfucache.c
		$(CC) $(CFLAGS) -c lfucache.c
kvclientlibrary.o: kvclientlibrary.c
		$(CC) $(CFLAGS) -c kvclientlibrary.c
client_terminal_read.o: client_terminal_read.c server.conf
		$(CC) $(CFLAGS) -c client_terminal_read.c
client_file_read.o: client_file_read.c server.conf
		$(CC) $(CFLAGS) -c client_file_read.c
server.o: server.c server.conf
		$(CC) $(CFLAGS) -c server.c -lpthread
kvstore.o: kvstore.c
		$(CC) $(CFLAGS) -c kvstore.c -lpthread
myqueue.o: myqueue.c 
		$(CC) $(CFLAGS) -c myqueue.c 
clean:
		$(RM) -rf *.o server client_file_read client_terminal_read *.log
empty:
		$(RM) -rf ./db/* ./inp_files/* ./op_files/*
