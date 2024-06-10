#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>
#define STDIN 0

//hold values for messages that are recieved and valid
struct keyValue
{
    char key[20][20];
    char value[20][100];
	int size;
	int fromPort;
	int toPort;
	int seq;
	int ack; //true or false whether or not message is an ack
	int resend; //decrement every time message is resent or set to zero if ack for message is seen
};


struct keyValue rec[1000];
int openIndex = 0;
//function to change address that is being sent to in main
void serverSetup(struct sockaddr_in *server_address, char ip[],int port)
{
	server_address->sin_family = AF_INET; /* use AF_INET addresses */
	server_address->sin_port = htons(port); /* convert port number */
	server_address->sin_addr.s_addr = inet_addr(ip); 
}
//find the index where the port currently received is stored, to detemine current seq number to send, ack number to receive, and message to receive from port
int findPort(int currentPort, int port[], int count)
{

	for(int i = 0; i < count; i++)
	{
		if(currentPort == port[i])
		{
			return i;
		}
	}
	return -1;
}


int checkPort(int portNumber, char port[])
{
	//check that port number is a number
	for (int i=0;i<strlen(port); i++){
		if (!isdigit(port[i]))
		{
			printf ("The Portnumber isn't a number!\n");
			return -1;
		}
	}
	
	// exit if a portnumber too big or too small  
	if ((portNumber > 65535) || (portNumber < 0)){
		printf ("you entered an invalid socket number\n");
		return -1;
	}
	
	return 0;
}
int checkIP(struct sockaddr_in inaddr, char ip[])
{
	//check for a valid ip address
	//dotted notation and has valid numbers
	if (!inet_pton(AF_INET, ip, &inaddr)){
		printf ("error, bad ip address\n");
		return -1; /* just leave if is incorrect */
	}
	return 0;
}

void storeMsg(char * input)
{
	//string to hold the key and the value
    char key[512] = "";
    char value[512] = ""; 
	int keyStart = 0;  //where the key value starts
	bool msgBegin = false; //indicates a " has been seen and we are looking at a message
    int limit = 0;		//key stop at limit -1 and value starts at limit + 1
	int i;				//variable for loop
	int j = 0;			//keep track of key values pairs in structure
	//loop through the string
	
	rec[openIndex].resend = 5;
	for(i = 0; i < strlen(input);i++){
		

		//if the character of the string is a punctation and not a '"' or '-' or the message of the string has begun
		if(input[i] == ':' && input[i] != '"' && input[i] != '-' && !msgBegin)
		{
			limit = i;
		}
		//if we see the '"' character
		else if(input[i] == '"')
		{
			//see the second quotation mark end the message
			if(msgBegin)
			{
				msgBegin = false;
			}
			//see the first quotation mark begin message
			else
			{
				msgBegin = true;
			}
		}
		//space character ends a key:value pair, only look at if not in the message
		else if(input[i] == ' ' && !msgBegin)
		{
			//the delimiter will be found at a greater index than the start of a key
			//only enter if this is true, else the input is bad and we ignore it
			if(limit > keyStart)
			{
				//reset strings
				memset(key, 0, 512);
				memset(value, 0, 512);
				
				//copy values from input to key and value
				strncpy(key, input + keyStart, limit - keyStart);
				strncpy(value, input + limit+1, i - limit - 1);
				
				//set values of struct for later usage
				if(strcmp(key, " toPort") == 0 || strcmp(key, "toPort") == 0)
				{
					rec[openIndex].toPort = atoi(value);
				}
				if(strcmp(key, " fromPort") == 0 || strcmp(key, "fromPort") == 0)
				{
					rec[openIndex].fromPort = atoi(value);
				}
				if(strcmp(key, " seqNumber") == 0 || strcmp(key, "seqNumber") == 0)
				{
					rec[openIndex].seq = atoi(value);
				}
				if(strcmp(key, " type") == 0 || strcmp(key, "type") == 0)
				{
					rec[openIndex].ack = 1;
				}
			
				//store key and value
				strncpy(rec[openIndex].key[j], key, strlen(key));
				strncpy(rec[openIndex].value[j], value,strlen(value));
				
				//index that is open in the struct
				j = j + 1;

			}
			
			keyStart = i;
		}
	   
	}
	//for last key value pair if input is valud
	if(limit > keyStart){
		
		//reset strings	
		memset(key, 0, 512);
		memset(value, 0, 512);
		
		//copy values from input to key and value
		strncpy(key, input + keyStart, limit - keyStart);
		strncpy(value, input + limit+1, i - limit - 1);
		
		//set values of struct for later usage
		if(strcmp(key, " toPort") == 0 || strcmp(key, "toPort") == 0)
		{
			rec[openIndex].toPort = atoi(value);
		}
		if(strcmp(key, " fromPort") == 0 || strcmp(key, "fromPort") == 0)
		{
			rec[openIndex].fromPort = atoi(value);
		}
		if(strcmp(key, " seqNumber") == 0 || strcmp(key, "seqNumber") == 0)
		{
			rec[openIndex].seq = atoi(value);
		}
		if(strcmp(key, " type") == 0 || strcmp(key, "type") == 0)
		{
			rec[openIndex].ack = 1;
		}
		
		//store key and value
		strncpy(rec[openIndex].key[j], key, strlen(key));
		strncpy(rec[openIndex].value[j], value,strlen(value));
		
		//index that is open in the struct		
		j = j + 1;
		
		
	}
	rec[openIndex].size = j;
	//openIndex = openIndex + 1;
	//printf("openIndex: %i size of last message: %i \n", openIndex, j);
}
//splits message into key and values and stores
void parseMsg(char * input)
{
	//string to hold the key and the value
    char key[512] = "";
    char value[512] = ""; 
	int keyStart = 0;  //where the key value starts
	bool msgBegin = false; //indicates a " has been seen and we are looking at a message
    int limit = 0;		//key stop at limit -1 and value starts at limit + 1
	int i;				//variable for loop
	//loop through the string
	for(i = 0; i < strlen(input);i++){
		

		//if the character of the string is a punctation and not a '"' or '-' or the message of the string has begun
		if(input[i] == ':' && input[i] != '"' && input[i] != '-' && !msgBegin)
		{
			limit = i;
		}
		//if we see the '"' character
		else if(input[i] == '"')
		{
			//see the second quotation mark end the message
			if(msgBegin)
			{
				msgBegin = false;
			}
			//see the first quotation mark begin message
			else
			{
				msgBegin = true;
			}
		}
		//space character ends a key:value pair, only look at if not in the message
		else if(input[i] == ' ' && !msgBegin)
		{
			//the delimiter will be found at a greater index than the start of a key
			//only enter if this is true, else the input is bad and we ignore it
			if(limit > keyStart)
			{
				//reset strings
				memset(key, 0, 512);
				memset(value, 0, 512);
				
				//copy values from input to key and value
				strncpy(key, input + keyStart, limit - keyStart);
				strncpy(value, input + limit+1, i - limit - 1);
				
				//store key and value
				//strncpy(rec[openIndex].key[j], key, strlen(key));
				//strncpy(rec[openIndex].value[j], value,strlen(value));
				
				//index that is open in the struct
				//j = j + 1;
				
				//print out the key and value
				printf("%20s", key);
				printf("%20s\n", value);
			}
			
			keyStart = i;
			
		}
	   
	}
	//for last key value pair if input is valud
	if(limit > keyStart){
		
		//reset strings	
		memset(key, 0, 512);
		memset(value, 0, 512);
		
		//copy values from input to key and value
		strncpy(key, input + keyStart, limit - keyStart);
		strncpy(value, input + limit+1, i - limit - 1);
		
		//store key and value
		//strncpy(rec[openIndex].key[j], key, strlen(key));
		//strncpy(rec[openIndex].value[j], value,strlen(value));
		
		//index that is open in the struct		
		//j = j + 1;
		
		//print out the key and value
		printf("%20s", key);
		printf("%20s\n", value);
		
	}
	printf("\n");

}
// function to check for correct port in message
int inspectMsg(char * input, char * port, int * loc, int * TTL, int * cport, int * cseq, int * ack, int * move, int * toPort, char * send)
{
	//string to hold the key and the value
    char key[512] = "";
    char value[512] = ""; 
	int keyStart = 0;  //where the key value starts
	bool msgBegin = false; //indicates a " has been seen and we are looking at a message
    int limit = 0;		//key stop at limit -1 and value starts at limit + 1
	int i;				//variable for loop
	int validPort = -1; //want to check that both port and
	int validVer = -1;	//version are valid based on agreed upon protocol
	//loop through the string
	for(i = 0; i < strlen(input);i++){

		//if the character of the string is a ':' or the message of the string has not begun
		if(input[i] == ':' && !msgBegin)
		{
			limit = i;
		}
		//if we see the '"' character
		else if(input[i] == '"')
		{
			//see the second quotation mark end the message
			if(msgBegin)
			{
				msgBegin = false;
			}
			//see the first quotation mark begin message
			else
			{
				msgBegin = true;
			}
		}
		//space character ends a key:value pair, only look at if not in the message
		else if(input[i] == ' ' && !msgBegin)
		{
			//the delimiter will be found at a greater index than the start of a key
			//only enter if this is true, else the input is bad and we ignore it
			if(limit > keyStart)
			{
				//reset strings
				memset(key, 0, 512);
				memset(value, 0, 512);
				
				//copy values from input to key and value
				strncpy(key, input + keyStart, limit - keyStart);
				strncpy(value, input + limit+1, i - limit - 1);
				//get location value for current message
				if(strcmp(key, "location") == 0)
				{
					*loc = atoi(value);
				}
				//get TTL of the current message
				if(strcmp(key, "TTL") == 0)
				{
					*TTL = atoi(value);
				}
				
				if(strcmp(key,"fromPort") == 0)
				{
					*cport = strtol(value, NULL, 10);
				}
				if(strcmp(key,"toPort")== 0)
				{
					*toPort = strtol(value, NULL, 10);
				}
				if(strcmp(key,"seqNumber") == 0)
				{
					*cseq = atoi(value);
				}
				if(strcmp(key,"send-path")==0)
				{
					strcpy(send, value);
				}
				if(strcmp(key,"type") == 0)
				{
					*ack = 1;
				}
				if(strcmp(key,"move") == 0)
				{
					*move = atoi(value);
				}	
				strcat(key, ":");
				strcat(key,value);
				//if comparsion is true correct port return
				
				if(strcmp(key, port) == 0)
				{
					validPort = 0;
				}
				
				if(strcmp(key, "version:8") == 0)
				{
					validVer = 0;
				}
				
			}
			
			keyStart = i+1;
			
		}
	   
	}
	//for last key value pair if input is valud
		
	if(limit > keyStart)
	{
		//reset strings
		memset(key, 0, 512);
		memset(value, 0, 512);
		
		//copy values from input to key and value
		strncpy(key, input + keyStart, limit - keyStart);
		strncpy(value, input + limit+1, i - limit - 1);

		value[strcspn(value, "\n")] = 0;
		
		if(strcmp(key, "location") == 0)
		{
			*loc = atoi(value);
		}
		
		//get TTL of the current message
		if(strcmp(key, "TTL") == 0)
		{
			*TTL = atoi(value);
		}
		
		if(strcmp(key,"fromPort") == 0)
		{
			*cport = strtol(value, NULL, 10);
		}
		if(strcmp(key,"toPort")== 0)
		{
			*toPort = strtol(value, NULL, 10);
		}
		if(strcmp(key,"seqNumber") == 0)
		{
			*cseq = atoi(value);
		}
		if(strcmp(key,"send-path")==0)
		{
			strcpy(send, value);
		}
		if(strcmp(key,"type") == 0)
		{
			*ack = 1;
		}
		if(strcmp(key,"move") == 0)
		{
			*move = atoi(value);
		}
		
		strcat(key, ":");
		strcat(key,value);
		//if comparsion is true correct port return
		if(strcmp(key, port) == 0)
		{
			validPort = 0;
		}
		
		if(strcmp(key, "version:8") == 0)
		{
			validVer = 0;
		}
		
	}
	//correct port not found
	
	if(validVer == 0 && validPort == 0)
	{
		return 0;
	}
	
	return -1;
}

int euclideanDistance(int location, int recLoc, int rows, int cols)
{
	
	if( recLoc > (rows * cols) || recLoc == 0)
	{
		return -1;	
	}
	
	double results;
	int out;
	int y1 = (location - 1) / cols;  //row of your location 
	int x1 = (location - 1) % cols;  //col of your location
	int y2 = (recLoc - 1) / cols;  //row of recLoc
	int x2 = (recLoc - 1) % cols;  //col of recLoc
	
	//printf("Your position row: %i col: %i\n", y1, x1);
	//printf("Rec position row: %i col: %i\n", y2, x2);
	
	results =  pow((x2 - x1),2) + pow((y2-y1),2); 
	
	out = sqrt(results);
	
	return out;
}

//check ports on send path if change onpath to true
void checkSend(char * sendPath, int pNumbers[], bool *onPath, int count)
{
	char * token = strtok(sendPath, ",");
	while(token != NULL)
	{		
		
		for(int i = 0; i < count; i++)
		{
			if(atoi(token) == pNumbers[i])
			{
				onPath[i] = true;
			}
		}
		
		token = strtok(NULL, ",");
		
		
	}
}
//this checks to see if current message received is stored, if true we return that index, else we return -1
int checkStorage(int toPort, int fromPort, int seq, int ack)
{
	for(int i =0; i < openIndex; i++)
	{
		//if all true we received the ack of the match and we can set resend to zero 
		if(rec[i].toPort == toPort && rec[i].fromPort == fromPort && rec[i].seq == seq && rec[i].ack == ack)
		{
			return i;
		}
	}
	return -1;
}
//toport would be fromport of current msg, and fromport is toport of current msg. this is checking for the message that coincides with ack to set resend value to zero
int checkAck(int toPort, int fromPort, int seq)
{
	for(int i =0; i < openIndex; i++)
	{
		//if all true we received the ack of the match and we can set resend to zero 
		if(rec[i].toPort == toPort && rec[i].fromPort == fromPort && rec[i].seq == seq && rec[i].ack == 0)
		{
			rec[i].resend = 0;
			return 0;
		}
	}
	return 0;
}
int main(int argc, char *argv[])
{
	int sd;
	int rc;
	int i;
	//server address for binding to socket and send address is for sending to others
	struct sockaddr_in server_address;
	struct sockaddr_in send_address;
	struct sockaddr_in from_address;
	struct sockaddr_in inaddr;
	int flags = 0;
	socklen_t fromLength = sizeof(struct sockaddr);
	//int DEBUG =0;
	int portNumber;
	char sendPort[10];
	//for creating message that is sent
	char toPort[40] = {0};
	char msgIN[150] = {0};
	char bufferOut[512];
	char bufferReceived[1000];
	char portVal[20] = "toPort:";	//for checking messages received
	//check if message is for correct port and version 
	int check;
	//hold TTL value of currently received message
	int TTL = 0;
	char sendTTL[5];
	//for parsing config file
	char line[30];
	
	char serverIP[100][20];	//arrays to store ips and ports 
	int pNumbers[100] = {0};	//store each port in an index and relates to all arrays below
	int seqNum[100]; //sequence numbers used to send start at 1
	int recSeq[100]; //what sequence number have been received, when equal plus one
	int recAck[100];	//what ack have been returned, when equal plus one, duplicates not shown
	bool onPath[100];	//if forwarding message check current send path and do not send if port index is true
	int count = 0; //number of servers to sendto
	
	int seqLoc;	//index of sequence number for current send
	int currentPort; //on receive hold the current fromport and sequence number
	int currentSeq;	//
	int tp;			//hold current messages toport
	char currentPath[30]; //if forwarding messaging store send path to use
	int ack; //true or false on receiving an acked message
	int move; //true or false on receiving a message to move locations
	char port[10];
	char location[10];//holds your location when found
	
	int rows = 0;
	int cols = 0;
	int myLoc = 0;
	int recLoc = 0;
	int dist;
	int msgIndex; //when forwarding message store index of message if duplicate or negative one if a new message
	//opening config file
	FILE* file1  = fopen("config.file", "r");
	
	//new thing for select 
	fd_set socketFDS; //the socket descriptor set
	int maxSD; // tells the OS how many sockets are set
	
	for(int j = 0; j < 100; j++)
	{
		seqNum[j] = 1;
		recSeq[j] = 0;
		recAck[j] = 0;
	}
	
	
	//check if file opened correctly
	if(!file1)
	{
		printf("\n unable to open file1");
		exit(1);
	}
	
	/* first, decide if we have the right number of parameters */
	if (argc < 4){
		printf("usage is: server <portnumber>\n");
		exit (1);
	}
	
	/* now fill in the address data structure we use to sendto the server */
	for (i=0;i<strlen(argv[1]); i++){
		if (!isdigit(argv[1][i]))
		{
			printf ("The Portnumber isn't a number!\n");
			exit(1);
		}
	}

	portNumber = strtol(argv[1], NULL, 10); /* many ways to do this */

	if ((portNumber > 65535) || (portNumber < 0)){
		printf ("you entered an invalid port number\n");
		exit (1);
	}
	
	//for adding to send-path field during routing of message
	sprintf(sendPort, "%i", portNumber);

	
	//parse config file
	while(fgets(line, sizeof(line), file1))
	{
		//null string for storage
		memset(serverIP[count], 0, 20);
		memset(port, 0, 10);

		sscanf(line, "%s %s" ,serverIP[count], port);
		
		pNumbers[count] = strtol(port, NULL, 10);

		//if this line is for your port then set location
		if(pNumbers[count] == portNumber)
		{
			sscanf(line, "%s %s %s", serverIP[count], port, location);
			//location is at end remove newline character
			//location[strcspn(location, "\n")] = 0;
			//printf("%s\n", location);
		}
		
		//printf("%s\n", serverIP[count]);
		//printf("%s\n", port);
		
		//check if ip and port are good, if not don't increment and write over this index
		if(checkIP(inaddr,serverIP[count]) == 0 && checkPort(pNumbers[count],port) == 0)
		{
			count++;
		}
			
	}
	myLoc = atoi(location);
	printf("%s\n", location);
	printf("number of servers to send to %d\n", count);
	
	
	
	sd = socket(AF_INET, SOCK_DGRAM, 0); // create socket

	/* always check for errors */
	if (sd == -1){ /* means some kind of error occured */
		perror ("socket");
		exit(1); /* just leave if wrong number entered */
	}
	
	server_address.sin_family = AF_INET; /* use AF_INET addresses */
	server_address.sin_port = htons(portNumber); /* convert port number */
	server_address.sin_addr.s_addr = INADDR_ANY; /* any adapter */
	
	/* the next step is to bind to the address */
	rc = bind (sd, (struct sockaddr *)&server_address,
		 sizeof(struct sockaddr ));

	if (rc < 0){
		perror("bind");
		exit (1);
	}
	//for checking the messages is from correct port
	strcat(portVal, argv[1]);
	printf("%s\n",portVal);
	
	//printf("enter the size of the matrix. rows then columns\n");
	//scanf("%i %i", &rows, &cols);
	
	struct timeval timeout;
	
	
	rows = atoi(argv[2]);
	cols = atoi(argv[3]);
	
	printf("rows: %i and cols: %i\n", rows, cols);
	
	printf("enter the port you would like to send. ## \n");
	
	for(;;){
		memset(bufferReceived, 0, 1000);
		memset(msgIN, 0, 150);
		memset(bufferOut,0,512);
		memset(sendTTL,0,5);
		memset(currentPath,0,30);
		memset(onPath, 0, sizeof(onPath));
		dist = 0;
		ack = 0;
		seqLoc = -1;
		move = 0;
		timeout.tv_sec = 20;
		timeout.tv_usec = 0;
		//select stuff
		FD_ZERO(&socketFDS); //zero out socketFDS
		FD_SET(sd, &socketFDS); //set bit gor socket descriptor 
		FD_SET(STDIN, &socketFDS);  //set bit in std in
		if(STDIN > sd)
			maxSD = STDIN;
		else
			maxSD = sd;
		
		rc = select(maxSD+1, &socketFDS, NULL, NULL, &timeout); //block until something arrives
		if(rc == 0)
		{
			printf("timeout\n");
			
			//loop through messages
			for(int m = 0; m < openIndex; m++)
			{
				memset(bufferOut,0,512);
				memset(currentPath,0,30);
				memset(onPath, 0, sizeof(onPath));
				memset(sendTTL,0,5);
				
				if(rec[m].resend != 0)
				{
					//reconstruct message from key value pairs
					for(int n = 0; n < rec[m].size; n++)
					{
						if(strcmp(rec[m].key[n], " location") == 0 || strcmp(rec[m].key[n], "location") == 0)
						{
							//printf("%s\n", rec[msgIndex].key[k]);
							strcat(bufferOut, rec[m].key[n]);
							strcat(bufferOut, ":");
							strcat(bufferOut, location);
						}
						else if(strcmp(rec[m].key[n], " send-path") == 0 || strcmp(rec[m].key[n], "send-path") == 0)
						{
							strcpy(currentPath, rec[m].value[n]);
							//change this process
							strcat(bufferOut, rec[m].key[n]);
							strcat(bufferOut, ":");
							strcat(bufferOut, currentPath);
							if(strcmp(rec[m].value[n], sendPort) != 0)
							{
								strcat(bufferOut, ",");
								strcat(bufferOut, sendPort);
							}
						}
						else if(strcmp(rec[m].key[n], " TTL") == 0 || strcmp(rec[m].key[n], "TTL")== 0)
						{
							TTL = atoi(rec[m].value[n]);
							TTL--;
							sprintf(sendTTL, "%i", TTL);
							strcat(bufferOut, rec[m].key[n]);
							strcat(bufferOut, ":");
							strcat(bufferOut, sendTTL);
						}
						else
						{
							strcat(bufferOut, rec[m].key[n]);
							strcat(bufferOut, ":");
							strcat(bufferOut, rec[m].value[n]);
						}
					}
					
					//function to help not send to ports that are on path
					checkSend(currentPath, pNumbers, onPath, count);
					
					//resend messsage below
					for(int j = 0; j < count; j++)
					{
						//printf("%s ", serverIP[j]);
						//printf("%i\n", pNumbers[j]);
						//change send address
						if(pNumbers[j] != portNumber && onPath[j] == false)
						{
							serverSetup(&send_address, serverIP[j],pNumbers[j]);
							//send message
							rc = sendto(sd, bufferOut, strlen(bufferOut), 0,
							(struct sockaddr *) &send_address, sizeof(send_address));
						}
					}
					rec[m].resend--;
				}
			}
		}
		if(FD_ISSET(STDIN, &socketFDS)){
			fgets(toPort, sizeof(toPort), stdin);
			toPort[strcspn(toPort, "\n")] = 0;
			
			printf("enter the msg you would like to send or enter move to change this ports location. ## \n");
			fgets(msgIN, sizeof(msgIN), stdin);
			msgIN[strcspn(msgIN, "\n")] = 0;
			
			
			seqLoc = findPort(atoi(toPort), pNumbers, count); 
			
			//printf("sent '%s'\n", bufferOut);
			if(strcmp(msgIN, "move") == 0)
			{
				memset(msgIN, 0, 150);
				
				printf("what location would you like this port to move to\n");
				
				fgets(msgIN, sizeof(msgIN), stdin);
				msgIN[strcspn(msgIN, "\n")] = 0;
				
				snprintf(bufferOut, 512, "toPort:%s move:%s version:8 fromPort:%i location:%i TTL:4 flag:0 seqNumber:%i send-path:%i",toPort,msgIN, portNumber, myLoc, seqNum[seqLoc], portNumber);
				
				for(int j = 0; j < count; j++)
				{
					//printf("%s ", serverIP[j]);
					//printf("%i\n", pNumbers[j]);
					//change send address
					serverSetup(&send_address, serverIP[j],pNumbers[j]);
					//send message
					rc = sendto(sd, bufferOut, strlen(bufferOut), 0,
					(struct sockaddr *) &send_address, sizeof(send_address));

					 //check return code of the sendto
					//if less than length of string there is an error
					//if (rc < strlen(bufferOut)){
					//	perror ("sendto");
					// do i exit?
					//}
				}
				seqNum[seqLoc] = seqNum[seqLoc] + 1;
				//Move messages will not be acked back, increase this to prepare for ack of next sequence number
				recAck[seqLoc] = recAck[seqLoc] + 1;
			}
			else
			{

				snprintf(bufferOut, 512, "toPort:%s msg:%s version:8 fromPort:%i location:%i TTL:4 flag:0 seqNumber:%i send-path:%i",toPort,msgIN, portNumber, myLoc, seqNum[seqLoc], portNumber);
				
				for(int j = 0; j < count; j++)
				{
					//printf("%s ", serverIP[j]);
					//printf("%i\n", pNumbers[j]);
					//change send address
					serverSetup(&send_address, serverIP[j],pNumbers[j]);
					//send message
					rc = sendto(sd, bufferOut, strlen(bufferOut), 0,
					(struct sockaddr *) &send_address, sizeof(send_address));

					 //check return code of the sendto
					//if less than length of string there is an error
					//if (rc < strlen(bufferOut)){
					//	perror ("sendto");
					// do i exit?
					//}
				}
				seqNum[seqLoc] = seqNum[seqLoc] + 1;
			}
			
			storeMsg(bufferOut);
			openIndex++;
			
		}
		if(FD_ISSET(sd, &socketFDS)){
			//receive message from port
			rc = recvfrom(sd, bufferReceived, 1000, flags,
			(struct sockaddr *)&from_address, &fromLength);
			
			//printf("%s\n", bufferReceived);
			
			/* check for any possible errors */
			if (rc <=0){
				perror ("recvfrom");
				printf ("leaving, due to socket error on recvfrom\n");
				exit (1);
			}
			
			//printf("received %s\n", bufferReceived);
			//check correct port and version 
			check = inspectMsg(bufferReceived, portVal, &recLoc, &TTL, &currentPort, &currentSeq, &ack, &move, &tp, currentPath);
			dist = euclideanDistance(myLoc, recLoc, rows, cols);
			


			//in range and message for current port
			if(check == 0 && dist <= 2 && dist >= 0 && TTL > 0)
			{
			
		
				//find seq number
				seqLoc = findPort(currentPort, pNumbers, count);
				//send ack
				//message is a move request
				if(move != 0 && (recSeq[seqLoc] < currentSeq))
				{
					printf("My location: %i Sending location: %i Euclidean Distance: %i\n", myLoc, recLoc, dist);
					parseMsg(bufferReceived);
					memset(location,0,10);
					sprintf(location, "%d", move);
					printf("%s\n", location);
					myLoc = move;
					
					recSeq[seqLoc] = currentSeq;
					
					//loop through messages
					for(int m = 0; m < openIndex; m++)
					{
						memset(bufferOut,0,512);
						memset(currentPath,0,30);
						memset(onPath, 0, sizeof(onPath));
						memset(sendTTL,0,5);
						
						if(rec[m].resend != 0)
						{
							//reconstruct message from key value pairs
							for(int n = 0; n < rec[m].size; n++)
							{
								if(strcmp(rec[m].key[n], " location") == 0 || strcmp(rec[m].key[n], "location") == 0)
								{
									//printf("%s\n", rec[msgIndex].key[k]);
									strcat(bufferOut, rec[m].key[n]);
									strcat(bufferOut, ":");
									strcat(bufferOut, location);
								}
								else if(strcmp(rec[m].key[n], " send-path") == 0 || strcmp(rec[m].key[n], "send-path") == 0)
								{
									strcpy(currentPath, rec[m].value[n]);
									//change this process
									strcat(bufferOut, rec[m].key[n]);
									strcat(bufferOut, ":");
									strcat(bufferOut, currentPath);
									if(strcmp(rec[m].value[n], sendPort) != 0)
									{
										strcat(bufferOut, ",");
										strcat(bufferOut, sendPort);
									}
								}
								else if(strcmp(rec[m].key[n], " TTL") == 0 || strcmp(rec[m].key[n], "TTL")== 0)
								{
									TTL = atoi(rec[m].value[n]);
									TTL--;
									sprintf(sendTTL, "%i", TTL);
									strcat(bufferOut, rec[m].key[n]);
									strcat(bufferOut, ":");
									strcat(bufferOut, sendTTL);
								}
								else
								{
									strcat(bufferOut, rec[m].key[n]);
									strcat(bufferOut, ":");
									strcat(bufferOut, rec[m].value[n]);
								}
							}
							
							//function to help not send to ports that are on path
							checkSend(currentPath, pNumbers, onPath, count);
							
							//resend messsage below
							for(int j = 0; j < count; j++)
							{
								//printf("%s ", serverIP[j]);
								//printf("%i\n", pNumbers[j]);
								//change send address
								if(pNumbers[j] != portNumber && onPath[j] == false)
								{
									serverSetup(&send_address, serverIP[j],pNumbers[j]);
									//send message
									rc = sendto(sd, bufferOut, strlen(bufferOut), 0,
									(struct sockaddr *) &send_address, sizeof(send_address));
								}
							}
							rec[m].resend--;
						}
					}
				}
				
				//message not an ack and first of that sequence number
				if(ack == 0 && (recSeq[seqLoc] < currentSeq) && move == 0)
				{
					
					
					printf("My location: %i Sending location: %i Euclidean Distance: %i\n", myLoc, recLoc, dist);
					parseMsg(bufferReceived);
					
					printf("%20s", "mylocation");
					printf("%20s\n", location);
					
					snprintf(bufferOut, 512, "toPort:%i version:8 fromPort:%i location:%i type:ACK TTL:4 flag:0 seqNumber:%i send-path:%i",currentPort, portNumber, myLoc, currentSeq, portNumber);
					
					
					for(int j = 0; j < count; j++)
					{
						serverSetup(&send_address, serverIP[j],pNumbers[j]);
						//send message
						rc = sendto(sd, bufferOut, strlen(bufferOut), 0,
							(struct sockaddr *) &send_address, sizeof(send_address));
					}
					
					recSeq[seqLoc] = currentSeq;
				}
				//duplicate message
				else if(ack == 0 &&(recSeq[seqLoc] == currentSeq) && move == 0)
				{
					printf("Duplicate message receive\n");
					snprintf(bufferOut, 512, "toPort:%i version:8 fromPort:%i location:%i type:ACK TTL:4 flag:0 seqNumber:%i send-path:%i",currentPort, portNumber, myLoc, currentSeq, portNumber);
					
					for(int j = 0; j < count; j++)
					{
						serverSetup(&send_address, serverIP[j],pNumbers[j]);
						//send message
						rc = sendto(sd, bufferOut, strlen(bufferOut), 0,
							(struct sockaddr *) &send_address, sizeof(send_address));
					}
				}
				
				//ack not duplicate
				if(ack == 1 && (recAck[seqLoc] < currentSeq))
				{
					parseMsg(bufferReceived);
					
					recAck[seqLoc] = currentSeq;
					
					checkAck(currentPort,tp,currentSeq);
					printf("here");
				}
				//duplicate ack
				else if(ack == 1 && (recAck[seqLoc] == currentSeq))
				{
					printf("Duplicate ack received\n");
				}
				
			}
			//in range and message not for current port, decrement TTL, change location, forward message
			else if(check == -1 && dist <= 2 && dist >= 0 && TTL > 0)
			{
				
				//decrement the TTL value
				TTL = TTL - 1;
				//check if message exist
				msgIndex = checkStorage(tp,currentPort,currentSeq,ack);
				//if message is an ack function to set resend value of original message to zero
				if(ack == 1)
				{
					checkAck(currentPort,tp,currentSeq);
				}
				
				//if stored forward old message, but change send path and ttl to new message and still change location
				if(msgIndex > -1)
				{
						for(int k =0; k < rec[msgIndex].size; k++)
						{
							
							if(strcmp(rec[msgIndex].key[k], " location") == 0 || strcmp(rec[msgIndex].key[k], "location") == 0)
							{
								//printf("%s\n", rec[msgIndex].key[k]);
								strcat(bufferOut, rec[msgIndex].key[k]);
								strcat(bufferOut, ":");
								strcat(bufferOut, location);
							}
							else if(strcmp(rec[msgIndex].key[k], " TTL") == 0 || strcmp(rec[msgIndex].key[k], "TTL") == 0)
							{
								//printf("%s\n", rec[msgIndex].key[k]);
								sprintf(sendTTL, "%i", TTL);
								strcat(bufferOut, rec[msgIndex].key[k]);
								strcat(bufferOut, ":");
								strcat(bufferOut, sendTTL);
							}
							else if(strcmp(rec[msgIndex].key[k], " send-path") == 0 || strcmp(rec[msgIndex].key[k], "send-path") == 0)
							{
								//strcpy(currentPath, rec[msgIndex].value[k]); not needed anymore get from inspect message
								//change this process
								strcat(bufferOut, rec[msgIndex].key[k]);
								strcat(bufferOut, ":");
								strcat(bufferOut, currentPath);
								strcat(bufferOut, ",");
								strcat(bufferOut, sendPort);
							}
							else
							{
								strcat(bufferOut, rec[msgIndex].key[k]);
								strcat(bufferOut, ":");
								strcat(bufferOut, rec[msgIndex].value[k]);
							}
						}
				}
				else if(msgIndex == -1)
				{
					//if no store msg then store msg and increment openIndex
					//store message
					storeMsg(bufferReceived);
					//get message values into single string
					for(int k =0; k < rec[openIndex].size; k++)
					{
						
						if(strcmp(rec[openIndex].key[k], " location") == 0 || strcmp(rec[openIndex].key[k], "location") == 0)
						{
							//printf("%s\n", rec[openIndex].key[k]);
							strcat(bufferOut, rec[openIndex].key[k]);
							strcat(bufferOut, ":");
							strcat(bufferOut, location);
						}
						else if(strcmp(rec[openIndex].key[k], " TTL") == 0 || strcmp(rec[openIndex].key[k], "TTL") == 0)
						{
							//printf("%s\n", rec[openIndex].key[k]);
							sprintf(sendTTL, "%i", TTL);
							strcat(bufferOut, rec[openIndex].key[k]);
							strcat(bufferOut, ":");
							strcat(bufferOut, sendTTL);
						}
						else if(strcmp(rec[openIndex].key[k], " send-path") == 0 || strcmp(rec[openIndex].key[k], "send-path") == 0)
						{
							//strcpy(currentPath, rec[openIndex].value[k]); not needed anymore get from inspect message
							//change this process
							strcat(bufferOut, rec[openIndex].key[k]);
							strcat(bufferOut, ":");
							strcat(bufferOut, rec[openIndex].value[k]);
							strcat(bufferOut, ",");
							strcat(bufferOut, sendPort);
						}
						else
						{
							strcat(bufferOut, rec[openIndex].key[k]);
							strcat(bufferOut, ":");
							strcat(bufferOut, rec[openIndex].value[k]);
						}
					}
					openIndex++;
				}
				
				//function to help not send to ports that are on path
				checkSend(currentPath, pNumbers, onPath, count);
				
				//forward messsage below
				for(int j = 0; j < count; j++)
				{
					//printf("%s ", serverIP[j]);
					//printf("%i\n", pNumbers[j]);
					//change send address
					if(pNumbers[j] != portNumber && onPath[j] == false)
					{
						serverSetup(&send_address, serverIP[j],pNumbers[j]);
						//send message
						rc = sendto(sd, bufferOut, strlen(bufferOut), 0,
						(struct sockaddr *) &send_address, sizeof(send_address));
					}
				}
				
			
			}
			//message not in grid
			else if(dist == -1)
			{
				printf("My location: %i Sending location: %i Euclidean Distance: %i\n", myLoc, recLoc, dist);
				printf("Not in Grid\n");
			}
			//message not in range
			else if(dist > 2)
			{
				//printf("My location: %i Sending location: %i Euclidean Distance: %i\n", myLoc, recLoc, dist);
				//printf("Message Not In Range\n");
			}	
		}
	}
	
	printf("%i", openIndex);
	return 0;
		
} // end of main
