all : lotto_client lotto_server

#make rule per il client
client : lotto_client.o
	gcc -Wall lotto_client.o -o lotto_client

#make rule per il server
server : lotto_server.o
	gcc -Wall lotto_server.o -o lotto_server

#pulizia
clean:
	rm *o lotto_client lotto_server
