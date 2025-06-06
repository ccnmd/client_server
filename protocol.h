// protocol.h
#ifndef PROTOCOL_H
#define PROTOCOL_H

#define USERNAME_SIZE 32
#define PASSWORD_SIZE 32
#define MAX_MESSAGE_SIZE 512

// Các lệnh/command trong gói tin
typedef enum {
    LOGIN = 1,       // Client gửi username+password đăng nhập
    LOGIN_SUCCESS,   // Server trả lời đăng nhập thành công
    LOGIN_FAIL,      // Server trả lời đăng nhập thất bại
    MESSAGE          // Tin nhắn chat
} Command;

// Cấu trúc gói tin đơn giản
typedef struct {
    int command;                // Loại lệnh
    char data[MAX_MESSAGE_SIZE];// Dữ liệu (username+password hoặc tin nhắn)
} Packet;

#endif

