#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <Accelerate/Accelerate.h>
#include <stdio.h>
#include <sys/sysctl.h>

/*
 * Accelerate Sparse'in varsayilan error handler'i bazi factor/solve hatalarinda
 * prosesi sonlandirabilir. FEMCAE bir kutuphane oldugu icin bu davranis kabul
 * edilmez. reportError callback'i thread-local bir bayraga cevrilir ve hata
 * Fortran status katmanina integer return code olarak tasinir.
 */
static _Thread_local int fem_accelerate_error_seen = 0;

static void fem_accelerate_report_error(const char *message) {
    (void)message;
    fem_accelerate_error_seen = 1;
}

/*
 * SparseFactorizationLU macOS 15.5 ile kullanilabilir. Final FEMCAE dylib'i
 * gfortran ile link edildigi icin __builtin_available kullanmak AppleClang'in
 * ___isPlatformVersionAtLeast runtime sembolunu ekler ve link'i bozar. Bu
 * nedenle product version compiler-runtime bagimliligi olmadan sysctl ile
 * okunur. Parse edilemeyen sistem guvenli tarafta kalir ve backend'i kapatir.
 */
static int fem_accelerate_sparse_lu_runtime_available(void) {
    char version[64] = {0};
    size_t size = sizeof(version);
    int major = 0;
    int minor = 0;

    if (sysctlbyname("kern.osproductversion", version, &size, NULL, 0) != 0) {
        return 0;
    }
    if (sscanf(version, "%d.%d", &major, &minor) < 2) {
        return 0;
    }
    return major > 15 || (major == 15 && minor >= 5);
}
#endif

int fem_accelerate_sparse_available(void) {
#ifdef __APPLE__
    return fem_accelerate_sparse_lu_runtime_available();
#else
    return 0;
#endif
}

int fem_accelerate_sparse_solve(int32_t n,
                                int64_t nnz,
                                const int64_t *row_ptr,
                                const int64_t *col_ind,
                                const double *values,
                                const double *rhs,
                                double *solution) {
#ifndef __APPLE__
    (void)n; (void)nnz; (void)row_ptr; (void)col_ind;
    (void)values; (void)rhs; (void)solution;
    return -100;
#else
    int *rows = NULL;
    int *cols = NULL;
    int64_t cursor = 0;
    int32_t row;
    SparseAttributes_t attributes = {0};
    SparseMatrix_Double matrix;
    SparseOpaqueSymbolicFactorization symbolic;
    SparseOpaqueFactorization_Double factorization;
    SparseSymbolicFactorOptions sfoptions = {0};
    DenseVector_Double xb;

    if (n <= 0 || nnz <= 0 || !row_ptr || !col_ind || !values || !rhs || !solution) {
        return -1;
    }
    if (!fem_accelerate_sparse_lu_runtime_available()) {
        return -101;
    }
    rows = (int *)malloc((size_t)nnz * sizeof(int));
    cols = (int *)malloc((size_t)nnz * sizeof(int));
    if (!rows || !cols) {
        free(rows); free(cols);
        return -2;
    }

    /* Fortran CSR row_ptr 1-tabanli offset, col_ind ise 0-tabanli equation ID'dir. */
    for (row = 0; row < n; ++row) {
        int64_t begin = row_ptr[row] - 1;
        int64_t end = row_ptr[row + 1] - 1;
        int64_t k;
        if (begin < 0 || end < begin || end > nnz) {
            free(rows); free(cols);
            return -3;
        }
        for (k = begin; k < end; ++k) {
            if (col_ind[k] < 0 || col_ind[k] >= n) {
                free(rows); free(cols);
                return -4;
            }
            rows[cursor] = row;
            cols[cursor] = (int)col_ind[k];
            ++cursor;
        }
    }
    if (cursor != nnz) {
        free(rows); free(cols);
        return -5;
    }

    attributes.kind = SparseOrdinary;
    matrix = SparseConvertFromCoordinate(n, n, (long)nnz, 1, attributes,
                                         rows, cols, values);

    /*
     * reportError, Apple dokumantasyonunun onerdiği sekilde ilk symbolic
     * factorization cagrisinda verilir. Sonraki numeric factor ve solve ayni
     * symbolic nesnenin error policy'sini kullanir.
     */
    fem_accelerate_error_seen = 0;
    sfoptions.control = SparseDefaultControl;
    sfoptions.orderMethod = SparseOrderDefault;
    sfoptions.order = NULL;
    sfoptions.ignoreRowsAndColumns = NULL;
    sfoptions.malloc = malloc;
    sfoptions.free = free;
    sfoptions.reportError = fem_accelerate_report_error;

    /*
     * Apple SDK'da SparseFactorizationLU'nun raw degeri 80'dir. Sabitin kendisi
     * macOS 15.5 availability annotation'i tasidigi icin 15.0 deployment
     * target'inda dogrudan referans vermek compiler warning'i uretir. Deger
     * yalniz yukaridaki runtime 15.5+ gate'inden sonra SparseFactorization_t'ye
     * cevrilir; 15.0-15.4 sistemlerinde bu kod yoluna girilmez.
     */
    {
        const SparseFactorization_t lu_factorization = (SparseFactorization_t)80;
        symbolic = SparseFactor(lu_factorization, matrix.structure, sfoptions);
    }
    if (symbolic.status < 0 || fem_accelerate_error_seen) {
        SparseCleanup(symbolic);
        SparseCleanup(matrix);
        free(rows); free(cols);
        return -9;
    }

    factorization = SparseFactor(symbolic, matrix);
    if (factorization.status < 0 || fem_accelerate_error_seen) {
        SparseCleanup(factorization);
        SparseCleanup(symbolic);
        SparseCleanup(matrix);
        free(rows); free(cols);
        return -10;
    }

    memcpy(solution, rhs, (size_t)n * sizeof(double));
    xb.count = n;
    xb.data = solution;
    SparseSolve(factorization, xb);
    if (fem_accelerate_error_seen) {
        SparseCleanup(factorization);
        SparseCleanup(symbolic);
        SparseCleanup(matrix);
        free(rows); free(cols);
        return -11;
    }

    SparseCleanup(factorization);
    SparseCleanup(symbolic);
    SparseCleanup(matrix);
    free(rows); free(cols);
    return 0;
#endif
}
