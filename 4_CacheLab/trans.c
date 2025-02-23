/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/////////// helper functions /////////////////

// 5 local variables(include arguments)
void swap_row(int row1[], int start1, int row2[], int start2, int count)
{
    int temp;

    while (--count >= 0)
    {
        temp = row1[start1 + count];
        row1[start1 + count] = row2[start2 + count];
        row2[start2 + count] = temp;
    }
}

// 5 local variables(include arguments)
void copy_row(int src[], int src_begin, int dst[], int dst_begin, int count)
{
    while (--count >= 0)
    {
        dst[dst_begin + count] = src[src_begin + count];
    }
}

// 9 local variables(include arguments)
void internal_transpose(int i, int j, int size, int M, int N, int B[M][N])
{
    int temp, k, l;

    for (k = 0; k < size; ++k)
    {
        for (l = k + 1; l < size; ++l)
        {
            temp = B[i + k][j + l];
            B[i + k][j + l] = B[i + l][j + k];
            B[i + l][j + k] = temp;
        }
    }
}

// 9 local variables(include arguments)
void external_transpose(int i, int j, int size, int M, int N, int A[N][M], int B[M][N])
{
    int k, l;

    for (k = i; k < i + size; ++k)
    {
        for (l = j; l < j + size; ++l)
        {
            B[l][k] = A[k][l];
        }
    }
}

////////////////////////////////////////////

char transpose_32x32_desc[] = "Transpose with blocking n = 8";
void transpose_32x32(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k, l;

    for (i = 0; i < N; i += 8)
    {
        for (j = 0; j < M; j += 8)
        {
            for (k = i; k < i + 8; ++k)
            {
                for (l = j; l < j + 8; ++l)
                {
                    B[l][k] = A[k][l];
                }
            }
        }
    }
}

char transpose_32x32_diagonal_desc[] = "Transpose with blocking n = 8 then special process along the diagonal";
void transpose_32x32_diagonal(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i += 8)
    {
        for (j = 0; j < M; j += 8)
        {
            if (i == j)
            {
                continue;
            }

            external_transpose(i, j, 8, M, N, A, B);
        }
    }

    for (i = 0; i < N; i += 8)
    {
        // copy A to B in reverse order
        for (j = 0; j < 8; ++j)
        {
            copy_row(A[i + j], i, B[i + 7 - j], i, 8);
        }

        // then reverse again
        for (j = 0; j < 4; ++j)
        {
            swap_row(B[i + j], i, B[i + 7 - j], i, 8);
        }

        internal_transpose(i, i, 8, M, N, B);
    }
}

char transpose_64x64_desc[] = "Transpose with blocking n = 4";
void transpose_64x64(int M, int N, int A[N][M], int B[M][N])
{
    int n = 4;
    int i, j, k, l;

    for (i = 0; i < N; i += n)
    {
        for (j = 0; j < M; j += n)
        {
            for (k = i; k < i + n; ++k)
            {
                for (l = j; l < j + n; ++l)
                {
                    B[l][k] = A[k][l];
                }
            }
        }
    }
}

char transpose_64x64_diagonal_desc[] = "Transpose with blocking n = 4, then special process along the diagonal";
void transpose_64x64_diagonal(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i += 8)
    {
        for (j = 0; j < M; j += 8)
        {
            if (i == j)
            {
                continue;
            }

            // split to four 4x4 block(without cache conflict)
            // up-left
            external_transpose(i, j, 4, M, N, A, B);

            // up-right
            external_transpose(i, j + 4, 4, M, N, A, B);

            // down-right
            external_transpose(i + 4, j + 4, 4, M, N, A, B);

            // down-left
            external_transpose(i + 4, j, 4, M, N, A, B);
        }
    }

    // copy A to B
    // then split to four 4x4 block then transpose in place
    // eventually swap up-right and down-left blocks
    for (i = 0; i < N; i += 8)
    {
        // copy from A to B but shift one row(prevent cache jiltering)
        for (j = 0; j < 4; ++j)
        {
            copy_row(A[i + ((j + 1) & 0x3)], i, B[i + j], i, 8);
        }

        // bubble last row
        for (j = 3; j > 0; --j)
        {
            swap_row(B[i + j], i, B[i + j - 1], i, 8);
        }

        // up-left
        internal_transpose(i, i, 4, M, N, B);

        // up-right
        internal_transpose(i, i + 4, 4, M, N, B);
        swap_row(B[i], i + 4, B[i + 3], i + 4, 4);
        swap_row(B[i + 1], i + 4, B[i + 2], i + 4, 4);

        for (j = 0; j < 4; ++j)
        {
            copy_row(A[i + 4 + ((j + 1) & 0x3)], i, B[i + 4 + j], i, 8);
        }

        for (j = 3; j > 0; --j)
        {
            swap_row(B[i + 4 + j], i, B[i + 4 + j - 1], i, 8);
        }

        // down-left
        internal_transpose(i + 4, i, 4, M, N, B);
        swap_row(B[i + 4], i, B[i + 7], i, 4);
        swap_row(B[i + 5], i, B[i + 6], i, 4);

        // down-right
        internal_transpose(i + 4, i + 4, 4, M, N, B);

        // swap up-right block and down-left block
        swap_row(B[i], i + 4, B[i + 7], i, 4);
        swap_row(B[i + 1], i + 4, B[i + 6], i, 4);
        swap_row(B[i + 2], i + 4, B[i + 5], i, 4);
        swap_row(B[i + 3], i + 4, B[i + 4], i, 4);
    }
}

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 * evaluated by cache parameters s = 5, E = 1, b = 5
 * cache block: 32 bytes = 8 ints
 * cache size: 5 * 8 = 40 ints;
 */
void trans(int M, int N, int A[N][M], int B[M][N]);

char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 32 && N == 32)
    {
        transpose_32x32_diagonal(M, N, A, B);
        return;
    }

    if (M == 64 && N == 64)
    {
        transpose_64x64(M, N, A, B);
        return;
    }

    if (M == 61 && N == 67)
    {
        trans(M, N, A, B);
        return;
    }

    trans(M, N, A, B);
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    // registerTransFunction(trans, trans_desc);

    registerTransFunction(transpose_64x64_diagonal, transpose_64x64_diagonal_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; ++j)
        {
            if (A[i][j] != B[j][i])
            {
                return 0;
            }
        }
    }
    return 1;
}
