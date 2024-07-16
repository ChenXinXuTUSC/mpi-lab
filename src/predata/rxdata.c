#include <common_c.h>

int main(int argc, char** argv)
{
    FILE* fp = fopen("10K.bin", "rb");
    if (fp == NULL)
    {
        perror("error open data bin");
        exit(1);
    }

    int num = 0;
    int cnt = 0;
    while (fread(&num, sizeof(int), 1, fp) == 1)
    {
        cnt++;
        printf("%d ", num);
    }

    if (feof(fp))
        printf("\n\nfinish reading data bin, total %E\n\n", (double)(cnt));
    else
        fprintf(stderr, "error reading data bin\n");

    return 0;
}
