/* Dongle proxy-only composition fragment. */
extern int svistok_dongle_impl_at_write(struct pvt *pvt, const char *buf, size_t count);
EXPORT_DEF int at_write(struct pvt *pvt, const char *buf, size_t count)
{
	svistok_hook_log_at_write(pvt, buf, count);
	return svistok_dongle_impl_at_write(pvt, buf, count);
}
