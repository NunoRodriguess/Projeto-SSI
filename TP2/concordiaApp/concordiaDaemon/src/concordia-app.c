#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include "mensagem.h"

#define  FIFO_FILE "/tmp/concordiamainfifo"
#define  DIR_PATH "/home/concordia/db"


#define MAX_LINE_LENGTH 2048

int check_user_in_file(int filepathfd, char *username) {
    // Check if the username already exists in the file
    int found = 0;
    char *line = NULL;
    size_t line_len = 0;
    ssize_t num_read;
    FILE *fp = fdopen(filepathfd, "r");
    off_t current_position; // To store the current position in the file

    // Save the current position in the file
    if ((current_position = lseek(filepathfd, 0, SEEK_CUR)) == -1) {
        perror("lseek");
        return -1;
    }

    // Reset file pointer to the beginning of the file
    if (lseek(filepathfd, 0, SEEK_SET) == -1) {
        perror("lseek");
        return -1;
    }

    // Read lines until the end of the file
    while ((num_read = getline(&line, &line_len, fp)) > 0) {
        // Remove newline character from the line if present
        if (line[num_read - 1] == '\n') {
            line[num_read - 1] = '\0';
        }

        if (strcmp(line, username) == 0) {
            found = 1;
            break;
        }
    }

    // Free allocated memory for getline
    free(line);

    return found;
}

int check_and_add_user_to_file(int filepathfd, char *username) {
    // Check if the username already exists in the file
    int found = 0;
    char *line = NULL;
    size_t line_len = 0;
    ssize_t num_read;
    off_t current_position; // To store the current position in the file

    // Save the current position in the file
    if ((current_position = lseek(filepathfd, 0, SEEK_CUR)) == -1) {
        perror("lseek");
        return 1;
    }

    // Reset file pointer to the beginning of the file
    if (lseek(filepathfd, 0, SEEK_SET) == -1) {
        perror("lseek");
        return 1;
    }
    FILE *fp = fdopen(filepathfd, "r");
    // Read lines until the end of the file
    while ((num_read = getline(&line, &line_len,fp)) > 0) {
        // Remove newline character from the line if present

        if (line[num_read - 1] == '\n') {
            line[num_read - 1] = '\0';
        }
        if (strcmp(line, username) == 0) {
            found = 1;
            break;
        }
    }

    // Restore the original position in the file
    if (lseek(filepathfd, current_position, SEEK_SET) == -1) {
        perror("lseek");
        return 1;
    }

    // Free allocated memory for getline
    free(line);

    // If username not found, add it to the file
    if (!found) {
        // Reset file pointer to the end to append
        if (lseek(filepathfd, 0, SEEK_END) == -1) {
            perror("lseek");
            return 1;
        }
        // Append the username to the file
        if (write(filepathfd, username, strlen(username)) == -1 ||
            write(filepathfd, "\n", 1) == -1) {
            perror("write");
            return 1;
        }
    }

    return found;
}

int handle_user_registration(int filepath,char *username){

    return check_and_add_user_to_file(filepath,username);

}

int handle_group_delete(int filepathuser, int filepathgroups, char *filepathg, char *username,char* groupname) {
    // Check if the user is already registered
    int user_exists = check_user_in_file(filepathuser, username);
    if (user_exists == -1) {
        fprintf(stderr, "Error checking user registration.\n");
        return -1;
    } else if (user_exists == 0) {
        fprintf(stderr, "User '%s' is not registered.\n", username);
        return -1;
    }
    FILE *file_groups = fopen(filepathg, "r");
    if (file_groups == NULL) {
        perror("fopen");
        return -1;
    }

    // Create a temporary file
    FILE *temp_file = tmpfile();
    if (temp_file == NULL) {
        perror("tmpfile");
        fclose(file_groups);
        return -1;
    }

    char *line = NULL;
    size_t line_len = 0;
    ssize_t num_read;
    int deleted = 0; // Flag to indicate if the user's group was found and deleted

    // Read lines from the original file
    while ((num_read = getline(&line, &line_len, file_groups)) != -1) {
        // Remove newline character from the line if present
        if (num_read <= 1) {
            continue;
        }
        char *copy = strdup(line);
        if (line[num_read - 1] == '\n') {
            line[num_read - 1] = '\0';
        }

        // Split the line at the semicolon to separate owner and group
        char *owner = strtok(line, ";");
        char *tempg = strtok(NULL, ";");
        char *group = strtok(tempg, ":");
        strtok(NULL, ";");
        // Check if the owner matches the provided username
        if ( owner != NULL && group != NULL && strcmp(owner, username) == 0 && strcmp(groupname,group) == 0) {
            // Mark that the user's group was found and deleted
            deleted = 1;
        } else {
            // Write the line to the temporary file
            fprintf(temp_file, "%s\n", copy);
        }
        free(copy);
    }

    // Free allocated memory
    free(line);

    // Close files
    fclose(file_groups);

    // If the user's group was not found
    if (!deleted) {
        fprintf(stderr, "User '%s' does not own any group.\n", username);
        fclose(temp_file);
        return -1;
    }

    // Open the original groups file for writing (truncate)
    file_groups = fopen(filepathg, "w");
    if (file_groups == NULL) {
        perror("fopen");
        fclose(temp_file);
        return -1;
    }

    // Rewind the temporary file pointer to the beginning
    rewind(temp_file);

    // Copy the content of the temporary file to the original groups file
    int c;
    while ((c = fgetc(temp_file)) != EOF) {
        fputc(c, file_groups);
    }

    // Close files
    fclose(file_groups);
    fclose(temp_file);

    return 1;
}

int handle_group_registration(int filepathuser, int filepathgroups,char *filepathg, char *username, char *groupname) {
    // Check if the user is already registered
    int user_exists = check_user_in_file(filepathuser, username);
    if (user_exists == -1) {
        fprintf(stderr, "Error checking user registration.\n");
        return -1;
    } else if (user_exists == 0) {
        fprintf(stderr, "User '%s' is not registered.\n", username);
        return -1;
    }


    char *line = NULL;
    size_t line_len = 0;
    ssize_t num_read;
    FILE *file_groups = fopen(filepathg, "r");
    if (file_groups == NULL) {
        perror("fopen");
        return -1;
    }

    // Read lines until the end of the file
    while ((num_read = getline(&line, &line_len,file_groups)) > 0) {
        // Remove newline character from the line if present
        if (line[num_read - 1] == '\n') {
            line[num_read - 1] = '\0';
        }

        // Split the line at the semicolon to separate owner and group
        char *owner = strtok(line, ";");
        char *tempg = strtok(NULL, ";");
        char *group = strtok(tempg, ":");

        // Check if the owner matches the provided username
        if (strcmp(group, groupname) == 0) {
            fprintf(stderr, "Name already taken.\n");
            free(line);
            return -2;
        }
        strtok(NULL, ";");
    }
    free(line);

    // Reset file pointer to the beginning of the file
    if (lseek(filepathgroups, 0, SEEK_END) == -1) {
        perror("lseek");
        return -1;
    }
    // Add the new group
    char group_entry[MAX_LINE_LENGTH];
    snprintf(group_entry, sizeof(group_entry), "%s;%s:%s\n", username, groupname,username);
    if (write(filepathgroups, group_entry, strlen(group_entry)) == -1) {
        perror("write");
        return -3;
    }
    // Reset file pointer to the beginning of the file
    if (lseek(filepathgroups, 0, SEEK_SET) == -1) {
        perror("lseek");
        return -1;
    }

    return 1;
}

int handle_list_members_from_group(int filepathuserfd, char *filepathgroups, char *owner_username,char*groupname, char **target) {
    // Check if the owner is already registered
    int owner_exists = check_user_in_file(filepathuserfd, owner_username);
    if (owner_exists == -1) {
        fprintf(stderr, "Error checking owner registration.\n");
        return -1;
    } else if (owner_exists == 0) {
        fprintf(stderr, "Owner '%s' is not registered.\n", owner_username);
        return -1;
    }

    // Open the groups file for reading
    FILE *groups_file = fopen(filepathgroups, "r");
    if (groups_file == NULL) {
        perror("fopen");
        return -1;
    }

    char *line = NULL;
    size_t line_len = 0;
    ssize_t num_read;

    // Read lines until the end of the file
    while ((num_read = getline(&line, &line_len, groups_file)) != -1) {
        // Remove newline character from the line if present
        if (line[num_read - 1] == '\n') {
            line[num_read - 1] = '\0';
        }
        char linecopy[1024];
        strcpy(linecopy,line);

        // Split the line at the semicolon to separate owner and group
        char *owner = strtok(line, ";");
        char *tempg = strtok(NULL, ";");
        char *group = strtok(tempg, ":");
        strtok(NULL, ";");
        if (owner == NULL) {
            fprintf(stderr, "Invalid line format: %s\n", line);
            continue; // Skip processing this line
        }
        printf("%s %s \n",groupname,owner);
        // Check if the owner matches the provided username
        if (groupname != NULL && strcmp(owner, owner_username) == 0 && strcmp(group,groupname) == 0) {
            // Extract users from the line after the colon
            char *users = strtok(linecopy, ":");
            users = strtok(NULL, ":");
            if (users == NULL) {
                fprintf(stderr, "Invalid line format: %s\n", line);
                continue; // Skip processing this line
            }

            // Allocate memory for the target string and copy users
            *target = strdup(users);
            if (*target == NULL) {
                perror("strdup");
                fclose(groups_file);
                return -1;
            }

            fclose(groups_file);
            free(line);
            return 1; // Return 1 indicating owner has a group
        }
    }

    free(line);
    fclose(groups_file);

    return 0; // Return 0 indicating owner does not have a group
}

void trim_newline(char *str) {
    int len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0'; // Replace newline character with null terminator
    }
}

int isSubstring(const char *str, const char *substr) {
    // Check if the substring is found in the main string
    return strstr(str, substr) != NULL;
}

int handle_add_member_to_group(int filepathuser, int filepathgroups, char* filepathg, char *owner_username, char *new_member,char*groupname) {
    // Check if the owner exists
    int owner_exists = check_user_in_file(filepathuser, owner_username);
    if (owner_exists < 0) {
        fprintf(stderr, "Error checking owner registration.\n");
        return -1;
    } else if (owner_exists == 0) {
        fprintf(stderr, "Owner '%s' is not registered.\n", owner_username);
        return -1;
    }

    // Check if the new member exists
    int new_member_exists = check_user_in_file(filepathuser, new_member);
    if (new_member_exists < 0) {
        fprintf(stderr, "Error checking new member registration.\n");
        return -1;
    } else if (new_member_exists == 0) {
        fprintf(stderr, "New member '%s' is not registered.\n", new_member);
        return -1;
    }
    char *users;
    handle_list_members_from_group(filepathuser,filepathg,owner_username,groupname,&users);
    printf("new:%s users:%s\n",new_member,users);
    if (isSubstring(users, new_member)) {
        fprintf(stderr, "%s already a member.\n", new_member);
        return -1;
    }

    // Create a temporary file for manipulation
    FILE *temp_file = tmpfile();
    if (temp_file == NULL) {
        perror("tmpfile");
        return -1;
    }
    FILE *gp_file = fopen(filepathg, "r");
    if (gp_file == NULL) {
        perror("fopen");
        fclose(temp_file);
        return -1;
    }

    // Copy lines from the original file to the temporary file
    char line[1024]; // Assuming each line in the file is not longer than 100 characters
    while (fgets(line, sizeof(line), gp_file) != NULL) {
        char linhacopy[1024];
        strcpy(linhacopy,line);
        char *owner = strtok(line, ";");
        char *tempg = strtok(NULL, ";");
        char *group = strtok(tempg, ":");
        strtok(NULL, ";");
        if (owner != NULL && strcmp(owner, owner_username) == 0 && strcmp(groupname,group) == 0) {
            // Append the new member to the line
            trim_newline(linhacopy);
            strcat(linhacopy, ",");
            strcat(linhacopy, new_member);
            strcat(linhacopy, "\n");
        }
        fputs(linhacopy, temp_file);
    }

    rewind(gp_file);
    fclose(gp_file);
    // Rewind the temporary file to the beginning
    rewind(temp_file);

    // Replace the original file with the temporary one
    FILE *original_file = fopen(filepathg, "w");
    if (original_file == NULL) {
        perror("fopen");
        fclose(temp_file);
        return -1;
    }

    // Copy contents from the temporary file to the original file
    while (fgets(line, sizeof(line), temp_file) != NULL) {
        fputs(line, original_file);
    }

    // Close both files
    fclose(original_file);
    fclose(temp_file);

    return 1; // Return 1 indicating successful addition of member
}

int remove_last_occurrence(char *str, char *word) {
    int i, j, found, index;
    int stringLen, wordLen;
    /* Input string and word from user */
    stringLen = strlen(str);  // Length of string
    wordLen   = strlen(word); // Length of word
    index = -1;
    for(i=0; i<stringLen - wordLen; i++)
    {
        // Match word at current position
        found = 1;
        for(j=0; j<wordLen; j++)
        {
            // If word is not matched
            if(str[i+j] != word[j])
            {
                found = 0;
                break;
            }
        }
        // If word is found then update index
        if(found == 1)
        {
            index = i;
        }
    }
    // If word not found
    if(index == -1)
    {
    }
    else
    {
        for(i=index; i <= stringLen - wordLen; i++)
        {
            str[i] = str[i + wordLen];
        }
    }

    return 0;
}

int handle_remove_member_from_group(char *filepathuser, char *filepathgroups, char *owner_username, char *oldMember,char*groupname) {
    // Open the group file for reading and writing
    FILE *groups_file = fopen(filepathgroups, "r+");
    if (groups_file == NULL) {
        perror("Error opening group file");
        return -1;
    }

    // Create a temporary file
    FILE *temp_file = tmpfile();
    if (temp_file == NULL) {
        perror("Error creating temporary file");
        fclose(groups_file);
        return -1;
    }

    char line[1024]; // Assuming each line in the file is not longer than 100 characters
    int modified = 0; // Flag to indicate if modification occurred

    // Read lines from the group file
    while (fgets(line, sizeof(line), groups_file) != NULL) {
        char linecopy[1024];
        strcpy(linecopy,line);
        char *owner = strtok(line, ";");
        char *tempg = strtok(NULL, ";");
        char *group = strtok(tempg, ":");
        strtok(NULL, ";");
        // Check if the owner matches the provided username
        if ( owner != NULL && group != NULL && strcmp(owner, owner_username) == 0 && strcmp(groupname,group) == 0) {
            char toRemove[1024] = ",";
            strcat(toRemove,oldMember);
            remove_last_occurrence(linecopy,toRemove);
            fputs(linecopy,temp_file);
            modified = 1; // Set the modification flag
        } else {
            // Write the unmodified line to the temporary file
            fputs(linecopy, temp_file);
        }
    }

    // Close the group file
    fclose(groups_file);

    // If no modification occurred, return indicating failure
    if (!modified) {
        fprintf(stderr, "Owner '%s' does not own any group.\n", owner_username);
        fclose(temp_file);
        return -1;
    }

    // Rewind the temporary file
    rewind(temp_file);

    // Open the group file for writing (truncate)
    groups_file = fopen(filepathgroups, "w");
    if (groups_file == NULL) {
        perror("Error opening group file for writing");
        fclose(temp_file);
        return -1;
    }

    // Copy the contents of the temporary file to the group file
    while (fgets(line, sizeof(line), temp_file) != NULL) {
        fputs(line, groups_file);
    }

    // Close both files
    fclose(groups_file);
    fclose(temp_file);

    return 1; // Return 1 indicating successful removal of member
}

int get_users_in_group(const char *groupName, char *users[]) {
    char command[MAX_LINE_LENGTH];
    char buffer[MAX_LINE_LENGTH];
    FILE *fp;

    // Construct the command
    snprintf(command, sizeof(command), "getent group %s", groupName);

    // Execute the command and read the output
    fp = popen(command, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to run command\n");
        return -1;
    }

    // Read the output
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        // Find the list of users in the group
        char *token = strtok(buffer, ":");
        while (token != NULL) {
            if (strcmp(token, groupName) == 0) {
                // Get the users in the group
                token = strtok(NULL, ":");
                if (token != NULL) {
                    // Extract users and put them in the provided list
                    int count = 0;
                    token = strtok(token, ",");
                    while (token != NULL) {
                        users[count++] = strdup(token);
                        token = strtok(NULL, ",");
                    }
                    pclose(fp);
                    return count;
                }
            }
            token = strtok(NULL, ":");
        }
    }

    // Group doesn't exist or no users in the group
    pclose(fp);
    return 0;
}

int group_exists(char *groupName,char *filepathgroups) {

    char buffer[MAX_LINE_LENGTH];
    FILE *fp;


    fp = fopen(filepathgroups, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to run command\n");
        return -1;
    }

   while(fgets(buffer, sizeof(buffer), fp) != NULL) {
       char owner[40];
       char name[200];
       char users[MAX_TEXT_LENGTH];
       sscanf(buffer, "%39[^;];%199[^:]:%[^\n]", owner, name, users);
       if (strcmp(name,groupName)==0){

           return 1;

       }
   }

   return 0;
}
int remove_user_from_file(char *filepath, char *username) {

    FILE *original_file = fopen(filepath, "r");
    if (original_file == NULL) {
        perror("Error opening file");
        return -1;
    }

    FILE *temp_file = tmpfile();
    if (temp_file == NULL) {
        perror("tmpfile");
        fclose(original_file);
        return -1;
    }

    char line[1024]; // Adjust the buffer size as needed
    int removed = 0; // Flag to track if username has been removed

    while (fgets(line, sizeof(line), original_file) != NULL) {
        // Check if line contains the username
        char linec[1024];
        strcpy(linec,line);
        trim_newline(line);

        if (strcmp(line,username) != 0) {
            // If username is not found, write the line to the temporary file
            fputs(linec, temp_file);
        } else {
            // If username is found, set the flag to indicate removal
            removed = 1;
        }
    }

    fclose(original_file);
    rewind(temp_file); // Reset temporary file pointer to the beginning

    // Reopen the original file in write mode
    original_file = fopen(filepath, "w+");
    if (original_file == NULL) {
        perror("Error reopening original file for writing");
        fclose(temp_file);
        return -1;
    }

    // Copy contents of temporary file back to original file
    while (fgets(line, sizeof(line), temp_file) != NULL) {
        fputs(line, original_file);
    }

    fclose(original_file);
    fclose(temp_file);

    return removed; // Return 1 if username was removed, 0 otherwise
}

int handle_send_message(int filepathuser,char *filepathgroups,Message m){

    //printf("No send message!\n");
    if (!verify_message(m)){
        return -1;
    }
    int t = -1;
    int u = -1;
    u = check_user_in_file(filepathuser,m->from);
    t = group_exists(m->destination,filepathgroups);
    //printf("%d %d\n",u,t);
        if (t>0){


        char *users;
        handle_list_members_from_group(filepathuser,filepathgroups,m->from,m->destination,&users);
        char *name;
        name = strtok(users, ",");

        if (name == NULL) {
            return -1;
        }
        while (name) {
                char receiver_username[1024];
                char fifo_path[1024];

                snprintf(fifo_path,sizeof(fifo_path), "/tmp/%sreceiver_fifo",name);
                snprintf(receiver_username,sizeof(receiver_username), "%sreceiver",name);
                printf("%s\n",fifo_path);
                int fd = open(fifo_path, O_WRONLY); // open fifo
                write_message(fd,m);
                close(fd);

            name = strtok(NULL, ",");
        }

        return 1;
    }
        if (u > 0){



        //printf("No branch certo!\n");
        char receiver_username[1024];
        char fifo_path[1024];
        //printf("build!\n");
        snprintf(fifo_path,sizeof(fifo_path), "/tmp/%sreceiver_fifo", m->destination);
        snprintf(receiver_username,sizeof(receiver_username), "%sreceiver", m->destination);
        int fd = open(fifo_path, O_WRONLY); // open fifo
        write_message(fd,m);
        close(fd);
        return 1;
        }
    return -1;

}

void get_back(char* from,char* to, MESSAGE_TYPE t, char* text) {
    char fifo_path[1024];
    snprintf(fifo_path, sizeof(fifo_path), "/tmp/%s", to);
    char topass[40];
    strcpy(topass,to);
    char frompass[40];
    strcpy(frompass,from);
    Message m = create_message(t, frompass, topass, text);
    int fd = open(fifo_path, O_WRONLY); // open fifo
    if (fd == -1) {
        perror("Error opening FIFO");
        free_message(m); // Free the message if FIFO opening failed
        return;
    }
    write_message(fd, m);
    close(fd); // Close the FIFO after writing

}

int main() {

    // Create the directory if it doesn't exist
    syslog(LOG_INFO,"Staring concordia!\n");
    if (mkdir(DIR_PATH, 0700) == -1 && errno != EEXIST) {
        // perror("mkdir");
        return 1;
    }

    char file1_path[100];
    char file2_path[100];
    snprintf(file1_path, sizeof(file1_path), "%s/users.txt", DIR_PATH);
    snprintf(file2_path, sizeof(file2_path), "%s/groups.txt", DIR_PATH);

    int fd_file1 = open(file1_path, O_CREAT | O_RDWR, 0600);
    if (fd_file1 == -1) {
        // perror("open file1");
        return 1;
    }

    int fd_file2 = open(file2_path, O_CREAT | O_RDWR, 0600);
    if (fd_file2 == -1) {
        // perror("open file2");
        return 1;
    }



    // Create the FIFO (named pipe)
    if (mkfifo(FIFO_FILE, 0730) == -1) { // only concordia members can write/read from the pipe

    }

    // Open the FIFO for reading

    int fd = open(FIFO_FILE, O_RDONLY,0500);
    if (fd == -1) {
        perror("open read fd");
        return 1;
    }
    int fd2 = open(FIFO_FILE, O_WRONLY,0300);
    if (fd2 == -1) {
        perror("open read fd");
        return 1;
    }

    while (1){
        printf("Creating buffer...\n");
        Message buffer = malloc(sizeof(struct message)); // Reading buffer
        ssize_t num_read;

        printf("Hopping on cycle\n");
        while((num_read = read(fd, buffer, sizeof(struct message))) > 0) { // Detected message

            // Print the received message
            printf("Received message:\n");
            printf("From: %s\n", buffer->from);
            printf("Type: %d\n", buffer->t);
            printf("Destination: %s\n", buffer->destination);
            printf("Text: %s\n", buffer->text);
            // Free the messages
            if (verify_message(buffer) == 1) {
                printf("Verificada!");
                syslog(LOG_INFO,"Message was verified\n");
                if (buffer->t == NEWUSER) {
                    int success = handle_user_registration(fd_file1, buffer->from);
                    printf("Result of registration %d\n", success);
                    syslog(LOG_INFO, "Result of registration %d", success);
                } else if (buffer->t == CREATEGRP) {
                    int success = handle_group_registration(fd_file1, fd_file2,file2_path, buffer->from, buffer->text);
                    printf("Result of group creation %d\n", success);
                    syslog(LOG_INFO, "Result of group creation %d", success);
                } else if (buffer->t == DELETEGRP) {
                    int success = handle_group_delete(fd_file1, fd_file2, file2_path, buffer->from,buffer->text);
                    printf("Result of group removal %d\n", success);
                    syslog(LOG_INFO, "Result of group removal %d", success);
                    fd_file2 = open(file2_path, O_RDWR, 0600);
                } else if (buffer->t == LISTALL) {
                    char *users;
                    int success = handle_list_members_from_group(fd_file1, file2_path, buffer->from,buffer->text, &users);
                    printf("Result of listing %d\n", success);
                    syslog(LOG_INFO, "Result of listing %d", success);
                    printf("Users:%s\n", users);
                    if (success == 1) {
                        get_back(buffer->destination, buffer->from, SUCCESS, users);
                    } else {
                        get_back(buffer->destination, buffer->from, ERROR, "Erro, não dá para listar");
                    }

                } else if (buffer->t == ADDTOGRP) {
                    char *newuser = strtok(buffer->text, ";");
                    char *tempg = strtok(NULL, ";");
                    strtok(NULL, ";");
                    int success = handle_add_member_to_group(fd_file1, fd_file2, file2_path, buffer->from,
                                                             newuser,tempg);
                    printf("Result of Adding %d\n", success);
                    syslog(LOG_INFO, "Result of Adding %d", success);
                    fd_file2 = open(file2_path, O_RDWR, 0600);
                } else if (buffer->t == SEND) {

                    int success = handle_send_message(fd_file1, file2_path, buffer);
                    syslog(LOG_INFO, "Result of sending message %d\n", success);
                } else if (buffer->t == REMOVEFRMGRP) {
                    char *newuser = strtok(buffer->text, ";");
                    char *tempg = strtok(NULL, ";");
                    strtok(NULL, ";");
                    int success = handle_remove_member_from_group(file1_path, file2_path, buffer->from, newuser,tempg);
                    printf("Result of Adding %d\n", success);
                    syslog(LOG_INFO, "Result of removing user from group %d\n", success);
                } else if (buffer->t == DELETEUSER) {
                    int success = remove_user_from_file(file1_path, buffer->from);
                    printf("Result of Removing User %d\n", success);
                    syslog(LOG_INFO, "Result of removing user %d\n", success);
                }
            }


            free_message(buffer);
            // alloc

        }
    }

    close(fd);
    close(fd2);



    if (unlink(FIFO_FILE) == -1) {
        perror("unlink");
        return 1;
    }

    return 0;
}
