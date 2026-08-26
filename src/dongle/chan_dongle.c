/* Dongle proxy-only composition fragment. */
extern int svistok_dongle_impl_load_module(void);
static int load_module(void)
{
	svistok_hook_initialize_svistok();
	return svistok_dongle_impl_load_module();
}
