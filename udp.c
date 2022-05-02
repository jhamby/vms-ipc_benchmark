#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

double
getdetlatimeofday(struct timeval *begin, struct timeval *end)
{
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}

int main(int argc, char *argv[]) {
    int                  fd, yes;
    int                  i, size, count, sum, n;
    char                *buf;
    struct timeval       begin, end;
    struct sockaddr_in   in;

#ifdef __VMS
    static char *child_flag = "----";
    int is_child = 0;
    if (argc == 4 && !strcmp(argv[3], child_flag)) {
        argc--;
        is_child = 1;
    }
#endif

    if (argc != 3) {
        printf("usage: ./udp <size> <count>\n");
        return 1;
    }

    size = atoi(argv[1]);
    count = atoi(argv[2]);
    buf = malloc(size);

    memset(&in, 0, sizeof(in));
#ifdef __VMS
    if (is_child)
#else
    if (fork() == 0)
#endif
    {
        fd = socket(AF_INET, SOCK_DGRAM, 0);

        in.sin_family = AF_INET;
        in.sin_port = htons(15323);
        inet_pton(AF_INET, "127.0.0.1", &in.sin_addr);

        yes = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        
        if (bind(fd, (struct sockaddr *)&in, sizeof(in)) == -1) {
            perror("bind");
            return 1;
        }

        sum = 0;
        for (i = 0; i < count; i++) {
            n = recv(fd, buf, size, 0);
            if (n == 0) {
                break;
            }
            if (n == -1) {
                perror("recv");
                return 1;
            }
            sum += n;
        }
        if (sum != count * size) {
            fprintf(stderr, "sum error: %d != %d\n", sum, count * size);
            return 1;
        }
        close(fd);
    } else {
#ifdef __VMS
        char *argv_copy[5];
        if (vfork() == 0) {
            /* exec myself with child_flag as the 3rd argument */
            for (i = 0; i < 3; i++) {
                argv_copy[i] = argv[i];
            }
            argv_copy[3] = child_flag;
            argv_copy[4] = NULL;
            if (execvp(argv[0], argv_copy)) {
                perror("execvp");
                return 1;
            }
        }
        /* continue as the parent */
#endif
        sleep(1);

        fd = socket(AF_INET, SOCK_DGRAM, 0);

        in.sin_family = AF_INET;
        in.sin_port = htons(15323);
        inet_pton(AF_INET, "127.0.0.1", &in.sin_addr);

        gettimeofday(&begin, NULL);

        for (i = 0; i < count; i++) {
            if (sendto(fd, buf, size, 0, (struct sockaddr *)&in, sizeof(in)) != size) {
                perror("sendto");
                return 1;
            }
        }

        gettimeofday(&end, NULL);

        double tm = getdetlatimeofday(&begin, &end);
        printf("%.0fMB/s %.0fmsg/s\n",
            count * size * 1.0 / (tm * 1024 * 1024),
            count * 1.0 / tm);
    }

    return 0;
}
