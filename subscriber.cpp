#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>

#define BUFLEN 2500
#define MAXCLIENTS 5
#define MAXIDLENGTH 10

using namespace std;

int main(int argc, char const *argv[])
{
	
	int sockfd, n, ret;
	struct sockaddr_in server_address;
	char buffer[BUFLEN];
	int fd_max = 0;

	fd_set read_fds;
	fd_set temp_fds;

	if(argc  != 4){
		cout << "Usage: ./client <client_id> <address> <port>\n";
		return 0;
	}

	FD_ZERO(&read_fds);
	FD_ZERO(&temp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		cout << "Error creating socket \n";
		return 0;
	} 

	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);

	if(sockfd > fd_max){
		fd_max = sockfd;
	}

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &server_address.sin_addr);

	if(ret == 0){
		cout << "Inet aton error \n";
		return 0;
	}

	ret = connect(sockfd, (struct sockaddr*) &server_address, sizeof(server_address));
	if(ret < 0){
		cout << "Error connecting";
		return 0;
	}

	if(strlen(argv[1]) > MAXIDLENGTH){
		cout << "Please choose a shorter ID. Must be max. 10 chars.\n";
		return 0;
	} else {

		// send the client ID to the server
		n = send(sockfd, argv[1], sizeof(argv[1]), 0);

		if(n < 0){
			cout << "Error sending";
			return 0;
		}

	}
	
	if(ret < 0){
		cout << "Connect error \n";
		return 0;
	} 

	while(1){

		temp_fds = read_fds;

		ret = select(sockfd + 1, &temp_fds, NULL, NULL, NULL);

		if(ret < 0){
			cout << "Select error \n";
			return 0;
		} 
		
		if(FD_ISSET(STDIN_FILENO, &temp_fds)){
			
			// input from stdin
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN, stdin);
			
			if(strncmp(buffer, "exit", 4) == 0){

				// in case of exit, clear all sets of file descriptors
			    FD_CLR(sockfd, &read_fds);
			    FD_CLR(sockfd, &temp_fds);
			    FD_CLR(STDIN_FILENO, &read_fds);
			    FD_CLR(STDIN_FILENO, &temp_fds);

			    close(sockfd);
				return 0;
			}
			 		
			char* word;
			char correct_mess[BUFLEN];
			
			memset(correct_mess, 0, BUFLEN);
			strcpy(correct_mess, buffer);


			word = strtok(buffer, " ");
			if(strcmp(word, "subscribe") == 0){
				// if input is a subcribe command, ignore the topic (it will be sent to server)
			 	word = strtok(NULL, " ");
			 	// extract the SF flag 
			 	word = strtok(NULL, "\n");
			 	if(word != NULL){
			 		// if the flag is completed (to avoid segfault) => check if it has a correct value

			 		if(atoi(word) != 0 && atoi(word) != 1){
			 			cout << "Please enter a valid SF flag: 0/1.\n";
			 		} else {
			 			n = send(sockfd, correct_mess, strlen(correct_mess), 0);
			 		}
			 				
			 	} else {

			 		cout << "Please enter a SF flag : 0/1\n";
			 	}
			 				
			 				
			} else if(strcmp(word, "unsubscribe") == 0){
			 	word = strtok(NULL, "\n");
			 	n = send(sockfd, correct_mess, strlen(correct_mess), 0);
			} else {
			 	cout << "Please enter a valid command. \nsubscribe <TOPIC> <SF> \nunsubscribe <TOPIC>\n";
			}
		} else if(FD_ISSET(sockfd, &temp_fds)){

			// recieved message from server
			memset(buffer, 0, BUFLEN);
			n = recv(sockfd, buffer, sizeof(buffer), 0);
			if(n < 0){
				cout << "Error recieving message from server\n";
				return 0;
			}
			if(strncmp(buffer, "exit", 4) == 0){
				close(sockfd);
				return 0;
			} else {
				printf("%s\n", buffer);
			}				
		}		
	}
	return 0;
}