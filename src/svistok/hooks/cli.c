/* Svistok hook composition fragment. */
static int32_t svistok_hook_normalize_acd(int32_t value)
{
	return value == -1 ? 0 : value;
}
