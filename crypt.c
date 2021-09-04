#include <stdint.h>
uint8_t block[64];
uint32_t words[80];
uint32_t H0 = 0x67452301;
uint32_t H1 = 0xEFCDAB89;
uint32_t H2 = 0x98BADCFE;
uint32_t H3 = 0x10325476;
uint32_t H4 = 0xC3D2E1F0;

#define SHA1LeftRotate(bits,word) \
                (((word) << (bits)) | ((word) >> (32-(bits))))

void SHA1GenerateHash(){

    int x;
    for(x = 0; x < 16; x++){
        words[x]  = block[x * 4] << 24;
        words[x] |= block[x*4+1] << 16;
        words[x] |= block[x*4+2] <<  8;
        words[x] |= block[x*4+3];
    }

    for(x = 16; x < 80; x++){
        words[x] = SHA1LeftRotate(1,words[x-3] ^ words[x-8] ^ words[x-14] ^ words[x-16]);
    }

    uint32_t a = H0;
    uint32_t b = H1;
    uint32_t c = H2;
    uint32_t d = H3;
    uint32_t e = H4;
    uint32_t f;
    uint32_t k;
    uint32_t temp;

    for(x = 0; x < 80; x++){

        if (x >= 20 && x < 40){
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (x >= 40 && x < 60){
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else if (x >= 60 && x < 80){
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        } else if(x < 20){
            f = ((b & c) | ((~b) & d));
            k = 0x5A827999;
        }

        temp = SHA1LeftRotate(5,a) + f + e + k + words[x];
        e = d;
        d = c;
        c = SHA1LeftRotate(30,b);
        b = a;
        a = temp;
        //printf("%x %x %x %x %x\n", a,b,c,d,e);
    }

    H0 = H0 + a;
    H1 = H1 + b;
    H2 = H2 + c;
    H3 = H3 + d;
    H4 = H4 + e;

    //sprintf(res,"%x%x%x%x%x", H0,H1,H2,H3,H4); for hex
}

void SHA1PadMessage(char *msg, uint64_t length){

    int x = 0;
    while(x < length){
        block[x] = msg[x];
        x++;
    }

    if(x < 56){
        block[x++] = 0x80;
        while(x < 56){
            block[x] = 0x00;
            x++;
        }
    } else {
        block[x++] = 0x80;
        while(x < 64){
            block[x] = 0x00;
            x++;
        }
        SHA1GenerateHash();
        x = 0;
        while(x < 56){
            block[x] = 0x00;
            x++;
        }
    }

    length *= 8; //size in bits
    block[56] = length >> 56;
    block[57] = length >> 48;
    block[58] = length >> 40;
    block[59] = length >> 32;
    block[60] = length >> 24;
    block[61] = length >> 16;
    block[62] = length >> 8;
    block[63] = length;
    SHA1GenerateHash();
}

void SHA1(char *msg, uint64_t length,uint32_t *res){

    H0 = 0x67452301;
    H1 = 0xEFCDAB89;
    H2 = 0x98BADCFE;
    H3 = 0x10325476;
    H4 = 0xC3D2E1F0;

    SHA1PadMessage(msg, length);

    res[0] = H0;
    res[1] = H1;
    res[2] = H2;
    res[3] = H3;
    res[4] = H4;
    // sprintf(res,"%x%x%x%x%x", H0,H1,H2,H3,H4);
}

void base64_encode(uint8_t *str, int size, char *enc){

    char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int x = 0;
    int temp[4];

    while(x < size){
        if(x <= (size - 3)){

            temp[0] = str[x] >> 2;
            *enc++  = base64[temp[0]];
            temp[1] = ((str[x] << 6) & 0xC0) >> 2;
            x++;
            temp[1] = temp[1] | (str[x] >> 4);
            *enc++  = base64[temp[1]];
            temp[2] = (str[x] & 0x0F) << 2;
            x++;
            temp[2] = temp[2] | ((str[x] & 0xC0) >> 6);
            *enc++ = base64[temp[2]];
            temp[3] = str[x] & 0x3F;
            x++;
            *enc++  = base64[temp[3]];
            continue;

        } else {
            if(x < size){

                if((size - x) == 1){

                    temp[0]  = str[x] >> 2;
                    *enc++   = base64[temp[0]];
                    temp[1]  = ((str[x] << 6) & 0xC0) >> 2;

                    *enc++ = base64[temp[1]];
                    *enc++ = '=';
                    *enc++ = '=';
                    *enc++ = '\0';
                    
                } else if((size - x) == 2){

                    temp[0] = str[x] >> 2;
                    *enc++ = base64[temp[0]];
                    temp[1] = ((str[x] << 6) & 0xC0) >> 2;
                    x++;
                    temp[1] = temp[1] | (str[x] >> 4);
                    *enc++ = base64[temp[1]];

                    temp[2] = (str[x] & 0x0F) << 2;
                    *enc++ = base64[temp[2]];

                    *enc++ = '=';
                    *enc++ = '\0';
                }
            }
        }
        break;
    }
}