/* Svistok-only composition fragment. */

void ast_channel_get_var(const struct ast_channel *parent, char *varname1, char *value)
{
        struct ast_var_t *current;
        const char *varname;
	strcpy(value,"");
        AST_LIST_TRAVERSE(ast_channel_varshead(parent), current, entries) {
//                int vartype = 0;

                varname = ast_var_full_name(current);
                if (!varname)
                        continue;
		if(strcmp(varname,varname1)==0) {strcpy(value, ast_var_value(current));}
//	    ast_verb(3, "%s=%s\n",varname, ast_var_value(current));
        }
}

void ast_channel_show_vars(const struct ast_channel *parent)
{
        struct ast_var_t *current;
        const char *varname;

        AST_LIST_TRAVERSE(ast_channel_varshead(parent), current, entries) {
//                int vartype = 0;

                varname = ast_var_full_name(current);
                if (!varname)
                        continue;

	    ast_verb(3, "%s=%s\n",varname, ast_var_value(current));
        }
}

EXPORT_DEF int can_sms(struct pvt* pvt)
{

	if(pvt->ring || PVT_STATE(pvt, chan_count[CALL_STATE_INCOMING])) return 0; //state = "Ring";
	if(pvt->cwaiting || PVT_STATE(pvt, chan_count[CALL_STATE_WAITING])) return 0; //state = "Waiting";
	if(pvt->dialing ||
			(PVT_STATE(pvt, chan_count[CALL_STATE_INIT])
				+
				PVT_STATE(pvt, chan_count[CALL_STATE_DIALING])
				+
				PVT_STATE(pvt, chan_count[CALL_STATE_ALERTING])) > 0)
			return 0; //state = "Dialing";

	if(PVT_STATE(pvt, chan_count[CALL_STATE_ACTIVE]) > 0) return 0; //state = "Active";
	if(PVT_STATE(pvt, chan_count[CALL_STATE_ONHOLD]) > 0) return 0; //state = "Held";

	return 1;
}
