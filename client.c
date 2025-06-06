//client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "protocol.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888

char current_user[USERNAME_SIZE];

void login(int sockfd) {
    Packet pkt;
    pkt.command = LOGIN;

    char username[USERNAME_SIZE];
    char password[PASSWORD_SIZE];

    printf("Username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;
    
    strncpy(current_user, username, USERNAME_SIZE);
    
    printf("Password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = 0;

    memset(pkt.data, 0, sizeof(pkt.data));
    strncpy(pkt.data, username, USERNAME_SIZE - 1);
    strncpy(pkt.data + USERNAME_SIZE, password, PASSWORD_SIZE - 1);

    send(sockfd, &pkt, sizeof(pkt), 0);

    Packet res;
    int n = recv(sockfd, &res, sizeof(res), 0);
    if (n <= 0 || res.command == LOGIN_FAIL) {
        printf("Login failed!\n");
        exit(1);
    }

    printf("Login successful!\n");
}

int main() {
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    login(sockfd);

    fd_set read_fds;
    char input_buf[MAX_MESSAGE_SIZE];

    printf("=== Chat started (type /quit to exit) ===\n");

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        int maxfd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

        int activity = select(maxfd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(sockfd, &read_fds)) {
            Packet pkt;
            int n = recv(sockfd, &pkt, sizeof(pkt), 0);
            if (n <= 0) {
                printf("Disconnected from server.\n");
                break;
            }

            if (pkt.command == MESSAGE) {
                pkt.data[MAX_MESSAGE_SIZE - 1] = 0; // đảm bảo chuỗi kết thúc
                
		char sender[USERNAME_SIZE];
		sscanf(pkt.data, "%31[^:]", sender);

		if (strcmp(sender, current_user) == 0)
		{
			int term_width = 80;
			int len = strlen(pkt.data);
			int space = term_width - len;
			if(space < 0) space = 0;
			printf("%*s\n", space + len, pkt.data);
		}else {
			
                       printf("%s\n", pkt.data); // đã có sẵn dấu newline khi lưu/truyền
		}
		fflush(stdout);
            }
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (!fgets(input_buf, sizeof(input_buf), stdin)) {
                printf("Input error.\n");
                break;
            }
            input_buf[strcspn(input_buf, "\n")] = 0;

            if (strcmp(input_buf, "/quit") == 0) {
                printf("Exiting...\n");
                break;
            }

            Packet pkt;
            pkt.command = MESSAGE;
            memset(pkt.data, 0, sizeof(pkt.data));
            // Đảm bảo gửi kèm dấu newline để hiển thị đẹp khi server broadcast
            snprintf(pkt.data, MAX_MESSAGE_SIZE, " [%s] \n", input_buf);
            send(sockfd, &pkt, sizeof(pkt), 0);
        }
    }

    close(sockfd);
    return 0;
}

