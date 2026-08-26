/* Dongle proxy-only composition fragment. */
extern int svistok_dongle_impl_at_parse_cpin(char *str, size_t len);
EXPORT_DEF int at_parse_cpin(char *str, size_t len)
{
	svistok_hook_log_cpin(str, len);
	return svistok_dongle_impl_at_parse_cpin(str, len);
}
