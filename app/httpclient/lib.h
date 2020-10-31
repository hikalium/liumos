#ifndef HTTPCLIENT_LIB_H
#define HTTPCLIENT_LIB_H

#include <stdint.h>

void PrintNum(int v);
void Println(char* text);
uint16_t StrToNum16(const char* s, const char** next);

#endif // HTTPCLIENT_LIB_H
