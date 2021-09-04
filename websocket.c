#include "crypt.h"
#include <string.h>
#include <stdio.h>

void WShandshake(char *read, int len, int cli){

    int x, k;
    char handshake_key[50] = {0};
    char *match = "Sec-WebSocket-Key: ";
    for(x = 0; x < len; x++){
        for(k = 0; read[x] == match[k]; x++, k++){
            if(k >= strlen(match)-1){
                x++;
                for(k = 0; read[x] != '\r' && read[x] != '\n' && read[x] != ' '; x++, k++){
                    handshake_key[k] = read[x];
                }
                handshake_key[k] = '\0';
            }
        }
    }

    char key[50] = {0};
    sprintf(key, "%s%s", handshake_key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11\0");

    uint32_t result[5] = {0};
    SHA1(key,strlen(key),result);
    
    k = 0;
    char enc[20];
    for(x = 0; x < 5; x++){
        enc[k] = (result[x] >> 28) << 4;
        enc[k] |= ((result[x] >> 24) & 0x0f);
        k++;
        enc[k] = (result[x] >> 20) << 4;
        enc[k] |= ((result[x] >> 16) & 0x00f);
        k++;
        enc[k] = (result[x] >> 12) << 4;
        enc[k] |= ((result[x] >> 8) & 0x000f);
        k++;
        enc[k] = (result[x] >> 4) << 4;
        enc[k] |= ((result[x]) & 0x0000f);
        k++;
    }

    char ws_key[50] = {0};
    base64_encode(enc,strlen(enc),ws_key);

    char *part = "HTTP/1.1 101 Web Socket Protocol Handshake\r\n"
                 "Upgrade: websocket\r\n"
                 "Connection: Upgrade\r\n"
                 "Sec-WebSocket-Accept: \0";
    char resp[1024] = {0};
    sprintf(resp,"%s%s\r\n\r\n",part,ws_key);

    if(send(cli,resp,strlen(resp),0)  == -1){
        printf("Websocket Error");
    }
}

void WSmask(char *msg,int len, char *ret){
    *ret++ = (char)129;

    if(len < 126){
        *ret++ = (char)len;

    } else if(len >= 126 && len < 65536){
        *ret++ = (char)126;
        *ret++ = (char)(len & 0xFF00) >> 8;
        *ret++ = (char)(len & 0x00FF);

    } else if(len >= 65536){
        *ret++ = (char)127;
        *ret++ = (char)(len & 0xFF000000) >> 24;
        *ret++ = (char)(len & 0x00FF0000) >> 16;
        *ret++ = (char)(len & 0x0000FF00) >>  8;
        *ret++ = (char)(len & 0x000000FF);
    }
    sprintf(ret,"%s%c",msg,'\0');
}

int WSunmask(char *msg, int msgLen, char *ret){

    if(msgLen <= 0) return 2;

    int opCode = msg[0] & 0x0F;
    /*  From the RFC: (http://tools.ietf.org/html/rfc6455#section-5.2)
        %x0 denotes a continuation frame
        %x1 denotes a text frame
        %x2 denotes a binary frame
        %x3-7 are reserved for further non-control frames
        %x8 denotes a connection close
        %x9 denotes a ping
        %xA denotes a pong
        %xB-F are reserved for further control frames  */ 
    if(opCode == 8){
        return 0; // Returns 0 on disconnect.
    }

    int  subLen;
    char mask[4];
    int  len = msg[1] & 127;
    char data[msgLen];

    if(len == 126){
        memmove(mask,msg+4,4);
        memmove(data,msg+8,msgLen-8);
        subLen = 8;
    } else if (len == 127){
        memmove(mask,msg+10,4);
        memmove(data,msg+14,msgLen-14);
        subLen = 14;
    } else if(len < 126) {
        memmove(mask,msg+2,4);
        memmove(data,msg+6,msgLen-6);
        subLen = 6;
    }

    int x;
    if(subLen != 0){
        for(x = 0; x < (msgLen - subLen); x++){
            *ret++ = data[x] ^ mask[x%4];
        }
    }

    return 1;
} 