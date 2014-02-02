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

#include "fmodex/fmod.h"
#include "fmodex/fmod_errors.h"
#include "fmodex/fmodlinux.h"
#include "fmodex/fmod_output.h"
#include "fmodex/fmod_codec.h"

#define BUFFER_SIZE 1*1024*1024

FMOD_SYSTEM *fmodsystem;
FMOD_SOUND *sound1;
FMOD_CHANNEL *channel = 0;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

int keepPlaying = 1;
int keepReading = 1;
char* buffer;
const char* playBuffer = 0;

char * nextOp = "read";

void ERRCHECK(FMOD_RESULT result) {
   if (result != FMOD_OK) {
       printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
       exit(-1);
   }
}

void closeServer() {
   keepPlaying = 0; 
   keepReading = 0; 
   FMOD_RESULT result = FMOD_OK;
   
   if (sound1) {
    result = FMOD_Sound_Release(sound1);
    ERRCHECK(result);  
   }
  
   result = FMOD_System_Close(fmodsystem);
   ERRCHECK(result);
   result = FMOD_System_Release(fmodsystem);
}

void playSong(char** v) {
  FMOD_RESULT result = FMOD_System_Create(&fmodsystem);
  ERRCHECK(result);
    
  result = FMOD_System_Init(fmodsystem, 32, FMOD_INIT_NORMAL, NULL);
  ERRCHECK(result);
     
  //FMOD_System_SetFileSystem(fmodsystem, 0, 0, 0, 0, 0, 0, 1);
  
  //Free the previous buffer;
  //free((void *)playBuffer);
  //const char* playBuffer = malloc(BUFFER_SIZE);
  //memcpy((void*)playBuffer, (void*)buffer, BUFFER_SIZE);   

  while(keepPlaying) {    
      pthread_mutex_lock(&mtx);
	      
      if (strcmp(nextOp, "play") != 0) {      
	  pthread_mutex_unlock(&mtx);
	  continue;
      }       
      
      FMOD_CREATESOUNDEXINFO info;
      memset(&info, 0, sizeof(info));
      info.length = BUFFER_SIZE;
      info.cbsize = sizeof(info);
	
      result = FMOD_System_CreateSound(fmodsystem, (const char*) buffer, FMOD_OPENMEMORY | FMOD_HARDWARE, &info, &sound1);
      ERRCHECK(result);
	
      unsigned int lenms = 0;
      result = FMOD_Sound_GetLength(sound1, &lenms, FMOD_TIMEUNIT_MS);
      ERRCHECK(result);

      result = FMOD_Sound_SetMode(sound1, FMOD_LOOP_OFF);
      ERRCHECK(result);   
	    
      result = FMOD_System_PlaySound(fmodsystem, FMOD_CHANNEL_FREE, sound1, 0, &channel);
      ERRCHECK(result);          
	  
      FMOD_System_Update(fmodsystem);      
	  
      unsigned int ms = 0;
      int startedReading = 0;
      while (ms < lenms) {
	  result = FMOD_Channel_GetPosition(channel, &ms, FMOD_TIMEUNIT_MS);
	  ERRCHECK(result);           
	  if (!startedReading && ms > (lenms / 2)) {
	      nextOp = "read";   
	      pthread_mutex_unlock(&mtx);
	      startedReading = 1;
	  }                     
      }         
  } //keep playing   
}

void readSong(char** v) {     
   //FILE *fp = fopen(v[1], "rb");
   //fseek(fp, 0, SEEK_END);
   //int length = ftell(fp);   
  
   int fd = open(v[1], O_RDONLY);
   if (fd < 0) {
       perror("Could not open fifo!");	     
   }
   
   while (keepReading) {
       pthread_mutex_lock(&mtx);
              
       if (strcmp(nextOp, "read") != 0) {      
	  pthread_mutex_unlock(&mtx);
	  continue;
       }  
       
       size_t bytes_read = 0;
       int currentRead = 0;
       
       while (bytes_read < BUFFER_SIZE) {
           currentRead = read(fd, buffer+bytes_read, BUFFER_SIZE - bytes_read);
	   if (currentRead < 0) {
               perror("Read failed:");
	   }
	   
	   if (currentRead == 0) {
	       printf("End of file!");	        	       
	       break;
	   }	   
	   
	   bytes_read += currentRead;
	   printf("Partial read: %lu remaining: %lu\n", bytes_read, BUFFER_SIZE - bytes_read);
       }
       printf("Read bytes:%lu\n", bytes_read);       
       nextOp = "play";
       pthread_mutex_unlock(&mtx);       
   }

}

int main (int argc, char** argv)
{   
   if (argc != 2) {
      printf("Incorrect usage, specify file to play as an argument!\n");
      return 1;
   }
  
   signal(SIGINT, closeServer);   
        
   buffer = malloc(BUFFER_SIZE);

   pthread_t readerThread;
   pthread_create(&readerThread, NULL, (void*) readSong, (void*) argv);      
   
   pthread_t playerThread;
   pthread_create(&playerThread, NULL, (void*) playSong, (void*) argv);      
   
   pthread_join(readerThread, NULL);
   pthread_join(playerThread, NULL);
   
   free(buffer);
   
   return 0;
}