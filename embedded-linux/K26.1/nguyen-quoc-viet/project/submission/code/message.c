#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/file.h>
#include <time.h>
#include <unistd.h>

#define MESSAGES_LOG "messages.log"
#define MAX_MESSAGE 256
#define MAX_CLIENTS 100

extern int epfd;
extern int client_count;

typedef struct {
	int fd;
	char username[64];
	int authenticated;
	char input_buffer[4096];
	int input_len;
	char output_buffer[4096];
	int output_len;
	int output_pos;
} client_t;

extern client_t clients[MAX_CLIENTS];

void send_message(client_t *client, const char *text);

void save_message_log(const char *username, const char *text)
{
	FILE *fp;
	time_t now = time(NULL);

	fp = fopen(MESSAGES_LOG, "a");
	if (!fp)
		return;

	flock(fileno(fp), LOCK_EX);

	fprintf(fp, "%s:%ld:%s\n", username, now, text);
	fflush(fp);

	flock(fileno(fp), LOCK_UN);
	fclose(fp);
}

void broadcast_message(const char *username, const char *text)
{
	int i;
	char buf[512];

	snprintf(buf, sizeof(buf), "FROM:%s:%s\n", username, text);

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (clients[i].fd >= 0 && clients[i].authenticated) {
			send_message(&clients[i], buf);
		}
	}

	save_message_log(username, text);
}

void send_message_history(int client_fd, const char *username)
{
	FILE *fp;
	char line[512];
	char user[64];
	long timestamp;
	char message[MAX_MESSAGE];
	int i;
	client_t *client = NULL;

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (clients[i].fd == client_fd) {
			client = &clients[i];
			break;
		}
	}

	if (!client)
		return;

	fp = fopen(MESSAGES_LOG, "r");
	if (!fp)
		return;

	flock(fileno(fp), LOCK_SH);

	while (fgets(line, sizeof(line), fp)) {
		line[strcspn(line, "\n")] = 0;

		if (sscanf(line, "%63[^:]:%ld:%255[^\n]", user, &timestamp, message) == 3) {
			char buf[512];
			snprintf(buf, sizeof(buf), "HISTORY:%s:%s\n", user, message);
			send_message(client, buf);
		}
	}

	flock(fileno(fp), LOCK_UN);
	fclose(fp);
}
