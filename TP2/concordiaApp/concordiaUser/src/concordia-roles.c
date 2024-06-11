#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "mensagem.h"

#define  FIFO_CONCORDIA "/tmp/concordiamainfifo"

void new_concordia_group(char *nome) {
    // Logic to create a new Concordia group with the provided name
    printf("Creating Concordia group: %s\n", nome);

    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL) {
        perror("Error getting username");
        return;
    }
    char *username = pw->pw_name; // Get the login name of the user
    printf("Sending registration request for user: %s\n", username);
    Message m = create_message(CREATEGRP,username,"concordia",nome);
    int fd = open(FIFO_CONCORDIA, O_WRONLY); // open fifo
    write_message(fd,m);
    // Create the FIFO (named pipe)
    close(fd);

    // free_message(m);
}

void remove_concordia_group(char* gname) {
    // Logic to remove a Concordia group
    printf("Removing Concordia group\n");
    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL) {
        perror("Error getting username");
        return;
    }
    char *username = pw->pw_name; // Get the login name of the user
    printf("Sending Delete request for user: %s\n", username);
    Message m = create_message(DELETEGRP,username,"concordia",gname);
    int fd = open(FIFO_CONCORDIA, O_WRONLY); // open fifo
    write_message(fd,m);
    close(fd);
}

void list_concordia_groups(char* groupname) {
    // Logic to list all Concordia groups
    printf("Listing Concordia group members\n");
    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL) {
        perror("Error getting username");
        return;
    }
    char *username = pw->pw_name; // Get the login name of the user
    char fifo_path[1024];
    snprintf(fifo_path, sizeof(fifo_path), "/tmp/%s", username);
    // Create the FIFO
    int resf = mkfifo(fifo_path, 0700);
    if (resf == - 1) {
        perror("mkfifo");

    }
    char setfacl_cmd3[1512];
    snprintf(setfacl_cmd3, sizeof(setfacl_cmd3), "setfacl -m u:concordia:w %s", fifo_path);
    int res = system(setfacl_cmd3);
    if (res != 0) {
        perror("system");

    }

    Message m = create_message(LISTALL,username,"concordia",groupname);
    int fd = open(FIFO_CONCORDIA, O_WRONLY); // open fifo
    write_message(fd,m);
    close(fd);

    int fd2 = open(fifo_path, O_RDONLY); // open fifo
    Message reply = malloc(sizeof (struct message));
    read(fd2, reply, sizeof(struct message));
    if (verify_message(reply)) {
        printf("Verificada!");
        printf("Received message:\n");
        printf("Type: %d\n", reply->t);
        printf("From: %s\n", reply->from);
        printf("Destination: %s\n", reply->destination);
        printf("Text: %s\n", reply->text);
        close(fd2);
        free_message(reply);
    }
    if (unlink(fifo_path) == - 1) {
        perror("unlink");

    }

}

void add_user_to_concordia_group(char* uid, char*group) {
    // Logic to add a user to a Concordia group
    printf("Adding user with UID %s to Concordia group %s\n", uid,group);
    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL) {
        perror("Error getting username");
        return;
    }
    char *username = pw->pw_name; // Get the login name of the user
    char text[MAX_TEXT_LENGTH];
    strcpy(text,uid);
    strcat(text,";");
    strcat(text,group);
    printf("%s\n",text);
    Message m = create_message(ADDTOGRP,username,"concordia",text);
    int fd = open(FIFO_CONCORDIA, O_WRONLY); // open fifo
    write_message(fd,m);
    close(fd);
}

void remove_user_from_concordia_group(char* uid,char*group) {
    // Logic to remove a user from a Concordia group
    printf("Removing user with UID %s from Concordia group\n", uid);
    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL) {
        perror("Error getting username");
        return;
    }
    char *username = pw->pw_name; // Get the login name of the user
    char text[MAX_TEXT_LENGTH];
    strcpy(text,uid);
    strcat(text,";");
    strcat(text,group);
    printf("%s\n",text);
    Message m = create_message(REMOVEFRMGRP,username,"concordia",text);
    int fd = open(FIFO_CONCORDIA, O_WRONLY); // open fifo
    write_message(fd,m);
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <command>\n", argv[0]);
        return 1;
    }

    // Extracting the command from the arguments
    char *command = argv[1];

    // Switch-case statement to handle different commands
    if (strcmp(command, "concordia-grupo-criar") == 0) {
        if (argc != 3) {
            printf("Usage: %s concordia-grupo-criar <nome>\n", argv[0]);
            return 1;
        }

        new_concordia_group(argv[2]);
    } else if (strcmp(command, "concordia-grupo-remover") == 0) {
        if (argc != 3) {
            printf("Usage: %s concordia-grupo-remover <nome>\n", argv[0]);
            return 1;
        }
        remove_concordia_group(argv[2]);
    } else if (strcmp(command, "concordia-grupo-listar") == 0) {
        if (argc != 3) {
            printf("Usage: %s concordia-grupo-listar <nome>\n", argv[0]);
            return 1;
        }
        list_concordia_groups(argv[2]);
    } else if (strcmp(command, "concordia-grupo-destinario-adicionar") == 0) {
        if (argc != 4) {
            printf("Usage: %s concordia-grupo-destinario-adicionar <uid> <group>\n", argv[0]);
            return 1;
        }

        add_user_to_concordia_group(argv[2],argv[3]);
    } else if (strcmp(command, "concordia-grupo-destinario-remover") == 0) {
        if (argc != 4) {
            printf("Usage: %s concordia-grupo-destinario-remover <uid> <group>\n", argv[0]);
            return 1;
        }

        remove_user_from_concordia_group(argv[2],argv[3]);
    } else {
        // Invalid command
        printf("Invalid command\n");
        return 1;
    }

    return 0;
}
