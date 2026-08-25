static int private_value = 7;

int unchanged(int value)
{
    return value + 1;
}

int changed(int value)
{
    return value + 2;
}

int removed(int value)
{
    return value - 1;
}
