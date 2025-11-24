#include "udpServer.h"
#include "beatbox.h"
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PORT 12345
#define MAX_LEN 1024

static pthread_t udpThreadId;
static int sockfd;
static struct sockaddr_in servaddr, cliaddr;
static int running = 0;

static void* udpThread(void* arg) {
    (void)arg; // Fix: Suppress unused parameter warning
    char buffer[MAX_LEN];
    socklen_t len = sizeof(cliaddr);

    while (running) {
        int n = recvfrom(sockfd, buffer, MAX_LEN, 0, (struct sockaddr *)&cliaddr, &len);
        if (n > 0) {
            buffer[n] = '\0';
            char reply[MAX_LEN] = "Unknown command";

            // Parse Command
            if (strncmp(buffer, "mode", 4) == 0) {
                if (strlen(buffer) > 5) { // Set mode
                    int val = atoi(buffer + 5);
                    Beatbox_setMode(val);
                }
                sprintf(reply, "mode %d", Beatbox_getMode());
            } 
            else if (strncmp(buffer, "volume", 6) == 0) {
                if (strlen(buffer) > 7) {
                    int val = atoi(buffer + 7);
                    Beatbox_setVolume(val);
                }
                sprintf(reply, "volume %d", Beatbox_getVolume());
            }
            else if (strncmp(buffer, "tempo", 5) == 0) {
                if (strlen(buffer) > 6) {
                    int val = atoi(buffer + 6);
                    Beatbox_setBPM(val);
                }
                sprintf(reply, "tempo %d", Beatbox_getBPM());
            }
            else if (strncmp(buffer, "play", 4) == 0) {
                int sound = atoi(buffer + 5);
                Beatbox_playSound(sound);
                strcpy(reply, "played");
            }
            else if (strncmp(buffer, "stop", 4) == 0) {
                strcpy(reply, "stopping");
            }

            sendto(sockfd, reply, strlen(reply), 0, (struct sockaddr *)&cliaddr, len);
        }
    }
    return NULL;
}

void UDP_init(void) {
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    running = 1;
    pthread_create(&udpThreadId, NULL, udpThread, NULL);
}

void UDP_cleanup(void) {
    running = 0;
    pthread_cancel(udpThreadId);
    pthread_join(udpThreadId, NULL);
    close(sockfd);
}