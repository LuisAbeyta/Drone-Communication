##Project description##

The is project consist of one file called drone6.c. The purpose of this file is to act as both the server and client of our system. The drone8.c must be able to both receive data from the port specified by the user. It must also read input from the command line and send these messages to the servers in the config.file. Messages received may not be for your port and will be checked before being printed and stored. Sent message are based on the port and the msg the user supplies, along with other parts of the protocol this includes version, fromPort, location, TTL, flag. This project must also define a grid for users and calculate the euclidean distance between the location values of the sender and receiver. Then print in range, out of range, or not in grid based on the distance. Then message is parsed and printed. Message will also include a sequence number and a send path. The sequence number begins at one and increments for every message, the receiver will ack for all message meant for them with the current sequence number. The send path will add every port along the path to the receiver. you will also be able to make ports move location and also check the send path to not forward to the same port. Messages will be stored and periodically resent or if the location of drone is moved the stored messages will be resent again.

##how to use##

1. open lab1 folder in multiple shells
	- files: drone8.c, config.file, readme.txt, makefile

2. $ make

3. start clienet/server by $ ./drone8 <port number> <row size> <col size> in one shell


##needed file###
- A config.file containing the list of servers and ports to send to is required for this program to run correctly.
