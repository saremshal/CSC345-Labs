#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT_NUM 1004
#define MAX_ROOMS 64

typedef enum COMMAND_CODE {
    COMMAND_DISCONNECT = 0,
    COMMAND_NEW_ROOM,
    COMMAND_REQUEST_ROOM,
    COMMAND_SHOW_ROOMS,
    COMMAND_USERNAME,
    COMMAND_INVALID_REQUEST,
    COMMAND_LIST_SIZE
} COMMAND_CODE;

#define MAX_COLORS 6
int counter = 0;


void error(const char *msg)
{
	perror(msg);
	exit(1);
}


typedef struct _USR {
	int clisockfd;		// socket file descriptor
    int room;
    int is_picking_room;
    char color_val[32];
    char username[64];
	struct _USR* next;	// for linked list queue
} USR;

int sockfd;
USR *head = NULL;
USR *tail = NULL;
int current_room_count = 0;

void display_clients(void)
{
    USR* cur = head;
    char message[256];
    memset(message,0,256);

    if(cur != NULL)
    {
        sprintf(message,"Current Client List:\n");
        while (cur != NULL)
        {
            sprintf(message,"%s\t- Sockfd %d\n", message, cur->clisockfd);
            cur = cur->next;
        }
        printf("%s",message);
    }
}

void add_tail(int newclisockfd)
{
	if (head == NULL) {
		head = (USR*) malloc(sizeof(USR));
		head->clisockfd = newclisockfd;
        head->room = 0;
        head->is_picking_room = 0;
		head->next = NULL;
        sprintf(head->color_val, "\033[%dm", (counter % MAX_COLORS) + 31);
        counter++;
		tail = head;
	} else {
		tail->next = (USR*) malloc(sizeof(USR));
		tail->next->clisockfd = newclisockfd;
        tail->next->room = 0;
        tail->next->is_picking_room = 0;
		tail->next->next = NULL;
        sprintf(tail->next->color_val, "\033[%dm", (counter % MAX_COLORS) + 31);
        counter++;
		tail = tail->next;
	}
}

void delete_tail(int oldclisockfd)
{
    USR* cur = head;
    USR* prev = NULL;
    while (cur != NULL)
    {
        if (cur->clisockfd == oldclisockfd)
        {
            if(prev == NULL) // removing head
            {
                head = cur->next;
            }
            else if (cur->next == NULL) // removing tail
            {
                prev->next = NULL;
                tail = prev;
            }
            else // removing center
            {
                prev->next = cur->next;
            }

            free(cur);
            break;
        }
        prev = cur;
        cur = cur->next;
    }
}

void room_list_message(char* message)
{
    int room[MAX_ROOMS+1] = {0};
    USR* cur = head;
    while (cur != NULL)
    {
        room[cur->room]++;
        cur = cur->next;
    }

    memset(message, 0, 256);
    for(int i=1;i<(MAX_ROOMS+1);i++)
    {
        if(room[i] > 0)
        {
            if(message[0] == 0)
            {
                sprintf(message,"Room %d: %d people\n",i,room[i]);
            }
            else
            {
                sprintf(message,"%sRoom %d: %d people\n",message,i,room[i]);
            }
        }
    }
    if(strlen(message) > 0)
    {
        sprintf(message,"%sChoose the room number or type [new] to create a new room:",message);
    }
}

void broadcast(USR fromfd, char* message)
{
	// figure out sender address
	struct sockaddr_in cliaddr;
	socklen_t clen = sizeof(cliaddr);
	if (getpeername(fromfd.clisockfd, (struct sockaddr*)&cliaddr, &clen) < 0) error("ERROR Unknown sender!");

    int fromRoom = 0;
    USR* cur = head;
    while (cur != NULL)
    {
        if (cur->clisockfd == fromfd.clisockfd)
        {
            fromRoom = cur->room;
            break;
        }
        cur = cur->next;
    }

	// traverse through all connected clients
	cur = head;
	while (cur != NULL) {
		// check if cur is not the one who sent the message
        printf("\tChecking %d -> %d\n", fromfd.clisockfd, cur->clisockfd);
		if ((cur->clisockfd != fromfd.clisockfd) && (cur->room == fromRoom) && (fromRoom > 0)) {
			char buffer[512];

			// prepare message
			sprintf(buffer, "%s[%s]:%s\033[0m", fromfd.color_val,fromfd.username, message);
			int nmsg = strlen(buffer);

			// send!
			int nsen = send(cur->clisockfd, buffer, nmsg, 0);
			if (nsen != nmsg) error("ERROR send() failed");
            printf("\t\tSENT\n");
		}

		cur = cur->next;
	}
}

void room_accept(USR fromfd)
{
    char buffer[512];
    sprintf(buffer, "Joining Room %d",fromfd.room);
    int nmsg = strlen(buffer);
    int nsen = send(fromfd.clisockfd, buffer, nmsg, 0);
    if (nsen != nmsg) error("ERROR send() failed");

    sprintf(buffer, "[has joined the room]");
    broadcast(fromfd, buffer);
}

void room_request(int clisockfd, int room)
{
    printf("REQUEST: %d is trying to join room %d\n", clisockfd, room);
    int does_room_exist = 0;
    USR* cur = head;
    while (cur != NULL)
    {
        if((cur->room == room) && (cur->room != 0))
        {
            does_room_exist = 1;
        }
        cur = cur->next;
    }
    if(does_room_exist == 1)
    {
        USR* cur = head;
        while (cur != NULL)
        {
            if (cur->clisockfd == clisockfd)
            {
                break;
            }
            cur = cur->next;
        }
        cur->room = room;
        printf("REQUEST: %d is joining room %d\n", clisockfd, cur->room);
        room_accept(*cur);
    }
    else
    {
        printf("ROOM NOT FOUND - SENDING ERROR TO %d\n", clisockfd);
        char buffer[512];
        buffer[0] = COMMAND_INVALID_REQUEST;
        int nmsg = 1;
        int nsen = send(clisockfd, buffer, nmsg, 0);
        if (nsen != nmsg) error("ERROR send() failed");
    }
}

void room_new(int clisockfd)
{
    USR* cur = head;
    while (cur != NULL)
    {
        if (cur->clisockfd == clisockfd)
        {
            break;
        }
        cur = cur->next;
    }
    cur->room = ++current_room_count;
    printf("REQUEST: %d created room %d\n", clisockfd, cur->room);
    room_accept(*cur);
}

typedef struct _ThreadArgs {
	int clisockfd;
} ThreadArgs;

void* thread_main(void* args)
{
	// make sure thread resources are deallocated upon return
	pthread_detach(pthread_self());

	// get socket descriptor from argument
	int clisockfd = ((ThreadArgs*) args)->clisockfd;
	free(args);

	//-------------------------------
	// Now, we receive/send messages
	char buffer[256];
	int nsen, nrcv;

	do {
		nrcv = recv(clisockfd, buffer, 255, 0);
        buffer[strcspn(buffer, "\n")] = 0;
		if (nrcv < 0) error("ERROR recv() failed");

        USR* cur = head;
        while (cur != NULL)
        {
            if (cur->clisockfd == clisockfd)
            {
                break;
            }
            cur = cur->next;
        }

        if(cur == NULL)
        {
            continue;
        }
        else
        {
            if(buffer[0] < COMMAND_LIST_SIZE)
            {
                printf("RECV CMD %d: %X\n", clisockfd, buffer[0]);
            }
            else
            {
                printf("RECV MSG %d: %s\n", clisockfd, buffer);
            }

                if(cur->is_picking_room == 1)
                {
                    if(strcmp(buffer,"new") == 0)
                    {
                        room_new(clisockfd);
                    }
                    else
                    {
                        room_request(clisockfd, atoi(buffer));
                    }
                    cur->is_picking_room = 0;
                }
                else
                {
                    switch (buffer[0])
                    {
                        case COMMAND_DISCONNECT:
                        {
                            printf("%d Disconnected\n", clisockfd);
                            delete_tail(clisockfd);
                            display_clients();
                            break;
                        }
                        case COMMAND_NEW_ROOM:
                        {
                            room_new(clisockfd);
                            break;
                        }
                        case COMMAND_REQUEST_ROOM:
                        {
                            room_request(clisockfd, buffer[1]);
                            break;
                        }
                        case COMMAND_SHOW_ROOMS:
                        {
                            char buffer[256];
                            room_list_message(buffer);
                            int nmsg = strlen(buffer);

                            if(nmsg == 0)
                            {
                                room_new(clisockfd);
                            }
                            else
                            {
                                int nsen = send(clisockfd, buffer, nmsg, 0);
                                if (nsen != nmsg) error("ERROR send() failed");

                                cur->is_picking_room = 1;
                            }

                            break;
                        }
                        case COMMAND_USERNAME:
                        {
                            sprintf(cur->username,"%s",buffer+1);
                            printf("REQUEST: %d's username is %s\n", clisockfd, cur->username);
                        }
                        default:
                        {
                            // we send the message to everyone except the sender
                            broadcast(*cur, buffer);
                        }
                    }
                }
            }
	} while (head != NULL);

    printf("No more clients available, shutting down server\n");
	close(sockfd);
    usleep(10);
    exit(0);

	return NULL;
}

int main(int argc, char *argv[])
{
    printf("Starting Server\n");

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) error("ERROR opening socket");

	struct sockaddr_in serv_addr;
	socklen_t slen = sizeof(serv_addr);
	memset((char*) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	//serv_addr.sin_addr.s_addr = inet_addr("192.168.1.171");
	serv_addr.sin_port = htons(PORT_NUM);

	int status = bind(sockfd,
			(struct sockaddr*) &serv_addr, slen);
	if (status < 0) error("ERROR on binding");

	listen(sockfd, 5); // maximum number of connections = 5

	while(1) {
		struct sockaddr_in cli_addr;
		socklen_t clen = sizeof(cli_addr);
		int newsockfd = accept(sockfd,
			(struct sockaddr *) &cli_addr, &clen);
		if (newsockfd < 0)
        {
            printf("Binding removed and server has been shut down!\n");
            exit(0);
        }

		printf("Connected: %s\n", inet_ntoa(cli_addr.sin_addr));
		add_tail(newsockfd); // add this new client to the client list
        display_clients();

        char buffer[512] = {0};
        send(newsockfd, buffer, 1, 0); //Clears send buffer

		// prepare ThreadArgs structure to pass client socket
		ThreadArgs* args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
		if (args == NULL) error("ERROR creating thread argument");

		args->clisockfd = newsockfd;

		pthread_t tid;
		if (pthread_create(&tid, NULL, thread_main, (void*) args) != 0) error("ERROR creating a new thread");
	}

	return 0;
}
