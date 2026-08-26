/* Svistok-only composition fragment. */

static char * pdiscovery_handle_sn(const char * devname, char * str)
{
	static const char SERIAL[] = "\r\n^SN:";
	char * serial = strstr(str, SERIAL);

	if(serial) {
		serial += STRLEN(SERIAL);
		while(serial[0] == ' ')
			serial++;
		str = serial;

		while((str[0] >= '0' && str[0] <= '9')||(str[0] >= 'A' && str[0] <= 'Z'))
			str++;
		if((str - serial) == SERIAL_SIZE && str[0] == '\r' && str[1] == '\n') {
			str[0] = 0;
			serial = ast_strdup(serial);
			str[0] = '\r';
			ast_debug(4, "[%s discovery] found S %s\n", devname, serial);
			return serial;
		}
	}

	return NULL;
}
