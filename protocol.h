// protocol.h
#ifndef PROTOCOL_H
#define PROTOCOL_H

#define USERNAME_SIZE 32
#define PASSWORD_SIZE 32
#define MAX_MESSAGE_SIZE 512


typedef enum {
    LOGIN = 1,       // Client gui username va pass de dang nhap
    LOGIN_SUCCESS,   // Server tra loi dang nhap thanh cong
    LOGIN_FAIL,      // Server tra loi dang nhap that bai
    MESSAGE          
} Command;

// Cau truc goi tin
typedef struct {
    int command;                // Loai lenh
    char data[MAX_MESSAGE_SIZE];// Data (username+password hoac tin nhan)
} Packet;

#endif

