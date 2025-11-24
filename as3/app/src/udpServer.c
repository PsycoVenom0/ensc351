#include "udpServer.h"
#include "audioLogic.h"
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h> // Needed for isdigit()

#define PORT 12345
#define MAX_LEN 1024

static pthread_t udpThreadId;
static int sockfd;
static struct sockaddr_in servaddr, cliaddr;
static int running = 0;

static void* udpThread(void* arg) {
    (void)arg;
    char buffer[MAX_LEN];
    socklen_t len = sizeof(cliaddr);

    while (running) {
        int n = recvfrom(sockfd, buffer, MAX_LEN, 0, (struct sockaddr *)&cliaddr, &len);
        if (n > 0) {
            buffer[n] = '\0';
            char reply[MAX_LEN] = "Unknown command";

            // --- Parse Command ---
            
            // MODE
            if (strncmp(buffer, "mode", 4) == 0) {
                // Check if a value was provided AND it is a digit (handles "mode undefined")
                if (strlen(buffer) > 5 && isdigit(buffer[5])) {
                    int val = atoi(buffer + 5);
                    Beatbox_setMode(val);
                }
                // Reply with ONLY the number (UI expects "1", not "mode 1")
                sprintf(reply, "%d", Beatbox_getMode());
            } 
            
            // VOLUME
            else if (strncmp(buffer, "volume", 6) == 0) {
                // Check for space + digit (handles "volume undefined")
                if (strlen(buffer) > 7 && isdigit(buffer[7])) {
                    int val = atoi(buffer + 7);
                    Beatbox_setVolume(val);
                }
                // Reply with ONLY the number
                sprintf(reply, "%d", Beatbox_getVolume());
            }
            
            // TEMPO
            else if (strncmp(buffer, "tempo", 5) == 0) {
                // Check for space + digit (handles "tempo undefined")
                if (strlen(buffer) > 6 && isdigit(buffer[6])) {
                    int val = atoi(buffer + 6);
                    Beatbox_setBPM(val);
                }
                // Reply with ONLY the number
                sprintf(reply, "%d", Beatbox_getBPM());
            }
            
            // PLAY
            else if (strncmp(buffer, "play", 4) == 0) {
                // "play" always comes with a number from the UI, but safety check anyway
                if (strlen(buffer) > 5 && isdigit(buffer[5])) {
                    int sound = atoi(buffer + 5);
                    Beatbox_playSound(sound);
                }
                strcpy(reply, "played");
            }
            
            // STOP
            else if (strncmp(buffer, "stop", 4) == 0) {
                strcpy(reply, "stopping");
                Beatbox_markStopping(); // Trigger graceful shutdown
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