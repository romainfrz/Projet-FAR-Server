#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <regex.h>

#include "../headers/channel.h"
#include "../headers/admin.h"
#include "../headers/server.h"
#include "../headers/sema.h"
#include "../headers/commandServer.h"
#include "../headers/colors.h"
#include "../headers/fileServer.h"
#include "../headers/stringFunc.h"

// Send a specific message to client selected
void sendSpecificMessage(int client, char *message)
{
    u_long sizeMessage = strlen(message) + 1;
    // Send connection message size
    if (send(client, &sizeMessage, sizeof(u_long), 0) == -1)
    {
        redErrorMessage("Error sending size\n");
    }
    // Send connection message
    if (send(client, message, sizeMessage, 0) == -1)
    {
        redErrorMessage("Error sending connection message\n");
    }
}

// check which command the user give
int checkCommand(char *msg, tsr *sock_cli, rk_sema sem, ChannelList *channelList)
{
    printf("Command detected\n");
    char *copyMessage = (char *)malloc(strlen(msg) + 1);
    strcpy(copyMessage, msg);
    char *strto = strtok(copyMessage, " ");
    if (strcmp(msg, "/quit") == 0)
    {
        userQuit((*sock_cli).client, (*sock_cli).clients, sem, (*sock_cli).mutex);
        return -1;
    }
    else if (strcmp(strto, "/pm") == 0)
    {
        printf("Go to private message\n");
        sendPrivateMessage(msg, (*sock_cli).client, (*sock_cli).clients);
    }
    else if (strcmp(strto, "/admin") == 0)
    {
        printf("Go to admin verification\n");
        adminVerification(msg, (*sock_cli).client, (*sock_cli).clients);
    }
    else if (strcmp(strto, "/kick") == 0)
    {
        printf("Go to kick function\n");
        kick(msg, (*sock_cli).client, (*sock_cli).clients);
    }
    else if (strcmp(strto, "/users") == 0)
    {
        printf("Go to displayUsers function \n");
        displayAllUsers((*sock_cli).client, (*sock_cli).clients);
    }
    else if (strcmp(strto, "/rename") == 0)
    {
        printf("Go to rename function \n");
        renameUser(msg, (*sock_cli).client, (*sock_cli).clients);
    }
    else if (strcmp(strto, "/files") == 0)
    {
        printf("Go to list file function \n");
        sendSpecificMessage((*sock_cli).client, listFile("./serverStorage"));
    }
    else if (strcmp(strto, "/lchannel") == 0)
    {
        printf("Go to list channel function \n");
        sendSpecificMessage((*sock_cli).client, listChannel(channelList));
    }
    else if (strcmp(strto, "/channel") == 0)
    {
        printf("Go to create channel function\n");
        checkChannel((*sock_cli).clients, (*sock_cli).client, channelList->freePlaces, msg);
    }
    else if (strcmp(strto, "/jchannel") == 0)
    {
        printf("Go to join channel function\n");
        joinChannel(msg, channelList, (*sock_cli).client, (*sock_cli).clients);
    }
    return 0;
}

void joinChannel(char* msg, ChannelList* channelList, int idClient, List* clients){
    char** cmd = str_split(msg, 1);
    int index = atoi(cmd[1]);
    Channel* chosenChannel = getChannelByIndex(channelList, index);

    int port = chosenChannel->port;
    char* portString = (char*)malloc(4*sizeof(char));
    sprintf(portString, "%d", port);

    char* sendCommand = (char*)malloc(strlen(portString) + strlen(cmd[0]) + 1);
    strcpy(sendCommand, cmd[0]);
    strcat(sendCommand, " ");
    strcat(sendCommand, portString);

    // We add to the client to the good channel
    changeACforJoin(clients, idClient);
    Link *client = getClientById(clients, idClient);
    char *clientPseudo = getPseudoById(clients, idClient);
    addFirstClient(chosenChannel->clients, client, clientPseudo);

    // We delete from this channel the good client
    delVal(clients, idClient);

    // We send the port to the client
    sendSpecificMessage(idClient, sendCommand);
}

void checkChannel(List* clients, int client, int freePlaces, char* message)
{
    if (isUserAdmin(clients, client) == 1)
    {
        if (freePlaces == 0)
        {
            sendSpecificMessage(client, "The maximum number of channels has been reached.\nYou can no longer add it for the moment.\n");
        }
        else
        {
            pthread_t createChannelThread;
            pthread_create(&createChannelThread, NULL, createNewChannel, message);
        }
    }
    else
    {
        sendSpecificMessage(client, "You can't create a channel if your are not an admin");
    }
}

void * createNewChannel(void *cmd)
{
    if (countSpaceCommand(cmd, 1) == 1)
    {
        char **msg = str_split(cmd, 1);
        prepareGenerateChannel(msg[1]);
    }
    else
    {
        printf("/channel channelName ! \n");
    }
    return NULL;
}

// Allows the server to stop and stop all the user connected
void channelQuit(int n, List *sockets, rk_sema sem, pthread_mutex_t mutexList)
{
    // Shutdown of all user sockets
    Link *current = sockets->head;
    while (current != NULL)
    {
        Link *next = current->next;
        userQuit(current->value, sockets, sem, mutexList);
        current = next;
    }
    redErrorMessage("\nThe server has been stopped !\n");
    exit(0);
}

// send a message of all user of the chat server
void displayAllUsers(int client, List *sockets)
{
    sendSpecificMessage(client, getAllUsers(sockets, client));
}

// Allows a user to leave the server
void userQuit(int socket, List *sockets, rk_sema sem, pthread_mutex_t mutexList)
{
    pthread_mutex_lock(&mutexList);
    delVal(sockets, socket);
    pthread_mutex_unlock(&mutexList);
    shutdown(socket, 2);
    rk_sema_post(&sem);
    printf("User %d has been stopped\n", socket);
}

// Check if a message contains insult. Return 0 if there is no insult
int checkCensorship(char *message)
{
    FILE *fp = fopen("lib/censorship_words.txt", "r");
    if (fp == NULL)
    {
        redErrorMessage("Error opening censor file...\n");
    }
    char *start = "^.*(";
    char *model = (char *)malloc(strlen(start));
    strcat(model, start);
    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, fp) != -1)
    {
        char *word = strtok(line, "\n");
        model = realloc(model, strlen(model) + strlen(word) + strlen("(|)"));
        strcat(model, "(");
        strcat(model, word);
        strcat(model, ")|");
    }
    model[strlen(model) - 1] = ')';
    char *end = ".*$";
    model = realloc(model, strlen(model) + strlen(end));
    strcat(model, end);
    fclose(fp);
    free(line);

    regex_t regex;
    int regexRes;
    regexRes = regcomp(&regex, model, REG_EXTENDED);
    regexRes = regexec(&regex, message, 0, NULL, 0);
    if (regexRes == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void renameUser(char *msg, int client, List *sockets)
{
    char **mess = str_split(msg, 2);
    char *newPseudo = mess[1];
    if (pseudoInList(sockets, newPseudo) == 0)
    {
        sendSpecificMessage(client, "Username already used !\nPlease choose another username");
    }
    else
    {
        setPseudo(sockets, client, newPseudo);
    }
}

// Allows sending a private message
void sendPrivateMessage(char *msg, int client, List *sockets)
{
    if (countSpaceCommand(msg, 2) == 1)
    {
        char **mess = str_split(msg, 3);
        char *cmd = (char *)malloc(strlen(mess[0]));
        cmd = mess[0];
        char *target = mess[1];
        int id = getIdByPseudo(sockets, target);
        if (id != -1)
        {
            char *pseudo = getPseudoById(sockets, client);
            char *pmPseudo = (char *)malloc(sizeof(char) * (strlen(pseudo) + strlen("(pm) ")));
            strcat(pmPseudo, "(pm) ");
            strcat(pmPseudo, pseudo);
            int commandSize = sizeof(cmd);
            int idSize = sizeof(target);
            tss *sendData = (tss *)malloc(sizeof(tss));
            (*sendData).client = id;
            (*sendData).size = strlen(mess[2]) + 1;
            (*sendData).message = mess[2];
            (*sendData).pseudoSender = pmPseudo;
            pthread_t send;
            pthread_create(&send, NULL, transmitMessage, (void *)sendData);
        }
        else
        {
            sendSpecificMessage(client, "This user doesn't exist !");
            printf("The user doesn't exist ! \n");
        }
    }
    else
    {
        sendSpecificMessage(client, "The command is : [/pm targetPseudo yourMessage] \n");
    }
}

// Sending a message to the client
void * transmitMessage(void *sock_client)
{
    tss *sock_cli = (tss *)sock_client;
    u_long pseudoSize = strlen((*sock_cli).pseudoSender) + 1;
    struct rk_sema semSend;
    rk_sema_init(&semSend, 1);

    // strcat
    u_long globalSize = pseudoSize + (*sock_cli).size + 1;
    char *messageTransmited = (char *)malloc(globalSize * sizeof(char));
    strcat(messageTransmited, (*sock_cli).pseudoSender);
    strcat(messageTransmited, " : ");
    strcat(messageTransmited, (*sock_cli).message);

    // Transmit message size
    sendSpecificMessage((*sock_cli).client, messageTransmited);
    greenMessage("Message transmitted\n");
    return NULL;
}

void sendSpecificNumber(int client, int number)
{
    int numberToSend = number;
    // Send connection message size
    if (send(client, &numberToSend, sizeof(int), 0) == -1)
    {
        redErrorMessage("Error sending number\n");
    }
}