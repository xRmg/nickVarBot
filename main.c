#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <stdarg.h>
#include "./inih/ini.h"

#include "nickVarBot.h"

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

int conn;
char sbuf[512];

void raw(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sbuf, 512, fmt, ap);
    va_end(ap);
    printf("<< %s", sbuf);
    write(conn, sbuf, strlen(sbuf));
}

int main() {
    nickVarBotState config;
    config.botIrcState = disconnected;

    char *user, *command, *where, *message, *sep, *target;
    int i, j, l, sl, o = -1, start, wordcount;
    char buf[513];
    struct addrinfo hints, *res;
    if(ini_parse("nickvarbot.cfg" , config_handler, &config) < 0)
    {
        printf("Can't load 'nickvarbot.cfg'\r\n");
        return 1;
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(config.server, config.port, &hints, &res);
    conn = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    connect(conn, res->ai_addr, res->ai_addrlen);

    raw("USER %s 0 0 :%s\r\n", config.nick, config.nick);
    raw("NICK %s\r\n", config.nick);

    while ((sl = read(conn, sbuf, 512))) {
        for (i = 0; i < sl; i++) {
            o++;
            buf[o] = sbuf[i];
            if ((i > 0 && sbuf[i] == '\n' && sbuf[i - 1] == '\r') || o == 512) {
                buf[o + 1] = '\0';
                l = o;
                o = -1;

                printf(">> %s", buf);

                if (!strncmp(buf, "PING", 4)) {
                    buf[1] = 'O';
                    raw(buf);
                } else if (buf[0] == ':') {
                    wordcount = 0;
                    user = command = where = message = NULL;
                    for (j = 1; j < l; j++) {
                        if (buf[j] == ' ') {
                            buf[j] = '\0';
                            wordcount++;
                            switch(wordcount) {
                                case 1: user = buf + 1; break;
                                case 2: command = buf + start; break;
                                case 3: where = buf + start; break;
                            }
                            if (j == l - 1)
                                continue;
                            start = j + 1;
                        } else if (buf[j] == ':' && wordcount == 3) {
                            if (j < l - 1) message = buf + j + 1;
                                break;
                        }
                    }

                    if (wordcount < 2)
                        continue;

                    // we are connected (timestamp 001), spam everything we got!
                    if  (!strncmp(command, "001", 3) && config.channel != NULL)
                    {
                        if(config.auth != NULL);
                           raw("%s\r\n", config.auth);
                        if(config.postAuthMode != NULL)
                            raw("%s\r\n", config.postAuthMode);
                        raw("JOIN %s\r\n", config.channel);
                    }
                    // dissect all messages
                    else if (!strncmp(command, "PRIVMSG", 7) || !strncmp(command, "NOTICE", 6))
                    {
                        if (where == NULL || message == NULL)
                            continue;
                        if ((sep = strchr(user, '!')) != NULL)
                            user[sep - user] = '\0';
                        if (where[0] == '#' || where[0] == '&' || where[0] == '+' || where[0] == '!')
                            target = where;
                        else
                            target = user;
                        printf("[from: %s] [reply-with: %s] [where: %s] [reply-to: %s] %s", user, command, where, target, message);
                        // here the monitoring magic happens
                    }
                }
            }
        }
    }

    return 0;

}
