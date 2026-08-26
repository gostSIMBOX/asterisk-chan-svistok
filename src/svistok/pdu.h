#ifndef SVISTOK_PDU_H_INCLUDED
#define SVISTOK_PDU_H_INCLUDED

EXPORT_DECL const char * pdu_parse_cds(char ** pdu, size_t tpdu_length, char * oa, size_t oa_len, str_encoding_t * oa_enc, char ** msg, str_encoding_t * msg_enc);

#endif /* SVISTOK_PDU_H_INCLUDED */
