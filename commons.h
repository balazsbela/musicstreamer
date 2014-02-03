#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <linux/limits.h>
#include <string.h>


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

int endsWith(const char *str, const char *suffix)
{
    if (!str || !suffix) {
        return 0;
    }    
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    
    if (lensuffix >  lenstr) {
        return 0;
    }
    
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

void findFile(char* name, char* rootDirectory, char** result) {
    DIR *dir;
    struct dirent *ent;
    
    if ((dir = opendir(rootDirectory)) != NULL) {      
	while ((ent = readdir(dir)) != NULL) {
	  	    
	    if ((strcmp(ent->d_name, "..") != 0) && (strcmp(ent->d_name, ".") != 0)) {
		
		if ((strlen(rootDirectory) + strlen(ent->d_name) + 1) > PATH_MAX) {
			printf("Path to long\n");
			return;
		}
		
		char fullpath[PATH_MAX + 1];;
		strcpy(fullpath, rootDirectory);
		strcat(fullpath, "/");
		strcat(fullpath, ent->d_name);
	      
		printf("Currently searching: %s\n", fullpath);
		fflush(stdout);
		
		if(ent->d_type == DT_DIR) {					      
		    findFile(name, fullpath, result);
		} else {
		    
		    printf("Comparing %s to %s \n", ent->d_name, name);
		    if ((strstr(ent->d_name, name) != NULL) && (endsWith(ent->d_name, ".mp3"))) {
			printf("Found file:%s \n", fullpath); 
			closedir (dir);
			*result= malloc(strlen(fullpath)+1);
			memcpy(*result, fullpath, strlen(fullpath)+1);		
			return;
		    }
		}	    
	    }      
	}     
	closedir (dir);
    } else {
	perror("Could not open directory:");
    }
}
