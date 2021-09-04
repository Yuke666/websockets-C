#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "websocket.h"
#include "window.h"
#include "gameHandler.h"

#define MAX_GAMES 100
#define MSG_LIMIT_PER_SECOND 200
#define MAX_CLIENTS 500

static int  serverSocket;
static struct sockaddr_in server;
static Client clients[MAX_CLIENTS];
static Game   games[MAX_GAMES];

static void mainConnections();

int main(int argc , char *argv[]){

    mysqlCon = mysql_init(NULL);
    // if(mysql_real_connect(mysqlCon, "yukiz.in", "yukizin_yukizin", "", "yukizin_game", 0 ,NULL, 0) == NULL)
    if(mysql_real_connect(mysqlCon, "localhost", "root", "", "game", 0 ,NULL, 0) == NULL)
        printf("%s\n", mysql_error(mysqlCon));
    else 
        printf("Connected to mysql database\n");

    server.sin_port         = htons(23415);
    server.sin_family       = AF_INET;
    server.sin_addr.s_addr  = INADDR_ANY;
    serverSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    int optval = 1;
    setsockopt(serverSocket, SOL_SOCKET,SO_REUSEADDR, &optval, sizeof(optval));
    
    if(serverSocket == -1) { printf("Error creating server socket\n"); return 1; }
    int res = bind(serverSocket,(struct sockaddr *)&server, sizeof(server));
    if(res == -1) { printf("Error binding server socket\n"); return 1; }
    res = listen(serverSocket,6);
    if(res == -1) { printf("Error listening on server socket\n"); return 1; }

    mainConnections();

    mysql_close(mysqlCon);
    close(serverSocket);
    return 0;
}

static void *updateGames(){
    int k;
    while(1){
        for(k = 0; k < MAX_GAMES; k++)
            if(games[k].gameOn)
                GameHandler_UpdateGame(&games[k]);
    }
    pthread_exit(NULL);
}

static void closeClient(Client *client){
    if(client->socket != 0)
        close(client->socket);
    client->socket = 0;
    client->lookingForGame = 0;
    client->messagesPerSecond = 0;
    strcpy(client->username,"");
}

static void ifGameCloseGame(Game *game){
    GameHandler_EraseGame(game);
    // if( game->gameOn )
    //     GameHandler_SendGameDisconnectMessage(*game);
}

static void sendByte(int socket, char b){ 
    if(socket == 0 ) return;
    char tosend[2] = { b };
    char ret[3];
    WSmask(tosend,1,ret);
    send(socket, ret, strlen(ret),0 );
}

static void findGameForClient(Client *client){
    int m, i;
    for(i = 0; i < MAX_CLIENTS; i++){
        if(clients[i].lookingForGame && &clients[i] != client){
            for(m = 0; m < MAX_GAMES; m++){
                if(games[m].players[0].client == NULL && games[m].players[1].client == NULL ){
                    games[m].players[0].client = client;
                    games[m].players[1].client = &clients[i];
                    GameHandler_NewGame(&games[m]);
                    clients[i].gameIndex = m;
                    client->gameIndex = m;
                    clients[i].lookingForGame = 0;
                    client->lookingForGame = 0;
                    return;
                }
            }
        }
    }
    client->lookingForGame = 1;
}


static void logInClient( char *data, int len, Client *client){

    char pword[45] = {0};
    char uname[20] = {0};

    int i, f;

    for(i = 0; i < 15; i++)
        if(i > len || data[i] == ':' ) break;

    memmove(uname, data, i);
    printf("%s\n",uname );

    i++;
    for(f = 0; f < 40; f++)
        if(f+i > len) break;

    memmove(pword, data+i, f);
    printf("%s\n",pword );

    char escapeduname[100] = {0};
    char escapedpword[100] = {0};
    mysql_real_escape_string(mysqlCon, escapeduname, uname, strlen(uname));
    mysql_real_escape_string(mysqlCon, escapedpword, pword, strlen(pword));

    char sendMsg[1000] = {0};
    sprintf(sendMsg, "SELECT `id` FROM `users` WHERE `username` = '%s' AND `password` = '%s'", escapeduname, escapedpword);

    mysql_query(mysqlCon,sendMsg);

    MYSQL_RES *mysqlResult = mysql_store_result(mysqlCon);
    if(mysqlResult == NULL) return;

    int num = mysql_num_rows(mysqlResult);
    if(num > 0) {
        MYSQL_ROW row = mysql_fetch_row(mysqlResult);
        client->userID = atoi(row[0]);
        mysql_free_result(mysqlResult);
        strcpy(client->username, uname);
        sendByte(client->socket,LOG_IN_OKAY_MESSAGE);
        return;
    }
    mysql_free_result(mysqlResult);
    sendByte(client->socket,LOG_IN_FAILED_MESSAGE);
}

static void mainConnections(int serv){

    char buf[1024];
    int  len;
    fd_set readfds;
    
    pthread_t thread;
    pthread_create(&thread, NULL, updateGames, NULL);

    while(1){

        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);

        int max = serverSocket;
        int k;
        for(k = 0; k < MAX_CLIENTS; k++){
            if(clients[k].socket == 0) continue;
            FD_SET(clients[k].socket, &readfds);

            if(clients[k].socket > max) 
                max = clients[k].socket;
        }
        int activity = select(max+1, &readfds, NULL, NULL, NULL);

        if(activity < 0) {
            printf("SERVER ERROR\n");
            break;
        }

        if(FD_ISSET(serverSocket, &readfds)){
            struct sockaddr_in client;
            int  addrlen = sizeof(client);
            int new_socket = accept(serv,(struct sockaddr*)&client,&addrlen);
            len = read(new_socket,buf,1024,0);
            if(len <= 0) continue;
            WShandshake(buf,len,new_socket);
            int k;
            for(k = 0; k < MAX_CLIENTS; k++){
                if(clients[k].socket == 0){
                    printf("new connection\n");
                    clients[k].socket = new_socket;
                    clients[k].gameIndex = -1;
                    sendByte(new_socket,SERVER_CONNECTED_MESSAGE);
                    break;
                }
            }
            if(k >= MAX_CLIENTS)
                sendByte(new_socket, SERVER_ERROR_MESSAGE);
        }

        for(k = 0; k < MAX_CLIENTS; k++){
            if(clients[k].socket == 0) continue;

            if(FD_ISSET(clients[k].socket, &readfds)){

                if(clients[k].messagesPerSecond >= MSG_LIMIT_PER_SECOND){
                    sendByte(clients[k].socket, SERVER_ERROR_MESSAGE);
                    closeClient(&clients[k]);
                    if(clients[k].gameIndex < MAX_GAMES && clients[k].gameIndex >= 0)
                        ifGameCloseGame(&games[clients[k].gameIndex]);
                    continue;
                }
    
                time_t t;
                time(&t);
                if(t - clients[k].lastSecond > 1){
                    clients[k].messagesPerSecond = 0;
                    time(&clients[k].lastSecond);
                }

                clients[k].messagesPerSecond++;

                int lenToRead = 0;
                ioctl(clients[k].socket, FIONREAD, &lenToRead);
                char buffer[lenToRead];
                len = recv(clients[k].socket,buffer,lenToRead,0);

                if(len <= 0){
                    printf("disconnect client from message length <=\n");
                    sendByte(clients[k].socket, SERVER_ERROR_MESSAGE);
                    closeClient(&clients[k]);
                    if(clients[k].gameIndex < MAX_GAMES && clients[k].gameIndex >= 0)
                        ifGameCloseGame(&games[clients[k].gameIndex]);
                    continue;
                }

                char data[1024];
                if(!WSunmask(buffer,len,data)){
                    printf("disconnected client from websocket disconnect message\n");
                    closeClient(&clients[k]);
                    if(clients[k].gameIndex < MAX_GAMES && clients[k].gameIndex >= 0)
                        ifGameCloseGame(&games[clients[k].gameIndex]);
                    continue;
                }

                if(clients[k].gameIndex == -1 && data[0] == (char)SEARCHING_FOR_GAME_MESSAGE && strlen(clients[k].username) > 0){
                    findGameForClient(&clients[k]);
                }

                if(clients[k].gameIndex != -1 && games[clients[k].gameIndex].gameOn){
                    Player *opponent = GameHandler_GetOpponent(clients[k].socket, &games[clients[k].gameIndex]);
                    Player *player   = GameHandler_GetPlayer(clients[k].socket, &games[clients[k].gameIndex]);

                    if(data[0] == GAME_CARD_HOVER_MESSAGE)
                        GameHandler_SendHoverMessage(*opponent, data[1], games[clients[k].gameIndex]);
                    
                    if(data[0] == GAME_CARD_PLAY_MESSAGE)
                        GameHandler_PlayCard(player, data[1], &games[clients[k].gameIndex]);
                    
                    if(data[0] == GAME_CARD_PUSH_MESSAGE)
                        GameHandler_SendPushMessage(*opponent, games[clients[k].gameIndex]);
                    
                    if(data[0] == GAME_SWICH_TURNS_MESSAGE)
                        GameHandler_SendSwitchTurnMessage(*opponent, &games[clients[k].gameIndex]);
                }

                if(data[0] == ACCOUNT_LOGIN_INFO_MESSAGE)
                    logInClient(&data[1], len, &clients[k]);
            }
        }
    }
    return;
}

// Fix games with self
// Make client prevent you from ddosing accidentally so only people actually doing it are kicked
// login timeout time
// timeout time iin game