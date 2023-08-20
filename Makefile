main_all: main_server main_client

main_server:
	gcc server/server.c -o server/server

main_client:
	gcc client/client.c -o client/client