#ifndef SVISTOK_CHARACTERIZATION_ASTERISK_LOGGER_H
#define SVISTOK_CHARACTERIZATION_ASTERISK_LOGGER_H

#define LOG_ERROR 1
#define LOG_WARNING 2

void ast_debug(int level, const char *format, ...);
void ast_log(int level, const char *format, ...);

#endif
