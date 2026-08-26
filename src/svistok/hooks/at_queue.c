/* Svistok hook composition fragment. */
static void svistok_hook_log_at_write(struct pvt *pvt, const char *buf, size_t count)
{
	char dn[256];
	timenow(dn);
	at_log(pvt,dn,strlen(dn));
	at_log(pvt," >> ",4);
	at_log(pvt,buf,count);
}
