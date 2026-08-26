#ifndef SVISTOK_CHAN_DONGLE_H_INCLUDED
#define SVISTOK_CHAN_DONGLE_H_INCLUDED

EXPORT_DECL void ast_channel_get_var(const struct ast_channel * parent, char * varname1, char * value);
EXPORT_DECL void ast_channel_show_vars(const struct ast_channel * parent);
EXPORT_DECL int can_sms(struct pvt* pvt);

#endif /* SVISTOK_CHAN_DONGLE_H_INCLUDED */
