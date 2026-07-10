#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/types.h>

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 5000
#define BUFFER_SIZE 4096
#define MAX_USERNAME 64
#define MAX_PASSWORD 256

volatile int running = 1;

void signal_handler(int sig)
{
	(void)sig;
	running = 0;
}

void show_banner(void)
{
	printf("\n");
	printf("╔═══════════════════════════════════════╗\n");
	printf("║     DevLinux Chat Client v1.0         ║\n");
	printf("╚═══════════════════════════════════════╝\n");
	printf("Server: %s:%d\n\n", SERVER_HOST, SERVER_PORT);
}

void show_help(void)
{
	printf("\n┌─ Available Commands ─────────────────────────────┐\n");
	printf("│ /who              - List online users            │\n");
	printf("│ /help             - Show this help               │\n");
	printf("│ /quit             - Exit chat                    │\n");
	printf("│ (other text)      - Send message to all users    │\n");
	printf("└──────────────────────────────────────────────────┘\n");
}

int connect_to_server(void)
{
	int sock;
	struct sockaddr_in server_addr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		return -1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);

	if (inet_pton(AF_INET, SERVER_HOST, &server_addr.sin_addr) <= 0) {
		perror("inet_pton");
		close(sock);
		return -1;
	}

	if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("connect");
		close(sock);
		return -1;
	}

	return sock;
}

void handle_login(int sock, char *username, char *password)
{
	char buf[BUFFER_SIZE];
	int len;

	snprintf(buf, sizeof(buf), "LOGIN:%s:%s\n", username, password);
	send(sock, buf, strlen(buf), 0);

	memset(buf, 0, sizeof(buf));
	len = recv(sock, buf, sizeof(buf) - 1, 0);
	if (len > 0) {
		buf[len] = '\0';
		if (strncmp(buf, "OK:LOGIN", 8) == 0) {
			printf("[✓] Login successful!\n\n");
			return;
		}
	}

	printf("[!] Login failed. Server error or invalid credentials.\n");
	exit(EXIT_FAILURE);
}

void handle_register(int sock, char *username, char *password)
{
	char buf[BUFFER_SIZE];
	int len;
	fd_set readfds;
	struct timeval tv;

	snprintf(buf, sizeof(buf), "REGISTER:%s:%s\n", username, password);
	send(sock, buf, strlen(buf), 0);

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = 3;
	tv.tv_usec = 0;

	if (select(sock + 1, &readfds, NULL, NULL, &tv) > 0) {
		memset(buf, 0, sizeof(buf));
		len = recv(sock, buf, sizeof(buf) - 1, 0);
		if (len > 0) {
			buf[len] = '\0';
			if (strncmp(buf, "OK:REGISTER", 11) == 0) {
				printf("[✓] Registration successful!\n\n");
				return;
			}
		}
	}

	printf("[!] Registration failed.\n");
	exit(EXIT_FAILURE);
}

void display_message(const char *line)
{
	if (strncmp(line, "HISTORY:", 8) == 0) {
		char *colon = strchr(line + 8, ':');
		if (colon) {
			char user[64];
			sscanf(line, "HISTORY:%63[^:]", user);
			printf("[history] %s: %s\n", user, colon + 1);
		}
	} else if (strncmp(line, "FROM:", 5) == 0) {
		char *colon = strchr(line + 5, ':');
		if (colon) {
			char user[64];
			sscanf(line, "FROM:%63[^:]", user);
			printf("[%s] %s: %s\n", user, user, colon + 1);
		}
	} else if (strncmp(line, "USERS:", 6) == 0) {
		printf("[users] Online: %s\n", line + 6);
	} else if (strncmp(line, "ERR:", 4) == 0) {
		printf("[!] Server error: %s\n", line + 4);
	} else if (strncmp(line, "OK:", 3) == 0) {
		printf("[✓] %s\n", line + 3);
	} else {
		printf("[server] %s\n", line);
	}
}

void chat_loop(int sock, const char *username)
{
	char send_buf[BUFFER_SIZE];
	char recv_buf[BUFFER_SIZE];
	int recv_len;
	fd_set readfds;

	printf("[Connected as %s]\n", username);
	show_help();
	printf("\n> ");
	fflush(stdout);

	while (running) {
		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);
		FD_SET(STDIN_FILENO, &readfds);

		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		int ret = select(sock + 1, &readfds, NULL, NULL, &tv);

		if (ret < 0) {
			if (errno != EINTR)
				perror("select");
			break;
		}

		if (FD_ISSET(sock, &readfds)) {
			memset(recv_buf, 0, sizeof(recv_buf));
			recv_len = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);

			if (recv_len <= 0) {
				printf("\n[!] Connection lost. Exiting.\n");
				break;
			}

			recv_buf[recv_len] = '\0';

			char *pos = recv_buf;
			char *newline;
			while ((newline = strchr(pos, '\n')) != NULL) {
				*newline = '\0';
				display_message(pos);
				pos = newline + 1;
			}
		}

		if (FD_ISSET(STDIN_FILENO, &readfds)) {
			int ch = getchar();

			if (ch == '\n') {
				char trimmed[BUFFER_SIZE];
				strcpy(trimmed, send_buf);

				if (strlen(trimmed) == 0) {
					printf("> ");
					fflush(stdout);
					continue;
				}

				if (strncmp(trimmed, "/quit", 5) == 0) {
					snprintf(send_buf, sizeof(send_buf), "LOGOUT\n");
					send(sock, send_buf, strlen(send_buf), 0);
					printf("[!] Logged out.\n");
					break;
				} else if (strncmp(trimmed, "/who", 4) == 0) {
					snprintf(send_buf, sizeof(send_buf), "USERLIST\n");
					send(sock, send_buf, strlen(send_buf), 0);
				} else if (strncmp(trimmed, "/help", 5) == 0) {
					show_help();
				} else {
					int maxlen = (int)sizeof(send_buf) - 6;
					snprintf(send_buf, sizeof(send_buf), "MSG:%.*s\n",
						maxlen > 0 ? maxlen : 0, trimmed);
					send(sock, send_buf, strlen(send_buf), 0);
				}

				memset(send_buf, 0, sizeof(send_buf));
				printf("> ");
				fflush(stdout);
			} else if (ch == EOF) {
				break;
			} else if (ch == '\b' || ch == 127) {
				if (strlen(send_buf) > 0)
					send_buf[strlen(send_buf) - 1] = '\0';
				printf("\b \b");
				fflush(stdout);
			} else if (ch >= 32 && ch < 127) {
				size_t len = strlen(send_buf);
				if (len < sizeof(send_buf) - 1) {
					send_buf[len] = ch;
					send_buf[len + 1] = '\0';
					printf("%c", ch);
					fflush(stdout);
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{
	int sock;
	char username[MAX_USERNAME];
	char password[MAX_PASSWORD];
	char choice[16];

	(void)argc;
	(void)argv;

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	show_banner();

	sock = connect_to_server();
	if (sock < 0) {
		fprintf(stderr, "[!] Failed to connect to %s:%d\n", SERVER_HOST, SERVER_PORT);
		return EXIT_FAILURE;
	}

	printf("Login or Register? [login/register]: ");
	fgets(choice, sizeof(choice), stdin);
	choice[strcspn(choice, "\n")] = '\0';

	printf("Username: ");
	fgets(username, sizeof(username), stdin);
	username[strcspn(username, "\n")] = '\0';

	printf("Password: ");
	fgets(password, sizeof(password), stdin);
	password[strcspn(password, "\n")] = '\0';

	if (strcmp(choice, "register") == 0) {
		handle_register(sock, username, password);
	} else {
		handle_login(sock, username, password);
	}

	fcntl(sock, F_SETFL, O_NONBLOCK);
	chat_loop(sock, username);

	close(sock);
	printf("\nGoodbye!\n");
	return EXIT_SUCCESS;
}
