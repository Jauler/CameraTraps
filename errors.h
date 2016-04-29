#ifndef ERRORS_H_INCLUDED
#define ERRORS_H_INCLUDED

#include <error.h>

#define ERR(exit, errno, format, ...)			error(exit, errno, format, ##__VA_ARGS__)
#define WARN(errno, format, ...)				error(0, errno, format, ##__VA_ARGS__)

#endif

