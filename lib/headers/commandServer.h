#ifndef COMMANDSERVER_H_ /* Include guard */
#define COMMANDSERVER_H_

#include "sema.h"
#include "server.h"
#include "channel.h"

void sendSpecificMessage(int client, char *message);                          // Send a specific message to client selected
int checkCommand(char *msg, tsr *sock_cli, rk_sema sem, ChannelList *channelList); // check wich command the user give
void channelQuit(int n, List *sockets, rk_sema sem, pthread_mutex_t mutexList);        // Allows the server to stop and stop all the user connected
void userQuit(int socket, List *sockets, rk_sema sem, pthread_mutex_t mutex); // Allows a user to leave the server
void displayAllUsers(int client, List *sockets);                              // send a message of all user of the chat server
int checkCensorship(char *message);                                           // Check if a message contains insult. Return 0 if there is no insult
void renameUser(char *msg, int client, List *sockets);                        // change user username
void sendPrivateMessage(char *msg, int client, List *sockets);
void * transmitMessage(void *sock_client);
void * createNewChannel(void *cmd);
void checkChannel(List *clients, int client, int freePlaces, char *message);
void joinChannel(char *msg, ChannelList *channelList, int client, List *clients);
void sendSpecificNumber(int client, int number);

#endif // COMMANDSERVER_H_