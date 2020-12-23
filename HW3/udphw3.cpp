// Tanvir Tatla
// CSS 432

#include "udphw3.h"

const int TIMEOUT = 1500;

// clientStopWait is client's side of reliable data transfer
// sends packets and waits for corresponding ack until timeout
// resend packet on timeout
// returns the number of retransmitted packets
int clientStopWait(UdpSocket& sock, const int max, int message[]) {
  int retransmissions = 0; // count retransmissions
  Timer timer;             // timer to check for timeouts

  // transfer message[] max times
  for (int i = 0; i < max; i++) {
    message[0] = i;                                   // message[0] has a sequence #
    sock.sendTo( ( char * )message, MSGSIZE );        // udp message send
    timer.start();                                    // start timer for this message

    // infinite loop to check for ACK
    for(;;) {
      // if there is data to receive
      if (sock.pollRecvFrom() > 0) {
        sock.recvFrom((char *) message, MSGSIZE); // receive data
        // if ACK for current sequence number, then send next packet
        if (message[0] == i) {
          break;
        }
      }

      // if timeout, go back and resend it
      if (timer.lap() > TIMEOUT) {
        retransmissions++;
        i--;
        break;
      }
    }
  }

  return retransmissions;
}

// serverReliable is server's side of reliable data transfer
// waits for packet and sends ack if packet has expected sequence #
// ignores packets with wrong sequence #s
void serverReliable(UdpSocket& sock, const int max, int message[]) {
  // receive message[] max times
  for (int i = 0; i < max; i++) {
    // infinite loop to check if correct message arrived
    for(;;) {
      if (sock.pollRecvFrom() > 0) { // if message arrived
        sock.recvFrom((char *) message, MSGSIZE); // receive message
        int sequence = message[0];                // sequence number
        if (sequence <= i) {                       // if message in order
          sock.ackTo((char *) &sequence, sizeof(sequence)); // send ack
          i = sequence;                                     // synchronize client and server
          break;                                                   // start waiting for next message
        }
      } // keep waiting for message of sequence i if message lost or wrong sequence
    }
  }
}

// clientSlidingWindow is client's side of Go-Back-N
// sends number of packets based on windowSize
// once windowSize packets sent, it starts a timer
// waits for ACK for minimum sequence # in window before sliding window
// resends entire window on timeout
// returns number of retransmitted packets
int clientSlidingWindow(UdpSocket& sock, const int max, int message[], int windowSize) {
  int retransmissions = 0; // count retransmissions
  int minUnacked = 0;      // smallest sequence that hasn't been acked
  int intransit = 0;       // number of packets in transit (unacked)
  Timer timer;             // timer to check for timeouts

  // transfer message[] max times
  for (int i = 0; i < max; i++) {
    // keep sending until number of packets in transit equals the window size
    if (intransit < windowSize) {
      message[0] = i;                                   // message[0] has a sequence #
      sock.sendTo( ( char * )message, MSGSIZE );        // udp message send
      intransit++;
    }

    // if number in transit is equal to window size
    if (intransit == windowSize) {
      timer.start();

      // keep checking for data to receive or timeout
      for(;;) {
        // if there is data to receive
        if (sock.pollRecvFrom() > 0) {
          sock.recvFrom((char *) message, MSGSIZE);       // receive data
          if (message[0] == minUnacked) {                 // if ack is for min unacked packet
            minUnacked++;                                 // increment min unacked
            intransit--;                                  // decrement in transit
            break;                                        // break to send next in sequence
          }
        }

        // if timeout, go back n and resend them
        if (timer.lap() > TIMEOUT) {
          retransmissions += windowSize; // retransmit entire window
          i = minUnacked;                // go back n
          intransit = 0;                 // reset number in transit
          break;                         // break to resend window
        }
      }
    }
  }

  return retransmissions;
}

// serverEarlyRetrans is server's side of GBN
// waits for in order packets and sends ACK
// if out of order, then sends ACK for most recent in order packet
void serverEarlyRetrans(UdpSocket& sock, const int max, int message[], int windowSize) {
  int recent = -1; // most recently acked message

  for (int i = 0; i < max; i++) {
    for(;;) {
      if (sock.pollRecvFrom() > 0) { // if message arrived
        sock.recvFrom((char *) message, MSGSIZE);           // receive message
        int sequence = message[0];                          // sequence number

        if (sequence == i) {                                // if message in order
          i = sequence;                                       // synchronize with client
          recent = i;                                         // update most recently acked message
        }
        
        sock.ackTo((char *) &recent, sizeof(recent)); // send ack for recent

        if (recent == i) {                            // if recent is equal to expected sequence
          break;                                        // break to wait for next sequence
        }
      }
    }
  }
}