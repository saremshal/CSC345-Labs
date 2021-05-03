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

// Command list client can send
typedef enum COMMAND_CODE {
    COMMAND_DISCONNECT = 0,
    COMMAND_NEW_ROOM,
    COMMAND_REQUEST_ROOM,
    COMMAND_SHOW_ROOMS,
    COMMAND_USERNAME,
    COMMAND_INVALID_REQUEST,
    COMMAND_LIST_SIZE
} COMMAND_CODE;

// Enum for the kind of room choice made at launch
typedef enum ROOM_CHOICES {
    CHOICE_NULL = 0,
    CHOICE_NEW,
    CHOICE_NUMBER
} ROOM_CHOICES;

// Error Message Function
void error(const char *msg)
{
	perror(msg);
	exit(0);
}

typedef struct _ThreadArgs {
	int clisockfd;   //client's sockfd
    int room_val;    //client's choosen room
    int room_choice; //client's choice at launch
} ThreadArgs;

//Function that disconnects client from server and ends client instance
void disconnect(int sockfd)
{
    //Sends disconnect message to client
    char buffer[256];
    memset(buffer, 0, 256);
    buffer[0] = COMMAND_DISCONNECT;
    int n = send(sockfd, buffer, 1, 0);

    //Delays a bit to prevent crashing and ends client instance
    usleep(10);
    printf("Disconnecting From Server\n");
    exit(0);
}

//Function to send a selected username to client
void send_username(int sockfd)
{
    char username[64];
    char buffer[512];
    int n;

    //Clears username buffer
	memset(username, 0, 64);
    //Asks client to select a username
    printf("Please enter a username...\n");
    //Delay to prevent issues between print and fgets
    usleep(10);
    //Gets client input
	fgets(username,64,stdin);

    //Sends username command and username to client
    buffer[0] = COMMAND_USERNAME;
    sprintf(buffer + 1,"%s",username);
	n = send(sockfd, buffer, strlen(username) + 1, 0);
    //Delay so client doesnt send another message immediatly
    usleep(10);
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

        //If client sends invalid request message, client will disconnect and terminate
        if(buffer[0] == COMMAND_INVALID_REQUEST)
        {
            printf("ERROR Invalid Input, Shutting Down Client\n");
            disconnect(sockfd);
        }
        //If message isnt the invalid request command, client will print out the message
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

    // User selects a username
    send_username(sockfd);

	// keep sending messages to the server
	char buffer[256];
	int n;

    // Sends room Request
    memset(buffer, 0, 256);
    //User did not put an argument
    if(room_choice == CHOICE_NULL)
    {
        //Requests room list from server
        buffer[0] = COMMAND_SHOW_ROOMS;
        n = send(sockfd, buffer, 1, 0);
        if (n < 0) error("ERROR writing to socket");
    }
    //User's argument was "new"
    else if(room_choice == CHOICE_NEW)
    {
        //Makes new room
        buffer[0] = COMMAND_NEW_ROOM;
        n = send(sockfd, buffer, 1, 0);
        if (n < 0) error("ERROR writing to socket");
    }
    //User's argument was a value
    else
    {
        //Joins requested room
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

    printf("Disconnecting From Server\n");
    exit(0);
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

    //Checks if there is no second argument
    if(argv[2] == NULL)
    {
        //Makes user's room choice NULL
        args->room_choice = CHOICE_NULL;
    }
    //Checks if second argument is "new"
    else if(strcmp("new",argv[2]) == 0)
    {
        //Makes user's room choice NEW
        args->room_choice = CHOICE_NEW;
    }
    //Else, the second argument is a value
    else
    {
        //Makes user's room choice NUMBER and saves the number
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
