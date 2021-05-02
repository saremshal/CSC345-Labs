#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#define PORT_NUM 1004

typedef enum COMMAND_CODE {
    COMMAND_DISCONNECT = 0,
    COMMAND_NEW_ROOM,
    COMMAND_REQUEST_ROOM,
    COMMAND_SHOW_ROOMS,
    COMMAND_INVALID_REQUEST,
    COMMAND_LIST_SIZE
} COMMAND_CODE;

typedef enum ROOM_CHOICES {
    CHOICE_NULL = 0,
    CHOICE_NEW,
    CHOICE_NUMBER
} ROOM_CHOICES;

void error(const char *msg)
{
	perror(msg);
	exit(0);
}

typedef struct _ThreadArgs {
	int clisockfd;
    int room_val;
    int room_choice;
} ThreadArgs;

void disconnect(int sockfd)
{
    char buffer[256];
    memset(buffer, 0, 256);
    buffer[0] = COMMAND_DISCONNECT;
    int n = send(sockfd, buffer, 1, 0);
    usleep(10);
    printf("Disconnecting From Server\n");
    exit(0);
}

void* thread_main_recv(void* args)
{
	pthread_detach(pthread_self());

	int sockfd = ((ThreadArgs*) args)->clisockfd;
	free(args);

	// keep receiving and displaying message from server
	char buffer[512];
	int n;

	n = recv(sockfd, buffer, 512, 0);
	while (n > 0) {
		memset(buffer, 0, 512);
		n = recv(sockfd, buffer, 512, 0);
		if (n < 0) error("ERROR recv() failed");

        if(buffer[0] == COMMAND_INVALID_REQUEST)
        {
            printf("ERROR Invalid Input, Shutting Down Client\n");
            disconnect(sockfd);
        }
        else
        {
            printf("%s\n", buffer);
        }
	}

	return NULL;
}

void* thread_main_send(void* args)
{
	pthread_detach(pthread_self());

	int sockfd = ((ThreadArgs*) args)->clisockfd;
    int room_choice = ((ThreadArgs*) args)->room_choice;
    int room_val = ((ThreadArgs*) args)->room_val;
	free(args);

	// keep sending messages to the server
	char buffer[256];
	int n;

    // Sends room Request
    memset(buffer, 0, 256);
    if(room_choice == CHOICE_NULL)
    {
        //Send room list and
        //Ask user what they want to do
        buffer[0] = COMMAND_SHOW_ROOMS;
        n = send(sockfd, buffer, 1, 0);
        if (n < 0) error("ERROR writing to socket");
    }
    else if(room_choice == CHOICE_NEW)
    {
        //Makes new room
        buffer[0] = COMMAND_NEW_ROOM;
        n = send(sockfd, buffer, 1, 0);
        if (n < 0) error("ERROR writing to socket");
    }
    else
    {
        //Join room
        buffer[0] = COMMAND_REQUEST_ROOM;
        buffer[1] = room_val;
        n = send(sockfd, buffer, 2, 0);
        if (n < 0) error("ERROR writing to socket");
    }

	while (1) {
		// You will need a bit of control on your terminal
		// console or GUI to have a nice input window.
		memset(buffer, 0, 256);
		fgets(buffer, 255, stdin);

		if (strlen(buffer) == 1)
        {
            // we stop transmission when user type empty string
            disconnect(sockfd);
        }
        else
        {
            n = send(sockfd, buffer, strlen(buffer), 0);
            if (n < 0) error("ERROR writing to socket");
        }
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	if (argc < 2) error("Please speicify hostname");

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) error("ERROR opening socket");

	struct sockaddr_in serv_addr;
	socklen_t slen = sizeof(serv_addr);
	memset((char*) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(PORT_NUM);

	printf("Try connecting to %s...\n", inet_ntoa(serv_addr.sin_addr));

	int status = connect(sockfd,
			(struct sockaddr *) &serv_addr, slen);
	if (status < 0) error("ERROR connecting");

	pthread_t tid1;
	pthread_t tid2;

	ThreadArgs* args;

	args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
	args->clisockfd = sockfd;
    if(argv[2] == NULL)
    {
        args->room_choice = CHOICE_NULL;
    }
    else if(strcmp("new",argv[2]) == 0)
    {
        args->room_choice = CHOICE_NEW;
    }
    else
    {
        args->room_choice = CHOICE_NUMBER;
        args->room_val = atoi(argv[2]);
    }
	pthread_create(&tid1, NULL, thread_main_send, (void*) args);

	args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
	args->clisockfd = sockfd;
	pthread_create(&tid2, NULL, thread_main_recv, (void*) args);

	// parent will wait for sender to finish (= user stop sending message and disconnect from server)
	pthread_join(tid1, NULL);

    while(1);

	return 0;
}
