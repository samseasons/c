// clang serve.c -o serve.out && ./serve.out a 1234

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char types[4][2][23] = {
    {"css", "text/css"},
    {"html", "text/html"},
    {"ico", "image/x-icon"},
    {"js", "application/javascript"}
};

void serve (char * folder, int port) {
    int server = socket(AF_INET, SOCK_STREAM, 0);
    int i = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, & i, sizeof(i));
    struct sockaddr_in sock;
    sock.sin_port = htons(port);
    socklen_t len = sizeof(sock);
    bind(server, (struct sockaddr *) & sock, len);
    listen(server, 5);
    printf("localhost:%d\n", port);
    while (1) {
        int client = accept(server, (struct sockaddr *) & sock, & len);
        char buffer[4096];
        read(client, buffer, 4096);
        char * start = strchr(buffer, ' ') + 1;
        int length = strchr(start, ' ') - start;
        char file[length + 1];
        strncpy(file, start, length);
        file[length] = '\0';
        char * p = strstr(file, "%20");
        while (p) {
            memcpy(p, " ", 1);
            memmove(p + 1, p + 3, strlen(p + 3) + 1);
            p = strstr(file, "%20");
        }
        length = strlen(folder) + strlen(file) + 1;
        char * path = malloc(length);
        snprintf(path, length, "%s%s", folder, file);
        char type[23];
        FILE * f = fopen(path, "r");
        if (file[0] != '/' || strcmp(file, "/") == 0 || !f) {
            fclose(f);
            strcpy(type, "text/html");
            length = strlen(folder) + 8;
            path = realloc(path, length);
            snprintf(path, length, "%s%s", folder, "/x.html");
            f = fopen(path, "r");
        } else {
            char * extension = strrchr(file, '.') + 1;
            for (i = 0; types[i][0]; i++) {
                if (strcmp(types[i][0], extension) == 0) {
                    strcpy(type, types[i][1]);
                    break;
                }
            }
        }
        if (f) {
            fseek(f, 0, SEEK_END);
            length = ftell(f);
            rewind(f);
            char * content = malloc(length);
            fread(content, 1, length, f);
            fclose(f);
            length += strlen(type) + 24;
            char * response = malloc(length);
            snprintf(response, length, "HTTP/1.\ncontent-type:%s\n\n%s", type, content);
            send(client, response, length - 1, 0);
        }
        close(client);
    }
}

int main (int argc, char * argv[]) {
    serve(argc > 1 ? argv[1] : "a", argc > 2 ? atoi(argv[2]) : 1234);
    return 0;
}