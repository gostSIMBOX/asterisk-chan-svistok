/* Svistok hook composition fragment. */
static void svistok_hook_log_cpin(char *str, size_t len)
{
	(void)len;
	ast_verb(3,"ATCPIN: %s",str);
}
