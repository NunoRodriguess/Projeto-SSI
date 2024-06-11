//
// Created by nuno on 30-04-2024.
//
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
#include <ctype.h>
#include "mensagem.h"

#define FIFO_PATH_MAX 256

int boot_manager(char *fifo_path) {
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

    // Get the home directory of the user
    char *home_dir = pw->pw_dir;

    // Construct the path to the "mail" folder
    char mail_dir[100];
    snprintf(mail_dir, sizeof(mail_dir), "%s/mail", home_dir);

    // Create the "mail" folder
    if (mkdir(mail_dir, 0700) == -1) {
        if (errno == EEXIST) {
            printf("Directory '%s' already exists.\n", mail_dir);
        } else {
            perror("Error creating directory");
            return 1;
        }
    } else {
        printf("Directory '%s' created successfully.\n", mail_dir);
    }



    // Find the position of "sender" in the username string
    char *sender_pos = strstr(username, "manager");
    if (sender_pos == NULL) {
        fprintf(stderr, "Error: 'manager' not found in the username\n");
        return 1;
    }
    char owner_user[200];
    // Copy the portion of the username before "sender"
    size_t len = sender_pos - username;
    if (len >= 40) {
        fprintf(stderr, "Error: Username prefix too long\n");
        return 1;
    }
    strncpy(owner_user, username, sender_pos - username);
    owner_user[len] = '\0'; // Null-terminate the string

    char setfacl_cmd[512];
    snprintf(setfacl_cmd, sizeof(setfacl_cmd), "setfacl -m u:%sreceiver:rwx %s", owner_user, mail_dir);
    int res = system(setfacl_cmd);
    if (res != 0) {
        perror("system");
        return 1;
    }

    // Construct the path to the "registos.txt" file
    char registos_file[256];
    snprintf(registos_file, sizeof(registos_file), "%s/registos.txt", mail_dir);

    // Create the "registos.txt" file
    FILE *file = fopen(registos_file, "w");
    if (file == NULL) {
        perror("Error creating registos.txt file");
        return 1;
    }

    // Set the file permissions for user only
    if (chmod(registos_file, S_IRUSR | S_IWUSR) == -1) {
        perror("Error setting file permissions");
        fclose(file);
        return 1;
    }
    // Close the file
    fclose(file);
    printf("%s/%s_fifo\n", home_dir,username);
    snprintf(fifo_path, FIFO_PATH_MAX, "%s/%s_fifo", home_dir,username); // Path to the FIFO
    int resf;
    // Create the FIFO
    resf = mkfifo(fifo_path, 0700);
    if (resf == -1) {
        perror("mkfifo");
        return 1;
    }
    char setfacl_cmd3[512];
    snprintf(setfacl_cmd3, sizeof(setfacl_cmd3), "setfacl -m u:%s:w %s", owner_user, fifo_path);
    res = system(setfacl_cmd3);
    if (res != 0) {
        perror("system");
        return 1;
    }
    char setfacl_cmd4[512];
    snprintf(setfacl_cmd4, sizeof(setfacl_cmd4), "setfacl -m u:%sreceiver:rw %s", owner_user, registos_file);
    res = system(setfacl_cmd4);
    if (res != 0) {
        perror("system");
        return 1;
    }


    return 0; // Success
}

int check_if_message_exists(char* id, char* path) {
    FILE *file = fopen(path, "r+");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    char line[100]; // Assuming each line in the file is not longer than 100 characters
    while (fgets(line, sizeof(line), file) != NULL) {
        // Split the line into id, date, size, and status
        char *token = strtok(line, ":");
        char *current_id = token;
        token = strtok(NULL, ":");
        char *date = token;
        token = strtok(NULL, ":");
        char *size_str = token;
        token = strtok(NULL, ":");
        char *status = token;

        // Check if the id matches
        if (strcmp(current_id, id) == 0) {
            // Change the status to "0"
            fseek(file, -strlen(status), SEEK_CUR); // Move back to the beginning of the status
            fprintf(file, "0"); // Overwrite with "0"
            fclose(file);
            return 1; // Status changed to "0"
        }
    }

    fclose(file);
    return 0; // ID not found
}

int remove_message_by_id(char* id, char* path) {
    // Open the file for reading and writing
    FILE *file = fopen(path, "r+");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    // Create a temporary file for writing the updated contents
    FILE *temp_file = tmpfile();
    if (temp_file == NULL) {
        perror("Error creating temporary file");
        fclose(file);
        return -1;
    }

    char line[1024];
    int found = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        // Split the line into id, date, size, and status
        char linereal[1024];
        strcpy(linereal,line);
        char *token = strtok(line, ":");
        char *current_id = token;


        // Check if the id matches
        if (strcmp(current_id, id) == 0) {
            found = 1; // Set the flag to indicate that the ID was found
        } else {
            // Write the line to the temporary file
            fputs(linereal, temp_file);
        }
    }

    // Close the original file
    fclose(file);

    // Close the temporary file
    rewind(temp_file); // Rewind to the beginning before reading
    file = fopen(path, "w"); // Reopen the original file for writing
    if (file == NULL) {
        perror("Error opening file for writing");
        fclose(temp_file);
        return -1;
    }

    // Rewrite the original file with the contents of the temporary file
    while (fgets(line, sizeof(line), temp_file) != NULL) {
        fputs(line, file);
    }

    // Close the temporary file
    fclose(temp_file);

    // Close the original file
    fclose(file);

    return found; // Return 1 if the ID was found and removed, 0 otherwise
}


int handle_getmsg(char* mid,char* user){

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


    // depois lógica de verificar a cena, return -1 se correr mal
    char fifo_path[FIFO_PATH_MAX];
    snprintf(fifo_path, FIFO_PATH_MAX, "/tmp/%s", user); // Path to the FIFO
    int fd = open(fifo_path, O_WRONLY);
    // depois coiso
    char r_path[FIFO_PATH_MAX];
    snprintf(r_path, FIFO_PATH_MAX, "/home/%s/mail/registos.txt",username); // Path to the FIFO

    if (check_if_message_exists(mid,r_path)!=-1){


    Message temp = malloc(sizeof(struct message)); // Allocate memory for the message;
    char m_path[FIFO_PATH_MAX];
    snprintf(m_path, FIFO_PATH_MAX, "/home/%s/mail/%s", username,mid); // Path to the FIFO
    printf("%s\n",m_path);
    if (parse_message_from_file(m_path,temp) < 0){
        free(temp);
        Message m = create_message(ERROR,"manager",user,"Ficheiro da mensagem não existe");
        write_message(fd,m);
        free(m);
    }else{
        Message m = create_message(SUCCESS,temp->from,temp->destination,temp->text);
        write_message(fd,m);
        free(temp);
        free(m);
    }
    }else{
        Message m = create_message(ERROR,"manager",user,"Mensagem não existe");
        write_message(fd,m);
    }
    return 0;


}

int list_all(char *filepath, int fd, char *owner) {
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    char line[100]; // Assuming each line in the file is not longer than 100 characters
    while (fgets(line, sizeof(line), file) != NULL) {

        char id[5];
        char date[11];
        int size;
        int status;
        sscanf(line, "%4[^:]:%10[^:]:%d:%d", id, date, &size, &status);


        char text[MAX_TEXT_LENGTH];
        snprintf(text,MAX_TEXT_LENGTH,"ID: %s, Date: %s, Size: %d, Status: %d\n", id, date, size,status);
        Message m = create_message(LISTALL,"manager",owner,text);
        printf("ID: %s, Date: %s, Size: %d, Status: %d\n", id, date, size,status);
        write_message(fd,m);
        free_message(m);
    }
    Message m = create_message(SUCCESS,"manager",owner,"Listagem terminada com sucesso");
    write_message(fd,m);
    free_message(m);
    fclose(file);
    return 0;
}

int list_unread(char *filepath, int fd, char *owner) {
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    char line[100]; // Assuming each line in the file is not longer than 100 characters
    while (fgets(line, sizeof(line), file) != NULL) {

        char id[5];
        char date[11];
        int size;
        int status;
        sscanf(line, "%4[^:]:%10[^:]:%d:%d", id, date, &size, &status);
        if (status == 1){
        char text[MAX_TEXT_LENGTH];
        snprintf(text,MAX_TEXT_LENGTH,"ID: %s, Date: %s, Size: %d, Status: %d\n", id, date, size,status);
        Message m = create_message(LIST,"manager",owner,text);
        printf("ID: %s, Date: %s, Size: %d, Status: %d\n", id, date, size,status);
        write_message(fd,m);
        free_message(m);
        }
    }
    Message m = create_message(SUCCESS,"manager",owner,"Listagem terminada com sucesso");
    write_message(fd,m);
    free_message(m);
    fclose(file);
    return 0;
}

int handle_list_all(){

    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL) {
        perror("Error getting username");
        return -1;
    }
    char *username = pw->pw_name; // Get the login name of the user
    if (username == NULL) {
        perror("getlogin");
        return -1;
    }

    char r_path[FIFO_PATH_MAX];
    snprintf(r_path, FIFO_PATH_MAX, "/home/%s/mail/registos.txt",username); // Path to the FIFO

    // Find the position of "sender" in the username string
    char *sender_pos = strstr(username, "manager");
    if (sender_pos == NULL) {
        fprintf(stderr, "Error: 'manager' not found in the username\n");
        return -1;
    }
    char owner_user[200];
    // Copy the portion of the username before "sender"
    size_t len = sender_pos - username;
    if (len >= 40) {
        fprintf(stderr, "Error: Username prefix too long\n");
        return -1;
    }
    strncpy(owner_user, username, sender_pos - username);
    owner_user[len] = '\0';

    char fifo_path[FIFO_PATH_MAX];
    snprintf(fifo_path, FIFO_PATH_MAX, "/tmp/%s", owner_user); // Path to the FIFO
    int fd = open(fifo_path, O_WRONLY);

    list_all(r_path,fd,owner_user);

    close(fd);
    return 1;

}

int handle_list(){

    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL) {
        perror("Error getting username");
        return -1;
    }
    char *username = pw->pw_name; // Get the login name of the user
    if (username == NULL) {
        perror("getlogin");
        return -1;
    }

    char r_path[FIFO_PATH_MAX];
    snprintf(r_path, FIFO_PATH_MAX, "/home/%s/mail/registos.txt",username); // Path to the FIFO

    // Find the position of "sender" in the username string
    char *sender_pos = strstr(username, "manager");
    if (sender_pos == NULL) {
        fprintf(stderr, "Error: 'manager' not found in the username\n");
        return -1;
    }
    char owner_user[200];
    // Copy the portion of the username before "sender"
    size_t len = sender_pos - username;
    if (len >= 40) {
        fprintf(stderr, "Error: Username prefix too long\n");
        return -1;
    }
    strncpy(owner_user, username, sender_pos - username);
    owner_user[len] = '\0';

    char fifo_path[FIFO_PATH_MAX];
    snprintf(fifo_path, FIFO_PATH_MAX, "/tmp/%s", owner_user); // Path to the FIFO
    int fd = open(fifo_path, O_WRONLY);

    list_unread(r_path,fd,owner_user);

    close(fd);
    return 1;

}

int handle_remove(char* mid,char* user){
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


    // depois lógica de verificar a cena, return -1 se correr mal
    char fifo_path[FIFO_PATH_MAX];
    snprintf(fifo_path, FIFO_PATH_MAX, "/tmp/%s", user); // Path to the FIFO
    int fd = open(fifo_path, O_WRONLY);
    // depois coiso
    char r_path[FIFO_PATH_MAX];
    snprintf(r_path, FIFO_PATH_MAX, "/home/%s/mail/registos.txt",username); // Path to the FIFO

    if (remove_message_by_id(mid,r_path)==1){



        Message m = create_message(SUCCESS,"manager",user,"Mensagem removida com sucesso");

        write_message(fd,m);

        free(m);
    }else{
        Message m = create_message(ERROR,"manager",user,"Mensagem não existe");
        write_message(fd,m);
    }
    return 0;


}

int main(){

    int read_fd, write_fd;
    char fifo_path[FIFO_PATH_MAX];

    if (boot_manager(fifo_path) != 0) {
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
                printf("Verificada!");

                if (buffer->t == GETMSG) {
                    int i = handle_getmsg(buffer->text, buffer->from);
                    printf("Resultado %d\n", i);
                } else if (buffer->t == LIST) {
                    int i = handle_list();
                    printf("Resultado %d\n", i);

                } else if (buffer->t == LISTALL) {
                    int i = handle_list_all();
                    printf("Resultado %d\n", i);
                } else if (buffer->t == REMOVEMSG) {
                    int i = handle_remove(buffer->text, buffer->from);
                    printf("Resultado %d\n", i);
                }
            }

        }
    }
    close(read_fd);
    close(write_fd);

    return 0;

    return 0;
}