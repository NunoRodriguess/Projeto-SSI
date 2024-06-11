//
// Created by nuno on 29-04-2024.
//
#ifndef CONCORDIAAPP_MENSAGEM_H
#define CONCORDIAAPP_MENSAGEM_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_TEXT_LENGTH 428 // Maximum length


typedef enum {
    SEND,
    LISTALL,
    GETMSG,
    DELETEUSER,
    CREATEGRP,
    ADDTOGRP,
    REMOVEFRMGRP,
    DELETEGRP,
    NEWUSER,
    SUCCESS,
    ERROR,
    LIST,
    REMOVEMSG
} MESSAGE_TYPE;


struct message {
    MESSAGE_TYPE t;
    char from[40];
    char destination[40];
    char text[MAX_TEXT_LENGTH];

};

typedef struct message *Message;

// Function to create a new Message
Message create_message(MESSAGE_TYPE type,char *from,char *destination,char *text);
// Function to free the Message
void free_message(Message msg);
// Read a message
Message read_message (int fd);
// Function to write a Message to a file descriptor (e.g., FIFO)
int write_message(int fd, Message msg);
// Get uid by username
uid_t get_uid(char *username);
// Check if the message is accepted by the concordia security standards
int verify_message(Message m);

int write_message_to_file(char *filename, Message msg, char* owner);

int parse_message_from_file(char *filename, Message msg);

#endif //CONCORDIAAPP_MENSAGEM_H

