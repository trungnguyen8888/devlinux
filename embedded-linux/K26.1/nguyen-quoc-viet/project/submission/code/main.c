#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define PORT 5000
#define MAX_CLIENTS 100
#define MAX_EVENTS 64
#define BUFFER_SIZE 4096

typedef struct {
	int fd;
	char username[64];
	int authenticated;
	int failed_attempts;
	char input_buffer[BUFFER_SIZE];
	int input_len;
	char output_buffer[BUFFER_SIZE];
	int output_len;
	int output_pos;
} client_t;

client_t clients[MAX_CLIENTS];
int client_count = 0;
int epfd;
volatile int running = 1;

extern int authenticate_user(const char *username, const char *password);
extern void broadcast_message(const char *username, const char *text);
extern void send_message_history(int client_fd, const char *username);

void signal_handler(int sig)
{
	running = 0;
}

void handle_client_event(int fd, uint32_t events);
void process_input(client_t *client);
void route_message(client_t *client, const char *message);
void handle_login(client_t *client, const char *credentials);
void handle_message(client_t *client, const char *text);
void handle_logout(client_t *client);
void handle_userlist(client_t *client);
void send_message(client_t *client, const char *text);
void send_error(client_t *client, const char *error);
void close_client(client_t *client);

int main(int argc, char *argv[])
{
	int listen_fd, client_fd;
	struct sockaddr_in server_addr, client_addr;
	socklen_t addr_len;
	struct epoll_event event, events[MAX_EVENTS];
	int nfds, i;

	printf("Starting Chat Server on port %d...\n", PORT);

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		perror("socket");
		return 1;
	}

	int reuse = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("bind");
		return 1;
	}

	if (listen(listen_fd, 5) < 0) {
		perror("listen");
		return 1;
	}

	epfd = epoll_create1(0);
	if (epfd < 0) {
		perror("epoll_create1");
		return 1;
	}

	event.events = EPOLLIN;
	event.data.fd = listen_fd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &event);

	printf("✅ Server listening on port %d\n", PORT);

	while (running) {
		nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
		if (nfds < 0) {
			if (errno == EINTR)
				continue;
			perror("epoll_wait");
			break;
		}

		for (i = 0; i < nfds; i++) {
			if (events[i].data.fd == listen_fd) {
				addr_len = sizeof(client_addr);
				client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
				if (client_fd < 0) {
					perror("accept");
					continue;
				}

				if (client_count < MAX_CLIENTS) {
					int j;
					for (j = 0; j < MAX_CLIENTS; j++) {
						if (clients[j].fd < 0) {
							clients[j].fd = client_fd;
							clients[j].authenticated = 0;
							clients[j].failed_attempts = 0;
							clients[j].input_len = 0;
							clients[j].output_len = 0;
							clients[j].output_pos = 0;
							client_count++;
							break;
						}
					}

					fcntl(client_fd, F_SETFL, O_NONBLOCK);
					event.events = EPOLLIN | EPOLLOUT;
					event.data.fd = client_fd;
					epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &event);
				} else {
					close(client_fd);
				}
			} else {
				handle_client_event(events[i].data.fd, events[i].events);
			}
		}
	}

	printf("🛑 Shutting down...\n");
	close(listen_fd);
	close(epfd);
	return 0;
}

void handle_client_event(int fd, uint32_t events)
{
	int i;
	client_t *client = NULL;

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (clients[i].fd == fd) {
			client = &clients[i];
			break;
		}
	}

	if (!client)
		return;

	if (events & EPOLLIN) {
		ssize_t n = read(fd, &client->input_buffer[client->input_len],
			BUFFER_SIZE - client->input_len);
		if (n < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK)
				close_client(client);
		} else if (n == 0) {
			close_client(client);
		} else {
			client->input_len += n;
			process_input(client);
		}
	}

	if (events & EPOLLOUT && client->fd >= 0) {
		if (client->output_len > 0) {
			ssize_t n = write(fd, &client->output_buffer[client->output_pos],
				client->output_len - client->output_pos);
			if (n < 0) {
				if (errno != EAGAIN && errno != EWOULDBLOCK)
					close_client(client);
			} else if (n > 0) {
				client->output_pos += n;
				if (client->output_pos >= client->output_len) {
					client->output_len = 0;
					client->output_pos = 0;
				}
			}
		}
	}
}

void process_input(client_t *client)
{
	char *newline;
	char message[BUFFER_SIZE];
	int msg_len;

	while ((newline = memchr(client->input_buffer, '\n', client->input_len)) != NULL) {
		msg_len = newline - client->input_buffer;
		if (msg_len > 0)
			memcpy(message, client->input_buffer, msg_len);
		message[msg_len] = '\0';

		route_message(client, message);

		memmove(client->input_buffer, newline + 1, client->input_len - msg_len - 1);
		client->input_len -= (msg_len + 1);
	}
}

void route_message(client_t *client, const char *message)
{
	if (strncmp(message, "LOGIN:", 6) == 0) {
		handle_login(client, message + 6);
	} else if (strncmp(message, "MSG:", 4) == 0) {
		handle_message(client, message + 4);
	} else if (strcmp(message, "LOGOUT") == 0) {
		handle_logout(client);
	} else if (strcmp(message, "USERLIST") == 0) {
		handle_userlist(client);
	} else if (strcmp(message, "HISTORY") == 0) {
		if (client->authenticated)
			send_message_history(client->fd, client->username);
	} else {
		send_error(client, "INVALID_FORMAT");
	}
}

void handle_login(client_t *client, const char *credentials)
{
	char username[64], password[256];
	const char *colon = strchr(credentials, ':');

	if (!colon) {
		send_error(client, "INVALID_FORMAT");
		return;
	}

	int user_len = colon - credentials;
	if (user_len >= 64)
		user_len = 63;

	strncpy(username, credentials, user_len);
	username[user_len] = '\0';
	strcpy(password, colon + 1);

	if (authenticate_user(username, password)) {
		strcpy(client->username, username);
		client->authenticated = 1;
		client->failed_attempts = 0;
		send_message(client, "OK:LOGIN\n");
	} else {
		client->failed_attempts++;
		if (client->failed_attempts >= 3) {
			send_error(client, "TOO_MANY_ATTEMPTS");
			close_client(client);
		} else {
			send_error(client, "INVALID_CREDENTIALS");
		}
	}
}

void handle_message(client_t *client, const char *text)
{
	if (!client->authenticated) {
		send_error(client, "NOT_AUTHENTICATED");
		return;
	}
	broadcast_message(client->username, text);
}

void handle_logout(client_t *client)
{
	close_client(client);
}

void handle_userlist(client_t *client)
{
	int i;
	char buf[BUFFER_SIZE] = "USERS:";

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (clients[i].fd >= 0 && clients[i].authenticated) {
			if (strlen(buf) > 6)
				strcat(buf, ",");
			strcat(buf, clients[i].username);
		}
	}
	strcat(buf, "\n");
	send_message(client, buf);
}

void send_message(client_t *client, const char *text)
{
	if (!client || client->fd < 0)
		return;

	size_t len = strlen(text);
	if (client->output_len + len <= BUFFER_SIZE) {
		memcpy(&client->output_buffer[client->output_len], text, len);
		client->output_len += len;
	}
}

void send_error(client_t *client, const char *error)
{
	char buf[BUFFER_SIZE];
	snprintf(buf, sizeof(buf), "ERR:%s\n", error);
	send_message(client, buf);
}

void close_client(client_t *client)
{
	if (!client || client->fd < 0)
		return;

	epoll_ctl(epfd, EPOLL_CTL_DEL, client->fd, NULL);
	close(client->fd);

	if (client->authenticated)
		client_count--;

	memset(client, 0, sizeof(*client));
	client->fd = -1;
}
