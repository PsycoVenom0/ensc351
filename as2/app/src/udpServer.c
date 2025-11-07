// app/src/udpServer.c
#define _POSIX_C_SOURCE 200809L
#include "udpServer.h"
#include "light_sampler.h"
#include "dipAnalyzer.h"

#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define UDP_PORT 12345
#define MAX_MSG 1024

static pthread_t server_thread;
static int running = 1; // FIX 1: Initialize to 1 to prevent race condition
static int sock_fd = -1;
static char last_cmd[128] = "";

static void send_text(struct sockaddr_in *client, const char *msg)
{
    sendto(sock_fd, msg, strlen(msg), 0, (struct sockaddr *)client, sizeof(*client));
}

// FIX 2: Corrected history function to match assignment guidelines
static void handle_history(struct sockaddr_in *client)
{
    int count = 0;
    double *data = LightSampler_getHistory(&count);
    if (!data || count == 0) {
        send_text(client, "History empty.\n");
        free(data);
        return;
    }

    char buffer[MAX_MSG];
    buffer[0] = '\0';
    
    for (int i = 0; i < count; i++) {
        char entry[32];
        
        // Format with comma, add newline every 10
        snprintf(entry, sizeof(entry), "%.3f", data[i]); //

        if (i % 10 != 9 && i != count - 1) { // Not the 10th item and not the last item
            strcat(entry, ", "); // Add comma separator
        } else { // The 10th item in a row or the very last item
            strcat(entry, "\n"); // Add newline
        }

        // Check if adding this entry will overflow the packet buffer
        if (strlen(buffer) + strlen(entry) >= sizeof(buffer) - 1) {
            send_text(client, buffer); // Send current buffer
            buffer[0] = '\0'; // Clear buffer
        }
        strcat(buffer, entry);
    }
    
    // Send any remaining data
    if (strlen(buffer) > 0)
        send_text(client, buffer);

    free(data);
}

static void *server_func(void *arg)
{
    (void)arg;
    struct sockaddr_in servaddr = {0}, client = {0};
    socklen_t client_len = sizeof(client);

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(UDP_PORT);

    // FIX 1 (continued): Check bind and set running to 0 on failure
    if (bind(sock_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("UDP bind");
        running = 0; // Tell main loop to stop
        return NULL;
    }

    char msg[MAX_MSG];

    while (running) {
        ssize_t n = recvfrom(sock_fd, msg, sizeof(msg) - 1, 0, (struct sockaddr *)&client, &client_len);
        if (n <= 0) continue;

        msg[n] = '\0';
        char *cmd = msg;
        while (*cmd == ' ' || *cmd == '\n' || *cmd == '\r') cmd++;

        // Handle <enter> to repeat last command
        if (*cmd == '\0' || *cmd == '\n') { // Check for empty string or just newline
            if (strlen(last_cmd) == 0) {
                send_text(&client, "Unknown command.\n");
                continue;
            }
            cmd = last_cmd;
        } else {
            // Trim trailing newline
            char *newline = strrchr(msg, '\n');
            if (newline) *newline = '\0';
            newline = strrchr(msg, '\r');
            if (newline) *newline = '\0';
            
            strncpy(last_cmd, cmd, sizeof(last_cmd) - 1);
        }

        for (char *p = cmd; *p; p++)
            if (*p >= 'A' && *p <= 'Z')
                *p = *p - 'A' + 'a';

        if (strncmp(cmd, "help", 4) == 0 || strcmp(cmd, "?") == 0) {
            send_text(&client,
                "Accepted command examples:\n"
                "count - get the total number of samples taken.\n"
                "length - get the number of samples taken in the previously completed second.\n"
                "dips - get the number of dips in the previously completed second.\n"
                "history - get all the samples in the previously completed second.\n"
                "stop - cause the server program to end.\n"
                "<enter> - repeat last command.\n");
        }
        else if (strncmp(cmd, "count", 5) == 0) {
            char out[64];
            snprintf(out, sizeof(out), "#samples taken total: %lld\n", LightSampler_getTotalSamples());
            send_text(&client, out);
        }
        else if (strncmp(cmd, "length", 6) == 0) {
            int n = 0;
            double *data = LightSampler_getHistory(&n); // Use history size for length
            free(data);
            char out[64];
            // The assignment PDF is ambiguous, but sample output implies 'length'
            // is the number of samples in the last second, not total intervals.
            snprintf(out, sizeof(out), "# samples taken last second: %d\n", n);
            send_text(&client, out);
        }
        else if (strncmp(cmd, "dips", 4) == 0) {
            int n = 0;
            double *data = LightSampler_getHistory(&n);
            double avg = LightSampler_getAverage();
            int dips = DipAnalyzer_countDips(data, n, avg);
            free(data);
            char out[64];
            snprintf(out, sizeof(out), "# Dips: %d\n", dips);
            send_text(&client, out);
        }
        else if (strncmp(cmd, "history", 7) == 0) {
            handle_history(&client);
        }
        else if (strncmp(cmd, "stop", 4) == 0) {
            send_text(&client, "Program terminating.\n");
            running = 0;
        }
        else {
            send_text(&client, "Unknown command.\n");
        }
    }

    close(sock_fd);
    sock_fd = -1;
    return NULL;
}

void UDPServer_init(void)
{
    pthread_create(&server_thread, NULL, server_func, NULL);
}

void UDPServer_cleanup(void)
{
    running = 0;
    if (sock_fd >= 0) {
        // Break out of recvfrom()
        struct sockaddr_in self = {0};
        self.sin_family = AF_INET;
        self.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        self.sin_port = htons(UDP_PORT);
        sendto(sock_fd, "\n", 1, 0, (struct sockaddr *)&self, sizeof(self));
    }
    pthread_join(server_thread, NULL);
}

int UDPServer_isRunning(void)
{
    return running;
}