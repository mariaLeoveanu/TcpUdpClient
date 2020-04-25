#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <iostream>
#include <sstream>
#include <math.h>
#include <map>
#include <vector>
#include <algorithm>
#include <bits/stdc++.h> 

#define BUFLEN 2500
#define MAXCLIENTS 5

using namespace std;

struct msg_udp{
	char topic[50] ;
	char type;
	char payload[1500];
}__attribute__((packed));


int main(int argc, char const *argv[])
{
	int sockfd, newsockfd, portno, sockfd_UDP;
	int opt;
	int n, i, ret;
	int fd_max;

	char buffer[BUFLEN];

	struct sockaddr_in server_address, client_address;

	socklen_t client_length;
	socklen_t address_length;

	std::map<int, std::string> sockfd_id;
	std::map<std::string, int> id_sockfd;
	std::map<std::string, std::vector<int>> topic_socks;
	std::map<string, std::vector<std::string>> stored_mess_clients;
	std::map<int, std::map<std::string, int>>sock_topic_sf;
	std::vector<string> disconnected_clients;

	fd_set read_fds; 
	fd_set temp_fds;
	
	if(argc < 2){
		cout<< "Usage: ./server <PORT>";
		return 0;
	}

	// empty read file descriptors 
	FD_ZERO(&read_fds);
	FD_ZERO(&temp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		cout << "Error initializing socket\n";
		return 0;
	} 

	sockfd_UDP = socket(AF_INET, SOCK_DGRAM, 0);

	portno = atoi(argv[1]);
	if(portno == 0){
		cout << "Error port number\n";
		return 0;
	} 

	memset((char*)&server_address, 0, sizeof(server_address));

	// fill server fields
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(portno);
	server_address.sin_addr.s_addr = INADDR_ANY;

	
	ret = bind(sockfd, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
	if(ret < 0){
		cout << "Binding error\n";
		return 0;
	}

	
	bind(sockfd_UDP, (struct sockaddr*)&server_address, sizeof (struct sockaddr));

	ret = listen(sockfd, MAXCLIENTS);
	if(ret < 0){
		cout << "Error occured while listening\n";
		return 0;
	}

	
	FD_SET(sockfd, &read_fds);
	FD_SET(sockfd_UDP, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);

	fd_max = sockfd;
	if(sockfd_UDP > fd_max){
		fd_max = sockfd_UDP;
	}

	while(1){

		// copy file descriptors
		temp_fds = read_fds;

		ret = select(fd_max + 1, &temp_fds, NULL, NULL, NULL);

		if(ret < 0){
			cout << "Error occured while selecting\n";
			return 0;
		}
		for(i = 0; i <= fd_max; i++){

			// find the file descriptor which is set
			if(FD_ISSET(i, &temp_fds)){

				if(i == sockfd){

					// connection request on listen socket
					client_length = sizeof(client_address);
					newsockfd = accept(sockfd, (struct sockaddr *)&client_address, &client_length);
					if(newsockfd < 0){
						cout << "Error occured while accepting connection";
						return 0;
					}

					// add new socket to file descriptors set 
					FD_SET(newsockfd, &read_fds);
					if(newsockfd > fd_max){
						// if it is the case, update the max file descriptor
						fd_max = newsockfd;
					}
					
					char new_id[10];

					// the client sends the client ID after connecting to the server
					// the server recieves it after the client was accepted;
					ret = recv(newsockfd, (char*)&new_id, sizeof(new_id), 0);

					// add corespondence between client socket and client id and viceversa
					sockfd_id[newsockfd] = new_id;
					id_sockfd[new_id] = newsockfd;

					printf("New client %s connected from %s : %u.\n",new_id,inet_ntoa(client_address.sin_addr),ntohs(client_address.sin_port));

					// check if the new client has been connected to the server before
					if(std::find(disconnected_clients.begin(), disconnected_clients.end(), new_id) != disconnected_clients.end()){
						// if this is true => send each message that it missed while offline
						for(int j = 0; j < stored_mess_clients[new_id].size(); j++){
							send(newsockfd, stored_mess_clients[new_id].at(j).c_str(), strlen(stored_mess_clients[new_id].at(j).c_str()), 0);
						}
						disconnected_clients.erase(std::remove(disconnected_clients.begin(), disconnected_clients.end(), new_id), disconnected_clients.end());
						stored_mess_clients.erase(new_id);
					}							

				} else if(i == STDIN_FILENO){

					// input from stdin
					// anything except "exit" is ignored
			  		memset(buffer, 0, BUFLEN);
					cin >> buffer;

					if(strncmp(buffer, "exit", 4) == 0){
						for(int j = 0; j <= fd_max; j++){
							// send the exit signal to all clients
							if(j != sockfd && j != sockfd_UDP && j != STDIN_FILENO){
								n = send(j,buffer,strlen(buffer),0);
							}							
						}
						close(sockfd);
						close(sockfd_UDP);
						return 0;
					}
					
				} else if(i == sockfd_UDP){

					// message from publisher
					struct msg_udp message_recieved;

					char buffer[BUFLEN];
				    char container [BUFLEN];
				   
				    memset(buffer, 0, BUFLEN);
				    memset(container, 0, BUFLEN);

					address_length = sizeof(client_address);
					recvfrom(sockfd_UDP, (char*)&message_recieved, sizeof(message_recieved), 0, (struct sockaddr*)&client_address, &address_length);
				    
				    sprintf(container, "%u", ntohs(client_address.sin_port));
				    strcat(buffer, inet_ntoa(client_address.sin_addr));
				    strcat(buffer, " : ");
				    strcat(buffer, container);
				    strcat(buffer, " ");
				    strcat(buffer, message_recieved.topic);
				    strcat(buffer, " - ");

				    memset(container, 0, BUFLEN);

				    if(message_recieved.type == 0){

				    	// int number 
				    	uint32_t number = 0;

				    	memcpy(&number, message_recieved.payload + 1, sizeof(number));
				    	number = ntohl(number);

				    	if(message_recieved.payload[0] != 0){
				    		// if payload[0] is set => negative number
				    		sprintf(container, "%d", (int)(-number));
				    	}else {
				    		sprintf(container, "%d", number);
				    	}
				    	strcat(buffer, container);

				    } else if(message_recieved.type == 1){

				    	// short real
				    	uint16_t number = 0;
				    	float fl_number;

				    	memcpy(&number, message_recieved.payload, sizeof(number));

				    	number = ntohs(number);
				    	fl_number = (float)number/(float)100;

				    	sprintf(container, "%.2f", fl_number);
				    	strcat(buffer, container);
				    	
				    } else if(message_recieved.type == 2){

				    	// float
				    	uint32_t number = 0;
				    	uint8_t power = 0;
				    	float result;
				    	
				    	// skip the sign (+ 1) and copy the number
				    	memcpy(&number, message_recieved.payload + 1, sizeof(number));
				    	number = ntohl(number);

				    	// the rest of the payload contains the uint8_t number
				    	memcpy(&power, message_recieved.payload + 1 + sizeof(number), sizeof(power));

				    	result = number / pow(10, power);

				    	if(message_recieved.payload[0] != 0){
				    		result = result * (-1);
				    	}

				    	sprintf(container, "%lf", result);
				    	strcat(buffer, container);

				    } else if(message_recieved.type == 3){

				    	// string 
				    	strcat(buffer, message_recieved.payload);

				    }


				    // send the composed message
				   	for(int j = 0; j <= fd_max; j++){
				   		// go through all file descriptors 
				   		if(j != sockfd && j != STDIN_FILENO && j != sockfd_UDP){
				   			if(std::find(topic_socks[message_recieved.topic].begin(), topic_socks[message_recieved.topic].end(), j) != topic_socks[message_recieved.topic].end()){
				   				// if the current client (j) is subscribed to the current topic
				   				if(std::find(disconnected_clients.begin(), disconnected_clients.end(), sockfd_id[j]) != disconnected_clients.end()){
				   					// if it is actually disconected 
				   					if(sock_topic_sf[j][message_recieved.topic] == 1){
				   						// save the message to be sent when the client reconnects
				   						strcat(buffer, "\n");
				   						stored_mess_clients[sockfd_id[j]].push_back(buffer);
				   					}		
				   				} else {
				   					// if the client is not disconnected but subscribed => send it now
				   					n = send(j, buffer, strlen(buffer), 0);
				   					if(n < 0){
										cout << "Error occured while sending message to client\n";
										return 0;
									}
				   				}				   						
				   			}		
				   		}
				   	}
				} else {

					// recieved data from one of the connected clients
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					if(n < 0){
						cout << "error recv";
						exit(0);
					}
					if(n == 0){

						// client closed connection
						cout << "Client " << sockfd_id[i] << " disconnected.\n";
						close(i);
						FD_CLR(i, &read_fds);
						disconnected_clients.push_back(sockfd_id[i]);

					} else {

						// message recieved from client socket
						char* first_word;
						// check if it is subscribe/unsubscribe
						first_word = strtok(buffer, " ");

						if(strcmp(first_word, "subscribe") == 0){

							char copy[100];
							first_word = strtok(NULL, " ");
							strcpy(copy, first_word);
							
							// add the topic recieved to the client's list of subscribed topics
							topic_socks[first_word].push_back(i);

							// move to the store & forward flag and store it 
							// for when the client will disconnect
							first_word = strtok(NULL, "\n");
							sock_topic_sf[i][copy] = atoi(first_word);

							// compose confirmation of subscription after the topic was inserted in the list
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "Subscribed to topic ");
							strcat(buffer, copy);
							strcat(buffer, "\n");

							send(i, buffer, BUFLEN, 0);

						} else if (strcmp(first_word, "unsubscribe") == 0){

							char unsubscribe_topic[50];
							memset(unsubscribe_topic, 0, 50);

							first_word = strtok(NULL, "\n");
							int initial_size = topic_socks[first_word].size();
							strcpy(unsubscribe_topic, first_word);

							// erase the recieved topic from client's list
							topic_socks[first_word].erase(std::remove(topic_socks[first_word].begin(), topic_socks[first_word].end(), i), topic_socks[first_word].end());

							if(topic_socks[first_word].size() < initial_size){

								// compose confirmation of unsubscribe to send it to the client
								memset(buffer, 0, BUFLEN);
								sprintf(buffer, "Unsubscribed");
								strcat(buffer, "\n");

								send(i, buffer, BUFLEN, 0);
							} else {

								// if the size of the map didn't change => there was nothing to erase
								memset(buffer, 0, BUFLEN);
								sprintf(buffer, "The topic you chose to unsubscribe from does not exist or you were not subscribed to it!\n");
								
								send(i, buffer, BUFLEN, 0);
							}

							// set the SF flag to 0 to stop the server from sending
							// to the client messages about the topic the client just unsubscribed from
							sock_topic_sf[i][unsubscribe_topic] = 0;

						} 
					}
				}
			}
		}
	}
	
	return 0;
}