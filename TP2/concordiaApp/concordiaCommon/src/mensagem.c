#include "mensagem.h"

// Function to create a new Message
Message create_message(MESSAGE_TYPE type, char *from, char *destination, char *text) {
    // Allocate memory for the Message struct
    Message new_message = (Message)malloc(sizeof(struct message));
    if (new_message == NULL) {
        // Memory allocation failed
        return NULL;
    }

    // Set message type
    new_message->t = type;

    // Copy 'from' field
    strncpy(new_message->from, from, sizeof(new_message->from) - 1);
    new_message->from[sizeof(new_message->from) - 1] = '\0'; // Ensure null-termination

    // Copy 'destination' field
    strncpy(new_message->destination, destination, sizeof(new_message->destination) - 1);
    new_message->destination[sizeof(new_message->destination) - 1] = '\0'; // Ensure null-termination

    // Copy 'text' field
    strncpy(new_message->text, text, sizeof(new_message->text) - 1);
    new_message->text[sizeof(new_message->text) - 1] = '\0'; // Ensure null-termination

    return new_message;
}

// Function to free the Message
void free_message(Message msg) {
    free(msg);
}

// Function to read a Message from a file descriptor
Message read_message(int fd) {
    // Allocate memory for the Message struct
    Message msg = (Message)malloc(sizeof(struct message));
    if (msg == NULL) {
        return NULL; // Memory allocation failed
    }

    // Read the Message struct from the file descriptor
    ssize_t bytes_read = read(fd, msg, sizeof(struct message));
    if (bytes_read == -1 || bytes_read != sizeof(struct message)) {
        free(msg);
        return NULL; // Failed to read message
    }

    return msg;
}

// Function to write a Message to a file descriptor
int write_message(int fd, Message msg) {
    // Write the Message struct to the file descriptor
    ssize_t bytes_written = write(fd, msg, sizeof(struct message));
    if (bytes_written == -1 || bytes_written != sizeof(struct message)) {
        return -1; // Failed to write message
    }

    return 0; // Message successfully written
}

uid_t get_uid(char *username) {
    struct passwd *pwd = getpwnam(username);
    if (pwd == NULL) {
        perror("getpwnam");
        return -1; // Indicates error
    }
    return pwd->pw_uid;
}
static int is_user_a_user_on_unix(char *possible_username) {
    struct passwd *pw;
    pw = getpwnam(possible_username);
    if (pw != NULL) {
        // User found
        return 1;
    } else {
        printf("USER NOT FOUND %s\n",possible_username);
        // User not found
        return 0;
    }
}

static int is_message_type_valid(MESSAGE_TYPE type) {
    // Check if the message type is within the defined range
    return (type >= SEND && type <= REMOVEMSG);
}

static int is_from_valid(const char *from) {
    // Check if the 'from' field is not empty
    return (strlen(from) > 0);
}

static int is_destination_valid(const char *destination) {
    // Check if the 'destination' field is not empty
    return (strlen(destination) > 0);
}

static int is_text_valid(const char *text) {
    // Check if the 'text' field is not empty and within the size limit
    return (strlen(text) > 0 && strlen(text) <= 428);
}

static int is_message_valid(Message msg) {
    // Verify every part of the message
    if (!is_message_type_valid(msg->t)) {
        printf("Invalid message type.\n");
        return 0;
    }
    if (!is_from_valid(msg->from)) {
        printf("Invalid 'from' field.\n");
        return 0;
    }
    if (!is_destination_valid(msg->destination)) {
        printf("Invalid 'destination' field.\n");
        return 0;
    }
    if (!is_text_valid(msg->text)) {
        printf("Invalid 'text' field.\n");
        return 0;
    }
    // All parts of the message are valid
    return 1;
}


int verify_message(Message m){
    return is_message_valid(m) && is_user_a_user_on_unix(m->from); // return true
}

int write_message_to_file(char *filename, Message msg, char* owner) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Error opening file for writing");
        return -1;
    }

    // Write the message structure to the file
    size_t elements_written = fwrite(msg, sizeof(struct message), 1, file);
    fclose(file);

    if (elements_written != 1) {
        perror("Error writing message to file");
        return -1;
    }
    // Set file permissions
    if (chmod(filename, S_IRUSR) == -1) {
        perror("Error setting file permissions");
        return -1;
    }
    char setfacl_cmd[512];
    snprintf(setfacl_cmd, sizeof(setfacl_cmd), "setfacl -m u:%smanager:r %s",owner,filename);
    int res = system(setfacl_cmd);
    if (res != 0) {
        perror("system");
        return 1;
    }

    return 0;
}

int parse_message_from_file(char *filename, Message msg) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file for reading");
        return -1;
    }

    // Read the message structure from the file
    size_t elements_read = fread(msg, sizeof(struct message), 1, file);
    fclose(file);

    if (elements_read != 1) {
        perror("Error reading message from file");
        return -1;
    }



    return 0;
}
