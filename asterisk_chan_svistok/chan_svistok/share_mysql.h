int a_mysql_conenct ();
int mysql_getfilei (char* devtype, char* fileitem, char* filetype, int *value);
int mysql_getfilel (char* devtype, char* fileitem, char* filetype, long int *value);
int mysql_getfiles (char* devtype, char* fileitem, char* filetype, char *value);
int mysql_putfilei (char* devtype, char* fileitem, char* filetype, int value);
int mysql_putfilei (char* devtype, char* fileitem, char* filetype, long int value);
int mysql_putfiles (char* devtype, char* fileitem, char* filetype, char* value);


