static int private_value = 7;

int unchanged(int value)
{
    return value + 1;
}

int changed(int value)
{
    return value + 20;
}

int added(int value)
{
    return value * 3;
}
