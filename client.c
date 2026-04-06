#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "protocol.h"

int read_all(int fd, void *buf, size_t count) {
    size_t total_read = 0;
    while (total_read < count) {
        ssize_t bytes_read = read(fd, ((char *)buf) + total_read, count - total_read);
        if (bytes_read == 0) {
            return 0;
        } else if (bytes_read == -1) {
            return -1;
        }
        total_read += bytes_read;
    }
    return 1;
}

int write_all(int fd, const void *buf, size_t count) {
    size_t total_written = 0;
    while (total_written < count) {
        ssize_t bytes_written = write(fd, ((const char *)buf) + total_written, count - total_written);
        if (bytes_written == -1) {
            return -1;
        } else if (bytes_written == 0) {
            return 0;
        }
        total_written += bytes_written;
    }
    return 1;
}

int main() {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("socket creation failed");
        exit(1);
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
    };
    
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        exit(1);
    }

    if (connect(client_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect failed");
        exit(1);
    }

    printf("Connected to server\n");

    model_t server_model;
    model_t hardcoded_model = {
        .weight = 1.0,
        .bias = 0.0,
    };
    
    while (1) {
        int read_status = read_all(client_socket, &server_model, sizeof(model_t));
        
        if (read_status == 0) {
            printf("Server disconnected.\n");
            break;
        } else if (read_status == -1) {
            perror("read_all failed");
            break;
        }

        int write_status = write_all(client_socket, &hardcoded_model, sizeof(model_t));
        
        if (write_status <= 0) {
            perror("write_all failed or disconnected");
            break;
        }
    }

    close(client_socket);
    return 0;
}