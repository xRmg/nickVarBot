/*
    nickname variable bot

    Various macros, structs and other stuff

*/
#ifndef __NICKVARBOT_H__
#define __NICKVARBOT_H__

#define MATCH(s, n) (strcmp(section, s) == 0 && strcmp(name, n) == 0)

#define MAXVARNAMESIZE      32
#define MAXVARCOUNT         10

// nickname variable data
typedef struct
{
    char name[MAXVARNAMESIZE];
    int namevalue;
    // how often a name is used, used by LRU algo for removal
    int nameused;
} nickvarVar;

// nickname data
typedef struct
{
    char name[MAXVARNAMESIZE];
    nickvarVar namevars[MAXVARCOUNT];
} nickvarNick;

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
