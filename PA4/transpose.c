/* Parallel matrix transpose, 1-D row-block decomposition.
 * usage: mpirun -n p ./transpose n      (n must be divisible by p)
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define TILE 32

int main(int argc, char *argv[])
{
    int p, rank, n;
    size_t N, nl, blk, j, r, c, rb, cb, rmax, cmax;
    double *a, *sendbuf, *recvbuf;
    double t0, tpack, tcomm, tunpack;
    double mpack, mcomm, munpack;
    long errs = 0, toterrs = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc != 2) {
        if (rank == 0) fprintf(stderr, "usage: %s n\n", argv[0]);
        MPI_Finalize();
        return 1;
    }
    n = atoi(argv[1]);
    if (n <= 0 || n % p != 0) {
        if (rank == 0) fprintf(stderr, "n must be positive and divisible by p\n");
        MPI_Finalize();
        return 1;
    }
    N = (size_t)n;
    nl = N / (size_t)p;           /* rows per process */
    blk = nl * nl;                /* words per block exchanged */
    if (blk > (size_t)INT_MAX) {
        if (rank == 0) fprintf(stderr, "block too large for an int count\n");
        MPI_Finalize();
        return 1;
    }

    a       = (double *) malloc(nl * N * sizeof(double));
    sendbuf = (double *) malloc(nl * N * sizeof(double));
    recvbuf = (double *) malloc(nl * N * sizeof(double));
    if (!a || !sendbuf || !recvbuf) {
        fprintf(stderr, "rank %d: out of memory\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* each process initializes its own rows: A[i][k] = i*n + k */
    for (r = 0; r < nl; r++)
        for (c = 0; c < N; c++)
            a[r*N + c] = (double)(((size_t)rank*nl + r) * N + c);

    /* Phase 1: local transpose of each nl x nl block into sendbuf */
    MPI_Barrier(MPI_COMM_WORLD);
    t0 = MPI_Wtime();
    for (j = 0; j < (size_t)p; j++) {
        double *dst = sendbuf + j*blk;
        for (rb = 0; rb < nl; rb += TILE) {
            rmax = (rb + TILE < nl) ? rb + TILE : nl;
            for (cb = 0; cb < nl; cb += TILE) {
                cmax = (cb + TILE < nl) ? cb + TILE : nl;
                for (r = rb; r < rmax; r++)
                    for (c = cb; c < cmax; c++)
                        dst[c*nl + r] = a[r*N + j*nl + c];
            }
        }
    }
    tpack = MPI_Wtime() - t0;

    /* Phase 2: all-to-all personalized exchange */
    MPI_Barrier(MPI_COMM_WORLD);
    t0 = MPI_Wtime();
    MPI_Alltoall(sendbuf, (int)blk, MPI_DOUBLE,
                 recvbuf, (int)blk, MPI_DOUBLE, MPI_COMM_WORLD);
    tcomm = MPI_Wtime() - t0;

    /* Phase 3: unpack received blocks into the row layout of A^T */
    MPI_Barrier(MPI_COMM_WORLD);
    t0 = MPI_Wtime();
    for (j = 0; j < (size_t)p; j++)
        for (c = 0; c < nl; c++)
            memcpy(&a[c*N + j*nl], &recvbuf[j*blk + c*nl], nl * sizeof(double));
    tunpack = MPI_Wtime() - t0;

    /* verify: row (rank*nl + c) of A^T, column k must equal A[k][rank*nl + c] */
    for (c = 0; c < nl; c++)
        for (j = 0; j < N; j++)
            if (a[c*N + j] != (double)(j*N + (size_t)rank*nl + c))
                errs++;

    MPI_Reduce(&tpack,   &mpack,   1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&tcomm,   &mcomm,   1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&tunpack, &munpack, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&errs,    &toterrs, 1, MPI_LONG,   MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0)
        printf("RESULT p=%d n=%d pack=%.6f comm=%.6f unpack=%.6f total=%.6f errors=%ld\n",
               p, n, mpack, mcomm, munpack, mpack + mcomm + munpack, toterrs);

    free(a); free(sendbuf); free(recvbuf);
    MPI_Finalize();
    return 0;
}
