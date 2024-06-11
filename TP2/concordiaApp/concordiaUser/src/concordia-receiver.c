#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pwd.h>
#include <time.h>
#include "mensagem.h"

#define FIFO_PATH_MAX 256

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

    char setfacl_cmd[512];
    snprintf(setfacl_cmd, sizeof(setfacl_cmd), "setfacl -m u:concordia:w %s",fifo_path);
    res = system(setfacl_cmd);
    if (res != 0) {
        perror("system");
        return 1;
    }

    printf("FIFO '%s' created with ACL for user 'concordia' to write.\n", fifo_path);

    return 0;
}
int pad_id(int id, char* string_id) {
    // Check if id is greater than 9999
    if (id > 9999) {
        return -1;
    }

    // Convert id to string
    snprintf(string_id, 5, "%04d", id); // Ensure a width of 4 with leading zeros

    return 0; // Success
}


int append_line_to_file(char *filename,char *line) {
    FILE *file = fopen(filename, "a"); // Open the file in append mode
    if (file == NULL) {
        perror("Error opening file for appending");
        return -1;
    }

    // Write the line to the file
    if (fprintf(file, "%s\n", line) < 0) {
        perror("Error writing to file");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

char* get_date() {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    static char buffer[11]; // Buffer for date in the format "YYYY-MM-DD"
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);

    return buffer;
}

int store_message(Message m,int counter){
    // write to the file registos not done
    // store in the directory the message with the filename being the id. done?
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

    char realid[5];
    pad_id(counter,realid);
    char path_for_file[1024];
    char path_for_registos[1024];

    // Find the position of "sender" in the username string
    char *sender_pos = strstr(username, "receiver");
    if (sender_pos == NULL) {
        fprintf(stderr, "Error: 'receiver' not found in the username\n");
        return 1;
    }
    char owner_user[200];
    // Copy the portion of the username before "receiver"
    size_t len = sender_pos - username;
    if (len >= 40) {
        fprintf(stderr, "Error: Username prefix too long\n");
        return 1;
    }
    strncpy(owner_user, username, sender_pos - username);
    owner_user[len] = '\0'; // Null-terminate the string

    snprintf(path_for_file, sizeof (path_for_file), "/home/%smanager/mail/%s",owner_user,realid);
    snprintf(path_for_registos, sizeof (path_for_registos), "/home/%smanager/mail/registos.txt",owner_user);
    char new_line[70];
    char* data = get_date();
    long tam = strlen(m->text) + strlen(m->destination) + strlen(m->from) + sizeof(MESSAGE_TYPE);
    snprintf(new_line, sizeof (new_line), "%s:%s:%ld:1",realid,data,tam);

    if (append_line_to_file(path_for_registos,new_line) == 0){

        write_message_to_file(path_for_file,m,owner_user);
        return 1;
    } else
        return -1;
}

int main() {
    int read_fd, write_fd;
    char fifo_path[FIFO_PATH_MAX];
    int id = 1;

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

        printf("Hopping on cycle\n");
        while((num_read = read(read_fd, buffer, sizeof(struct message))) > 0) { // Detected message

            // Print the received message
            printf("Received message:\n");
            printf("Type: %d\n", buffer->t);
            printf("Destination: %s\n", buffer->destination);
            printf("Text: %s\n", buffer->text);
            // Free the messages
            if (verify_message(buffer)) {

                int i = store_message(buffer, id);
                if (i) {
                    id++;
                }
                printf("Result of storing the message: %d, next id %d\n", i, id);
            }
            free_message(buffer);




        }
    }
    close(read_fd);
    close(write_fd);

    return 0;
}
