#include <stdio.h>
#include <stdlib.h>

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

struct Node
{
    unsigned val;
    struct Node *next;
};

typedef struct Node *NodePtr;

NodePtr HEAD;

void phase_6(char *input)
{
    // ...

    // <read_six_numbers>
    unsigned nums[6] = {1, 2, 3, 4, 5, 6};

    // from 0x401114 to 0x401153
    // check input numbers. each number <= 0x6 and no duplicated number
    // note asm used jbe instruction so nums is unsigned type, the value is between 0 and 0x6
    for (size_t i = 0, j = 0; i < 6; ++i)
    {
        if (nums[i] > 0x6)
        {
            // explode
            exit(1);
        }

        for (size_t j = i + 1; j < 6; ++j)
        {
            if (nums[j] == nums[i])
            {
                // explode
                exit(1);
            }
        }
    }

    // from 0x401153 to 0x40116d
    for (size_t i = 0; i < 6; ++i)
    {
        nums[i] = 0x7 - nums[i];
    }

    NodePtr nodes[6];

    // from 0x40116f to 0x4011ab
    // sort a global linked list using input numbers
    for (size_t i = 0; i < 0x6; ++i)
    {
        NodePtr node = HEAD;

        for (size_t j = 1; j < nums[i]; ++j)
        {
            node = node->next;
        }

        nodes[i] = node;
    }

    // from 0x4011ab to 0x4011d2
    // re-link linked list nodes
    NodePtr head = nodes[0];

    for (size_t i = 1; i < 0x6; ++i)
    {
        nodes[i - 1]->next = nodes[i];
    }

    nodes[0x5]->next = NULL;

    // from 0x4011da to 0x4011f7
    // check whether linked list is descending
    for (size_t i = 0; i < 0x5; ++i)
    {
        if (head->val < head->next->val)
        {
            // explode
            exit(1);
        }

        head = head->next;
    }
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

    for (unsigned i = 0; i <= 0xe; ++i)
    {
        printf("Phase 4: func4(%d, 0x0, 0xe) = %d\n", i, func4(i, 0x0, 0xe));
    }

    char phase_5[] = {
        0x66, 0x6c, 0x79, 0x65, 0x72, 0x73,

        '\0'};

    printf("Phase 5: %s\n", phase_5);

    char phase_5_map[] = {
        0x6d, 0x61, 0x64, 0x75, 0x69, 0x65, 0x72, 0x73, 0x6e, 0x66, 0x6f, 0x74, 0x76, 0x62, 0x79, 0x6c, 0x53, 0x6f, 0x20, 0x79, 0x6f, 0x75, 0x20, 0x74, 0x68, 0x69, 0x6e, 0x6b, 0x20, 0x79, 0x6f, 0x75, 0x20, 0x63, 0x61, 0x6e, 0x20, 0x73, 0x74, 0x6f, 0x70, 0x20, 0x74, 0x68, 0x65, 0x20, 0x62, 0x6f, 0x6d, 0x62, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20, 0x63, 0x74, 0x72, 0x6c, 0x2d, 0x63, 0x2c, 0x20, 0x64, 0x6f, 0x20, 0x79, 0x6f, 0x75, 0x3f, 0x00, 0x43, 0x75, 0x72, 0x73, 0x65, 0x73, 0x2c, 0x20, 0x79, 0x6f, 0x75, 0x27, 0x76, 0x65, 0x20, 0x66, 0x6f, 0x75, 0x6e, 0x64, 0x20, 0x74, 0x68, 0x65, 0x20, 0x73, 0x65, 0x63, 0x72, 0x65, 0x74, 0x20, 0x70, 0x68, 0x61, 0x73, 0x65, 0x21, 0x00, 0x00, 0x42, 0x75, 0x74, 0x20, 0x66, 0x69, 0x6e, 0x64, 0x69, 0x6e, 0x67, 0x20, 0x69, 0x74, 0x20, 0x61, 0x6e,

        '\0'};

    char phase_5_input[] = {
        0x69, 0x6f, 0x6e, 0x65, 0x66, 0x67,
        '\0'};

    printf("Phase 5 key: %s\n", phase_5_input);

    for (unsigned i = 0; i < 6; ++i)
    {
        printf("%c", phase_5_map[phase_5_input[i] & 0xf]);
    }

    printf("\n");

    // 0x4024f8
    char secret_phase_1[] = {
        0x43, 0x75, 0x72, 0x73, 0x65, 0x73, 0x2c, 0x20, 0x79, 0x6f, 0x75, 0x27, 0x76, 0x65, 0x20, 0x66, 0x6f, 0x75, 0x6e, 0x64, 0x20, 0x74, 0x68, 0x65, 0x20, 0x73, 0x65, 0x63, 0x72, 0x65, 0x74, 0x20, 0x70, 0x68, 0x61, 0x73, 0x65, 0x21, 0x00, '\0'};

    printf("secret phase: %s\n", secret_phase_1);

    // 0x402520
    char secret_phase_2[] = {
        0x42, 0x75, 0x74, 0x20, 0x66, 0x69, 0x6e, 0x64, 0x69, 0x6e, 0x67, 0x20, 0x69, 0x74, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x73, 0x6f, 0x6c, 0x76, 0x69, 0x6e, 0x67, 0x20, 0x69, 0x74, 0x20, 0x61, 0x72, 0x65, 0x20, 0x71, 0x75, 0x69, 0x74, 0x65, 0x20, 0x64, 0x69, 0x66, 0x66, 0x65, 0x72, 0x65, 0x6e, 0x74, 0x2e, 0x2e, 0x2e, '\0'};

    printf("secret phase: %s\n", secret_phase_2);

    // 0x402622
    char secret_phase_key[] = {0x44, 0x72, 0x45, 0x76, 0x69, 0x6c, '\0'};
    printf("secret phase key: %s\n", secret_phase_key);

    // 0x402619
    char secret_phase_format[] = {0x25, 0x64, 0x20, 0x25, 0x64, 0x20, 0x25, 0x73, '\0'};
    printf("secret phase input: %s\n", secret_phase_format);

    return 0;
}