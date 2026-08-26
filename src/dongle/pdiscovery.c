/* Dongle proxy-only composition fragment. */
extern void svistok_dongle_impl_info_copy(struct pdiscovery_result *, const struct pdiscovery_result *);
extern void svistok_dongle_impl_info_free(struct pdiscovery_result *);
static void info_copy(struct pdiscovery_result *dst, const struct pdiscovery_result *src)
{
	svistok_dongle_impl_info_copy(dst,src);
	svistok_hook_copy_serial(dst,src);
}
static void info_free(struct pdiscovery_result *res)
{
	svistok_dongle_impl_info_free(res);
	svistok_hook_free_serial(res);
}
