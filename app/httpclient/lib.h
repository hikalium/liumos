#ifndef HTTPCLIENT_LIB_H
#define HTTPCLIENT_LIB_H

#include <stdint.h>

void printnum(int v);
void println(char* text);
uint16_t str_to_num16(const char* s, const char** next);

#endif // HTTPCLIENT_LIB_H
