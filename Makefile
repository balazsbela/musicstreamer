  # the compiler: gcc for C program, define as g++ for C++
  CC = gcc

  # compiler flags:
  #  -g    adds debugging information to the executable file
  #  -Wall turns on most, but not all, compiler warnings
  CFLAGS  = -g -Wall -pthread

  # the build target executable:
  TARGET_SERVER = server
  TARGET_CLIENT = client
  TARGET_PLAYER = player
  
  all: $(TARGET_SERVER) $(TARGET_CLIENT) $(TARGET_PLAYER)

  $(TARGET_SERVER): $(TARGET_SERVER).c
	$(CC) $(CFLAGS) -o $(TARGET_SERVER) $(TARGET_SERVER).c 

  $(TARGET_CLIENT): $(TARGET_CLIENT).c
	$(CC) $(CFLAGS) -o $(TARGET_CLIENT) $(TARGET_CLIENT).c 
	
  $(TARGET_PLAYER): $(TARGET_PLAYER).c
	$(CC) $(CFLAGS) -o $(TARGET_PLAYER) $(TARGET_PLAYER).c -lfmodex
  	
  clean:
	$(RM) $(TARGET_SERVER) $(TARGET_CLIENT) $(TARGET_PLAYER)