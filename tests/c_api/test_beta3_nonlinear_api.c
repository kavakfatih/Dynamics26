#include "femcae/femcae.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>

enum { NODE_COUNT = 12, ELEMENT_COUNT = 2, HISTORY_CAPACITY = 512 };

typedef struct nonlinear_run_t {
    double displacement[3 * NODE_COUNT];
    double reaction[3 * NODE_COUNT];
    double stress[ELEMENT_COUNT];
    int converged;
    double load_factor;
    double residual;
    double minimum_j;
    int accepted_steps;
    int step_attempts;
    int iterations;
    int cutbacks;
    int history_count;
    int history_required;
    int history_attempt[HISTORY_CAPACITY];
    int history_accepted_before[HISTORY_CAPACITY];
    int history_iteration[HISTORY_CAPACITY];
    double history_load_factor[HISTORY_CAPACITY];
    double history_increment[HISTORY_CAPACITY];
    double history_residual[HISTORY_CAPACITY];
    double history_relative_residual[HISTORY_CAPACITY];
    double history_displacement_increment[HISTORY_CAPACITY];
    double history_relative_displacement[HISTORY_CAPACITY];
    double history_alpha[HISTORY_CAPACITY];
    double history_minimum_j[HISTORY_CAPACITY];
    int history_converged[HISTORY_CAPACITY];
} nonlinear_run_t;

static const long long node_ids[NODE_COUNT] = {
    101, 55, 700, 9, 42, 808, 3, 99, 502, 77, 12, 909
};

static const double coordinates[3 * NODE_COUNT] = {
    0.0, 0.0, 0.0,
    1.0, 0.0, 0.0,
    2.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
    1.0, 1.0, 0.0,
    2.0, 1.0, 0.0,
    0.0, 0.0, 1.0,
    1.0, 0.0, 1.0,
    2.0, 0.0, 1.0,
    0.0, 1.0, 1.0,
    1.0, 1.0, 1.0,
    2.0, 1.0, 1.0
};

static const long long element_ids[ELEMENT_COUNT] = {600, 17};
static const long long connectivity[8 * ELEMENT_COUNT] = {
    101, 55, 42, 9, 3, 99, 12, 77,
    55, 700, 808, 42, 99, 502, 909, 12
};

static const long long constraint_nodes[12] = {
    101, 101, 101, 9, 9, 9, 3, 3, 3, 77, 77, 77
};
static const int constraint_components[12] = {
    1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3
};
static const double constraint_values[12] = {
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0
};

static const long long load_nodes[4] = {700, 808, 502, 909};
static const int load_components[4] = {1, 1, 1, 1};
static const double load_values[4] = {250.0, 250.0, 250.0, 250.0};

static int solve_case(int method,
                      int line_search,
                      int history_capacity,
                      int linear_backend,
                      nonlinear_run_t *run)
{
    return fem_solve_nonlinear_static_hex8_v1(
        FEM_NONLINEAR_STATIC_HEX8_API_VERSION,
        NODE_COUNT,
        node_ids,
        coordinates,
        ELEMENT_COUNT,
        element_ids,
        connectivity,
        1.0e6,
        0.30,
        12,
        constraint_nodes,
        constraint_components,
        constraint_values,
        4,
        load_nodes,
        load_components,
        load_values,
        method,
        30,
        100,
        1,
        0.25,
        1.0e-4,
        0.50,
        0.50,
        1.50,
        6,
        line_search,
        8,
        0.50,
        1.0e-4,
        1,
        1,
        1.0e-8,
        1.0e-10,
        1.0e-8,
        linear_backend,
        run->displacement,
        run->reaction,
        run->stress,
        &run->converged,
        &run->load_factor,
        &run->residual,
        &run->minimum_j,
        &run->accepted_steps,
        &run->step_attempts,
        &run->iterations,
        &run->cutbacks,
        history_capacity,
        &run->history_count,
        &run->history_required,
        run->history_attempt,
        run->history_accepted_before,
        run->history_iteration,
        run->history_load_factor,
        run->history_increment,
        run->history_residual,
        run->history_relative_residual,
        run->history_displacement_increment,
        run->history_relative_displacement,
        run->history_alpha,
        run->history_minimum_j,
        run->history_converged);
}

static double average_loaded_face_x(const nonlinear_run_t *run)
{
    /* Input-order positions of IDs 700, 808, 502 and 909. */
    return 0.25 * (run->displacement[3 * 2]
                 + run->displacement[3 * 5]
                 + run->displacement[3 * 8]
                 + run->displacement[3 * 11]);
}

static void verify_success(const nonlinear_run_t *run)
{
    double rx = 0.0;
    double ry = 0.0;
    double rz = 0.0;
    int i;

    assert(run->converged == 1);
    assert(fabs(run->load_factor - 1.0) < 1.0e-12);
    assert(isfinite(run->residual));
    assert(isfinite(run->minimum_j) && run->minimum_j > 0.0);
    assert(run->accepted_steps > 0);
    assert(run->step_attempts >= run->accepted_steps);
    assert(run->iterations > 0);
    assert(run->history_count > 0);
    assert(run->history_required >= run->history_count);
    assert(run->history_converged[run->history_count - 1] == 1);
    assert(fabs(run->history_load_factor[run->history_count - 1] - 1.0) < 1.0e-12);
    assert(average_loaded_face_x(run) > 0.0);

    for (i = 0; i < 3 * NODE_COUNT; ++i) {
        assert(isfinite(run->displacement[i]));
        assert(isfinite(run->reaction[i]));
    }
    for (i = 0; i < ELEMENT_COUNT; ++i) {
        assert(isfinite(run->stress[i]));
        assert(run->stress[i] > 0.0);
    }
    for (i = 0; i < NODE_COUNT; ++i) {
        rx += run->reaction[3 * i];
        ry += run->reaction[3 * i + 1];
        rz += run->reaction[3 * i + 2];
    }
    assert(fabs(rx + 1000.0) < 1.0e-5);
    assert(fabs(ry) < 1.0e-5);
    assert(fabs(rz) < 1.0e-5);
}

int main(void)
{
    nonlinear_run_t full = {0};
    nonlinear_run_t modified = {0};
    nonlinear_run_t no_line_search = {0};
    nonlinear_run_t truncated = {0};
    nonlinear_run_t invalid = {0};
    double full_tip;

    assert(solve_case(FEM_NONLINEAR_METHOD_FULL_NEWTON, 1,
                      HISTORY_CAPACITY, FEM_LINEAR_BACKEND_DENSE_REFERENCE,
                      &full) == 0);
    verify_success(&full);
    assert(full.history_required == full.history_count);
    full_tip = average_loaded_face_x(&full);

    assert(solve_case(FEM_NONLINEAR_METHOD_MODIFIED_NEWTON, 1,
                      HISTORY_CAPACITY, FEM_LINEAR_BACKEND_DENSE_REFERENCE,
                      &modified) == 0);
    verify_success(&modified);
    assert(fabs(average_loaded_face_x(&modified) - full_tip)
           < 1.0e-8 * fmax(1.0, fabs(full_tip)));

    assert(solve_case(FEM_NONLINEAR_METHOD_FULL_NEWTON, 0,
                      HISTORY_CAPACITY, FEM_LINEAR_BACKEND_DENSE_REFERENCE,
                      &no_line_search) == 0);
    verify_success(&no_line_search);
    assert(fabs(average_loaded_face_x(&no_line_search) - full_tip)
           < 1.0e-8 * fmax(1.0, fabs(full_tip)));

    assert(solve_case(FEM_NONLINEAR_METHOD_FULL_NEWTON, 1, 1,
                      FEM_LINEAR_BACKEND_DENSE_REFERENCE, &truncated) == 0);
    assert(truncated.converged == 1);
    assert(truncated.history_count == 1);
    assert(truncated.history_required > truncated.history_count);

    assert(solve_case(FEM_NONLINEAR_METHOD_FULL_NEWTON, 1, 1, 99, &invalid) == 10);
    assert(invalid.converged == 0);
    return 0;
}
