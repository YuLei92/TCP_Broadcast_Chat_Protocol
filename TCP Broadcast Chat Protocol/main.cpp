#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>  //added
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
//#pragma comment(lib,"Ws2_32.lib") //
//#include <ws2def.h>


static const int MAX_PEDING = 512;
static const int PAYLOAD_LEN = 512;
static const int USERNAME_MAX = 16; //define the max of username.
//#define PAYLOAD_LEN 512

//define for select
static const int STDIN = 0;

//type of message
static const int JOIN = 2;
static const int SEND = 4;
static const int FWD = 3;

//type of attribute
static const int USERNAME = 2;
static const int MESSAGE = 4;
static const int REASON = 1;
static const int CLIENT_COUNT = 3;
char user_fwd_name[16];
struct SBCP_Message *message_to_server;
struct SBCP_Message *message_from_client;
struct SBCP_Message *message_to_client;
struct SBCP_Message *message_from_server;

struct SBCP_Attribute {
	unsigned int Type : 16;
	unsigned int Length : 16;
	char Payload[512];
};

struct SBCP_Message {
	unsigned int Vrsn : 9;
	unsigned int Type : 7;
	unsigned int Length : 16;
	struct SBCP_Attribute attribute;
};
/*
void subprocess(int socket, struct SBCP_Message *message_from_client){
	int msglen= sizeof(message_from_client);
//    bzero(&message_from_client, msglen);
	int n=read(socket, message_from_client, msglen);
	printf("Length is %d\n", (unsigned int) message_from_client->attribute.Length);
	int i;
	for(i=0;i<(message_from_client->attribute.Length-4);i++){
		printf("%c",(message_from_client->attribute.Payload[i]));
	}
};
*/



void forward_MSG(int socket_fd, struct SBCP_Message *message_to_client) {
	char buf[PAYLOAD_LEN];
	fgets(buf, sizeof(buf) - 1, stdin);
	int lastIndex = strlen(buf) - 1;
	if (buf[lastIndex] == '\n') {
		buf[lastIndex] == '\0';
	}
	struct SBCP_Attribute attribute;
	attribute.Type = MESSAGE;
	attribute.Length = 4 + strlen(buf);
	bzero((char*)&attribute.Payload, sizeof(attribute.Payload));
	strcpy(attribute.Payload, buf);

	message_to_client->Vrsn = 3;
	message_to_client->Type = SEND;
	message_to_client->Length = 8 + strlen(buf);
	message_to_client->attribute = attribute;

	printf("Ready to SEND...\n");

	if (write(socket_fd, message_to_server, sizeof(struct SBCP_Message)) < 0) {
		printf("Failed...\n");
		perror("Error : Failed to forward to the client...\n");
		exit(0);
	}
	else {
		printf("SEND message to the client successfully...\n");
		printf("The length is %d...\n", sizeof(struct SBCP_Message));
	}
	return;
}

void read_MSG(int socket_fd, struct SBCP_Message *message_from_client) {
	int readno;
	readno = read(socket_fd, message_from_client, sizeof(struct SBCP_Message));
	if (readno < 0) {
		printf("Failed...\n");
		perror("Error : Failed to read the FWD message...\n");
		exit(0);
	}

	if (message_from_client->Vrsn != 3 || message_from_client->Type != FWD || message_from_client->Length <= 0) {
		return;
	} // can not read the header

	if ((message_from_client->attribute.Type != 2 && message_from_client->attribute.Type != 4) || message_from_client->attribute.Length <= 0) {
		return;
	} // can not read the header of attribute

//    printf("Try to read...\n");

	if (message_from_client->attribute.Type == 2) {
		strcpy(user_fwd_name, message_from_client->attribute.Payload);
		printf("The user is %s.\n", user_fwd_name);
	}

	if (message_from_client->attribute.Type == 4) {
		printf("User %s says: %s\n", user_fwd_name, message_from_client->attribute.Payload);
	}
	return;
}
int main(int argc, char* argv[]) {

	static const int MAX_PEDING = 512;
	static const int PAYLOAD_LEN = 512;
	static const int USERNAME_MAX = 16; //define the max of username.
	//#define PAYLOAD_LEN 512

	//define for select
	static const int STDIN = 0;

	//type of message
	static const int JOIN = 2;
	static const int SEND = 4;
	static const int FWD = 3;

	//type of attribute
	static const int USERNAME = 2;
	static const int MESSAGE = 4;
	static const int REASON = 1;
	static const int CLIENT_COUNT = 3;


	struct SBCP_Attribute {
		unsigned int Type : 16;
		unsigned int Length : 16;
		char Payload[512];
	};

	struct SBCP_Message {
		unsigned int Vrsn : 9;
		unsigned int Type : 7;
		unsigned int Length : 16;
		struct SBCP_Attribute attribute;
	};


	struct sockaddr_in sin;
	int len;  //by zeyu
	int socket_id, new_socket_id; //This is the socket
//we don't need buf in the main.
	char Payload[512];
	int socket_fd;

	if (argc != 4) {
		fprintf(stderr, "usage: simplex - talk host\n");
		exit(0);
	}

	char* server_address = argv[1];
	char* port_no = argv[2];
	int max_clients = atoi(argv[3]);

	printf("The port number from command line is %s\n", port_no);

	//    bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(server_address); // maysbe ip address;
	sin.sin_port = htons(atoi(port_no));
	printf("The max_clients is %d\n", max_clients);

	struct SBCP_Message *message_from_client;
	struct SBCP_Message *message_to_client;
	//    message_from_client = malloc(sizeof(struct SBCP_Message));
	//    bzero()
		/* setup passive open*/
	if ((socket_id = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Simplex - talk : socket");
		exit(0);
	}
	printf("Socket successfully created!\n");


	if ((bind(socket_id, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
		perror("simplex -talk: bind\n");
		exit(0);
	}
	else {
		printf("Socket binded.\n");
	}

	printf("Listening to the client...\n");

	if (listen(socket_id, MAX_PEDING) < 0) {
		perror("Cannot find the client!\n");
	}

	//    message_to_client = malloc(sizeof(struct SBCP_Message));

	while (1) {
		if ((new_socket_id = accept(socket_id, (struct sockaddr *)&sin, &len)) < 0) {
			perror("simplex - talk: accept\n");
			exit(0);
		}
		printf("begin\n");

		/*        while(len = read(new_socket_id, &message_from_client, sizeof(struct SBCP_Message))){
					printf("%d\n", message_from_client.attribute.Length - 4);
					printf("%s\n", message_from_client.attribute.Payload);
				}

		// The committed lines is another test for JOIN.
		*/
		fd_set readfds;
		int fdmax;
		FD_ZERO(&readfds);
		FD_SET(STDIN, &readfds);
		FD_SET(socket_fd, &readfds);
		fdmax = socket_fd + 1;
		//End initialize. 

		while (1) {
			        printf("Enter while loop for select again...\n");
			if (select(fdmax, &readfds, NULL, NULL, NULL) < 0) {
				perror("Error: connect\n");
				close(socket_fd);
				exit(0);
			}

			        printf("Select successfully...\n");

			if (FD_ISSET(STDIN, &readfds)) {
				forward_MSG(socket_fd, message_to_client);
			}

			if (FD_ISSET(socket_fd, &readfds)) {
				read_MSG(socket_fd, message_from_client);
			}
		}

	}
	return 0;
}
