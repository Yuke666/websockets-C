#ifndef GAME_HANDLER_DEF
#define GAME_HANDLER_DEF

#include <time.h>

#define DECK_SIZE 60
#define STARTING_MANA 1
#define STARTING_HEALTH 30
#define MAX_CARDS_IN_HAND 10
#define STARTING_HAND_SIZE 6
#define ALL_CARDS_IN_GAME 6
#define ALL_MINIONS_IN_GAME 2
#define MAX_MINIONS 10
#define MAXIMUM_TURN_LENGTH 60

typedef struct {
    char manaCost;
    char isAnime;
    void *(*functions)();
} Card;

typedef struct {
    char health;
    char str;
    char taunt;
} Minion;

typedef struct  {
    int socket;
    char gameIndex;
    time_t lastSecond;
    int messagesPerSecond;
    char username[15];
    char lookingForGame;
    int userID;
} Client;

typedef struct {
	char cardsInHand;
	int deck[DECK_SIZE];
	int hand[MAX_CARDS_IN_HAND];
	char mana;
	Minion minions[MAX_MINIONS];
	char minionsDown;
	char health;
	char playerIndex;
	Client *client;
} Player;

typedef struct {
	Player players[2];
	char gameOn;
	char allTurns;
	char whosTurn;
	time_t lastTurn;
} Game;

void GameHandler_SendSwitchTurnMessage(Player, Game *);
void GameHandler_SwitchTurns(Game *);
void GameHandler_UpdateGame(Game *);
void GameHandler_ExchangeUsernames(Game *);
void GameHandler_SendHoverMessage(Player, char, Game);
void GameHandler_SendPushMessage(Player, Game);
void GameHandler_PlayCard(Player *, char, Game*);
void GameHandler_SendGameDisconnectMessage(Game);
void GameHandler_EraseGame(Game*);
void GameHandler_NewGame(Game*);
Player *GameHandler_GetOpponent(int,Game*);
Player *GameHandler_GetPlayer(int,Game*);

typedef enum {
	GAME_ON_MESSAGE = 0x01,
	GAME_DISCONNECT_MESSAGE = 0x02,
	SERVER_ERROR_MESSAGE = 0x03,
	GAME_CARD_HOVER_MESSAGE = 0x04,
	STARTING_HAND_MESSAGE = 0X05,
	GAME_CARD_PLAY_MESSAGE = 0x06,
	GAME_CARD_PUSH_MESSAGE = 0x07,
	GAME_CARD_REMOVE_MESSAGE = 0x08,
	ACCOUNT_LOGIN_INFO_MESSAGE = 0x09,
	GAME_TELL_OTHER_PLAYERS_NAME = 0x10,
	SERVER_CONNECTED_MESSAGE = 0x11,
	SEARCHING_FOR_GAME_MESSAGE = 0x12,
	LOG_IN_FAILED_MESSAGE = 0x13,
	LOG_IN_OKAY_MESSAGE = 0x14,
	GAME_SWICH_TURNS_MESSAGE = 0x15,
	GAME_OPPONENT_DISCONNECT_MESSAGE = 0x16,
} GAME_HANDER_ENUM;

#endif