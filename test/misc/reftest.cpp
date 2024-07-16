#include <stdio.h>

int main(int argc, char** argv)
{
    int a = 1;
    int& b = a;
    int c = b;
    int& d = b;

    a = 2;

    printf("%d %d %d %d\n", a, b, c, d);

    return 0;
}
