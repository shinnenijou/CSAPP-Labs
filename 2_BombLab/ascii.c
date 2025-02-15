#include <stdio.h>

unsigned func4(unsigned a, unsigned b, unsigned c)
{
    int x = c;      // mov    %edx,%eax
    x -= b;         // sub    %esi,%eax
    unsigned y = x; // mov    %eax,%ecx
    y >>= 0x1f;     // shr    $0x1f,%ecx
    x += y;         // add    %ecx,%eax
    x >>= 1;        // sar    %eax
    y = x + b;      // lea    (%rax,%rsi,1),%ecx
    // y = ROUND_TO_ZERO((c + b) / 2)

    if (y <= a)     // cmp    %edi,%ecx
    {               // jle    400ff2
        if (y >= a) // cmp    %edi,%ecx
        {
            return 0; // mov    $0x0,%eax
        }
        else
        {
            b = y + 0x1;               // 0x1(%rcx),%esi
            int temp = func4(a, b, c); // call   400fce <func4>
            return temp + temp + 0x1;  // lea    0x1(%rax,%rax,1),%eax
        }
    }

    c = y - 0x1;               // -0x1(%rcx),%edx
    int temp = func4(a, b, c); // call   400fce <func4>
    return temp + temp;        // add    %eax,%eax
}

int main()
{
    char phase_1[] = {
        0x42, 0x6f, 0x72, 0x64, 0x65, 0x72, 0x20, 0x72, 0x65,
        0x6c, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x73, 0x20, 0x77,
        0x69, 0x74, 0x68, 0x20, 0x43, 0x61, 0x6e, 0x61, 0x64,
        0x61, 0x20, 0x68, 0x61, 0x76, 0x65, 0x20, 0x6e, 0x65,
        0x76, 0x65, 0x72, 0x20, 0x62, 0x65, 0x65, 0x6e, 0x20,
        0x62, 0x65, 0x74, 0x74, 0x65, 0x72, 0x2e,

        '\0'};

    printf("Phase 1: %s\n", phase_1);

    char phase_2[] = {
        0x25, 0x64, 0x20, 0x25, 0x64, 0x20, 0x25, 0x64, 0x20,
        0x25, 0x64, 0x20, 0x25, 0x64, 0x20, 0x25, 0x64,

        '\0'};

    printf("Phase 2: %s\n", phase_2);

    char phase_3[] = {
        0x25, 0x64, 0x20, 0x25, 0x64,

        '\0'};

    printf("Phase 3: %s\n", phase_3);

    for (unsigned i = 0; i < 0xe; ++i)
    {
        printf("Phase 4: func4(%d, 0x0, 0xe) = %d\n", i, func4(i, 0x0, 0xe));
    }

    return 0;
}