#ifndef _SHELL_H_
#define _SHELL_H_

void dialogue_get_filename(int idx);
void pstat(void);
int ls(void);
void cat(unsigned short *file_name);
void edit(unsigned short *file_name);
void view(unsigned short *img_name);
void shell(void);

#endif
