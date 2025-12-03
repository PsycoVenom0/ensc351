#pragma once
/**
 * udpServer.h
 *
 * Simple UDP command server.
 * Listens on port 12345 and responds to commands:
 *   help, ?, count, length, dips, history, stop, <enter> (repeat last)
 *
 * Handles all networking in its own thread.
 */

void UDPServer_init(void);
void UDPServer_cleanup(void);
int  UDPServer_isRunning(void);
