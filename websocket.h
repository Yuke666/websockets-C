#ifndef WEBSOCKETS_DEF
#define WEBSOCKETS_DEF
void WShandshake(char *read, int len, int cli);
void WSmask(char *msg,int len, char *ret);
int WSunmask(char *msg, int msgLen, char *ret); 
#endif