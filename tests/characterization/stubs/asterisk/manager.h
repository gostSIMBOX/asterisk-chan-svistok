#ifndef SVISTOK_CHARACTERIZATION_ASTERISK_MANAGER_H
#define SVISTOK_CHARACTERIZATION_ASTERISK_MANAGER_H
struct mansession;
struct message;
#define EVENT_FLAG_CALL 1
#define EVENT_FLAG_SYSTEM 2
#define EVENT_FLAG_REPORTING 4
#define EVENT_FLAG_CONFIG 8
const char *astman_get_header(const struct message *, const char *);
int astman_send_ack(struct mansession *, const struct message *, const char *);
int astman_send_error(struct mansession *, const struct message *, const char *);
int astman_send_listack(struct mansession *, const struct message *, const char *, const char *);
#endif
