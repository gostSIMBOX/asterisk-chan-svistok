/* Dongle proxy-only composition fragment. */
extern int32_t svistok_dongle_impl_getACD(uint32_t calls, uint32_t duration);
static int32_t getACD(uint32_t calls, uint32_t duration)
{
	return svistok_hook_normalize_acd(svistok_dongle_impl_getACD(calls, duration));
}
