


//void ast_mutex_lock_pvt(struct pvt* pvt);
void ast_mutex_unlock_pvt(struct pvt* pvt);
//int ast_mutex_trylock_pvt(struct pvt* pvt);


int mutex_lock_pvt_e(struct pvt* pvt, const char* filename, int lineno);
int mutex_unlock_pvt_e(struct pvt* pvt, const char* filename, int lineno);
int mutex_trylock_pvt_e(struct pvt* pvt, const char* filename, int lineno);



#define ast_mutex_lock_pvt(a)  mutex_lock_pvt_e(a, __FILE__, __LINE__)
/*
#define ast_mutex_unlock_pvt(a)  mutex_unlock_pvt_e(a, __FILE__, __LINE__)
*/
#define ast_mutex_trylock_pvt(a)  mutex_trylock_pvt_e(a, __FILE__, __LINE__)


void readglsettings();
void readpvtlimits(struct pvt* pvt);
void writepvtlimits(struct pvt* pvt);
void writepvtstate(struct pvt* pvt);
void readpvterrors(struct pvt* pvt);
void writepvterrors(struct pvt* pvt);
void readpvtinfo(struct pvt* pvt);
void writepvtinfo(struct pvt* pvt);

int getfilei (char* devtype, char* fileitem, char* filetype, int *value);
int getfiles (char* devtype, char* fileitem, char* filetype, char *value);
int putfilei (char* devtype, char* fileitem, char* filetype, int value);
int putfiles (char* devtype, char* fileitem, char* filetype, char* value);
int putgetfilei (char putget, char* devtype, char* fileitem, char* filetype, int value,  struct ast_cli_args* a);

int putfileslog2 (char* devtype, char* fileitem, char* filetype, const char* valueformat, va_list va);