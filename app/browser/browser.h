#ifndef BROWSER_H 
#define BROWSER_H 

//typedef unsigned int size_t;
int read(int fd, void *buf, int nbyte);
int write(int fd, const void *buf, int nbyte);
void exit(int);

#endif // BROWSER_H
