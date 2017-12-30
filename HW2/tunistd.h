#define TUNISTD_H
#include "tty_public.h"

int exit(int exitcode);
int write(int dev, char *buf, int nchar);
int read(int dev, char *buf, int nchar);
int control(int dev, int code, int val);

