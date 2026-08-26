/* Svistok hook composition fragment. */
static void svistok_hook_copy_serial(struct pdiscovery_result *dst, const struct pdiscovery_result *src)
{
	if(src->serial) dst->serial=ast_strdup(src->serial);
}
static void svistok_hook_free_serial(struct pdiscovery_result *res)
{
	if(res->serial) { ast_free(res->serial); res->serial=NULL; }
}
