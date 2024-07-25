#include <common_c.h>

int main(int argc, char** argv)
{
    FILE* fp = fopen(argv[1], "rb");
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
        printf("\n\nfinish reading data bin, total %d\n\n", cnt);
    else
        fprintf(stderr, "error reading data bin\n");

    fclose(fp);
    return 0;
}
