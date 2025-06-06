// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "protocol.h"

#define PORT 4953
#define MAX_CLIENT 100

typedef struct 
{
    char username[USERNAME_SIZE];
    char password[PASSWORD_SIZE];
} Account;

Account users[100];
int user_count;

int client_socks[MAX_CLIENT];
char client_usernames[MAX_CLIENT][USERNAME_SIZE];

int load_users(Account users[], int max) 
{
    FILE *fp = fopen("user_db.txt", "r");
    if (!fp) 
    {
        perror("user_db.txt");
        exit(1);
    }
    int count = 0;
    while (fscanf(fp, "%s %s", users[count].username, users[count].password) == 2) 
    {
        count++;
        if (count >= max) break;
    }
    fclose(fp);
    return count;
}

int check_login(Account users[], int count, const char *u, const char *p) 
{
    for (int i = 0; i < count; i++) 
    {
        if (strcmp(users[i].username, u) == 0 && strcmp(users[i].password, p) == 0)
            return 1;
    }
    return 0;
}

void save_log(const char *msg) 
{
    FILE *fp = fopen("chat_log.txt", "a");
    if (fp) 
    {
        fprintf(fp, "%s\n", msg);  
        fclose(fp);
    }
}

void broadcast_message(const char *msg, int except_sock) 
{
    Packet pkt;
    pkt.command = MESSAGE;
    memset(pkt.data, 0, sizeof(pkt.data));
    snprintf(pkt.data, sizeof(pkt.data), "%s", msg);

    for (int i = 0; i < MAX_CLIENT; i++) 
    {
        if (client_socks[i] != 0 && client_socks[i] != except_sock) 
        {
            send(client_socks[i], &pkt, sizeof(pkt), 0);
        }
    }
}

void send_chat_history(int sockfd) 
{
    FILE *fp = fopen("chat_log.txt", "r");
    if (!fp) return;

    char line[USERNAME_SIZE + MAX_MESSAGE_SIZE + 10];
    Packet pkt;
    pkt.command = MESSAGE;

    while (fgets(line, sizeof(line), fp)) 
    {
        line[strcspn(line, "\n")] = 0;  // remove newline
        memset(pkt.data, 0, sizeof(pkt.data));
        snprintf(pkt.data, sizeof(pkt.data), "%s", line);
        send(sockfd, &pkt, sizeof(pkt), 0);
    }

    fclose(fp);
}

int main() 
{
    int listenfd, maxfd, activity;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    fd_set readfds;

    user_count = load_users(users, 100);

    listenfd = socket(AF_INET, SOCK_STREAM, 0); 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    listen(listenfd, MAX_CLIENT);

    printf("Server listening on port %d...\n", PORT);

    // Initial client list
    for (int i = 0; i < MAX_CLIENT; i++) 
    {
        client_socks[i] = 0;
        client_usernames[i][0] = 0;
    }

    while (1) 
    {
        FD_ZERO(&readfds);
        FD_SET(listenfd, &readfds);
        maxfd = listenfd;

        for (int i = 0; i < MAX_CLIENT; i++) 
        {
            if (client_socks[i] > 0) 
            {
                FD_SET(client_socks[i], &readfds);
                if (client_socks[i] > maxfd)
                    maxfd = client_socks[i];
            }
        }

        activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) 
        {
            perror("select");
            continue;
        }

        // New client connect
        if (FD_ISSET(listenfd, &readfds)) 
        {
            int new_sock = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);
            if (new_sock < 0) 
            {
                perror("accept");
                continue;
            }

            // Add new client
            int i;
            for (i = 0; i < MAX_CLIENT; i++) 
            {
                if (client_socks[i] == 0) 
                {
                    client_socks[i] = new_sock;
                    break;
                }
            }
            if (i == MAX_CLIENT) 
            {
                printf("Too many clients\n");
                close(new_sock);
            } else {
                printf("New client connected, socket fd: %d\n", new_sock);
            }
        }

        // Check client send data
        for (int i = 0; i < MAX_CLIENT; i++) 
        {
            int sd = client_socks[i];
            if (sd > 0 && FD_ISSET(sd, &readfds)) 
            {
                Packet pkt;
                int n = recv(sd, &pkt, sizeof(pkt), 0);
                if (n <= 0) 
                {
                    // Client disconnect
                    printf("Client [%s] disconnected.\n", client_usernames[i]);
                    close(sd);
                    client_socks[i] = 0;
                    client_usernames[i][0] = 0;
                } else {
                    if (pkt.command == LOGIN) 
                    {
                        // Get username and password from pkt.data
                        char username[USERNAME_SIZE];
                        strncpy(username, pkt.data, USERNAME_SIZE - 1);
                        username[USERNAME_SIZE - 1] = 0;
                        char *password = pkt.data + USERNAME_SIZE;

                        if (check_login(users, user_count, username, password)) 
                        {
                            Packet res = {.command = LOGIN_SUCCESS};
                            send(sd, &res, sizeof(res), 0);
                            strncpy(client_usernames[i], username, USERNAME_SIZE);
                            printf("User [%s] logged in.\n", username);

                            // Send chat_history to new login client
                            send_chat_history(sd);
                        } else {
                            Packet res = {.command = LOGIN_FAIL};
                            send(sd, &res, sizeof(res), 0);
                            close(sd);
                            client_socks[i] = 0;
                            client_usernames[i][0] = 0;
                        }
                    } else if (pkt.command == MESSAGE) {
                        pkt.data[MAX_MESSAGE_SIZE - 1] = 0;

                        // Create buffer "username: message"
                        char buf[USERNAME_SIZE + MAX_MESSAGE_SIZE + 5];
                        snprintf(buf, sizeof(buf), "[%s]: %s", client_usernames[i], pkt.data);

                        // Save log
                        save_log(buf);

                        // print out server console
                        printf("%s\n", buf);
                        fflush(stdout);

                        // Send broadcast to all other client
                        broadcast_message(buf, sd);
                    }
                }
            }
        }
    }

    close(listenfd);
    return 0;
}
