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

char transpose_32x32_desc[] = "Transpose with blocking n = 8";
void transpose_32x32(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, ki, kj;

    for (i = 0; i < N; i += 8)
    {
        for (j = 0; j < M; j += 8)
        {
            for (ki = i; ki < i + 8; ++ki)
            {
                for (kj = j; kj < j + 8; ++kj)
                {
                    B[kj][ki] = A[ki][kj];
                }
            }
        }
    }
}

char transpose_32x32_diagonal_desc[] = "Transpose with blocking n = 8 then special process along the diagonal";
void transpose_32x32_diagonal(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, ki, kj;
    int temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;

    for (i = 0; i < N; i += 8)
    {
        for (j = 0; j < M; j += 8)
        {
            if (i == j)
            {
                continue;
            }

            for (ki = i; ki < i + 8; ++ki)
            {
                for (kj = j; kj < j + 8; ++kj)
                {
                    B[kj][ki] = A[ki][kj];
                }
            }
        }
    }

    for (i = 0; i < N; i += 8)
    {
        for (ki = i; ki < i + 8; ++ki)
        {
            temp0 = A[ki][i];
            temp1 = A[ki][i + 1];
            temp2 = A[ki][i + 2];
            temp3 = A[ki][i + 3];
            temp4 = A[ki][i + 4];
            temp5 = A[ki][i + 5];
            temp6 = A[ki][i + 6];
            temp7 = A[ki][i + 7];

            B[i][ki] = temp0;
            B[i + 1][ki] = temp1;
            B[i + 2][ki] = temp2;
            B[i + 3][ki] = temp3;
            B[i + 4][ki] = temp4;
            B[i + 5][ki] = temp5;
            B[i + 6][ki] = temp6;
            B[i + 7][ki] = temp7;
        }
    }
}

char transpose_64x64_desc[] = "Transpose with blocking n = 4";
void transpose_64x64(int M, int N, int A[N][M], int B[M][N])
{
    int n = 4;
    int i, j, ki, kj;

    for (i = 0; i < N; i += n)
    {
        for (j = 0; j < M; j += n)
        {
            for (ki = i; ki < i + n; ++ki)
            {
                for (kj = j; kj < j + n; ++kj)
                {
                    B[kj][ki] = A[ki][kj];
                }
            }
        }
    }
}

char transpose_64x64_diagonal_desc[] = "Transpose with blocking n = 4, then special process along the diagonal";
void transpose_64x64_diagonal(int M, int N, int A[N][M], int B[M][N])
{
    int n = 4;

    int i = 0;
    int j = 0;
    int temp = 0;

    for (i = 0; i < N; i += n)
    {
        for (j = 0; j < M; j += n)
        {
            for (int ki = i; ki < i + n; ++ki)
            {
                for (int kj = j; kj < j + n; ++kj)
                {
                    temp = A[ki][kj];
                    B[kj][ki] = temp;
                }
            }
        }
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
        transpose_32x32(M, N, A, B);
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
    registerTransFunction(trans, trans_desc);

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
