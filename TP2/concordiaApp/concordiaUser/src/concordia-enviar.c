//
// Created by nuno on 29-04-2024.
//
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pwd.h>
#include "mensagem.h"

// Handler function for 'concordia-enviar' command
int handle_enviar(char *dest, char *msg) {
    printf("Sending message to %s: %s\n", dest, msg);
    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL) {
        perror("Error getting username");
        return 1;
    }
    char *username = pw->pw_name; // Get the login name of the user
    if (username == NULL) {
        perror("getlogin");
        return 1;
    }
    char sender_username[1024];
    char fifo_path[1024];

    snprintf(fifo_path,sizeof(fifo_path), "/tmp/%ssender_fifo", username); // Path to the FIFO
    snprintf(sender_username,sizeof(sender_username), "%ssender", username); // Path to the FIFO
    // Add your code here to handle 'concordia-enviar'
    Message m = create_message(SEND,username,dest,msg);
    int fd = open(fifo_path, O_WRONLY); // open fifo
    write_message(fd,m);
    close(fd);
    return 0;
}

// Handler function for 'concordia-responder' command
void handle_responder(char *mid, char *msg) {
    printf("Responding to message with MID %s: %s\n", mid, msg);
    // Add your code here to handle 'concordia-responder'
    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL) {
        perror("Error getting username");

    }
    char *username = pw->pw_name; // Get the login name of the user
    if (username == NULL) {
        perror("getlogin");

    }


    char manager[40];
    snprintf(manager, sizeof(manager), "%smanager", username);
    Message m = create_message(GETMSG, username, manager, mid);
    char fifo_path[1024];
    snprintf(fifo_path, sizeof(fifo_path), "/home/%s/%s_fifo", manager, manager);
    int fd = open(fifo_path, O_WRONLY); // open fifo

    snprintf(fifo_path, sizeof(fifo_path), "/tmp/%s", username);
    // Create the FIFO
    int resf = mkfifo(fifo_path, 0700);
    if (resf == - 1) {
        perror("mkfifo");

    }
    char setfacl_cmd3[1512];
    snprintf(setfacl_cmd3, sizeof(setfacl_cmd3), "setfacl -m u:%smanager:w %s", username, fifo_path);
    int res = system(setfacl_cmd3);
    if (res != 0) {
        perror("system");

    }

    write_message(fd, m);
    close(fd);
    int fd2 = open(fifo_path, O_RDONLY); // open fifo
    Message reply = malloc(sizeof(struct message)); // Reading buffer
    read(fd2, reply, sizeof(struct message));
    if (unlink(fifo_path) == - 1) {
        perror("unlink");

    }

        char sender_username[1024];
        snprintf(fifo_path,sizeof(fifo_path), "/tmp/%ssender_fifo", username); // Path to the FIFO
        snprintf(sender_username,sizeof(sender_username), "%ssender", reply->from); // Path to the FIFO
        // Add your code here to handle 'concordia-enviar'
        Message m2 = create_message(SEND,username,reply->from,msg);
        int fd3 = open(fifo_path, O_WRONLY); // open fifo
        write_message(fd3,m2);
        close(fd3);
        free_message(reply);


}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <command> <args...>\n", argv[0]);
        return 1;
    }

    char *command = argv[1];

    if (strcmp(command, "concordia-enviar") == 0) {
        if (argc != 4) {
            printf("Usage: %s concordia-enviar <dest> <msg>\n", argv[0]);
            return 1;
        }
        char *dest = argv[2];
        char *msg = argv[3];
        handle_enviar(dest, msg);
    } else if (strcmp(command, "concordia-responder") == 0) {
        if (argc != 4) {
            printf("Usage: %s concordia-responder <mid> <msg>\n", argv[0]);
            return 1;
        }
        char *mid = argv[2];
        char *msg = argv[3];
        handle_responder(mid, msg);
    } else {
        printf("Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
