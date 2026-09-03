#ifndef FEMCAE_H
#define FEMCAE_H

#ifdef __cplusplus
extern "C" {
#endif

int fem_api_version(void);
int fem_project_schema_version(void);
int fem_result_schema_version(void);
int fem_version_major(void);
int fem_version_minor(void);
int fem_version_patch(void);

/* V0.5: GUI smoke/demo path. Returns 0 on success. */
int fem_demo_axial_bar(double young_modulus,
                       double area,
                       double length,
                       double force,
                       double *tip_displacement,
                       double *axial_stress,
                       double *support_reaction);

/* V0.6: two-element assembled axial modal preset. Returns 0 on success. */
int fem_demo_axial_modal(double young_modulus,
                         double density,
                         double area,
                         double total_length,
                         double *frequency_1_hz,
                         double *frequency_2_hz,
                         double *mid_mode_1,
                         double *tip_mode_1,
                         double *mid_mode_2,
                         double *tip_mode_2);

/* V0.8: load-controlled Total-Lagrangian HEX8 nonlinear preset. Returns 0 on success. */
int fem_demo_nonlinear_hex8(double young_modulus,
                            double poisson_ratio,
                            double area,
                            double length,
                            double force,
                            double initial_increment,
                            double minimum_increment,
                            double maximum_increment,
                            int method,
                            int line_search_enabled,
                            int max_iterations,
                            int adaptive_stepping,
                            double *tip_displacement,
                            double *completed_load_factor,
                            double *final_residual_norm,
                            int *accepted_steps,
                            int *total_iterations,
                            int *cutbacks,
                            int history_capacity,
                            int *history_count,
                            int *history_attempt,
                            int *history_iteration,
                            double *history_load_factor,
                            double *history_relative_residual,
                            double *history_relative_displacement,
                            double *history_alpha,
                            int *history_converged);

/* Beta.2 B2.5: additive advanced diagnostics ABI for the same nonlinear
 * verification model. The V0.8 function above remains binary/source compatible. */
int fem_demo_nonlinear_hex8_diagnostics(
    double young_modulus,
    double poisson_ratio,
    double area,
    double length,
    double force,
    double initial_increment,
    double minimum_increment,
    double maximum_increment,
    int method,
    int line_search_enabled,
    int max_iterations,
    int adaptive_stepping,
    double *tip_displacement,
    double *completed_load_factor,
    double *final_residual_norm,
    double *minimum_j,
    int *accepted_steps,
    int *total_iterations,
    int *cutbacks,
    int history_capacity,
    int *history_count,
    int *history_attempt,
    int *history_accepted_step_before,
    int *history_iteration,
    double *history_load_factor,
    double *history_load_increment,
    double *history_residual_norm,
    double *history_relative_residual,
    double *history_displacement_increment_norm,
    double *history_relative_displacement,
    double *history_alpha,
    double *history_minimum_j,
    int *history_converged);

/* V0.10: mixed u-p HEX8/P0 manufactured simple-shear verification preset.
 * The requested shear gamma is converted to an internally equilibrated nodal
 * load pattern; returns the coupled Newton solution and element P0 pressure. */
int fem_demo_mixed_up_hex8_shear(double c10,
                                 double bulk_modulus,
                                 double shear_gamma,
                                 double *recovered_shear_gamma,
                                 double *element_pressure,
                                 double *completed_load_factor,
                                 double *pressure_residual_norm,
                                 int *total_iterations);

/* Beta.2 B2.5 additive mixed u-p diagnostics. Legacy V0.10 signature is unchanged. */
int fem_demo_mixed_up_hex8_shear_diagnostics(
    double c10,
    double bulk_modulus,
    double shear_gamma,
    double *recovered_shear_gamma,
    double *element_pressure,
    double *completed_load_factor,
    double *final_residual_norm,
    double *final_pressure_residual_norm,
    double *minimum_j,
    int *accepted_steps,
    int *total_iterations,
    int *cutbacks,
    int history_capacity,
    int *history_count,
    int *history_attempt,
    int *history_accepted_step_before,
    int *history_iteration,
    double *history_load_factor,
    double *history_load_increment,
    double *history_residual_norm,
    double *history_relative_residual,
    double *history_displacement_increment_norm,
    double *history_relative_displacement,
    double *history_pressure_residual_norm,
    double *history_relative_pressure_residual,
    double *history_pressure_increment_norm,
    double *history_alpha,
    double *history_minimum_j,
    int *history_converged);

/* V0.11: rigid-master frictionless contact verification preset.
 * enforcement: 1=penalty, 2=augmented Lagrangian. */
int fem_demo_contact_hex8(double young_modulus,
                          double poisson_ratio,
                          double normal_penalty,
                          double total_force,
                          int enforcement,
                          double *maximum_penetration,
                          double *total_normal_force,
                          int *active_contacts,
                          int *total_iterations);

/* Beta.2 B2.5 additive contact diagnostics for the real V0.11 verification
 * consumer. This does not imply general document-model Contact solve support. */
int fem_demo_contact_hex8_diagnostics(
    double young_modulus,
    double poisson_ratio,
    double normal_penalty,
    double total_force,
    int enforcement,
    double *maximum_penetration,
    double *total_normal_force,
    int *active_contacts,
    int *stick_contacts,
    int *slip_contacts,
    double *completed_load_factor,
    double *final_residual_norm,
    double *minimum_j,
    int *accepted_steps,
    int *total_iterations,
    int *cutbacks,
    int history_capacity,
    int *history_count,
    int *history_attempt,
    int *history_accepted_step_before,
    int *history_iteration,
    double *history_load_factor,
    double *history_load_increment,
    double *history_residual_norm,
    double *history_relative_residual,
    double *history_displacement_increment_norm,
    double *history_relative_displacement,
    double *history_alpha,
    double *history_minimum_j,
    int *history_active_contacts,
    int *history_stick_contacts,
    int *history_slip_contacts,
    double *history_maximum_penetration,
    int *history_converged);

/* V0.9: hyperelastic Material Studio C ABI. Model IDs: 1=Neo-Hookean,
 * 2=Mooney-Rivlin, 3=Yeoh, 4=Ogden. Parameter arrays use Pa for stress-like
 * coefficients. Ogden parameters are [mu1,alpha1,mu2,alpha2,...]. */
int fem_hyperelastic_validate(int model,
                              double bulk_modulus,
                              int parameter_count,
                              const double *parameters,
                              double *initial_shear_modulus);

int fem_hyperelastic_isochoric_uniaxial_preview(int model,
                                                 double bulk_modulus,
                                                 int parameter_count,
                                                 const double *parameters,
                                                 double stretch,
                                                 double *nominal_stress,
                                                 double *strain_energy);

/* V1.0: arbitrary linear HEX8 mesh solve. Components are 1=x, 2=y, 3=z.
 * Connectivity is element-major: 8 node IDs per element. Results are returned
 * in the same node/element order supplied by the caller. */
int fem_solve_linear_hex8_mesh(int node_count,
                               const long long *node_ids,
                               const double *coordinates_xyz,
                               int element_count,
                               const long long *element_ids,
                               const long long *connectivity8,
                               double young_modulus,
                               double poisson_ratio,
                               int constraint_count,
                               const long long *constraint_node_ids,
                               const int *constraint_components,
                               const double *constraint_values,
                               int load_count,
                               const long long *load_node_ids,
                               const int *load_components,
                               const double *load_values,
                               double *displacements_xyz,
                               double *reactions_xyz,
                               double *element_von_mises);

/* Beta.3: versioned general nonlinear static Total-Lagrangian HEX8 solve.
 *
 * This is an additive product API: no verification geometry or hidden load is
 * created. All coordinates/displacements are [m], forces/reactions [N], Young's
 * modulus and element_equivalent_cauchy [Pa]. Connectivity stores Node IDs and
 * results preserve the caller's node/element order. The material authoring
 * contract is Linear Elastic; its finite-deformation reference response is
 * St. Venant-Kirchhoff (not a hyperelastic material card).
 *
 * Memory remains owned by the caller. Every output array must have the documented
 * node/element/history capacity. history_count is the number written;
 * history_required_count reports the complete solver history length, so a short
 * history buffer is explicit and does not make an otherwise successful solve fail.
 * Status codes: 0=success, 10=invalid argument, 20=size mismatch,
 * 30=numerical failure, 40=not initialized. Beta.3 accepts dense reference
 * backend only; unsupported backend IDs are rejected without fallback. */
enum {
    FEM_NONLINEAR_STATIC_HEX8_API_VERSION = 1,
    FEM_NONLINEAR_METHOD_FULL_NEWTON = 1,
    FEM_NONLINEAR_METHOD_MODIFIED_NEWTON = 2,
    FEM_LINEAR_BACKEND_DENSE_REFERENCE = 1
};

int fem_solve_nonlinear_static_hex8_v1(
    int api_version,
    int node_count,
    const long long *node_ids,
    const double *coordinates_xyz,
    int element_count,
    const long long *element_ids,
    const long long *connectivity8,
    double young_modulus,
    double poisson_ratio,
    int constraint_count,
    const long long *constraint_node_ids,
    const int *constraint_components,
    const double *constraint_values,
    int load_count,
    const long long *load_node_ids,
    const int *load_components,
    const double *load_values,
    int method,
    int max_iterations,
    int max_step_attempts,
    int adaptive_stepping,
    double initial_increment,
    double minimum_increment,
    double maximum_increment,
    double cutback_factor,
    double growth_factor,
    int target_iterations,
    int line_search_enabled,
    int line_search_max_iterations,
    double line_search_reduction,
    double line_search_min_alpha,
    int use_residual_criterion,
    int use_displacement_criterion,
    double residual_relative_tolerance,
    double residual_absolute_tolerance,
    double displacement_relative_tolerance,
    int linear_backend,
    double *displacements_xyz,
    double *reactions_xyz,
    double *element_equivalent_cauchy,
    int *converged,
    double *completed_load_factor,
    double *final_residual_norm,
    double *minimum_j,
    int *accepted_steps,
    int *step_attempts,
    int *total_iterations,
    int *cutbacks,
    int history_capacity,
    int *history_count,
    int *history_required_count,
    int *history_attempt,
    int *history_accepted_step_before,
    int *history_iteration,
    double *history_load_factor,
    double *history_load_increment,
    double *history_residual_norm,
    double *history_relative_residual,
    double *history_displacement_increment_norm,
    double *history_relative_displacement,
    double *history_alpha,
    double *history_minimum_j,
    int *history_converged);

#ifdef __cplusplus
}
#endif

#endif /* FEMCAE_H */
