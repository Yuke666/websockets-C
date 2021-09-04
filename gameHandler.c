#include "gameHandler.h"
#include "cards.h"
#include "window.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

static void sendByteToPlayers(char b, Game game){
    if(game.players[0].client == NULL || game.players[1].client == NULL) return;
    char tosend[2] = { b };
    char ret[3];
    WSmask(&tosend,1,ret);
    if(game.players[0].client->socket != 0) send(game.players[0].client->socket, ret, strlen(ret),0 );
    if(game.players[1].client->socket != 0) send(game.players[1].client->socket, ret, strlen(ret),0 );
}

static void sendByte(int socket, char b){
    if(socket == 0) return;
    char tosend[2] = { b };
    char ret[3];
    WSmask(tosend,1,ret);
    send(socket, ret, strlen(ret),0 );
}

Player *GameHandler_GetOpponent(int socket, Game *game){
    if(game->players[0].client->socket == socket) return &game->players[1];
    if(game->players[1].client->socket == socket) return &game->players[0];
    return &(Player){};
}

Player *GameHandler_GetPlayer(int socket, Game *game){
    if(game->players[0].client->socket == socket) return &game->players[0];
    if(game->players[1].client->socket == socket) return &game->players[1];
    return &(Player){};
}

void GameHandler_SendGameDisconnectMessage(Game game){
    sendByteToPlayers(GAME_DISCONNECT_MESSAGE,game);
}

void GameHandler_SendHoverMessage(Player to, char cardIndex, Game game){
    if(cardIndex <= 0 || to.client->socket == 0) return; 
    if(cardIndex-2 < GameHandler_GetOpponent(to.client->socket,&game)->cardsInHand){
        char tosend[2] ;
        tosend[0] = (char)GAME_CARD_HOVER_MESSAGE;
        tosend[1] = cardIndex;
        char ret[10] = {0};
        WSmask(tosend,2,ret);
        send(to.client->socket, ret, strlen(ret), 0 );
    }
}

void GameHandler_SendPushMessage(Player to, Game game){
    sendByte(to.client->socket, GAME_CARD_PUSH_MESSAGE);
}

void GameHandler_PlayCard(Player *from, char cardIndex, Game *game){

    if(cardIndex < 2 || from->client->socket == 0 || 
        (from->playerIndex != game->whosTurn))
        return;
    
    if(cardIndex-2 < from->cardsInHand){

        // if(Cards_GetCard(from->hand[cardIndex-1]).manaCost > from->mana) 
        //     return;

        Player *opp = GameHandler_GetOpponent(from->client->socket,game);
        
        int card = from->hand[cardIndex-2];

        int k;
        for(k = cardIndex-2; k < from->cardsInHand-1; k++)
            from->hand[k] = from->hand[k+1];
        
        from->cardsInHand--;
    
        if(opp->client->socket == 0)
            return;

        if(game->players[0].client->socket == from->client->socket) 
            Cards_ExecCardFunc(card, &game->players[0], &game->players[1], 1);

        if(game->players[1].client->socket == from->client->socket) 
            Cards_ExecCardFunc(card, &game->players[1], &game->players[0], 1);

        char tosend[2];
        tosend[0] = (char)GAME_CARD_PLAY_MESSAGE;
        tosend[1] = (char)card;

        char ret[10] = {0};
        WSmask(tosend,2,ret);
        send(opp->client->socket, ret, strlen(ret), 0 );
    }
}

void GameHandler_EraseGame(Game *game){

    int winnerPIndex = 0;
    if(game->players[0].client!=NULL)
        if(game->players[0].client->socket != 0){
            winnerPIndex = 1;
            sendByte(game->players[0].client->socket, GAME_OPPONENT_DISCONNECT_MESSAGE);
        }

    if(game->players[1].client!=NULL)
        if(game->players[1].client->socket != 0){
            winnerPIndex = 2;
            sendByte(game->players[1].client->socket, GAME_OPPONENT_DISCONNECT_MESSAGE);
        }

    if(winnerPIndex){
        char query[200] = {0};
        sprintf(query, "INSERT INTO `games` VALUES (NULL, %i, %i, %i, NULL)%c", 
            game->players[0].client->userID, game->players[1].client->userID, winnerPIndex,'\0' );
        mysql_query(mysqlCon,query);
    }

    game->players[0].client->gameIndex = -1;
    game->players[1].client->gameIndex = -1;
    *game = (Game){};
}

void GameHandler_ExchangeUsernames(Game *game){
    int i;
    for(i = 0; i < 2; i++){
        Player *opponent = GameHandler_GetOpponent(game->players[i].client->socket, game);
        if(opponent->client->username){
            char ret[50];
            char usernameMessage[1+strlen(opponent->client->username)];
            strcpy(&usernameMessage[1], opponent->client->username);
            usernameMessage[0] = (char)GAME_TELL_OTHER_PLAYERS_NAME;
            WSmask(usernameMessage,strlen(usernameMessage),ret);
            send(game->players[i].client->socket, ret, strlen(ret),0);
        }
    }
}

void GameHandler_SwitchTurns(Game *game){
    if(game->whosTurn == 1) game->whosTurn = 2;
    else if(game->whosTurn == 2) game->whosTurn = 1;
    sendByteToPlayers(GAME_SWICH_TURNS_MESSAGE, *game);
    time(&game->lastTurn);
}

void GameHandler_SendSwitchTurnMessage(Player to, Game *game){

    Player *from = GameHandler_GetOpponent(to.client->socket, game);

    if(game->whosTurn == from->playerIndex){
        
        sendByte(to.client->socket, GAME_SWICH_TURNS_MESSAGE);
        game->whosTurn = to.playerIndex;
        time(&game->lastTurn);
    }
}

void GameHandler_UpdateGame(Game *game){
    time_t temp;
    time(&temp);
    if(temp - game->lastTurn > MAXIMUM_TURN_LENGTH){
        GameHandler_SwitchTurns(game);
    }
}

void GameHandler_NewGame(Game *game){

    if(game->players[0].client->socket == 0 || game->players[0].client->socket == 0)
        return;

    GameHandler_ExchangeUsernames(game);

    time(&game->lastTurn);
    game->whosTurn = 1;
    game->gameOn = 1;
    game->players[0].playerIndex = 1;
    game->players[1].playerIndex = 2;
    game->players[0].cardsInHand = STARTING_HAND_SIZE;
    game->players[1].cardsInHand = STARTING_HAND_SIZE;
    game->players[0].mana = STARTING_MANA;
    game->players[1].mana = STARTING_MANA;
    game->players[0].health = STARTING_HEALTH;
    game->players[1].health = STARTING_HEALTH;

    char msgs[2][STARTING_HAND_SIZE+1]= { {0}, {0} };
    msgs[0][0] = (char)STARTING_HAND_MESSAGE;
    msgs[1][0] = (char)STARTING_HAND_MESSAGE;

    int i;
    for(i = 0; i < 2; i++){
        int k;
        for(k = 0; k < STARTING_HAND_SIZE; k++){
            srand(clock());
            game->players[i].hand[k] = (rand() % ALL_CARDS_IN_GAME + 1);
            msgs[i][k+1] = (int)game->players[i].hand[k];
        }
        char ret[30] = {0};
        WSmask(msgs[i],strlen(msgs[i]),ret);
        send(game->players[i].client->socket, ret, strlen(ret), 0 );
    }

    for(i = 0; i < 2; i++){
        char tosend[2];
        tosend[0] = (char)GAME_ON_MESSAGE;
        tosend[1] = (char)i+1;
        char ret[2] = {0};
        WSmask(tosend,2,ret);
        send(game->players[i].client->socket, ret, strlen(ret),0);
    }
}