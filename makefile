# Makefile for compiling the encryption/decryption programs and keygen

CC = gcc
CFLAGS = -o

all: enc_server enc_client dec_server dec_client keygen

enc_server: enc_server.c
	$(CC) $(CFLAGS) enc_server enc_server.c

enc_client: enc_client.c
	$(CC) $(CFLAGS) enc_client enc_client.c

dec_server: dec_server.c
	$(CC) $(CFLAGS) dec_server dec_server.c

dec_client: dec_client.c
	$(CC) $(CFLAGS) dec_client dec_client.c

keygen: keygen.c
	$(CC) $(CFLAGS) keygen keygen.c

clean:
	rm -f enc_server enc_client dec_server dec_client keygen
