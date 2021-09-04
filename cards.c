#include "cards.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Card allCards[ALL_CARDS_IN_GAME] = {
    { 1, 1 },
    { 1, 0 },
    { 1, 0 },
    { 3, 0 },
    { 0, 0 },
    { 3, 0 },
};

Minion allMinions[ALL_MINIONS_IN_GAME] = {
    { 1,2 },
    { 5,5,1 },
};

void (*cardFuncs)(Game *);

void CardOneFunc(Player *from, Player *to, char jpOrEot){
	to->health -= 3;
}

void CardTwoFunc(Player *from, Player *to, char jpOrEot){
	if(from->minionsDown < MAX_MINIONS)
		from->minions[from->minionsDown++] = allMinions[0];
}

void CardThreeFunc(Player *from, Player *to, char jpOrEot){
	from->health += 2;
}

void CardFourFunc(Player *from, Player *to, char jpOrEot){
	if(from->minionsDown < MAX_MINIONS)
		from->minions[from->minionsDown++] = allMinions[1];
}

void CardSixFunc(Player *from, Player *to, char jpOrEot){

	struct animeCard {
		int cardIndex;
		Player *player;
	} animeCards[to->cardsInHand + from->cardsInHand];

	int animeCardsIndex = 0;

	int k;
	for(k = 0; k < from->cardsInHand; k++){
		if(allCards[from->hand[k]-1].isAnime != 0){
			animeCards[animeCardsIndex++] = (struct animeCard){ k, from };
		} 
	}

	for(k = 0; k < to->cardsInHand; k++){
		if(allCards[to->hand[k]-1].isAnime != 0){ 
			animeCards[animeCardsIndex++] =(struct animeCard) {k , to};
		}	
	}

    if(animeCardsIndex == 0) 
    	return;

	srand(clock());
	int randCard = (rand() % animeCardsIndex);

	Player *removePlayer = animeCards[randCard].player;

	int m;
	for(m = animeCards[randCard].cardIndex; m < removePlayer->cardsInHand-1; m++)
		removePlayer->hand[m] = removePlayer->hand[m+1];
	
	removePlayer->cardsInHand--;
    char tosend[3];
    tosend[0] = (char)GAME_CARD_REMOVE_MESSAGE;
    tosend[1] = (char)animeCards[randCard].player->playerIndex;
    tosend[2] = (char)animeCards[randCard].cardIndex + 1;
    char ret[10] = {0};
    WSmask(tosend,3,ret);
    if(from->client->socket != 0) send(from->client->socket, ret, strlen(ret),0 );
    if(to->client->socket   != 0) send(to->client->socket,   ret, strlen(ret),0 );
}

Card Cards_GetCard(int index){
	return allCards[index];
}

void Cards_ExecCardFunc(int cardIndex, Player *from, Player *to, char jpOrEot){
	switch(cardIndex){
		case 1: CardOneFunc(from, to, jpOrEot); break;
		case 2: CardTwoFunc(from, to, jpOrEot); break;
		case 3: CardThreeFunc(from, to, jpOrEot); break;
		case 4: CardFourFunc(from, to, jpOrEot); break;
		case 6: CardSixFunc(from, to, jpOrEot); break;
	}
}