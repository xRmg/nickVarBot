#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <stdarg.h>
#include <regex.h>
#include <time.h>
#include <unistd.h>
#include "./lib/inih/ini.h"

#include "nickVarBot.h"
#define RX_BUFFER 1024

const char *irc_cmd_ping = "PING";
const char *irc_cmd_connected = "001";
const char *irc_cmd_privmsg = "PRIVMSG";
const char *irc_cmd_notice = "NOTICE";

void mod_irc_join_channel(const char *channel);
void mod_auth_authenticate(const char *auth, const char *postAuthMode );
void mod_url_parse(char *user,char *command,char *where,char *target,char *message);

int conn;
char sbuf[RX_BUFFER];
nickvarNick currentnickvars[MAXVARCOUNT];

static int config_handler(void* cfg, const char* section,  const char* name, const char* value)
{
	nickVarBotState* pconfig = (nickVarBotState*)cfg;
	if(MATCH("irc","server"))
	{
		pconfig->server = strdup(value);
	}
	else if (MATCH("irc","port"))
	{
		pconfig->port = strdup(value);
	}
	else if (MATCH("irc","nick"))
	{
		pconfig->nick = strdup(value);
	}
	else if (MATCH("irc","channel"))
	{
		pconfig->channel = strdup(value);
	}
	else if (MATCH("irc","auth"))
	{
		pconfig->auth = strdup(value);
	}
	else if (MATCH("irc","postAuthMode"))
	{
		pconfig->postAuthMode = strdup(value);
	}
	else
	{
		return 0;
	}
	return 1;
}

void raw(char *fmt, ...)
{
	// TODO flood protection
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(sbuf, RX_BUFFER, fmt, ap);
	va_end(ap);
	printf("<< %s", sbuf);
	write(conn, sbuf, strlen(sbuf));
}

int main()
{
	int status;
	nickVarBotState config;

	char *user, *command, *where, *message, *sep, *target;
	int i, j, k, l, sl, o = -1, start, wordcount;
	char buf[RX_BUFFER];
	
	
	struct addrinfo hints, *res;

	config.botIrcState = disconnected;


	// TODO create a .nickVarBot dir for config and state storage
	memset(currentnickvars, 0, sizeof currentnickvars);
	// TODO load current vars from config file
	printf("Loading Config'\r\n");
	if(ini_parse("nickvarbot.cfg" , config_handler, &config) < 0)
	{
		printf("Can't load 'nickvarbot.cfg'\r\n");
		return 1;
	}
	printf("Loaded Config'\r\n");
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(config.server, config.port, &hints, &res);
	conn = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	printf("Connected 1'\r\n");
	connect(conn, res->ai_addr, res->ai_addrlen);
	printf("Connected'\r\n");
	raw("USER %s 0 0 :%s\r\n", config.nick, config.nick);
	raw("NICK %s\r\n", config.nick);

	while ((sl = read(conn, sbuf, RX_BUFFER)))
	{
		for (i = 0; i < sl; i++)
		{
			o++;
			buf[o] = sbuf[i];
			if ((i > 0 && sbuf[i] == '\n' && sbuf[i - 1] == '\r') || o == RX_BUFFER)
			{
				buf[o + 1] = '\0';
				l = o;
				o = -1;

				printf(">> %s", buf);
				if (!strncmp(buf, irc_cmd_ping, strlen(irc_cmd_connected)))
				{
					buf[1] = 'O';
					raw(buf);
				}
				else if (buf[0] == ':')
				{
					wordcount = 0;
					user = command = where = message = NULL;
					for (j = 1; j < l; j++)
					{
						if (buf[j] == ' ')
						{
							buf[j] = '\0';
							wordcount++;
							switch(wordcount)
							{
							case 1:
								user = buf + 1;
								break;
							case 2:
								command = buf + start;
								break;
							case 3:
								where = buf + start;
								break;
							}
							if (j == l - 1)
								continue;
							start = j + 1;
						}
						else if (buf[j] == ':' && wordcount == 3)
						{
							if (j < l - 1) message = buf + j + 1;
							break;
						}
					}

					// we are connected (timestamp 001), spam everything we got!
					if  (!strncmp(command, irc_cmd_connected, strlen(irc_cmd_connected)) && config.channel != NULL)
					{
						//Timestap is received
						mod_auth_authenticate(config.auth, config.postAuthMode);
						mod_irc_join_channel(config.channel);

					}
					// dissect all messages
					else if (!strncmp(command, irc_cmd_privmsg, strlen(irc_cmd_privmsg)))
					{
						if ((sep = strchr(user, '!')) != NULL)
							user[sep - user] = '\0';
						if (where[0] == '#' || where[0] == '&' || where[0] == '+' || where[0] == '!')
							target = where;
						else
							target = user;
						printf("[from: %s] [reply-with: %s] [where: %s] [reply-to: %s] %s", user, command, where, target, message);

						mod_url_parse(user,command,where,target,message);
					}
				}
			}
		}
	}

	return 0;

}


void mod_auth_authenticate(const char *auth, const char *postAuthMode )
{
	raw("%s\r\n", auth);
	raw("%s\r\n", postAuthMode);
}

void mod_irc_join_channel(const char *channel)
{
	raw("JOIN %s\r\n", channel);
}

void mod_url_parse(char *user,char *command,char *where,char *target,char *message)
{
	regex_t regex;
	int regexurl;
	char buf_timeStamp[64];
	time_t rawtime;
	struct tm * timeinfo;
	FILE *urllog_file;
	FILE *wget_fp;
	char path[1024];
	char cmd[1024];


	regexurl = regcomp(&regex, "http", 0);
	if( regexurl )
	{
		fprintf(stderr, "Could not compile regex\n");
		exit(1);
	}

	regexurl = regexec(&regex, message, 0, NULL, 0);
	if( !regexurl )
	{
		urllog_file = fopen("urllog.txt","a+");
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(buf_timeStamp,80,"%d-%m-%Y %H:%M:%S",timeinfo);
		printf("%s [%s]%s | %s", buf_timeStamp, where, user, message);
		fprintf(urllog_file,"%s [%s]%s | %s", buf_timeStamp, where, user, message);
		fclose(urllog_file);

		sprintf(cmd, "wget -q -O - \"$@\" \"http://alienpro.be/urlTitleFromString.php?url=%s", message);
		cmd[strlen(cmd)-2] = '\0';
		sprintf(cmd, "%s\"", cmd);
		printf("%s",cmd);


		wget_fp = popen(cmd, "r");
		wget_fp = popen(cmd, "r");
		if (wget_fp == NULL)
		{
			printf("Failed to run command\n" );
		}
		/* Read the output a line at a time - output it. */
		fgets(path, sizeof(path)-1, wget_fp);
		if(strlen(path) > 3)
		{
			printf("\r\n%s\r\n", path);
			raw("PRIVMSG %s :%s \r\n", target, path);
			path[0] = '\0';

		}
		pclose(wget_fp);
	}
}
