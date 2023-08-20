#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE 1024
#define MAXBUF 1024

int sendall(int socket, const void *buffer, size_t length, int flags);
int recvall(int socket, void *buffer, size_t length, int flags);
int send_file(int connfd, const char *filename);
int recv_file(int connfd, const char *filename);
int list_files(int connfd);
int is_user_logged_in(int client_sock);
void handle_port(int connfd, char *arg);
void handle_cwd(char *foldername, int client_socket);
void handle_connection(int client_sock);

#define MAX_CLIENTS 5
#define BUFFERSIZE 256

struct user
{
    char username[50];
    char password[50];
    int logged_in;
    int client_socket;
    int data_socket;
};

struct user users[] = {{"a", "a", 0, -1, 0}};

int main(int argc, char *argv[])
{

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1)
    {
        perror("socket failed");
        return 1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(21);

    if (bind(server_sock, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        perror("bind failed");
        return 1;
    }

    if (listen(server_sock, MAX_CLIENTS) == -1)
    {
        perror("listen failed");
        return 1;
    }

    printf("FTP server listening on port 21...\n");

    while (1)
    {
        struct sockaddr_in client;
        int client_size = sizeof(client);
        int client_sock = accept(server_sock, (struct sockaddr *)&client, (socklen_t *)&client_size);

        if (client_sock == -1)
        {
            perror("accept failed");
            continue;
        }

        printf("New client connected: %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

        if (fork() == 0)
        {
            close(server_sock);
            handle_connection(client_sock);
            close(client_sock);
            exit(0);
        }
        else
        {
            close(client_sock);
        }
    }

    close(server_sock);
    return 0;
}

void handle_connection(int client_sock)
{
    char buffer[1024];
    int valread;

    send(client_sock, "220 Welcome to my FTP server\n", 28, 0);

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        valread = recv(client_sock, buffer, sizeof(buffer), 0);

        if (valread == -1)
        {
            perror("recv failed");
            return;
        }
        else if (valread == 0)
        {
            printf("Client disconnected\n");
            for (int i = 0; i < sizeof(users) / sizeof(users[0]); i++)
            {
                if (users[i].client_socket == client_sock)
                {
                    users[i].logged_in = 0;
                    users[i].client_socket = 0;
                    break;
                }
            }
            return;
        }
        else
        {
            printf("Client: %s\n", buffer);
        }

        if (strncmp(buffer, "PORT ", 5) == 0)
        {
            if (is_user_logged_in(client_sock) == 0)
            {
                send(client_sock, "530 User is not authenticated.\r\n", 35, 0);
                continue;
            }

            handle_port(client_sock, buffer + 5);
        }
        else if (strncmp(buffer, "USER ", 5) == 0)
        {
            char *username = buffer + 5;
            int found = 0;
            for (int i = 0; i < sizeof(users) / sizeof(users[0]); i++)
            {
                if (strcmp(users[i].username, username) == 0)
                {
                    found = 1;
                    break;
                }
            }
            if (found == 1)
            {
                send(client_sock, "331 Username OK, need password.\r\n", 33, 0);
            }
            else
            {
                send(client_sock, "530 Invalid username or password.\r\n", 35, 0);
            }
        }
        else if (strncmp(buffer, "PASS ", 5) == 0)
        {
            char *password = buffer + 5;
            for (int i = 0; i < sizeof(users) / sizeof(users[0]); i++)
            {
                if (strcmp(users[i].password, password) == 0)
                {
                    users[i].logged_in = 1;
                    users[i].client_socket = client_sock;
                    break;
                }
            }
            if (is_user_logged_in(client_sock) == 0)
            {
                send(client_sock, "530 Invalid username or password.\r\n", 35, 0);
            }
            else
            {
                send(client_sock, "230 User logged in.\r\n", 21, 0);
            }
        }
        else if (strncmp(buffer, "QUIT", 4) == 0)
        {

            send(client_sock, "221 Goodbye\n", 12, 0);
            for (int i = 0; i < sizeof(users) / sizeof(users[0]); i++)
            {
                if (users[i].client_socket == client_sock)
                {
                    users[i].logged_in = 0;
                    users[i].client_socket = -1;
                    users[i].data_socket = -1;
                    break;
                }
            }
            return;
        }
        else if (strncmp(buffer, "PWD", 3) == 0)
        {
            if (is_user_logged_in(client_sock) == 0)
            {
                send(client_sock, "530 User is not authenticated.\r\n", 35, 0);
                continue;
            }

            char cwd[4096];
            if (getcwd(cwd, 4096) == NULL)
            {
                send(client_sock, "550 Failed to get current directory.\r\n", 38, 0);
                continue;
            }
            char response[5012];
            snprintf(response, 5012, "257 \"%s\"\r\n", cwd);
            sendall(client_sock, response, strlen(response), 0);
            // send(client_sock, response, strlen(response), 0);
        }
        else if (strncmp(buffer, "STOR ", 5) == 0)
        {
            if (is_user_logged_in(client_sock) == 0)
            {
                send(client_sock, "530 User is not authenticated.\r\n", 35, 0);
                continue;
            }
            int data_socket;
            int found = 0;
            for (int i = 0; i < sizeof(users) / sizeof(users[0]); i++)
            {
                if (users[i].client_socket == client_sock)
                {
                    data_socket = users[i].data_socket;
                    found = 1;
                    break;
                }
            }
            // handle STOR command
            if (found == 1)
                recv_file(data_socket, buffer + 5);
        }
        else if (strncmp(buffer, "RETR ", 5) == 0)
        {
            if (is_user_logged_in(client_sock) == 0)
            {
                send(client_sock, "530 User is not authenticated.\r\n", 35, 0);
                continue;
            }
            int data_socket;
            int found = 0;
            for (int i = 0; i < sizeof(users) / sizeof(users[0]); i++)
            {
                if (users[i].client_socket == client_sock)
                {
                    data_socket = users[i].data_socket;
                    found = 1;
                    break;
                }
            }
            // handle RETR command
            if (found == 1)
                send_file(data_socket, buffer + 5);
        }
        else if (strncmp(buffer, "LIST", 4) == 0)
        {
            if (is_user_logged_in(client_sock) == 0)
            {
                send(client_sock, "530 User is not authenticated.\r\n", 35, 0);
                continue;
            }
            int data_socket;
            int found = 0;
            for (int i = 0; i < sizeof(users) / sizeof(users[0]); i++)
            {
                if (users[i].client_socket == client_sock)
                {
                    data_socket = users[i].data_socket;
                    found = 1;
                    break;
                }
            }
            // handle LIST command
            if (found == 1)
                list_files(data_socket);
        }
        else if (strncmp(buffer, "CWD", 3) == 0)
        {
            if (is_user_logged_in(client_sock) == 0)
            {
                send(client_sock, "530 User is not authenticated.\r\n", 35, 0);
                continue;
            }
            int data_socket;
            for (int i = 0; i < sizeof(users) / sizeof(users[0]); i++)
            {
                if (users[i].client_socket == client_sock)
                {
                    data_socket = users[i].data_socket;
                    break;
                }
            }
            char foldername[4096];
            if (sscanf(buffer, "CWD %s", foldername) == 1)
            {
                handle_cwd(foldername, data_socket);
            }
            else
            {
                char resp[] = "501 Syntax error in parameters or arguments.";
                send(client_sock, resp, strlen(resp), 0);
            }
        }
        else
        {
            send(client_sock, "500 Command not supported\n", 26, 0);
        }
    }
}

int sendall(int socket, const void *buffer, size_t length, int flags)
{
    const char *p = buffer;
    while (length > 0)
    {
        int n = send(socket, p, length, flags);
        if (n <= 0)
            return -1;
        p += n;
        length -= n;
    }
    return 0;
}

int recvall(int socket, void *buffer, size_t length, int flags)
{
    char *p = buffer;
    size_t n = length;
    while (n > 0)
    {
        int rc = recv(socket, p, n, flags);
        if (rc < 0)
            return -1;
        if (rc == 0)
            break;
        p += rc;
        n -= rc;
    }
    return length - n;
}

int send_file(int connfd, const char *filename)
{
    FILE *fp;
    char buf[MAXBUF];
    size_t n;
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        return -1;
    }
    while ((n = fread(buf, 1, MAXBUF, fp)) > 0)
    {
        if (sendall(connfd, buf, n, 0) < 0)
        {
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);
    return 0;
}

int recv_file(int connfd, const char *filename)
{
    FILE *fp;
    char buf[MAXBUF];
    size_t n;
    fp = fopen(filename, "w");
    if (fp == NULL)
    {
        return -1;
    }
    while ((n = recvall(connfd, buf, MAXBUF, 0)) > 0)
    {
        if (fwrite(buf, 1, n, fp) != n)
        {
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);
    return 0;
}

int list_files(int connfd)
{
    DIR *dir;
    struct dirent *ent;
    char buf[MAXLINE];
    dir = opendir(".");
    if (dir == NULL)
    {
        return -1;
    }
    while ((ent = readdir(dir)) != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
        {
            continue;
        }
        snprintf(buf, sizeof(buf), "%s", ent->d_name);
        if (sendall(connfd, buf, strlen(buf), 0) < 0)
        {
            closedir(dir);
            return -1;
        }
    }
    closedir(dir);
    return 0;
}

void handle_cwd(char *foldername, int client_socket)
{
    if (chdir(foldername) != 0)
    {
        // error changing directory
        char err_msg[] = "331 Username OK, need password.\r\n";
        send(client_socket, err_msg, strlen(err_msg), 0);
    }
    else
    {
        // successfully changed directory
        char cwd[4096];
        if (getcwd(cwd, sizeof(cwd)) == NULL)
        {
            // error getting current directory path
            char err_msg[] = "550 Failed to get current directory path.";
            send(client_socket, err_msg, strlen(err_msg), 0);
        }
        else
        {
            // send success response to client
            char response[4096];
            sprintf(response, "200 directory changed to %s.", cwd);
            send(client_socket, response, strlen(response), 0);
        }
    }
}

void handle_port(int connfd, char *arg)
{
    // Parse the IP address and port number from the argument
    char *token;
    char ip[16];
    int port, h1, h2, h3, h4, p1, p2;
    token = strtok(arg, ",");
    h1 = atoi(token);
    token = strtok(NULL, ",");
    h2 = atoi(token);
    token = strtok(NULL, ",");
    h3 = atoi(token);
    token = strtok(NULL, ",");
    h4 = atoi(token);
    token = strtok(NULL, ",");
    p1 = atoi(token);
    token = strtok(NULL, ",");
    p2 = atoi(token);
    sprintf(ip, "%d.%d.%d.%d", h1, h2, h3, h4);
    printf("ip: %s\n", ip);
    port = p1 * 256 + p2;

    // Create the data connection socket using the client's IP address and port number
    int datafd;
    struct sockaddr_in clientaddr;
    clientaddr.sin_family = AF_INET;
    clientaddr.sin_addr.s_addr = inet_addr(ip);
    clientaddr.sin_port = htons(port);
    // inet_pton(AF_INET, ip, &clientaddr.sin_addr);
    if ((datafd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket error");
        exit(1);
    }
    printf("data_socket: %d, port: %d\n", datafd, port);
    int b = connect(datafd, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
    printf("b: %d\n", b);
    if (b < 0)
    {
        perror("connect error");
        exit(1);
    }
    printf("after connect()\n");

    // Store the data connection socket
    for (int i = 0; i < sizeof(users) / sizeof(users[0]); i++)
    {
        if (users[i].client_socket == connfd)
        {
            users[i].data_socket = datafd;
            break;
        }
    }

    // Send a response to the client
    char response[] = "200 PORT command successful.\r\n";
    send(connfd, response, strlen(response), 0);
}

int is_user_logged_in(int client_sock)
{
    int isUserLoggedIn = 0;
    for (int i = 0; i < sizeof(users) / sizeof(users[0]); i++)
    {
        if (users[i].client_socket == client_sock)
        {
            isUserLoggedIn = users[i].logged_in;
            break;
        }
    }
    return isUserLoggedIn;
}