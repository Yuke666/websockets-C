#ifndef CARDS_DEF
#define CARDS_DEF
#include "gameHandler.h"



Card Cards_GetCard(int index);
void Cards_ExecCardFunc(int cardIndex, Player *from, Player *to, char jpOrEot);

#endif