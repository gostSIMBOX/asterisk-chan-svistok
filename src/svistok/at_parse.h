#ifndef SVISTOK_AT_PARSE_H_INCLUDED
#define SVISTOK_AT_PARSE_H_INCLUDED

EXPORT_DECL const char* at_parse_cds (char** str, size_t len, char* oa, size_t oa_len, str_encoding_t* oa_enc, char** msg, str_encoding_t* msg_enc);
EXPORT_DECL char* at_parse_spn (char* str);
EXPORT_DECL int at_parse_sysinfo (char * str, int * srvst, int * srvd, int * roamst, int * sysmode, int * simst);

#endif /* SVISTOK_AT_PARSE_H_INCLUDED */
