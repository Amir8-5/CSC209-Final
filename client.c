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

model_t compute_gradient(HouseData *data, int n, model_t current) {
    model_t grad = {0.0, 0.0};

    for(int i = 0; i < n; i++) {
        float prediction = current.weight * data[i].sqft + current.bias;
        float error = prediction - data[i].price;
        grad.weight += error * data[i].sqft;
        grad.bias += error;
    }
    grad.weight /= n;
    grad.bias /= n;
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

    int num_points;
    int read_status = read_all(client_socket, &num_points, sizeof(int));
    if (read_status == 0) {
        printf("Server disconnected.\n");
        exit(1);
    } else if (read_status == -1) {
        perror("read_all failed");
        exit(1);
    }
    
    HouseData *my_shard = malloc(sizeof(HouseData) * num_points);
    read_status = read_all(client_socket, my_shard, sizeof(HouseData) * num_points);
    if (read_status == 0) {
        printf("Server disconnected.\n");
        exit(1);
    } else if (read_status == -1) {
        perror("read_all failed");
        exit(1);
    }
    
    printf("Worker ready. Received shard of %d points.\n", num_points);

    model_t server_model;
    
    while (1) {
        int read_status = read_all(client_socket, &server_model, sizeof(model_t));
        
        if (read_status == 0) {
            printf("Server disconnected.\n");
            break;
        } else if (read_status == -1) {
            perror("read_all failed");
            break;
        }

        model_t gradient = compute_gradient(my_shard, num_points, server_model);

        int write_status = write_all(client_socket, &gradient, sizeof(model_t));
        
        if (write_status <= 0) {
            perror("write_all failed or disconnected");
            break;
        }
    }
    
    printf("Training complete. Shutting down worker.\n");
cleanup:
    if (my_shard != NULL) free(my_shard);
    if (client_socket != -1) close(client_socket);
    return 0;
}
