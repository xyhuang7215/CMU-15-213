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

/* 
 * Helper function for submit code
 */
void trans_64x64(int M, int N, int A[N][M], int B[M][N]);
void trans_32x32(int M, int N, int A[N][M], int B[M][N]);
void trans_61x67(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{    
    if (M == 64 && N == 64)
        trans_64x64(M, N, A, B);
    else if (M == 32 && N == 32)
        trans_32x32(M, N, A, B);
    else
        trans_61x67(M, N, A, B);
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

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}


char trans_88_desc[] = "8x8 Block";
void trans_88(int M, int N, int A[N][M], int B[M][N])
{
    int bsize = 8;
    int iStart, jStart, k;
    int a1, a2, a3, a4, a5, a6, a7, a8;

    for (iStart = 0; iStart < N; iStart += bsize)
    {
        for (jStart = 0; jStart < M; jStart += bsize)
        {
            for (k = 0; k < bsize; k++)
            {
                a1 = A[iStart+k][jStart+0];
                a2 = A[iStart+k][jStart+1];
                a3 = A[iStart+k][jStart+2];
                a4 = A[iStart+k][jStart+3];
                a5 = A[iStart+k][jStart+4];
                a6 = A[iStart+k][jStart+5];
                a7 = A[iStart+k][jStart+6];
                a8 = A[iStart+k][jStart+7];

                B[jStart+0][iStart+k] = a1;   
                B[jStart+1][iStart+k] = a2;  
                B[jStart+2][iStart+k] = a3;  
                B[jStart+3][iStart+k] = a4;  
                B[jStart+4][iStart+k] = a5;  
                B[jStart+5][iStart+k] = a6;  
                B[jStart+6][iStart+k] = a7;  
                B[jStart+7][iStart+k] = a8;  
            }
        }
    }
}


char trans_44_desc[] = "4x4 Block";
void trans_44(int M, int N, int A[N][M], int B[M][N])
{
    int bsize = 4;
    int iStart, jStart, k;
    int a1, a2, a3, a4;

    for (iStart = 0; iStart < N; iStart += bsize)
    {
        for (jStart = 0; jStart < M; jStart += bsize)
        {
            for (k = 0; k < bsize; k++)
            {
                a1 = A[iStart+k][jStart+0];
                a2 = A[iStart+k][jStart+1];
                a3 = A[iStart+k][jStart+2];
                a4 = A[iStart+k][jStart+3];

                B[jStart+0][iStart+k] = a1;   
                B[jStart+1][iStart+k] = a2;  
                B[jStart+2][iStart+k] = a3;  
                B[jStart+3][iStart+k] = a4;  
            }
        }
    }
}

char trans_88to44_desc[] = "Split 8x8 Block into 4x4 (For 64x64 Matrix)";
void trans_88to44(int M, int N, int A[N][M], int B[M][N])
{
    int bsize = 8;
    int iStart, jStart, k;
    int a1, a2, a3, a4, a5, a6, a7, a8;

    for (iStart = 0; iStart < N; iStart += bsize)
    {
        for (jStart = 0; jStart < M; jStart += bsize)
        {
            // step 1 (8 miss)
            // for loop along A row
            for (k = 0; k < 4; k++)
            {
                a1 = A[iStart+k][jStart+0];
                a2 = A[iStart+k][jStart+1];
                a3 = A[iStart+k][jStart+2];
                a4 = A[iStart+k][jStart+3];
                a5 = A[iStart+k][jStart+4];
                a6 = A[iStart+k][jStart+5];
                a7 = A[iStart+k][jStart+6];
                a8 = A[iStart+k][jStart+7];

                B[jStart+0][iStart+k] = a1;   
                B[jStart+1][iStart+k] = a2;  
                B[jStart+2][iStart+k] = a3;  
                B[jStart+3][iStart+k] = a4;

                B[jStart+0][iStart+k+4] = a5;  
                B[jStart+1][iStart+k+4] = a6;  
                B[jStart+2][iStart+k+4] = a7;  
                B[jStart+3][iStart+k+4] = a8;
            }

            for (k = 0; k < 4; k++)
            {
                a1 = B[jStart+k][iStart+4];  
                a2 = B[jStart+k][iStart+5];
                a3 = B[jStart+k][iStart+6];  
                a4 = B[jStart+k][iStart+7];

                B[jStart+k][iStart+4] = A[iStart+4][jStart+k];  
                B[jStart+k][iStart+5] = A[iStart+5][jStart+k];
                B[jStart+k][iStart+6] = A[iStart+6][jStart+k];  
                B[jStart+k][iStart+7] = A[iStart+7][jStart+k];

                B[jStart+k+4][iStart+0] = a1;
                B[jStart+k+4][iStart+1] = a2;
                B[jStart+k+4][iStart+2] = a3;
                B[jStart+k+4][iStart+3] = a4;
            }

            for (k = 4; k < 8; k++)
            {
                B[jStart+k][iStart+4] = A[iStart+4][jStart+k];
                B[jStart+k][iStart+5] = A[iStart+5][jStart+k];
                B[jStart+k][iStart+6] = A[iStart+6][jStart+k];
                B[jStart+k][iStart+7] = A[iStart+7][jStart+k];
            }
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
    registerTransFunction(trans_88to44, trans_88to44_desc);
    registerTransFunction(trans_88, trans_88_desc);
    registerTransFunction(trans_44, trans_44_desc);
    
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}


void trans_64x64(int M, int N, int A[N][M], int B[M][N])
{
    int bsize = 8;
    int iStart, jStart, k;
    int a1, a2, a3, a4, a5, a6, a7, a8;

    // Handle the left top square matrix
    for (iStart = 0; iStart <= N - bsize; iStart += bsize)
    {
        for (jStart = 0; jStart <= M - bsize; jStart += bsize)
        {
            for (k = 0; k < 4; k++)
            {
                a1 = A[iStart+k][jStart+0];
                a2 = A[iStart+k][jStart+1];
                a3 = A[iStart+k][jStart+2];
                a4 = A[iStart+k][jStart+3];
                a5 = A[iStart+k][jStart+4];
                a6 = A[iStart+k][jStart+5];
                a7 = A[iStart+k][jStart+6];
                a8 = A[iStart+k][jStart+7];

                B[jStart+0][iStart+k] = a1;   
                B[jStart+1][iStart+k] = a2;  
                B[jStart+2][iStart+k] = a3;  
                B[jStart+3][iStart+k] = a4;

                B[jStart+0][iStart+k+4] = a5;  
                B[jStart+1][iStart+k+4] = a6;  
                B[jStart+2][iStart+k+4] = a7;  
                B[jStart+3][iStart+k+4] = a8;
            }

            for (k = 0; k < 4; k++)
            {
                a1 = B[jStart+k][iStart+4];  
                a2 = B[jStart+k][iStart+5];
                a3 = B[jStart+k][iStart+6];  
                a4 = B[jStart+k][iStart+7];

                B[jStart+k][iStart+4] = A[iStart+4][jStart+k];  
                B[jStart+k][iStart+5] = A[iStart+5][jStart+k];
                B[jStart+k][iStart+6] = A[iStart+6][jStart+k];  
                B[jStart+k][iStart+7] = A[iStart+7][jStart+k];

                B[jStart+k+4][iStart+0] = a1;
                B[jStart+k+4][iStart+1] = a2;
                B[jStart+k+4][iStart+2] = a3;
                B[jStart+k+4][iStart+3] = a4;
            }

            for (k = 4; k < 8; k++)
            {
                B[jStart+k][iStart+4] = A[iStart+4][jStart+k];
                B[jStart+k][iStart+5] = A[iStart+5][jStart+k];
                B[jStart+k][iStart+6] = A[iStart+6][jStart+k];
                B[jStart+k][iStart+7] = A[iStart+7][jStart+k];
            }
        }
    }
}


void trans_32x32(int M, int N, int A[N][M], int B[M][N])
{
    int bsize = 8;
    int iStart, jStart, k;
    int a1, a2, a3, a4, a5, a6, a7, a8;

    for (iStart = 0; iStart <= N - bsize; iStart += bsize)
    {
        for (jStart = 0; jStart <= M - bsize; jStart += bsize)
        {
            for (k = 0; k < bsize; k++)
            {
                a1 = A[iStart+k][jStart+0];
                a2 = A[iStart+k][jStart+1];
                a3 = A[iStart+k][jStart+2];
                a4 = A[iStart+k][jStart+3];
                a5 = A[iStart+k][jStart+4];
                a6 = A[iStart+k][jStart+5];
                a7 = A[iStart+k][jStart+6];
                a8 = A[iStart+k][jStart+7];

                B[jStart+0][iStart+k] = a1;   
                B[jStart+1][iStart+k] = a2;  
                B[jStart+2][iStart+k] = a3;  
                B[jStart+3][iStart+k] = a4;  
                B[jStart+4][iStart+k] = a5;  
                B[jStart+5][iStart+k] = a6;  
                B[jStart+6][iStart+k] = a7;  
                B[jStart+7][iStart+k] = a8;  
            }
        }
    }

    a1 = (N / bsize) * bsize;
    a2 = (M / bsize) * bsize;

    for (iStart = a1; iStart < N; ++iStart)
    {
        for (jStart = 0; jStart < a2; ++jStart)
        {
            B[jStart][iStart] = A[iStart][jStart];
        }
    }


    for (iStart = 0; iStart < N; ++iStart)
    {
        for (jStart = a2; jStart < M; ++jStart)
        {
            B[jStart][iStart] = A[iStart][jStart];
        }
    }
}

void trans_61x67(int M, int N, int A[N][M], int B[M][N])
{
    int bsize = 8;
    int iStart, jStart;
    int a1, a2, a3, a4, a5, a6, a7, a8;
    int m = (M / bsize) * bsize;

    for (jStart = 0; jStart < m; jStart += bsize)
    {
        for (iStart = 0; iStart < N; ++iStart)
        {
            a1 = A[iStart][jStart+0];
            a2 = A[iStart][jStart+1];
            a3 = A[iStart][jStart+2];
            a4 = A[iStart][jStart+3];
            a5 = A[iStart][jStart+4];
            a6 = A[iStart][jStart+5];
            a7 = A[iStart][jStart+6];
            a8 = A[iStart][jStart+7];

            B[jStart+0][iStart] = a1;   
            B[jStart+1][iStart] = a2;  
            B[jStart+2][iStart] = a3;  
            B[jStart+3][iStart] = a4;  
            B[jStart+4][iStart] = a5;  
            B[jStart+5][iStart] = a6;  
            B[jStart+6][iStart] = a7;  
            B[jStart+7][iStart] = a8;  
        }
    }

    for (iStart = 0; iStart < N; ++iStart)
    {
        for (jStart = m; jStart < M; ++jStart)
        {
            B[jStart][iStart] = A[iStart][jStart];
        }
    }
}