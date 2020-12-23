#ifndef _UDPHW3_H_
#define _UDPHW3_H_

#include "UdpSocket.h"
#include "Timer.h"
#include "vector"

int clientStopWait(UdpSocket& sock, const int max, int message[]);
void serverReliable(UdpSocket& sock, const int max, int message[]);
int clientSlidingWindow(UdpSocket& sock, const int max, int message[], int windowSize);
void serverEarlyRetrans(UdpSocket& sock, const int max, int message[], int windowSize);

#endif