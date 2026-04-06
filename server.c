#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <string.h>
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

int write_global(const void *buf, size_t count, fd_set *read_fds, int max_fd, int server_socket) {
    for (int i = 0; i <= max_fd; i++) {
        if (FD_ISSET(i, read_fds)) {
            if (i == server_socket) {
                continue;
            } else {
                if (write_all(i, buf, count) == -1) {
                    return -1;
                }
            }
        }
    }
    return 1;
}

int main() {
    int MAX_ITERATIONS = 10;

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket creation failed");
        exit(1);
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(1);
    }

    if (listen(server_socket, 3) == -1) {
        perror("listen failed");
        exit(1);
    }

    printf("Server is listening on port %d\n", PORT);

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(server_socket, &read_fds);
    int max_fd = server_socket;
    int client_count = 0;

    float global_weight = 0;
    float global_bias = 0;
    int global_iteration = 0;

    while (client_count < 3) {
        fd_set tmp_fds = read_fds;
        if (select(max_fd + 1, &tmp_fds, NULL, NULL, NULL) == -1) {
            perror("select failed");
            continue;
        }

        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == server_socket) {
                    int client_soc = accept(server_socket, NULL, NULL);
                    if (client_soc != -1) {
                        FD_SET(client_soc, &read_fds);
                        client_count++;
                        if (client_soc > max_fd) {
                            max_fd = client_soc;
                        }
                        printf("New client connected: %d\n", client_soc);
                    }
                }
            }
        }
    }

    for (; global_iteration < MAX_ITERATIONS; global_iteration++) {
        model_t new_model = {
            .weight = global_weight,
            .bias = global_bias,
        };
        
        write_global(&new_model, sizeof(model_t), &read_fds, max_fd, server_socket);
        printf("Weight: %f, Bias: %f, Iteration: %d\n", global_weight, global_bias, global_iteration);

        int worker_responses = 0;
        float delta_weight_sum = 0;
        float delta_bias_sum = 0;
        
        while (worker_responses < client_count && client_count > 0) {
            fd_set tmp_fds = read_fds;
            if (select(max_fd + 1, &tmp_fds, NULL, NULL, NULL) == -1) {
                perror("select");
                break;
            } else {
                for(int i = 0; i <= max_fd; i++) {
                    if (FD_ISSET(i, &tmp_fds) && i != server_socket) {
                        model_t gradient;
                        int status = read_all(i, &gradient, sizeof(model_t));
                        if (status == -1) {
                            perror("read failed");
                            continue;
                        } else if (status == 0) {
                            close(i);
                            FD_CLR(i, &read_fds);
                            client_count--;
                        }
                        else {
                            delta_weight_sum += gradient.weight;
                            delta_bias_sum += gradient.bias;
                            worker_responses++;
                        }
                    }
                }
            }
        }

        if (client_count > 0 && worker_responses == client_count) {
            global_weight += delta_weight_sum / (float) client_count;
            global_bias += delta_bias_sum / (float) client_count;
        } else if (client_count == 0) {
            break;
        }
    }
    
    model_t final_model = {
        .weight = global_weight,
        .bias = global_bias,
    };

    for (int i = 0; i <= max_fd; i++) {
        if (FD_ISSET(i, &read_fds)) {
            if (i != server_socket) {
                write_all(i, &final_model, sizeof(model_t));
                close(i);
            }
        }
    }

    // Training complete

    // Getting user data
    
    
    char input_buf[256];
    while (1) {
        printf("Enter square footage (or 'q' to quit): ");
        if (fgets(input_buf, sizeof(input_buf), stdin) == NULL) {
            break;
        }

        // Remove trailing newline
        input_buf[strcspn(input_buf, "\n")] = '\0';

        if (strcmp(input_buf, "q") == 0) {
            // The user has quit
            break;
        }

        char *endptr;
        float sqft = strtof(input_buf, &endptr);

        // Error checking conversion
        if (endptr == input_buf || *endptr != '\0') {
            printf("Invalid input. Please enter a valid number or 'q'.\n");
        } else {
            float predicted_weight = (sqft * global_weight) + global_bias;
            printf("Predicted weight: %f\n", predicted_weight);
        }
    }

    close(server_socket);
    return 0;
}