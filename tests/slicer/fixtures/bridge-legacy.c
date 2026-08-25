static int upstream_helper(int value)
{
    return value + 1;
}

static int overlay_helper(int value)
{
    return value + 20;
}

static int shared_counter = 10;
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
    shared_counter += 1;
    return upstream_helper(value) + shared_values[0];
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

int overlay_variadic_call(void)
{
    return variadic_sum(4, 5);
}
