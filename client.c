#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "commons.h"

#define PORT 1571
#define HOST_ADDR "127.0.0.1"
#define BUFFER_SIZE 64*1024
#define FIFO_NAME "songfifo"
#define PLAYER_PROCESS "./player"

//Fifo file descriptor
int fd = 0;

int main(int argc, char** argv) {

   char* song;
   
   if (argc>1) {
      song = argv[1];
   } else {
      printf("Give the song as a parameter\n");
      exit(0);
   }
   
   int sock;
   struct sockaddr_in addr;
      
   sock = socket(AF_INET,SOCK_STREAM,0);
   
   addr.sin_family = AF_INET;
   addr.sin_port = htons(PORT);
   addr.sin_addr.s_addr = inet_addr(HOST_ADDR);
   
   if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
      perror("ERROR: Could not connect");
   }   
   
   int size = htonl(strlen(song));
   
   safeSend(sock, &size, sizeof(int), 0);   
   safeSend(sock, song, strlen(song) + 1, 0);      
   
   off_t fileSize = 0;
   safeRecv(sock, &fileSize, sizeof(fileSize), 0);
   fileSize = ntohl(fileSize);   
   printf("Filesize %lu\n", fileSize);
   
   if (mkfifo(FIFO_NAME, S_IRUSR|S_IWUSR) < 0 ) {   
     perror("Could not create fifo!");
   }
   
   //Now read the file into a buffer and play.
   size_t bytesRead = 0; 
   int playerStarted = 0;
      
   while (bytesRead < fileSize) {
      char buffer[BUFFER_SIZE];
      
      printf("Waiting for data on the socket!\n");
      
      bytesRead += safeRecv(sock, buffer, sizeof(buffer), 0);
      
      printf("Received %lu bytes, writing to fifo!\n", bytesRead);
      
      //Spawn player process
      if (!playerStarted) {
	playerStarted = 1;
	if (fork() == 0) {
	    int outputfd = open("player_output", O_WRONLY|O_CREAT, 0666);
	    dup2(outputfd, 1);
	    close(outputfd);
	    if (execlp(PLAYER_PROCESS, PLAYER_PROCESS, FIFO_NAME, NULL)) {
	      perror("Could not execute player:");
	    }
	}	
      }
      
      fd = open(FIFO_NAME, O_WRONLY);	 
      if (fd < 0) {
	perror("Could not open fifo:");
      }
      
      int totalFifoBytes = 0;
      int currentWrite = 0;
      while (totalFifoBytes < bytesRead) {
	currentWrite += write(fd, buffer + totalFifoBytes, bytesRead - totalFifoBytes);
	if (totalFifoBytes < 0) {
	    perror("Could not write to fifo:");	  
	}
	totalFifoBytes += currentWrite;
	printf("Written to fifo: %d !\n", totalFifoBytes);
      }           
                  
   }
         
   close(fd);        
   close(sock);
   
   wait(0);
   
   return 0;
}
