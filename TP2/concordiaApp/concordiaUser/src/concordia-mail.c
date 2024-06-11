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


int handle_listar_novas_mensagens(int mostrar_todas) {
    // Implementation for listing new messages
    printf("Listar novas mensagens...\n");

    printf("Mostrar todas as mensagens.\n");
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


    char manager[40];
    snprintf(manager, sizeof(manager), "%smanager", username);
    MESSAGE_TYPE mt;
    if (mostrar_todas) {
        mt = LISTALL;
    } else {
        mt = LIST;
    }
    Message m = create_message(mt, username, manager, "Send all of them!");
    char fifo_path[1024];
    snprintf(fifo_path, sizeof(fifo_path), "/home/%s/%s_fifo", manager, manager);
    int fd = open(fifo_path, O_WRONLY); // open fifo

    snprintf(fifo_path, sizeof(fifo_path), "/tmp/%s", username);
    // Create the FIFO
    int resf = mkfifo(fifo_path, 0700);
    if (resf == - 1) {
        perror("mkfifo");
        return 1;
    }
    char setfacl_cmd3[1512];
    snprintf(setfacl_cmd3, sizeof(setfacl_cmd3), "setfacl -m u:%smanager:w %s", username, fifo_path);
    int res = system(setfacl_cmd3);
    if (res != 0) {
        perror("system");
        return 1;
    }
    write_message(fd, m);
    close(fd);
    printf("A abrir descritor de leitura %s ..\n", fifo_path);
    int fd2 = open(fifo_path, O_RDONLY); // open fifo
    Message reply = malloc(sizeof(struct message)); // Reading buffer
    while (read(fd2, reply, sizeof(struct message)) > 0) {

            if (reply->t == SUCCESS) {
                free_message(reply);
                if (mostrar_todas) {
                    printf("Lista de mensagens na sua totalidade!\n");
                } else {
                    printf("Mensagens por ler\n");
                }
                break;
            }
            printf("Text: %s\n", reply->text);
            free_message(reply);


    }

    if (unlink(fifo_path) == - 1) {
        perror("unlink");
        return 1;
    }

}

int handle_ler_mensagem(char *indice) {
    // Implementation for reading a message
    printf("Ler mensagem com índice %s...\n", indice);
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


    char manager[40];
    snprintf(manager, sizeof(manager), "%smanager", username);
    Message m = create_message(GETMSG, username, manager, indice);
    char fifo_path[1024];
    snprintf(fifo_path, sizeof(fifo_path), "/home/%s/%s_fifo", manager, manager);
    int fd = open(fifo_path, O_WRONLY); // open fifo

    snprintf(fifo_path, sizeof(fifo_path), "/tmp/%s", username);
    // Create the FIFO
    int resf = mkfifo(fifo_path, 0700);
    if (resf == - 1) {
        perror("mkfifo");
        return 1;
    }
    char setfacl_cmd3[1512];
    snprintf(setfacl_cmd3, sizeof(setfacl_cmd3), "setfacl -m u:%smanager:w %s", username, fifo_path);
    int res = system(setfacl_cmd3);
    if (res != 0) {
        perror("system");
        return 1;
    }

    write_message(fd, m);
    close(fd);
    printf("A abrir descritor de leitura %s ..\n", fifo_path);
    int fd2 = open(fifo_path, O_RDONLY); // open fifo
    Message reply = malloc(sizeof(struct message)); // Reading buffer
    read(fd2, reply, sizeof(struct message));

        printf("Verificada!");
        printf("Received message:\n");
        printf("Type: %d\n", reply->t);
        printf("From: %s\n", reply->from);
        printf("Destination: %s\n", reply->destination);
        printf("Text: %s\n", reply->text);
        close(fd2);
        free_message(reply);

    if (unlink(fifo_path) == - 1) {
        perror("unlink");
        return 1;
    }


}

int handle_remover_mensagem(char *indice){
    // Implementation for reading a message
    printf("Ler mensagem com índice %s...\n", indice);
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


    char manager[40];
    snprintf(manager, sizeof(manager), "%smanager", username);
    Message m = create_message(REMOVEMSG, username, manager, indice);
    char fifo_path[1024];
    snprintf(fifo_path, sizeof(fifo_path), "/home/%s/%s_fifo", manager, manager);
    int fd = open(fifo_path, O_WRONLY); // open fifo

    snprintf(fifo_path, sizeof(fifo_path), "/tmp/%s", username);
    // Create the FIFO
    int resf = mkfifo(fifo_path, 0700);
    if (resf == - 1) {
        perror("mkfifo");
        return 1;
    }
    char setfacl_cmd3[1512];
    snprintf(setfacl_cmd3, sizeof(setfacl_cmd3), "setfacl -m u:%smanager:w %s", username, fifo_path);
    int res = system(setfacl_cmd3);
    if (res != 0) {
        perror("system");
        return 1;
    }

    write_message(fd, m);
    close(fd);
    printf("A abrir descritor de leitura %s ..\n", fifo_path);
    int fd2 = open(fifo_path, O_RDONLY); // open fifo
    Message reply = malloc(sizeof(struct message)); // Reading buffer
    read(fd2, reply, sizeof(struct message));

        printf("Verificada!");
        printf("Text: %s\n", reply->text);
        close(fd2);
        free_message(reply);

    if (unlink(fifo_path) == - 1) {
        perror("unlink");
        return 1;
    }

}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <command> [options]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "concordia-listar") == 0) {
        int mostrar_todas = 0;
        if (argc == 3 && strcmp(argv[2], "-a") == 0) {
            mostrar_todas = 1;
        }
        handle_listar_novas_mensagens(mostrar_todas);
    } else if (strcmp(argv[1], "concordia-ler") == 0) {
        if (argc != 3) {
            printf("Usage: %s concordia-ler <índice>\n", argv[0]);
            return 1;
        }
        handle_ler_mensagem(argv[2]);
    }else if (strcmp(argv[1], "concordia-remover") == 0) {
        if (argc != 3) {
            printf("Usage: %s concordia-remover <índice>\n", argv[0]);
            return 1;
        }
        handle_remover_mensagem(argv[2]);
    } else {
        printf("Unknown command: %s\n", argv[1]);
        return 1;
    }

    return 0;
}
