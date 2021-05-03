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

// Command List Client can send
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

// Error Message Function
void error(const char *msg)
{
	perror(msg);
	exit(1);
}

// Struct to store client data
typedef struct _USR {
	int clisockfd;       // socket file descriptor
    int room;            // Room client is in
    int is_picking_room; //Informs the server that this client is picking a room
    char color_val[32];  //User given color
    char username[64];   //User's chosen username
	struct _USR* next; 	 // for linked list queue
} USR;

int sockfd;
USR *head = NULL;
USR *tail = NULL;
int room_counter = 0; //Counter that helps assign new rooms

// Displays all the clients currently connected to the server
void display_clients(void)
{
    USR* cur = head;
    char message[256];
    memset(message,0,256);

    //If there is no clients, it will make msg empty (size 0)
    if(cur != NULL)
    {
        //Header for displaying clients
        sprintf(message,"Current Client List:\n");
        // Loops through all the clients
        while (cur != NULL)
        {
            //Concatinates the next client to the string
            sprintf(message,"%s\t- Sockfd %d\n", message, cur->clisockfd);
            // Increments to the next client
            cur = cur->next;
        }
        // Prints out string to display clients
        printf("%s",message);
    }
}

// Adds new client to the list
void add_tail(int newclisockfd)
{
	if (head == NULL) {
		head = (USR*) malloc(sizeof(USR));
		head->clisockfd = newclisockfd;
        head->room = 0;
        head->is_picking_room = 0;
		head->next = NULL;

        //Assigns a color to the client
        sprintf(head->color_val, "\033[%dm", (counter % MAX_COLORS) + 31);
        counter++;

		tail = head;
	} else {
		tail->next = (USR*) malloc(sizeof(USR));
		tail->next->clisockfd = newclisockfd;
        tail->next->room = 0;
        tail->next->is_picking_room = 0;
		tail->next->next = NULL;

        //Assigns a color to the client
        sprintf(tail->next->color_val, "\033[%dm", (counter % MAX_COLORS) + 31);
        counter++;

		tail = tail->next;
	}
}

// Removes client from linked list
void delete_tail(int oldclisockfd)
{
    USR* cur = head;
    USR* prev = NULL;
    //Searches for client within linked list
    while (cur != NULL)
    {
        if (cur->clisockfd == oldclisockfd)
        {
            // removing head
            if(prev == NULL)
            {
                //Makes 2nd client the 1st client
                head = cur->next;
            }
            // removing tail
            else if (cur->next == NULL)
            {
                //Makes 2nd to last client the last client
                prev->next = NULL;
                tail = prev;
            }
            // removing center
            else
            {
                // Links the client ahead and behind together
                prev->next = cur->next;
            }

            //Deletes object
            free(cur);
            break;
        }
        // Helps loop through linked list
        prev = cur;
        cur = cur->next;
    }
}

// Helps sends a message to the client on the amount of people in a room
void room_list_message(char* message)
{
    // Array of rooms to contain how many people are at each room index
    int room[MAX_ROOMS+1] = {0};
    //Goes through all the clients and adds all the rooms they are in
    USR* cur = head;
    while (cur != NULL)
    {
        room[cur->room]++;
        cur = cur->next;
    }

    //Clears message
    memset(message, 0, 256);
    //Goes through all the rooms
    for(int i=1;i<(MAX_ROOMS+1);i++)
    {
        //Checks to see if a room contains more than 0 people
        if(room[i] > 0)
        {
            //If this is the first entry to message, dont concatinate
            if(message[0] == 0)
            {
                sprintf(message,"Room %d: %d people\n",i,room[i]);
            }
            //Else, concatinate
            else
            {
                sprintf(message,"%sRoom %d: %d people\n",message,i,room[i]);
            }
        }
    }
    //If message has content, add the instructions the client should follow
    if(strlen(message) > 0)
    {
        sprintf(message,"%sChoose the room number or type [new] to create a new room:",message);
    }
    //else, message will be the size of 0 (this is done implicitly)
}

void broadcast(USR fromfd, char* message)
{
	// figure out sender address
	struct sockaddr_in cliaddr;
	socklen_t clen = sizeof(cliaddr);
	if (getpeername(fromfd.clisockfd, (struct sockaddr*)&cliaddr, &clen) < 0) error("ERROR Unknown sender!");

	// traverse through all connected clients
	USR* cur = head;
	while (cur != NULL) {
		// check if cur is not the one who sent the message
        printf("\tChecking %d -> %d\n", fromfd.clisockfd, cur->clisockfd);
        // Only sends message when client is not itself, the rooms match, and the room exists
		if ((cur->clisockfd != fromfd.clisockfd) && (cur->room == fromfd.room) && (fromfd.room > 0)) {
			char buffer[512];

			// prepares message with username and color
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

//Function that runs through added a client into a room
void room_accept(USR fromfd)
{
    char buffer[512];
    //Sends a join accept to the client
    sprintf(buffer, "Joining Room %d",fromfd.room);
    int nmsg = strlen(buffer);
    int nsen = send(fromfd.clisockfd, buffer, nmsg, 0);
    if (nsen != nmsg) error("ERROR send() failed");

    //Informs other clients in the room about the client joining the room
    sprintf(buffer, "[has joined the room]");
    broadcast(fromfd, buffer);
}

//Function that checks/adds a client into an existing room
void room_request(int clisockfd, int room)
{
    printf("REQUEST: %d is trying to join room %d\n", clisockfd, room);
    int does_room_exist = 0; // Flag to check if room exists
    // Loops through all the clients
    USR* cur = head;
    while (cur != NULL)
    {
        //Checks to see if any of the clients are currently in the requested room
        if((cur->room == room) && (cur->room != 0))
        {
            //Sets it to 1 if it does exist
            does_room_exist = 1;
        }
        cur = cur->next;
    }

    //If it exists, adds the client into the room and sends room accept
    if(does_room_exist == 1)
    {
        //Loop to find the client
        USR* cur = head;
        while (cur != NULL)
        {
            if (cur->clisockfd == clisockfd)
            {
                break;
            }
            cur = cur->next;
        }
        //Adds client to room
        cur->room = room;
        printf("REQUEST: %d is joining room %d\n", clisockfd, cur->room);
        room_accept(*cur);
    }
    //If it doesnt exist, sends a client an invalid request command to disconnect the client
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

//Function to create a new room
void room_new(int clisockfd)
{
    //Finds client
    USR* cur = head;
    while (cur != NULL)
    {
        if (cur->clisockfd == clisockfd)
        {
            break;
        }
        cur = cur->next;
    }
    //Makes new room by incrementing room counter and assigning it
    cur->room = ++room_counter;
    printf("REQUEST: %d created room %d\n", clisockfd, cur->room);
    //Runs room accept function to let the client know it joined the room
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
        //Recieves client's message
		nrcv = recv(clisockfd, buffer, 255, 0);
        buffer[strcspn(buffer, "\n")] = 0;
		if (nrcv < 0) error("ERROR recv() failed");

        //Finds client in linked list
        USR* cur = head;
        while (cur != NULL)
        {
            if (cur->clisockfd == clisockfd)
            {
                break;
            }
            cur = cur->next;
        }

        //Safty for if the client was not able to be found to avoid crashing
        if(cur == NULL)
        {
            continue;
        }
        else
        {
            //Prints out if the client had recieved a command or a message
            if(buffer[0] < COMMAND_LIST_SIZE)
            {
                printf("RECV CMD %d: %X\n", clisockfd, buffer[0]);
            }
            else
            {
                printf("RECV MSG %d: %s\n", clisockfd, buffer);
            }

            //Checks if client is picking a room
            if(cur->is_picking_room == 1)
            {
                //Client sends "new" so they want a new room
                if(strcmp(buffer,"new") == 0)
                {
                    room_new(clisockfd);
                }
                //Otherwise, they selected a room to join
                else
                {
                    room_request(clisockfd, atoi(buffer));
                }
                //Removes flag that client is pikcing a room
                cur->is_picking_room = 0;
            }
            else
            {
                //Looks at first byte to see if a command was sent
                switch (buffer[0])
                {
                    //Client requests a disconnect
                    case COMMAND_DISCONNECT:
                    {
                        printf("%d Disconnected\n", clisockfd);
                        //Removes from linked list and shows updated client list
                        delete_tail(clisockfd);
                        display_clients();
                        break;
                    }
                    //Client requests a new room
                    case COMMAND_NEW_ROOM:
                    {
                        //Runs new room function
                        room_new(clisockfd);
                        break;
                    }
                    //Client requests an existing room
                    case COMMAND_REQUEST_ROOM:
                    {
                        room_request(clisockfd, buffer[1]);
                        break;
                    }
                    //Client requests to see all the rooms available
                    case COMMAND_SHOW_ROOMS:
                    {
                        //Makes string that contains all the existing rooms
                        char buffer[256];
                        room_list_message(buffer);
                        int nmsg = strlen(buffer);

                        //If the buffer is empty, then no rooms exists
                        if(nmsg == 0)
                        {
                            //Makes new room if no other rooms exist
                            room_new(clisockfd);
                        }
                        else
                        {
                            //If rooms exist, it will show the client the message
                            int nsen = send(clisockfd, buffer, nmsg, 0);
                            if (nsen != nmsg) error("ERROR send() failed");

                            //Tells server that it is expecting a response from the client
                            cur->is_picking_room = 1;
                        }

                        break;
                    }
                    //Client picked a user name
                    case COMMAND_USERNAME:
                    {
                        //Adds the username into the client's datatype
                        sprintf(cur->username,"%s",buffer+1);
                        printf("REQUEST: %d's username is %s\n", clisockfd, cur->username);
                    }
                    //If no commands were ran, then a message is sent
                    default:
                    {
                        // we send the message to everyone except the sender
                        broadcast(*cur, buffer);
                    }
                }
            }
        }
	} while (head != NULL); //Stops loop if all clients have disconnected

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
