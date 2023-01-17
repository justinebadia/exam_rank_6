#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

int port;
int serv_fd;
int max_fd = 0;
fd_set readfds, writefds, setfds;
int client_nb = 0;
int clientfd_id[4096];
char readbuf[4096 * 42];
char writebuf[4096 * 42 + 42];
char savebuf[4096 * 42];


void print_error()
{
	write(2, "Fatal error\n", strlen("Fatal error\n"));
	exit(1);
}

void sendToAll(int sender)
{
	for(int i = 0; i <= max_fd; i++)
	{
		if (FD_ISSET(i, &writefds) && i != sender)
			send(i, writebuf, strlen(writebuf), 0);
	}
}

int main (int argc, char** argv)
{
	int ret_recv = 0;
	if (argc != 2)
	{
		print_error("Wrong number of arguments\n");
		exit (1);
	}
	
	//setting up server
	port = atoi(argv[1]);
	struct sockaddr_in serv;
	serv.sin_family = AF_INET;
	serv.sin_port = htons(port);
	serv.sin_addr.s_addr = (1 << 24) | 127;

	if((serv_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		print_error();
	if((bind(serv_fd, (struct sockaddr*)&serv, sizeof(serv))) < 0)
	{
		close(serv_fd);
		print_error();
	}
	if((listen(serv_fd, 10)) < 0)
	{
		close(serv_fd);
		print_error();
	}
	bzero(clientfd_id, sizeof(clientfd_id));
	FD_ZERO(&setfds);
	FD_SET(serv_fd, &setfds);
	max_fd = serv_fd;

	while (1)
	{
		readfds = writefds = setfds;
		if((select(max_fd + 1, &readfds, &writefds, 0, 0)) <= 0)
			continue;
		for (int client = 0; client <= max_fd; client++)
		{
			if (FD_ISSET(client, &readfds)) // si le fd du client est prêt à recv des données
			{
				if (client == serv_fd) //nouvelle connexion
				{
					struct sockaddr_in new_client;
					socklen_t len = sizeof(new_client);
					int client_fd;

					if((client_fd = accept(serv_fd, (struct sockaddr*)&new_client, &len)) == -1)
						continue ; //le client n'est pas ajouté
					FD_SET(client_fd, &setfds);
					clientfd_id[client_fd] = client_nb++;
					if(max_fd < client_fd)
						max_fd = client_fd;
					sprintf(writebuf, "server: client %d just arrived\n", clientfd_id[client_fd]);
					sendToAll(serv_fd);
					break;
				}
				else 
				{
					if((ret_recv = recv(client, readbuf, sizeof(readbuf), 0)) <= 0) //client ferme sa connexion
					{
						sprintf(writebuf, "server: client %d just left\n", clientfd_id[client]);
						sendToAll(client);
						FD_CLR(client, &setfds);
						close(client);
						break;
					}
					else // il y a un msg
					{
						int i = 0;
						int j = 0;
						while (readbuf[i])
						{
							savebuf[j] = readbuf[i];
							if(readbuf[i] == '\n')
							{
								sprintf(writebuf, "client %d: %s", clientfd_id[client], savebuf);
								sendToAll(client);
								bzero(&savebuf, sizeof(savebuf));
								bzero(&writebuf, sizeof(writebuf));
								j = -1;
							}
							else if (i == ret_recv - 1) // pas de \n mais la fin du msg
							{
								sprintf(writebuf, "client %d: %s", clientfd_id[client], savebuf);
								sendToAll(client);
								bzero(&savebuf, sizeof(savebuf));
								bzero(&writebuf, sizeof(writebuf));
								j = -1;
							}
							i++;
							j++;
						}
						bzero(&readbuf, sizeof(readbuf));
						break;
					}
				}
			}
		}
	}
	return (0);
}