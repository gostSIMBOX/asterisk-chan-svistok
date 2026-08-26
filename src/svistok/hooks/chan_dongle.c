/* Svistok hook composition fragment. */
static void svistok_hook_initialize_svistok(void)
{
	dserial_init();
	clear_state();
	putfiles("","svistok","version",svistok_version);
	IAXME_get();
}
