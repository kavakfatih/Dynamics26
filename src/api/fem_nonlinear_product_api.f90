module fem_nonlinear_product_api
    !! Beta.3 general nonlinear product C ABI.
    !!
    !! Bu entry point preset/demo geometri kurmaz. Caller tarafindan verilen SI
    !! node, HEX8 connectivity, constraint ve equivalent nodal load dizilerini
    !! model aggregate'ine aktarir; mevcut fem_nonlinear_solver Newton motorunu
    !! kullanir. ABI sinirinda yalniz bind(C)-guvenli scalar ve array'ler vardir.
    use, intrinsic :: iso_c_binding, only : c_int, c_double, c_int64_t
    use, intrinsic :: ieee_arithmetic, only : ieee_is_finite
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT
    use fem_topology, only : TOPOLOGY_HEX8
    use fem_element_registry, only : ELEMENT_TOTAL_LAGRANGIAN_HEX8
    use fem_linear_elastic_material, only : linear_elastic_material_t
    use fem_linear_solver, only : LINEAR_SOLVER_DENSE_REFERENCE
    use fem_nonlinear_solver, only : nonlinear_solver_options_t, &
        nonlinear_static_result_t, solve_nonlinear_static, &
        NONLINEAR_FULL_NEWTON, NONLINEAR_MODIFIED_NEWTON
    use fem_nonlinear_results, only : nonlinear_final_results_t, &
        recover_nonlinear_final_results
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, &
        FEM_STATUS_NUMERICAL_FAILURE
    implicit none
    private

    integer(c_int), parameter :: PRODUCT_API_VERSION = 1_c_int
    integer(c_int), parameter :: PRODUCT_DENSE_REFERENCE = 1_c_int

    public :: fem_solve_nonlinear_static_hex8_v1

contains

    integer(c_int) function fem_solve_nonlinear_static_hex8_v1( &
        api_version, node_count, node_ids, coordinates_xyz, element_count, &
        element_ids, connectivity8, young_modulus, poisson_ratio, &
        constraint_count, constraint_node_ids, constraint_components, &
        constraint_values, load_count, load_node_ids, load_components, load_values, &
        method, max_iterations, max_step_attempts, adaptive_stepping, &
        initial_increment, minimum_increment, maximum_increment, cutback_factor, &
        growth_factor, target_iterations, line_search_enabled, &
        line_search_max_iterations, line_search_reduction, line_search_min_alpha, &
        use_residual_criterion, use_displacement_criterion, &
        residual_relative_tolerance, residual_absolute_tolerance, &
        displacement_relative_tolerance, linear_backend, displacements_xyz, &
        reactions_xyz, element_equivalent_cauchy, converged, &
        completed_load_factor, final_residual_norm, minimum_j, accepted_steps, &
        step_attempts, total_iterations, cutbacks, history_capacity, history_count, &
        history_required_count, history_attempt, history_accepted_step_before, &
        history_iteration, history_load_factor, history_load_increment, &
        history_residual_norm, history_relative_residual, &
        history_displacement_increment_norm, history_relative_displacement, &
        history_alpha, history_minimum_j, history_converged) &
        bind(C, name="fem_solve_nonlinear_static_hex8_v1")

        integer(c_int), value, intent(in) :: api_version
        integer(c_int), value, intent(in) :: node_count, element_count
        integer(c_int64_t), intent(in) :: node_ids(*), element_ids(*)
        integer(c_int64_t), intent(in) :: connectivity8(*)
        real(c_double), intent(in) :: coordinates_xyz(*)
        real(c_double), value, intent(in) :: young_modulus, poisson_ratio
        integer(c_int), value, intent(in) :: constraint_count, load_count
        integer(c_int64_t), intent(in) :: constraint_node_ids(*), load_node_ids(*)
        integer(c_int), intent(in) :: constraint_components(*), load_components(*)
        real(c_double), intent(in) :: constraint_values(*), load_values(*)
        integer(c_int), value, intent(in) :: method, max_iterations
        integer(c_int), value, intent(in) :: max_step_attempts, adaptive_stepping
        real(c_double), value, intent(in) :: initial_increment, minimum_increment
        real(c_double), value, intent(in) :: maximum_increment, cutback_factor
        real(c_double), value, intent(in) :: growth_factor
        integer(c_int), value, intent(in) :: target_iterations
        integer(c_int), value, intent(in) :: line_search_enabled
        integer(c_int), value, intent(in) :: line_search_max_iterations
        real(c_double), value, intent(in) :: line_search_reduction
        real(c_double), value, intent(in) :: line_search_min_alpha
        integer(c_int), value, intent(in) :: use_residual_criterion
        integer(c_int), value, intent(in) :: use_displacement_criterion
        real(c_double), value, intent(in) :: residual_relative_tolerance
        real(c_double), value, intent(in) :: residual_absolute_tolerance
        real(c_double), value, intent(in) :: displacement_relative_tolerance
        integer(c_int), value, intent(in) :: linear_backend
        real(c_double), intent(out) :: displacements_xyz(*), reactions_xyz(*)
        real(c_double), intent(out) :: element_equivalent_cauchy(*)
        integer(c_int), intent(out) :: converged
        real(c_double), intent(out) :: completed_load_factor
        real(c_double), intent(out) :: final_residual_norm, minimum_j
        integer(c_int), intent(out) :: accepted_steps, step_attempts
        integer(c_int), intent(out) :: total_iterations, cutbacks
        integer(c_int), value, intent(in) :: history_capacity
        integer(c_int), intent(out) :: history_count, history_required_count
        integer(c_int), intent(out) :: history_attempt(*)
        integer(c_int), intent(out) :: history_accepted_step_before(*)
        integer(c_int), intent(out) :: history_iteration(*), history_converged(*)
        real(c_double), intent(out) :: history_load_factor(*)
        real(c_double), intent(out) :: history_load_increment(*)
        real(c_double), intent(out) :: history_residual_norm(*)
        real(c_double), intent(out) :: history_relative_residual(*)
        real(c_double), intent(out) :: history_displacement_increment_norm(*)
        real(c_double), intent(out) :: history_relative_displacement(*)
        real(c_double), intent(out) :: history_alpha(*), history_minimum_j(*)

        type(model_t) :: model
        type(linear_elastic_material_t) :: material
        type(nonlinear_solver_options_t) :: options
        type(nonlinear_static_result_t) :: solve_result
        type(nonlinear_final_results_t) :: final_results
        type(status_t) :: status
        integer(id_kind), allocatable :: connectivity(:)
        integer(id_kind) :: node_id, element_id, dof_id, constraint_id, load_id
        integer(index_kind) :: dof_pos
        real(rk) :: coordinates(3)
        integer :: i, e, a, component

        call initialize_scalar_outputs(converged, completed_load_factor, &
            final_residual_norm, minimum_j, accepted_steps, step_attempts, &
            total_iterations, cutbacks, history_count, history_required_count)
        fem_solve_nonlinear_static_hex8_v1 = FEM_STATUS_INVALID_ARGUMENT

        if (api_version /= PRODUCT_API_VERSION .or. node_count < 8_c_int .or. &
            element_count < 1_c_int .or. constraint_count < 1_c_int .or. &
            load_count < 1_c_int .or. history_capacity < 0_c_int) return
        if (young_modulus <= 0.0_c_double .or. poisson_ratio <= -1.0_c_double .or. &
            poisson_ratio >= 0.5_c_double .or. .not. ieee_is_finite(young_modulus) .or. &
            .not. ieee_is_finite(poisson_ratio)) return
        if (.not. ieee_is_finite(initial_increment) .or. &
            .not. ieee_is_finite(minimum_increment) .or. &
            .not. ieee_is_finite(maximum_increment) .or. &
            .not. ieee_is_finite(cutback_factor) .or. &
            .not. ieee_is_finite(growth_factor) .or. &
            .not. ieee_is_finite(line_search_reduction) .or. &
            .not. ieee_is_finite(line_search_min_alpha) .or. &
            .not. ieee_is_finite(residual_relative_tolerance) .or. &
            .not. ieee_is_finite(residual_absolute_tolerance) .or. &
            .not. ieee_is_finite(displacement_relative_tolerance)) return
        if (method /= int(NONLINEAR_FULL_NEWTON,c_int) .and. &
            method /= int(NONLINEAR_MODIFIED_NEWTON,c_int)) return
        if (linear_backend /= PRODUCT_DENSE_REFERENCE) return
        if (.not. binary_flag(adaptive_stepping) .or. &
            .not. binary_flag(line_search_enabled) .or. &
            .not. binary_flag(use_residual_criterion) .or. &
            .not. binary_flag(use_displacement_criterion)) return

        ! Gecerli boyutlar bilindikten sonra caller-owned output alanlari
        ! deterministik olarak sifirlanir. ABI bellek ownership'i caller'dadir.
        do i = 1, 3*int(node_count)
            displacements_xyz(i) = 0.0_c_double
            reactions_xyz(i) = 0.0_c_double
        end do
        do e = 1, int(element_count)
            element_equivalent_cauchy(e) = 0.0_c_double
        end do

        do i = 1, int(node_count)
            if (.not. ieee_is_finite(coordinates_xyz(3*i-2)) .or. &
                .not. ieee_is_finite(coordinates_xyz(3*i-1)) .or. &
                .not. ieee_is_finite(coordinates_xyz(3*i))) return
            node_id = int(node_ids(i),id_kind)
            coordinates = [real(coordinates_xyz(3*i-2),rk), &
                real(coordinates_xyz(3*i-1),rk), real(coordinates_xyz(3*i),rk)]
            call model%mesh%add_node(node_id, coordinates, status)
            if (.not. status%is_ok()) goto 990
        end do

        allocate(connectivity(8))
        do e = 1, int(element_count)
            element_id = int(element_ids(e),id_kind)
            do a = 1, 8
                connectivity(a) = int(connectivity8(8*(e-1)+a),id_kind)
            end do
            call model%mesh%add_element(element_id, TOPOLOGY_HEX8, connectivity, status)
            if (.not. status%is_ok()) goto 990
            call model%mesh%assign_element_formulation(element_id, &
                ELEMENT_TOTAL_LAGRANGIAN_HEX8, status)
            if (.not. status%is_ok()) goto 990
        end do

        ! Linear Elastic authoring parametreleri, finite-deformation elementinde
        ! acikca St. Venant-Kirchhoff reference constitutive response'tur.
        material = linear_elastic_material_t(id=1_id_kind, &
            name="Beta.3 Product Linear Elastic / StVK", &
            young_modulus=real(young_modulus,rk), &
            poisson_ratio=real(poisson_ratio,rk))
        call model%materials%add(material,status)
        if (.not. status%is_ok()) goto 990
        do e = 1, int(element_count)
            call model%mesh%assign_element_properties(int(element_ids(e),id_kind), &
                1_id_kind, -1_id_kind, status)
            if (.not. status%is_ok()) goto 990
        end do

        call model%initialize_standard_registries(status)
        if (.not. status%is_ok()) goto 990
        call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status)
        if (.not. status%is_ok()) goto 990

        do i = 1, int(constraint_count)
            component = int(constraint_components(i))
            if (component < 1 .or. component > 3 .or. &
                .not. ieee_is_finite(constraint_values(i))) return
            dof_pos = model%dofs%find_by_address( &
                int(constraint_node_ids(i),id_kind), FIELD_ID_DISPLACEMENT, component)
            if (dof_pos == 0_index_kind) return
            dof_id = model%dofs%dofs(dof_pos)%id
            call model%constraints%add(dof_id, real(constraint_values(i),rk), &
                constraint_id, status)
            if (.not. status%is_ok()) goto 990
        end do

        do i = 1, int(load_count)
            component = int(load_components(i))
            if (component < 1 .or. component > 3 .or. &
                .not. ieee_is_finite(load_values(i))) return
            dof_pos = model%dofs%find_by_address(int(load_node_ids(i),id_kind), &
                FIELD_ID_DISPLACEMENT, component)
            if (dof_pos == 0_index_kind) return
            dof_id = model%dofs%dofs(dof_pos)%id
            call model%loads%add(dof_id, real(load_values(i),rk), load_id, status)
            if (.not. status%is_ok()) goto 990
        end do

        options%method = int(method)
        options%max_iterations = int(max_iterations)
        options%max_step_attempts = int(max_step_attempts)
        options%adaptive_stepping = adaptive_stepping /= 0_c_int
        options%initial_load_increment = real(initial_increment,rk)
        options%minimum_load_increment = real(minimum_increment,rk)
        options%maximum_load_increment = real(maximum_increment,rk)
        options%cutback_factor = real(cutback_factor,rk)
        options%growth_factor = real(growth_factor,rk)
        options%target_iterations = int(target_iterations)
        options%line_search = line_search_enabled /= 0_c_int
        options%line_search_max_iterations = int(line_search_max_iterations)
        options%line_search_reduction = real(line_search_reduction,rk)
        options%line_search_min_alpha = real(line_search_min_alpha,rk)
        options%use_residual_criterion = use_residual_criterion /= 0_c_int
        options%use_displacement_criterion = use_displacement_criterion /= 0_c_int
        options%use_energy_criterion = .false.
        options%residual_relative_tolerance = real(residual_relative_tolerance,rk)
        options%residual_absolute_tolerance = real(residual_absolute_tolerance,rk)
        options%displacement_relative_tolerance = &
            real(displacement_relative_tolerance,rk)
        options%linear%backend = LINEAR_SOLVER_DENSE_REFERENCE
        call options%validate(status)
        if (.not. status%is_ok()) goto 990

        call solve_nonlinear_static(model, options, solve_result, status)
        call copy_summary_and_history(solve_result, history_capacity, converged, &
            completed_load_factor, final_residual_norm, minimum_j, accepted_steps, &
            step_attempts, total_iterations, cutbacks, history_count, &
            history_required_count, history_attempt, history_accepted_step_before, &
            history_iteration, history_load_factor, history_load_increment, &
            history_residual_norm, history_relative_residual, &
            history_displacement_increment_norm, history_relative_displacement, &
            history_alpha, history_minimum_j, history_converged)
        if (.not. status%is_ok()) goto 990
        if (.not. solve_result%converged .or. &
            abs(solve_result%completed_load_factor-1.0_rk) > 1.0e-10_rk) then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, &
                "Nonlinear product solve tam load factorunde converge olmadi.")
            goto 990
        end if

        call recover_nonlinear_final_results(model, solve_result%active_displacement, &
            solve_result%completed_load_factor, final_results, status)
        if (.not. status%is_ok()) goto 990
        do i = 1, int(node_count)
            do component = 1, 3
                displacements_xyz(3*i-3+component) = &
                    real(final_results%displacement_xyz(component,i),c_double)
                reactions_xyz(3*i-3+component) = &
                    real(final_results%reaction_xyz(component,i),c_double)
            end do
        end do
        do e = 1, int(element_count)
            element_equivalent_cauchy(e) = &
                real(final_results%element_equivalent_cauchy(e),c_double)
        end do

        fem_solve_nonlinear_static_hex8_v1 = 0_c_int
        return

990     continue
        if (status%code /= 0) then
            fem_solve_nonlinear_static_hex8_v1 = int(status%code,c_int)
        else
            fem_solve_nonlinear_static_hex8_v1 = FEM_STATUS_INVALID_ARGUMENT
        end if
    end function fem_solve_nonlinear_static_hex8_v1

    pure logical function binary_flag(value)
        integer(c_int), intent(in) :: value
        binary_flag = value == 0_c_int .or. value == 1_c_int
    end function binary_flag

    subroutine initialize_scalar_outputs(converged, completed_load_factor, &
        final_residual_norm, minimum_j, accepted_steps, step_attempts, &
        total_iterations, cutbacks, history_count, history_required_count)
        integer(c_int), intent(out) :: converged, accepted_steps, step_attempts
        integer(c_int), intent(out) :: total_iterations, cutbacks
        integer(c_int), intent(out) :: history_count, history_required_count
        real(c_double), intent(out) :: completed_load_factor
        real(c_double), intent(out) :: final_residual_norm, minimum_j
        converged = 0_c_int
        completed_load_factor = 0.0_c_double
        final_residual_norm = 0.0_c_double
        minimum_j = 0.0_c_double
        accepted_steps = 0_c_int
        step_attempts = 0_c_int
        total_iterations = 0_c_int
        cutbacks = 0_c_int
        history_count = 0_c_int
        history_required_count = 0_c_int
    end subroutine initialize_scalar_outputs

    subroutine copy_summary_and_history(result, history_capacity, converged, &
        completed_load_factor, final_residual_norm, minimum_j, accepted_steps, &
        step_attempts, total_iterations, cutbacks, history_count, &
        history_required_count, history_attempt, history_accepted_step_before, &
        history_iteration, history_load_factor, history_load_increment, &
        history_residual_norm, history_relative_residual, &
        history_displacement_increment_norm, history_relative_displacement, &
        history_alpha, history_minimum_j, history_converged)
        type(nonlinear_static_result_t), intent(in) :: result
        integer(c_int), value, intent(in) :: history_capacity
        integer(c_int), intent(out) :: converged, accepted_steps, step_attempts
        integer(c_int), intent(out) :: total_iterations, cutbacks
        integer(c_int), intent(out) :: history_count, history_required_count
        real(c_double), intent(out) :: completed_load_factor
        real(c_double), intent(out) :: final_residual_norm, minimum_j
        integer(c_int), intent(out) :: history_attempt(*)
        integer(c_int), intent(out) :: history_accepted_step_before(*)
        integer(c_int), intent(out) :: history_iteration(*), history_converged(*)
        real(c_double), intent(out) :: history_load_factor(*)
        real(c_double), intent(out) :: history_load_increment(*)
        real(c_double), intent(out) :: history_residual_norm(*)
        real(c_double), intent(out) :: history_relative_residual(*)
        real(c_double), intent(out) :: history_displacement_increment_norm(*)
        real(c_double), intent(out) :: history_relative_displacement(*)
        real(c_double), intent(out) :: history_alpha(*), history_minimum_j(*)
        integer :: i, required_count, ncopy

        converged = merge(1_c_int,0_c_int,result%converged)
        completed_load_factor = real(result%completed_load_factor,c_double)
        final_residual_norm = real(result%final_residual_norm,c_double)
        minimum_j = real(result%minimum_j,c_double)
        accepted_steps = int(result%accepted_steps,c_int)
        step_attempts = int(result%step_attempts,c_int)
        total_iterations = int(result%total_iterations,c_int)
        cutbacks = int(result%cutback_count,c_int)
        required_count = 0
        if (allocated(result%history)) required_count = size(result%history)
        ncopy = min(int(history_capacity),required_count)
        history_count = int(ncopy,c_int)
        history_required_count = int(required_count,c_int)
        do i = 1, ncopy
            history_attempt(i) = int(result%history(i)%attempt,c_int)
            history_accepted_step_before(i) = &
                int(result%history(i)%accepted_step_before,c_int)
            history_iteration(i) = int(result%history(i)%iteration,c_int)
            history_load_factor(i) = real(result%history(i)%load_factor,c_double)
            history_load_increment(i) = &
                real(result%history(i)%load_increment,c_double)
            history_residual_norm(i) = &
                real(result%history(i)%residual_norm,c_double)
            history_relative_residual(i) = &
                real(result%history(i)%relative_residual,c_double)
            history_displacement_increment_norm(i) = &
                real(result%history(i)%displacement_increment_norm,c_double)
            history_relative_displacement(i) = &
                real(result%history(i)%relative_displacement,c_double)
            history_alpha(i) = real(result%history(i)%line_search_alpha,c_double)
            history_minimum_j(i) = real(result%history(i)%minimum_j,c_double)
            history_converged(i) = &
                merge(1_c_int,0_c_int,result%history(i)%converged)
        end do
    end subroutine copy_summary_and_history

end module fem_nonlinear_product_api
