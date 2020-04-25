#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <math.h>
#include <map>
#include <vector>
#include <algorithm>

#define BUFLEN 2500
#define MAXCLIENTS 5

using namespace std;

struct msg_udp{
	char topic[50] ;
	char data_type;
	char content[1500];
}__attribute__((packed));


int main(int argc, char const *argv[])
{
	int sockfd, newsockfd, portno, sockfd_UDP;
	char buffer[BUFLEN];
	struct sockaddr_in server_address, client_address;
	int n, i, ret;
	socklen_t client_length;
	socklen_t address_length;

	
	
	std::map<int, std::string> sockfd_id;
	std::map<std::string, int> id_sockfd;
	std::map<std::string, std::vector<int>> topic_socks;
	std::map<string, std::vector<std::string>> stored_mess_clients;
	std::vector<string> disconnected_clients;


	// read file descriptors
	fd_set read_fds; 
	// backup file descriptors
	fd_set temp_fds;
	int fd_max;

	if(argc < 2){
		cout<< "Usage: ./server <PORT>";
		exit(0);
	}

	// empty read file descriptors 
	FD_ZERO(&read_fds);
	FD_ZERO(&temp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		cout << "error initializing socket";
		exit(0);
	} else {
		cout <<"socket successfully initialized\n";
	}
	sockfd_UDP = socket(AF_INET, SOCK_DGRAM, 0);

	portno = atoi(argv[1]);
	if(portno == 0){
		cout << "error port number";
		exit(0);
	} else {
		cout << "port number ok\n";
	}

	memset((char*)&server_address, 0, sizeof(server_address));
	// fill server fields
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(portno);
	server_address.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
	if(ret < 0){
		cout << "bind";
		exit(0);
	} else {
		cout << "bind completed successfully \n";
	}

	ret = bind(sockfd_UDP, (struct sockaddr*)&server_address, sizeof (struct sockaddr));
	ret = listen(sockfd, MAXCLIENTS);
	if(ret < 0){
		cout << "listen";
		exit(0);
	} else {
		cout << "listening ... \n";
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
			cout << "error select";
			exit(0);
		}
		for(i = 0; i <= fd_max; i++){
			// find the file descriptor which is set
			if(FD_ISSET(i, &temp_fds)){
				if(i == sockfd){
					// connection request on listen socket

					client_length = sizeof(client_address);
					newsockfd = accept(sockfd, (struct sockaddr *)&client_address, &client_length);
					if(newsockfd < 0){
						cout << "accept error";
						exit(0);
					}
					// add new socket to file descriptors
					FD_SET(newsockfd, &read_fds);
					if(newsockfd > fd_max){
						fd_max = newsockfd;
					}
					// cout << "Noua conexiune de la " << inet_ntoa(client_address.sin_addr) << 
					// " , port" << ntohs(client_address.sin_port) << " socket client " << newsockfd
					// << "\n";

				

					char new_id[10];
					
					ret = recv(newsockfd, (char*)&new_id, sizeof(new_id), 0);
					printf("Id user:%s\n", new_id);

					// add corespondence between client socket and client id and viceversa
					sockfd_id[newsockfd] = new_id;
					id_sockfd[new_id] = newsockfd;

					// cout << "New client " << new_id << " connected from " << inet_ntoa(client_address.sin_addr)
					// << ":" << ntohs(client_address.sin_port) << ".\n";	
					printf("New client %s connected from %s : %u.\n",new_id,inet_ntoa(client_address.sin_addr),ntohs(client_address.sin_port));

					// if(std::find(disconnected_clients.begin(), disconnected_clients.end(), new_id) != disconnected_clients.end()){
						
					// 	for(int j = 0; j < stored_mess_clients[new_id].size(); j++){
						
					// 		send(newsockfd, stored_mess_clients[new_id].at(j).c_str(), strlen(stored_mess_clients[new_id].at(j).c_str()), 0);
					// 	}
					// 	disconnected_clients.erase(std::remove(disconnected_clients.begin(), disconnected_clients.end(), new_id), disconnected_clients.end());
					// }							

				} else if(i == STDIN_FILENO){
			  		memset(buffer, 0, BUFLEN);
					cin >> buffer;
					if(strncmp(buffer, "exit", 4) == 0){
						for(int j = 0; j <= fd_max; j++){
							if(j != sockfd && j != sockfd_UDP && j != STDIN_FILENO){
								n = send(j,buffer,strlen(buffer),0);
							}
						}
						close(sockfd);
						close(sockfd_UDP);
						return 0;
					}
					for(int j = 0; j <= fd_max; j++){
						if(j != sockfd && j != STDIN_FILENO && j != sockfd_UDP){
							n = send(j, buffer, strlen(buffer), 0);
						}
					}
					
				} else if(i == sockfd_UDP){
					struct msg_udp message_recieved;
					
					address_length = sizeof(client_address);
					ret = recvfrom(sockfd_UDP, (char*)&message_recieved, 
						sizeof(message_recieved), 0, 
						(struct sockaddr*)&client_address, &address_length);
				    
				    char buffer[BUFLEN];
				    char container [BUFLEN];
				   
				    memset(buffer, 0, BUFLEN);
				    memset(container, 0, BUFLEN);

				    sprintf(container, "%u", ntohs(client_address.sin_port));
				    strcat(buffer, inet_ntoa(client_address.sin_addr));
				    strcat(buffer, ":");
				    strcat(buffer, container);
				    strcat(buffer, message_recieved.topic);
				    strcat(buffer, " - ");

				    memset(container, 0, BUFLEN);

				    if(message_recieved.data_type == 0){
				    	// int number 
				    	uint32_t number = 0;
				    	memcpy(&number, message_recieved.content + 1, sizeof(number));
				    	number = ntohl(number);
				    	if(message_recieved.content[0] != 0){
				    		sprintf(container, "%d", (int)(-number));
				    	}else {
				    		sprintf(container, "%d", number);
				    	}
				    	strcat(buffer, container);
				    	//printf("%s\n", buffer);
				    } else if(message_recieved.data_type == 1){
				    	// short real
				    	uint16_t number = 0;
				    	float fl_number;

				    	memcpy(&number, message_recieved.content, sizeof(number));
				    	number = ntohs(number);
				    
				    	fl_number = (float)number/(float)100;
				    	sprintf(container, "%.2f", fl_number);
				    	strcat(buffer, container);
				    	//printf("%s\n", buffer);
				    } else if(message_recieved.data_type == 2){
				    	uint32_t number = 0;
				    	uint8_t power = 0;
				    	float result;
				    	

				    	// skip the sign (+ 1) and cpy the number
				    	memcpy(&number, message_recieved.content + 1, sizeof(number));
				    	number = ntohl(number);
				    	// the rest of the payload contains the uint8_t number
				    	memcpy(&power, message_recieved.content + 1 + sizeof(number), sizeof(power));

				    	result = number / pow(10, power);
				    	if(message_recieved.content[0] != 0){
				    		result = result * (-1);
				    	}
				    	sprintf(container, "%lf", result);
				    	strcat(buffer, container);
				    } else if(message_recieved.data_type == 3){
				    	strcat(buffer, message_recieved.content);
				    }

				   //printf("Recv from udp: Topic: %s, Type: %hhu \n",message_recieved.topic, message_recieved.data_type);
				   
				   			for(int j = 0; j <= fd_max; j++){
				   				if(j != sockfd && j != STDIN_FILENO && j != sockfd_UDP){
				   					if(std::find(topic_socks[message_recieved.topic].begin(), topic_socks[message_recieved.topic].end(), j) != topic_socks[message_recieved.topic].end()){
				   						//if(std::find(disconnected_clients.begin(), disconnected_clients.end(), sockfd_id[j]) != disconnected_clients.end()){
				   							//stored_mess_clients[sockfd_id[j]].push_back(buffer);
				   						//} else {
				   							n = send(j, buffer, strlen(buffer), 0);
				   						//}
				   						
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

						//disconnected_clients.push_back(sockfd_id[i]);


					} else {
						// message recieved from client socket
						char* first_word;
						first_word = strtok(buffer, " ");
						if(strcmp(first_word, "subscribe") == 0){
							//recv(i, buffer, 0, BUFLEN);
							first_word = strtok(NULL, "\n");
							printf("Recieved topic: %s\n", first_word);
							//first_word = strtok(NULL, " ");

							// add the recieved topic to the client's list
							topic_socks[first_word].push_back(i);
							for(int j = 0; j < topic_socks[first_word].size(); j++){
								cout << topic_socks[first_word].at(j) << " ";
							}
							printf("\n");
						} else if (strcmp(first_word, "unsubscribe") == 0){

							first_word = strtok(NULL, "\n");
							topic_socks[first_word].erase(std::remove(topic_socks[first_word].begin(), topic_socks[first_word].end(), i), topic_socks[first_word].end());
							for(int j = 0; j < topic_socks[first_word].size(); j++){
								cout << topic_socks[first_word].at(j) << " ";
							}
							printf("\n");
						} else {
							printf("%s\n",buffer);
						}
						



					}
					
					
		
					}
				}
			}
		}
	

	close(sockfd);

	return 0;
}