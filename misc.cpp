#include <cstdio>
#include <iostream>

#include "misc.hpp"

bool
get_bit(char x, int n)
{
    return x & (1u << n);
}

void
set_bit(char& x, int n, bool v)
{
    if (v) {
        x |= 1u << n;
    } else {
        x &= ~(1u << n);
    }
}

void
print_hex(char x)
{
    std::cout.flush();
    printf("$%02x", x);
    fflush(stdout);
}

void
print_hex(unsigned long x)
{
    std::cout.flush();
    printf("$%04lx", x);
    fflush(stdout);
}
