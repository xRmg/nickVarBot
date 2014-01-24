/*
    nickname variable bot

    Various macros, structs and other stuff

*/
#ifndef __NICKVARBOT_H__
#define __NICKVARBOT_H__

#define MATCH(s, n) (strcmp(section, s) == 0 && strcmp(name, n) == 0)

typedef struct
{

} ircEvent;

typedef enum
{
    disconnected,
    connected,
    authed,
    moded,
    joined,
} ircState;

typedef struct
{
    ircState    botIrcState;
	const char* server;
	const char* port;
	const char* channel;
	const char* nick;
	const char* auth;
	const char* postAuthMode;
} nickVarBotState;

#endif
