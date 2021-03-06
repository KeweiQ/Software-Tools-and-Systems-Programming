#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#ifndef PORT
  #define PORT 30000
#endif
#define BUF_SIZE 128

int main(void) {
    // Read username from stdin
    char buf[BUF_SIZE + 1];
    int num_read = read(STDIN_FILENO, buf, BUF_SIZE);
    if (num_read == -1) {
        perror("read");
        exit(0);
    }
    buf[num_read] = '\0';

    // Create the socket FD.
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("client: socket");
        exit(1);
    }

    // Set the IP and port of the server to connect to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) < 1) {
        perror("client: inet_pton");
        close(sock_fd);
        exit(1);
    }

    // Connect to the server.
    if (connect(sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("client: connect");
        close(sock_fd);
        exit(1);
    }

    // Write username into socket
    int num_written = write(sock_fd, buf, num_read);
    if (num_written != num_read) {
        perror("client: write");
        close(sock_fd);
        exit(1);
    }

    // Read input from the user, send it to the server, and then accept the
    // echo that returns. Exit when stdin is closed.
    // First set up for select
    int max_fd = sock_fd;
    if (STDIN_FILENO > sock_fd) {
        max_fd = STDIN_FILENO;
    }
    fd_set all_fds;
    FD_ZERO(&all_fds);
    FD_SET(sock_fd, &all_fds);
    FD_SET(STDIN_FILENO, &all_fds);

    while (1) {
        // Update the set of ready fds using select
        fd_set listen_fds = all_fds;
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (nready == -1) {
            perror("server: select");
            exit(1);
        }

        int num_read;
        // If stdin is ready them read the value and write it into socket
        if (FD_ISSET(STDIN_FILENO, &listen_fds)) {
            int num_read = read(STDIN_FILENO, buf, BUF_SIZE);
            if (num_read == 0) {
                break;
            }
            buf[num_read] = '\0';

            int num_written = write(sock_fd, buf, num_read);
            if (num_written != num_read) {
                perror("client: write");
                close(sock_fd);
                exit(1);
            }
        }

        // If server socket is ready them read and print the value to stdout
        if (FD_ISSET(sock_fd, &listen_fds)) {
            num_read = read(sock_fd, buf, BUF_SIZE);
            buf[num_read] = '\0';
            printf("Received from server: %s", buf);
        }
    }

    close(sock_fd);
    return 0;
}
