#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pwd.h>
#include <syslog.h>
#include "mensagem.h"

#define FIFO_PATH_MAX 256
#define FIFO_CONCORDIA "/tmp/concordiamainfifo"

int boot_sequence(char *fifo_path) {
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

    snprintf(fifo_path, FIFO_PATH_MAX, "/tmp/%s_fifo", username); // Path to the FIFO

    int res;

    // Create the FIFO
    res = mkfifo(fifo_path, 0700);
    if (res == -1) {
        perror("mkfifo");
        return 1;
    }

    // Find the position of "sender" in the username string
    char *sender_pos = strstr(username, "sender");
    if (sender_pos == NULL) {
        fprintf(stderr, "Error: 'sender' not found in the username\n");
        return 1;
    }
    char owner_user[300];
    // Copy the portion of the username before "sender"
    size_t len = sender_pos - username;
    if (len >= 40) {
        fprintf(stderr, "Error: Username prefix too long\n");
        return 1;
    }
    strncpy(owner_user, username, sender_pos - username);
    owner_user[len] = '\0'; // Null-terminate the string
    // Execute setfacl command to set ACL for the FIFO
    char setfacl_cmd[512];
    snprintf(setfacl_cmd, sizeof(setfacl_cmd), "setfacl -m u:%s:w %s", owner_user,fifo_path);
    res = system(setfacl_cmd);
    if (res != 0) {
        perror("system");
        return 1;
    }

    printf("FIFO '%s' created with ACL for user 'concordia' to write.\n", fifo_path);

    return 0;
}

void send_to_concordia(Message m){
    int fd = open(FIFO_CONCORDIA, O_WRONLY); // open fifo
    write_message(fd,m);
    close(fd);
}
int main() {
    int read_fd, write_fd;
    char fifo_path[FIFO_PATH_MAX];

    if (boot_sequence(fifo_path) != 0) {
        fprintf(stderr, "Error in boot sequence\n");
        exit(EXIT_FAILURE);
    }

    // Open the FIFO for reading
    read_fd = open(fifo_path, O_RDONLY);
    if (read_fd == -1) {
        perror("open read_fd");
        exit(EXIT_FAILURE);
    }

    // Open the FIFO for writing
    write_fd = open(fifo_path, O_WRONLY);
    if (write_fd == -1) {
        perror("open write_fd");
        exit(EXIT_FAILURE);
    }

    while (1){
        printf("Creating buffer...\n");
        Message buffer = malloc(sizeof(struct message)); // Reading buffer
        ssize_t num_read;
        syslog(LOG_INFO,"Hopping on cycle\n");
        printf("Hopping on cycle\n");
        while((num_read = read(read_fd, buffer, sizeof(struct message))) > 0) { // Detected message

            // Print the received message
            printf("Received message:\n");
            printf("Type: %d\n", buffer->t);
            printf("Destination: %s\n", buffer->destination);
            printf("Text: %s\n", buffer->text);
            syslog(LOG_INFO,"Destination: %s\n", buffer->destination);
            syslog(LOG_INFO,"Text: %s\n", buffer->text);
            // Free the messages

            if (verify_message(buffer)){
                send_to_concordia(buffer);
            }
            free_message(buffer);


        }
    }
    close(read_fd);
    close(write_fd);

    return 0;
}
