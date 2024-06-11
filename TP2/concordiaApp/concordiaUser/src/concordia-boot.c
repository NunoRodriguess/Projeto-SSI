//
// Created by nuno on 29-04-2024.
//

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "mensagem.h"

#define  FIFO_CONCORDIA "/tmp/concordiamainfifo"

void new_concordia_user() {
    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL) {
        perror("Error getting username");
        return;
    }
    char *username = pw->pw_name; // Get the login name of the user
    printf("Sending registration request for user: %s\n", username);

    Message m = create_message(NEWUSER, username, "concordia", "Please add me");
    int fd = open(FIFO_CONCORDIA, O_WRONLY); // open fifo
    if (fd == -1) {
        perror("Error opening FIFO");
        return;
    }
    write_message(fd, m);
    close(fd); // Close the FIFO after writing
    free_message(m);
}

void delete_concordia_user() {
    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL) {
        perror("Error getting username");
        return;
    }
    char *username = pw->pw_name; // Get the login name of the user
    printf("Sending registration request for user: %s\n", username);

    Message m = create_message(DELETEUSER, username, "concordia", "Please Remove me");
    int fd = open(FIFO_CONCORDIA, O_WRONLY); // open fifo
    if (fd == -1) {
        perror("Error opening FIFO");
        return;
    }
    write_message(fd, m);
    close(fd); // Close the FIFO after writing
    free_message(m);
}

int execute_shell_command(char* command){
    // Execute the shell command
    int status = system(command);

    // Check if the command executed successfully
    if (status == -1) {
        // Error executing the command
        perror("Error executing command");
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    /*
    // Define the shell command to execute your script
    const char *command = "src/boostrap.sh";

    // Execute the shell command
    int status = system(command);

    // Check if the command executed successfully
    if (status == -1) {
        // Error executing the command
        perror("Error executing command");
        return 1;
    }
     sscanf(buffer, "%39[^;];%199[^:]:%[^\n]", owner, name, users)
    */
    // Para já, assumir que está tudo direito e não há chatisses
    if (argc < 2) {
        printf("Usage: %s <command>\n", argv[0]);
        return 1;
    }
    if (strcmp("concordia-ativar",argv[1]) == 0){
        execute_shell_command("src/boostrapsimple.sh");
        new_concordia_user();
    }else if(strcmp("concordia-desativar",argv[1]) == 0){
        execute_shell_command("src/cleansimple.sh");
        delete_concordia_user();
    } else{
        printf("Unknown <command>\n");
    }

    return 0;
}
