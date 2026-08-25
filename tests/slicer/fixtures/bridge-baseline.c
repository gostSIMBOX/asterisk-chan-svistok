static int upstream_helper(int value)
{
    return value + 1;
}

static int overlay_helper(int value)
{
    return value + 2;
}

static int shared_counter = 1;
static const int shared_values[2] = {6, 7};

int upstream_call(int value)
{
    return overlay_helper(value);
}

int read_counter(void)
{
    return shared_counter;
}

int svistok_call(int value)
{
    return value;
}

static int variadic_sum(int first, ...)
{
    __builtin_va_list arguments;
    int second;
    __builtin_va_start(arguments, first);
    second = __builtin_va_arg(arguments, int);
    __builtin_va_end(arguments);
    return first + second;
}
