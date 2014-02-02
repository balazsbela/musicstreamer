#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>

ssize_t safeSend(int sockfd, const void *buf, size_t len, int flags) {
  ssize_t totalSent = 0;
  ssize_t sent = 0;  
   
  while (totalSent < len) {    
    sent = send(sockfd, buf, len, flags);
    if (sent < 0) {    
      perror("Could not send!\n");    
      exit(1);
    }
    totalSent += sent;
  }
  
  return totalSent;
}

ssize_t safeRecv(int sockfd, void *buf, size_t len, int flags) {
  ssize_t totalReceived = 0;
  ssize_t received = 0;  
  
  while (totalReceived < len) {  
    received = recv(sockfd, buf, len, flags);
    if (received < 0) {
      perror("Could not receive!\n");
      exit(1);
    }
    totalReceived += received;    
  }
  return totalReceived;
}