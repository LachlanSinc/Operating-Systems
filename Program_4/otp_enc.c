#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

//client

//Note: I should have created a header file and had all of the repeated functions included there.
//I had issues with getting 5 processes working correctly (dang zombies)
//and completely ran out of time. Got everything in working order now its time to cram from my other finals, 
//sorry to leave this as somewhat of a mess. 
//Thanks for all the help in slack. 

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

//validate a string to insure it onlu has capital letters andd spaces, returns 1 if the string is good, 0 if not
int validateString(char str[], int length)
{
  int i=0;
  for (i; i < length - 1; i++)
    {
      //if the char isnt upper case or space return 1
      if((str[i] < 65 || str[i] > 90) && str[i] != 32)
        {
          return 1;
        }
    }
  //make sure the null termination was done correctly
  if(str[length-1] != '\0')
    {
      return 1;
    }
  //return the all clear
  return 0;
}

int main(int argc, char *argv[])
{
  int socketFD, portNumber, charsWritten, charsRead;
  struct sockaddr_in serverAddress;
  struct hostent* serverHostInfo;
  char *bufferKey;//string containing the key
  char *bufferPlain;//string containing the buffer
  char localHost[10]="localhost";
  char confirmationBuffer[256];//used for hand shakes
  char smallInfoBuffer[256];//used for passing ints

  //ptrs to the two files
  FILE *ptr_keyfile;
  FILE *ptr_plainfile;

  //number of chars in each file
  int numOfKeyBytes, numOfPlainBytes;
    
  //make sure the right amount of args was recieved
  if (argc < 4) { fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); exit(1); } // Check usage & args

  // Set up the server address struct
  memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
  portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
  serverAddress.sin_family = AF_INET; // Create a network-capable socket
  serverAddress.sin_port = htons(portNumber); // Store the port number
  serverHostInfo = gethostbyname(localHost); // Convert the machine name into a special form of address
  if (serverHostInfo == NULL) { fprintf(stderr, "otp_enc: ERROR, no such daemon\n"); exit(2); }
  memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

  // Set up the socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
  if (socketFD < 0) error("CLIENT: ERROR opening socket");
        
  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
    { fprintf(stderr, "otp_enc: ERROR cannot connect to host.\n"); exit(2); }

  // Get input message from user
        
  //open the key file
  ptr_keyfile = fopen(argv[2], "r");
  if(ptr_keyfile==NULL)
    return 1;

  //find the number of bytes
  fseek(ptr_keyfile, 0L, SEEK_END);
  numOfKeyBytes = ftell(ptr_keyfile);
  fseek(ptr_keyfile, 0L, SEEK_SET); //reset the pointer

  //create the buffer based of the number of bytes      
  bufferKey = (char *)malloc(numOfKeyBytes * sizeof(char));
  fread(bufferKey, sizeof(char), numOfKeyBytes, ptr_keyfile); //read the file into the buffer
  bufferKey[numOfKeyBytes-1]='\0';//replace the newline
  fclose(ptr_keyfile);

  //open the plain tesxt file
  ptr_plainfile = fopen(argv[1], "r");
  if(ptr_plainfile==NULL)
    return 1;

  //find the number of bytes
  fseek(ptr_plainfile, 0L, SEEK_END);
  numOfPlainBytes = ftell(ptr_plainfile);
  fseek(ptr_plainfile, 0L, SEEK_SET);//reset the pointer to the start of the file

  //create the buffer
  bufferPlain = (char *)malloc(numOfPlainBytes * sizeof(char));
  fread(bufferPlain, sizeof(char), numOfPlainBytes, ptr_plainfile); //read the file into the buffer
  bufferPlain[numOfPlainBytes-1]='\0';//replace the newline

  //make sure the key is the right length
  if(numOfKeyBytes<numOfPlainBytes)
    {
      fprintf(stderr, "Size of key: %s is to small\n", argv[2]);
      exit(1);
    }

  //make sure the strings are valid
  if((validateString(bufferKey, numOfKeyBytes) != 0)||(validateString(bufferPlain, numOfPlainBytes) != 0))
    {
      fprintf(stderr, "Input contains invalid characters.\n");
      close(socketFD);  
      exit(1);
    }

  //send the enc message to insure connection to the right server
  charsWritten = send(socketFD, "enc", 4, 0); // Write to the server
  if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
  if (charsWritten < 4) printf("CLIENT: WARNING: Not all data written to socket!\n");

  //make sure we get back the correct confirmation message
  memset(confirmationBuffer, '\0', sizeof(confirmationBuffer));
  charsRead = recv(socketFD, confirmationBuffer, sizeof(confirmationBuffer) - 1, 0); // Read data from the socket, leaving \0 at end
  if (charsRead < 0) error("CLIENT: ERROR reading from socket");

  //check the handshakes return message
  if(strcmp(confirmationBuffer, "confirmed") != 0)
    {
      fprintf(stderr, "otp_enc error: could not contact otp_enc_d on port: %s .\n", argv[3]);
      close(socketFD);
      exit(2);
    }

  //clear the strings
  memset(confirmationBuffer, '\0', sizeof(smallInfoBuffer));
  memset(smallInfoBuffer, '\0', sizeof(smallInfoBuffer));

  //copy the key size into a string
  sprintf(smallInfoBuffer, "%d", numOfKeyBytes);

  //send the number of key bytes to the server
  charsWritten = send(socketFD, smallInfoBuffer, sizeof(smallInfoBuffer)-1, 0); // Write to the server
  if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
  if (charsWritten < sizeof(smallInfoBuffer) - 1) printf("CLIENT: WARNING: Not all data written to socket!\n");

  //recieve the return message
  charsRead = recv(socketFD, confirmationBuffer, sizeof(confirmationBuffer) - 1, 0); // Read data from the socket, leaving \0 at end
  if (charsRead < 0) error("CLIENT: ERROR reading from socket");

  //make sure the int was recieved correctly
  if(strcmp(confirmationBuffer, "key sucess")!=0)
    {
      error("error confirmed encryption program\n");
      close(socketFD);
      free(bufferKey);
      free(bufferPlain); 
      exit(1);
    }

  //repeat the same process for the plain text
  memset(confirmationBuffer, '\0', sizeof(smallInfoBuffer));
  memset(smallInfoBuffer, '\0', sizeof(smallInfoBuffer));

  //copy the plain text size into a string
  sprintf(smallInfoBuffer, "%d", numOfPlainBytes);

  //send the plain text size to the server
  charsWritten = send(socketFD, smallInfoBuffer, sizeof(smallInfoBuffer) - 1, 0); // Write to the server
  if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
  if (charsWritten < sizeof(smallInfoBuffer) - 13) printf("CLIENT: WARNING: Not all data written to socket!2\n");

  //recieve the return message
  charsRead = recv(socketFD, confirmationBuffer, sizeof(confirmationBuffer) - 1, 0); // Read data from the socket, leaving \0 at end
  if (charsRead < 0) error("CLIENT: ERROR reading from socket");

  //make sure the plain size was recieved correctly
  if(strcmp(confirmationBuffer, "plain sucess") != 0)
    {
      printf("error confirmed encryption program\n");
    }

  //create a string that is a combination of text and key so only one send is required.
  int totalSize = numOfKeyBytes+numOfPlainBytes;
  char *fullString = malloc(totalSize*sizeof(char));
  strcpy(fullString, bufferPlain);//copy the plain text into the start of the new string
  strcpy(fullString+numOfPlainBytes-1,bufferKey);//copy the key string into the string after the plain string getting rid of the \0

  // Send message to server
  charsWritten = send(socketFD, fullString, strlen(fullString), 0); // Write to the server
  if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
  if (charsWritten < strlen(bufferKey) - 1) printf("CLIENT: WARNING: Not all data written to socket!1\n");

  // Get return message from server that should just be the encrypted plain text
  memset(bufferPlain, '\0', numOfPlainBytes); // Clear out the buffer again for reuse

  //loop until the fully encrpted statement is read
  charsRead = 0;
  while(charsRead < numOfPlainBytes-1)
    {
      charsRead = charsRead + recv(socketFD, bufferPlain+charsRead, numOfPlainBytes - 1, 0); // Read data from the socket, leaving \0 at end
      if (charsRead < 0) error("CLIENT: ERROR reading from socket");
    }
        
  //print the ecrypted message
  printf("%s\n", bufferPlain);

  close(socketFD);
        
  //free memory
  free(bufferKey);
  free(fullString);
  free(bufferPlain);    
  return 0;
}
