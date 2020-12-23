#ifndef _UDPHW3CASE4_H_
#define _UDPHW3CASE4_H_

#include "UdpSocket.h"
#include "Timer.h"
#include "vector"
#include <cstdlib>

int clientStopWait(UdpSocket& sock, const int max, int message[]);
void serverReliable(UdpSocket& sock, const int max, int message[]);
int clientSlidingWindow(UdpSocket& sock, const int max, int message[], int windowSize);
void serverEarlyRetrans(UdpSocket& sock, const int max, int message[], int windowSize);
void serverEarlyRetrans(UdpSocket& sock, const int max, int message[], int windowSize, int dropPercentage);

#endif