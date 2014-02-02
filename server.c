/*
   Client-Server, Client sends a number, the server sends back the double.
*/

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "commons.h"

#define MAX 100
#define MAX_LISTEN_QUEUE 10
#define PORT 1571
#define HOST_ADDR "127.0.0.1"
#define BUFFER_SIZE 64*1024


struct param {
   int sock;
   struct sockaddr_in* addr;
};
typedef struct param param;
  
int tcpSocket;
int keepAlive=1;
pthread_t thr[MAX];

void handleClient(param* p) {
  
   printf("Client connected!:\n");
  
   unsigned int size;
   char* song;
   
   safeRecv(p->sock, &size, sizeof(int), 0);
   
   size = ntohl(size);  
   song = malloc(size+1);
   safeRecv(p->sock, song, size+1, 0);
   
   printf("Requested song:%s \n", song);
   
   int fd = 0;
   if ((fd = open(song, O_RDONLY)) > 0) {
      free(song);
      struct stat buf;          

      fstat(fd, &buf);
      off_t size = htonl(buf.st_size);
      
      printf("Song has size of:%lu\n", buf.st_size);
      
      safeSend(p->sock, &size, sizeof(off_t), 0);
     
      char buffer[BUFFER_SIZE];
      size_t total = 0; 
      int bufferTotal = 0;
      do {
		
	//Fill the buffer 
	bufferTotal = 0;
	int currentRead = 0;
	while(bufferTotal < BUFFER_SIZE) {
	  currentRead = read (fd, buffer + bufferTotal, BUFFER_SIZE - bufferTotal); 			  
	  if (currentRead < 0) {
	      perror("Could not read file:");
	      break;
	  }	  
	  if (currentRead == 0) {
	      bufferTotal = 0;
	      break;
	  }
	  bufferTotal += currentRead; 
	}

	total += bufferTotal; 
	
	safeSend(p->sock, buffer, bufferTotal, 0);	
	
	printf("Sent so far: %lu out of %lu\n", total, buf.st_size);
      } while (bufferTotal > 0);             
   } else {
     perror("Could not open song:");
   }
   
   size = htonl(size);   
   
   safeSend(p->sock, &size, sizeof(int), 0);   
   free(p);
}

void closeServer() {
   close(tcpSocket);
   keepAlive=0;   
}

int main() {
   
   int sock;
   unsigned int len;
   struct sockaddr_in addr;
   struct sockaddr_in caddr;
   
   signal(SIGINT, closeServer);
   
   tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
     
   addr.sin_family = AF_INET;
   addr.sin_port = htons(PORT);
   addr.sin_addr.s_addr = inet_addr(HOST_ADDR);
   
   if(bind(tcpSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
     perror("ERROR: bind failed");
   }
   
   if(listen(tcpSocket, MAX_LISTEN_QUEUE) < 0) {
     perror("Listen failed!");
   }
   
   param* p;
   int threadcount=0;
   len = 0;
   
   while (keepAlive) {
      sock = accept(tcpSocket, (struct sockaddr*)&caddr, &len);
            
      p = malloc(sizeof(struct param));
      p->sock = sock;
      p->addr = &caddr;
   
      pthread_create(&(thr[threadcount]), NULL, (void*) handleClient, (void*) p);      
      threadcount++;
   }      
   
   int i;
   for(i=0;i<threadcount;i++) {
      pthread_join(thr[i],NULL);
   }
      
   return 0;
}
