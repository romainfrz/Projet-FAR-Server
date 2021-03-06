/**
 * @file channelList.c
 * @authors Romain FREZIER
 * @authors Etienne TILLIER
 * @brief Channel list functions implementation
 * @version 0.1
 * @date 2022-05-26
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "../headers/channel.h"

ChannelList* createChannelList(int size){
    ChannelList* newChannelList = (ChannelList*)malloc(sizeof(ChannelList));
    newChannelList->freePlaces = size;
    newChannelList->head = NULL;
    newChannelList->maxPlaces = size;
    return newChannelList;
}

int channelListIsEmpty(ChannelList* list)
{
    if (list->head == NULL)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void addLastChannel(ChannelList* list, Channel* channel)
{
    if (list->freePlaces == 0)
    {
        printf("There is no more space in the channel list !\n");
    }
    else
    {
        if (list->head == NULL)
        {
            list->head = channel;
            channel->next = NULL;
        }
        else
        {
            Channel *current = list->head;
            while (current->next != NULL)
            {
                current = current->next;
            }
            current->next = channel;
        }
        list->freePlaces = list->freePlaces - 1;
    }
}

char* listChannel(ChannelList* list, int client){
    Channel *current = list->head;

    char *start = "\nList of channel(s) : \n\n";
    char *finalString = (char *)malloc(strlen(start));
    strcat(finalString, start);
    int count = 1;
    int sizeBuffer = 50;
    char *name= (char*)malloc(sizeBuffer);
    char *theme = (char*)malloc(sizeBuffer);
    while (current != NULL)
    {
        if (strlen(current->name)>sizeBuffer)
        {
            name = realloc(name,strlen(current->name));
        }
        if (strlen(current->theme)>sizeBuffer)
        {
            theme = realloc(theme,strlen(current->name));
        }

        strcpy(name, current->name);
        strcpy(theme, current->theme);
        size_t size = strlen(name) + strlen("\t00. : \n") + strlen(theme);
        char *line = (char*)malloc(size);
        char *number;
        asprintf(&number, "%d", count);
        strcat(line, "\t");
        strcat(line, number);
        strcat(line, ". ");
        strcat(line, name);
        strcat(line, " : ");
        strcat(line, theme);

        if (getClientById(current->clients, client) != NULL)
        {
            line = realloc(line, strlen(line) + strlen(" (in)\n"));
            strcat(line, " (in)\n");
        }
        else
        {
            strcat(line, "\n");
        }
        
        finalString = realloc(finalString, strlen(finalString) + strlen(line));
        strcat(finalString, line);
        free(line);
        current = current->next;
        count ++;
    }
//    free(name);
//    free(theme);
    return finalString;
}

Channel* getChannelByIndex(ChannelList* list, int index){
    Channel *current = list->head;
    int index_current = 1;
    if (index == 1){
        return current;
    }
    else {
        if (index <= list->maxPlaces){
            while (index_current != index)
            {
                current = current->next;
                index_current++;
            }
            return current;
        }
        else
        {
            return NULL;
        }
    }

}

Channel *getCurrentChannel(ChannelList *channels)
{
    Channel *currentChannel = channels->head;
    while (currentChannel != NULL)
    {
        if (currentChannel->thread == pthread_self())
        {
            break;
        }
        currentChannel = currentChannel->next;
    }
    return currentChannel;
}

int pseudoInAllChannel(ChannelList* list, char* pseudo){
    Channel *currentChannel = list->head;
    int res = 1;
    while (currentChannel != NULL && res != 0)
    {
        res = pseudoInList(currentChannel->clients, pseudo);
        currentChannel = currentChannel->next;
    }
    return res;
}

void delChannel(ChannelList *list, int index){
    Channel *currentChannel = list->head;
    int i = 1;
    while ((currentChannel != NULL) && ((i+1) != index))
    {
        currentChannel = currentChannel->next;
        i++;
    }
    if (currentChannel != NULL)
    {
        Channel *deleted = currentChannel->next;
        currentChannel->next = currentChannel->next->next;
        free(deleted);
    }
    
}

char *getAllUsers(ChannelList *list, int client, List* clients)
{
    if (isUserAdmin(clients,client) == 1)
    {
        Channel *current = list->head;
        char *start = "\nList of all users : ";
        char *final = (char *)malloc(strlen(start));
        strcat(final, start);
        while (current != NULL)
        {
            char *channelStart = "\n\nUser of channel ";
            char *realStart = (char *)malloc(strlen(channelStart) + strlen(current->name) + 5);
            strcat(realStart, channelStart);
            strcat(realStart, current->name);
            strcat(realStart, " :\n");
            char *userListChannel = getAllChannelUsers(current->clients, client, "\n");
            realStart = realloc(realStart, strlen(realStart) + strlen(userListChannel));
            strcat(realStart, userListChannel);
            final = realloc(final, strlen(final) + strlen(realStart));
            strcat(final, realStart);
            current = current->next;
//            free(realStart);
        }
        return final;
    }
    else
    {
        return NULL;
    }
}