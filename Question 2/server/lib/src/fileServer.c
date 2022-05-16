#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>

#include "../headers/fileServer.h"
#include "../headers/colors.h"

#define SIZE 1024

// connect socket for get file
void fileGetThreadFunc(void* arg)
{
  int socket = *((int *)arg);

  while (1)
  {
    struct sockaddr_in aCfile;
    socklen_t lgFile = sizeof(struct sockaddr_in);
    int acceptation = accept(socket, (struct sockaddr *)&aCfile, &lgFile);

    // Size reception
    int size;
    if (recv(acceptation, &size, sizeof(int), 0) == -1)
    {
      redErrorMessage("Error struct size received\n");
    }
    blueMessage("Size received\n");

    // Struct reception
    fileStruct *fileInfo = (fileStruct*)malloc(size);
    if (recv(acceptation, fileInfo, size, 0) == -1)
    {
      redErrorMessage("Error struct received\n");
    }
    blueMessage("Struct received\n");

    // filename reception
    char *filename = (char*)malloc(fileInfo->filenameSize);
    if (recv(acceptation, filename, fileInfo->filenameSize, 0) == -1)
    {
      redErrorMessage("Error filename received\n");
    }
    blueMessage("Filename received\n");

    receiveFile(fileInfo, acceptation, filename);
    }

  free(arg);
  // File thread shutdown
  shutdown(socket, 2);
}

// prepare the receiving of the file
void receiveFile(fileStruct *fileInfo, int client, char *filename)
{
  fileStruct *file = fileInfo;
  pthread_t receiveThread;
  trf receiveFileData;
  receiveFileData.client = client;
  receiveFileData.fileSize = file->fileSize;
  receiveFileData.fileName = filename;
  pthread_create(&receiveThread, NULL, fileTransferReception, &receiveFileData);
  pthread_join(receiveThread, 0);
  shutdown(client, 2);
}

// receive the file from the user
void fileTransferReception(void *receiveFileData)
{
  trf *data = (trf *)receiveFileData;
  char *filename = data->fileName;
  int socket = data->client;

  FILE *fprecv;
  char buffer[SIZE];
  int recvBuffer;

  char *folder = "serverStorage/";
  char *path = (char *)malloc((strlen(folder) + strlen(filename)) * sizeof(char));
  strcat(path, folder);
  strcat(path, filename);

  long fileSize = data->fileSize;
  fprecv = fopen(path, "w+"); // open the file in "path" to write inside. Overwrite it if it already exists, create it if not

  int count;
  for (int i = 0; i < fileSize; i += SIZE) // receive file block by block of SIZE byts until there are no more byts to receive
  {
    if (i + SIZE < fileSize) // Calculate the size of the block to receive
    {
      count = SIZE;
    }
    else
    {
      count = fileSize - i;
    }

    recvBuffer = recv(socket, buffer, SIZE, 0); // receive the block of bytes from the user
    if (recvBuffer <= 0)
    {
      perror("Error in receiving buffer.");
      exit(1);
    }
    fwrite(buffer, sizeof(buffer), 1, fprecv); // write file
    bzero(buffer, SIZE);
  }
  greenMessage("File written as ");
  greenMessage(path);
  printf("\n");
  fclose(fprecv);
}

// list the file of the server
char *listFile(char *folder)
{
  DIR *d;
  struct dirent *dir;
  char *fileList = "List of server files : \n\n";
  char *finalString = (char *)malloc(strlen(fileList));
  strcpy(finalString, fileList);
  d = opendir("./serverStorage");
  if (d)
  {
    while ((dir = readdir(d)) != NULL)
    {
      if ((strcmp(dir->d_name, ".") != 0) && (strcmp(dir->d_name, "..") != 0))
      {
        char *filename = (char *)malloc((strlen(dir->d_name)) * sizeof(char) + strlen("\n") + strlen("  - "));
        strcat(filename, "  - ");
        strcat(filename, dir->d_name);
        strcat(filename, "\n");
        finalString = (char *)realloc(finalString, (strlen(finalString) + strlen(filename)) * sizeof(char));
        strcat(finalString, filename);
      }
    }
    closedir(d);
  }
  return finalString;
}

// connect socket for send file
void fileSendThreadFunc(void* arg)
{
  int socket = *((int *)arg);

  while (1)
  {
    struct sockaddr_in aCfile;
    socklen_t lgFile = sizeof(struct sockaddr_in);
    int acceptation = accept(socket, (struct sockaddr *)&aCfile, &lgFile);

    // Size reception
    u_long size;
    if (recv(acceptation, &size, sizeof(u_long), 0) == -1)
    {
      redErrorMessage("Error filename size received\n");
    }
    blueMessage("filename size received\n");

    // filename reception
    char *filename = (char*)malloc(size*sizeof(char));
    if (recv(acceptation, filename, size, 0) == -1)
    {
      redErrorMessage("Error filename received\n");
    }
    blueMessage("Filename received\n");

    sendFileStruct* data = (sendFileStruct*)malloc(sizeof(sendFileStruct));
    data->filename = filename;
    data->client = acceptation;

    pthread_t sendFileThread;
    pthread_create(&sendFileThread, NULL, prepareSendingFile, data);
  }

  free(arg);

  // File thread shutdown
  shutdown(socket, 2);
}

// prepare the sending of the file
void prepareSendingFile(void* data){

  sendFileStruct* dataSend = (sendFileStruct*)data;
  FILE *fp;
  size_t filenameSize = strlen(dataSend->filename);

  // get file path
  char *folder = "serverStorage/";
  char *path = (char *)malloc((strlen(folder) + filenameSize) * sizeof(char));
  strcat(path, folder);
  strcat(path, dataSend->filename);

  // get the file size
  fp = fopen(path, "rb");
  if (fp == NULL)
  {
      redMessage("The file doesn't exist !\n");
      //send something to the user
  }
  else
  {
      fseek(fp, 0, SEEK_END);    // Jump to the end of the file
      long fileSize = ftell(fp); // Get the current byte offset in the file
      rewind(fp);                // Jump back to the beginning of the file
      fclose(fp);

      // fill the struct
      fileStruct *file = (fileStruct *)malloc(sizeof(fileStruct));
      file->filenameSize = filenameSize;
      file->fileSize = fileSize;

      int structSize = sizeof(*file);
      if (send(dataSend->client, &structSize, sizeof(int), 0) == -1) // send the size of the struct
      {
          redErrorMessage("Error in sending struct size\n");
      }

      if (send(dataSend->client, file, structSize, 0) == -1) // send the struct
      {
          redErrorMessage("Error in sending struct size\n");
      }

      /* in comment because the client already has the name unless he selects it by an ID
      if (send(dataSend->client, dataSend->filename, file->filenameSize, 0) == -1) // send the filename
      {
          redErrorMessage("Error in sending filename\n");
      }
      */

      sendFile(dataSend->client, file, dataSend->filename);
  }
}

// transfer the file to the user
void sendFile(int client, fileStruct* file, char* name){
  
  FILE *fp;
  char buffer[SIZE];
  fileStruct *fileData = file;

  char *folder = "serverStorage/";
  char *filename = name;
  long fileSize = fileData->fileSize;
  char *path = (char *)malloc((strlen(folder) + fileData->filenameSize) * sizeof(char));
  strcat(path, folder);
  strcat(path, filename);
  fp = fopen(path, "rb"); // Open the file in binary mode

  int count;
  for (int i = 0; i < fileSize; i += SIZE) // send file block by block until there are no more byts to send
  {
      if (i + SIZE < fileSize) // Calculate the size of the block to send
      {
          count = SIZE;
      }
      else
      {
          count = fileSize - i;
      }

      fread(buffer, count, 1, fp);                       // read the file
      if (send(client, buffer, sizeof(buffer), 0) == -1) // send the block of bytes to the server
      {
          perror("Error in sending file.");
          exit(1);
      }
      bzero(buffer, SIZE); // Reset the buffer
  }
  fclose(fp); // Close the file
  greenMessage("File send succesfully\n");

}