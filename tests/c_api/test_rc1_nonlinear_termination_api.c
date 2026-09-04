#include "femcae/femcae.h"

#include <assert.h>
#include <math.h>
#include <string.h>

enum { NODE_COUNT = 8, HISTORY_CAPACITY = 256 };

typedef struct termination_run_t {
    int status;
    int converged;
    double completed_load_factor;
    double last_attempted_load_factor;
    double last_load_increment;
    double residual;
    double minimum_j;
    int accepted_steps;
    int step_attempts;
    int iterations;
    int cutbacks;
    int phase;
    int reason;
} termination_run_t;

static const long long node_ids[NODE_COUNT] = {1, 2, 3, 4, 5, 6, 7, 8};
static const double coordinates[3 * NODE_COUNT] = {
    0.0, 0.0, 0.0,  1.0, 0.0, 0.0,
    1.0, 1.0, 0.0,  0.0, 1.0, 0.0,
    0.0, 0.0, 1.0,  1.0, 0.0, 1.0,
    1.0, 1.0, 1.0,  0.0, 1.0, 1.0
};
static const long long element_ids[1] = {11};
static const long long valid_connectivity[8] = {1, 2, 3, 4, 5, 6, 7, 8};
/* Yerel xi eksenini tersleyen permutation: det(dX/dxi) tum Gauss
 * noktalarinda negatiftir; test distorted ama pozitif bir eleman uretmez. */
static const long long inverted_connectivity[8] = {2, 1, 4, 3, 6, 5, 8, 7};
static const long long fixed_face_nodes[12] = {
    1, 1, 1, 4, 4, 4, 5, 5, 5, 8, 8, 8
};
static const int fixed_face_components[12] = {
    1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3
};
static const double fixed_face_values[12] = {0.0};
static const long long one_node_constraints[3] = {1, 1, 1};
static const int one_node_components[3] = {1, 2, 3};
static const double one_node_values[3] = {0.0, 0.0, 0.0};
static const long long load_nodes[4] = {2, 3, 6, 7};
static const int load_components[4] = {1, 1, 1, 1};

static termination_run_t run_case(const long long *connectivity,
                                  int constraint_count,
                                  const long long *constraint_nodes,
                                  const int *constraint_components,
                                  const double *constraint_values,
                                  double total_force,
                                  int max_iterations,
                                  int max_step_attempts,
                                  int adaptive,
                                  double initial_increment,
                                  double minimum_increment,
                                  int line_search,
                                  int line_search_iterations)
{
    termination_run_t run;
    double displacement[3 * NODE_COUNT] = {0.0};
    double reaction[3 * NODE_COUNT] = {0.0};
    double stress[1] = {0.0};
    double load_values[4];
    int history_count = 0;
    int history_required = 0;
    int history_attempt[HISTORY_CAPACITY] = {0};
    int history_accepted[HISTORY_CAPACITY] = {0};
    int history_iteration[HISTORY_CAPACITY] = {0};
    int history_converged[HISTORY_CAPACITY] = {0};
    double history_load[HISTORY_CAPACITY] = {0.0};
    double history_increment[HISTORY_CAPACITY] = {0.0};
    double history_residual[HISTORY_CAPACITY] = {0.0};
    double history_relative_residual[HISTORY_CAPACITY] = {0.0};
    double history_du[HISTORY_CAPACITY] = {0.0};
    double history_relative_du[HISTORY_CAPACITY] = {0.0};
    double history_alpha[HISTORY_CAPACITY] = {0.0};
    double history_minimum_j[HISTORY_CAPACITY] = {0.0};
    int i;

    memset(&run, 0, sizeof(run));
    for (i = 0; i < 4; ++i) {
        load_values[i] = total_force / 4.0;
    }

    run.status = fem_solve_nonlinear_static_hex8_v2(
        FEM_NONLINEAR_STATIC_HEX8_API_VERSION_V2,
        NODE_COUNT, node_ids, coordinates, 1, element_ids, connectivity,
        1.0e6, 0.30,
        constraint_count, constraint_nodes, constraint_components, constraint_values,
        4, load_nodes, load_components, load_values,
        FEM_NONLINEAR_METHOD_FULL_NEWTON, max_iterations, max_step_attempts,
        adaptive, initial_increment, minimum_increment, 1.0, 0.5, 1.5, 6,
        line_search, line_search_iterations, 0.5, 1.0e-4, 1, 1,
        1.0e-8, 1.0e-10, 1.0e-8, FEM_LINEAR_BACKEND_DENSE_REFERENCE,
        displacement, reaction, stress, &run.converged,
        &run.completed_load_factor, &run.residual, &run.minimum_j,
        &run.accepted_steps, &run.step_attempts, &run.iterations, &run.cutbacks,
        HISTORY_CAPACITY, &history_count, &history_required,
        history_attempt, history_accepted, history_iteration, history_load,
        history_increment, history_residual, history_relative_residual,
        history_du, history_relative_du, history_alpha, history_minimum_j,
        history_converged, &run.last_attempted_load_factor,
        &run.last_load_increment, &run.phase, &run.reason);
    return run;
}

int main(void)
{
    termination_run_t run;

    run = run_case(valid_connectivity, 12, fixed_face_nodes,
                   fixed_face_components, fixed_face_values,
                   1000.0, 30, 100, 1, 0.25, 1.0e-4, 1, 8);
    assert(run.status == 0);
    assert(run.converged == 1);
    assert(run.phase == FEM_NONLINEAR_PHASE_NONE);
    assert(run.reason == FEM_NONLINEAR_REASON_CONVERGED);
    assert(fabs(run.completed_load_factor - 1.0) < 1.0e-12);
    assert(fabs(run.last_attempted_load_factor - 1.0) < 1.0e-12);

    run = run_case(valid_connectivity, 12, fixed_face_nodes,
                   fixed_face_components, fixed_face_values,
                   1000.0, 0, 100, 1, 0.25, 1.0e-4, 1, 8);
    assert(run.status == 10);
    assert(run.phase == FEM_NONLINEAR_PHASE_INPUT_VALIDATION);
    assert(run.reason == FEM_NONLINEAR_REASON_INVALID_INPUT);

    run = run_case(valid_connectivity, 12, fixed_face_nodes,
                   fixed_face_components, fixed_face_values,
                   1.0e9, 1, 1, 1, 0.25, 1.0e-4, 0, 8);
    assert(run.status == 30);
    assert(run.phase == FEM_NONLINEAR_PHASE_LOAD_STEPPING);
    assert(run.reason == FEM_NONLINEAR_REASON_MAXIMUM_STEP_ATTEMPTS_REACHED);
    assert(run.step_attempts == 1);

    run = run_case(valid_connectivity, 12, fixed_face_nodes,
                   fixed_face_components, fixed_face_values,
                   1.0e9, 1, 100, 1, 0.25, 0.20, 0, 8);
    assert(run.status == 30);
    assert(run.phase == FEM_NONLINEAR_PHASE_LOAD_STEPPING);
    assert(run.reason == FEM_NONLINEAR_REASON_MINIMUM_INCREMENT_REACHED);
    assert(run.cutbacks == 1);

    run = run_case(valid_connectivity, 12, fixed_face_nodes,
                   fixed_face_components, fixed_face_values,
                   1.0e9, 1, 100, 0, 0.25, 1.0e-4, 0, 8);
    assert(run.status == 30);
    assert(run.phase == FEM_NONLINEAR_PHASE_NEWTON_ITERATION);
    assert(run.reason == FEM_NONLINEAR_REASON_NEWTON_ITERATION_LIMIT);
    assert(run.cutbacks == 0);

    run = run_case(valid_connectivity, 3, one_node_constraints,
                   one_node_components, one_node_values,
                   1000.0, 5, 100, 0, 0.25, 1.0e-4, 0, 8);
    assert(run.status == 30);
    assert(run.phase == FEM_NONLINEAR_PHASE_LINEAR_SOLVE);
    assert(run.reason == FEM_NONLINEAR_REASON_SINGULAR_OR_ILL_CONDITIONED_TANGENT);

    run = run_case(inverted_connectivity, 12, fixed_face_nodes,
                   fixed_face_components, fixed_face_values,
                   1000.0, 5, 100, 1, 0.25, 1.0e-4, 0, 8);
    assert(run.status == 30);
    assert(run.phase == FEM_NONLINEAR_PHASE_ELEMENT_KINEMATICS);
    assert(run.reason == FEM_NONLINEAR_REASON_INVALID_REFERENCE_JACOBIAN);

    run = run_case(valid_connectivity, 12, fixed_face_nodes,
                   fixed_face_components, fixed_face_values,
                   -1.0e12, 5, 100, 0, 1.0, 1.0e-4, 0, 8);
    assert(run.status == 30);
    assert(run.phase == FEM_NONLINEAR_PHASE_ELEMENT_KINEMATICS);
    assert(run.reason == FEM_NONLINEAR_REASON_INVALID_DEFORMATION_JACOBIAN);

    run = run_case(valid_connectivity, 12, fixed_face_nodes,
                   fixed_face_components, fixed_face_values,
                   -1.0e12, 5, 100, 0, 1.0, 1.0e-4, 1, 1);
    assert(run.status == 30);
    assert(run.phase == FEM_NONLINEAR_PHASE_LINE_SEARCH);
    assert(run.reason == FEM_NONLINEAR_REASON_LINE_SEARCH_FAILURE);

    return 0;
}
