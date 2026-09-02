module fem_nonlinear_diagnostics_api
    !! Beta.2 B2.5 advanced nonlinear diagnostics C ABI.
    !!
    !! Mevcut fem_demo_nonlinear_hex8 ABI'si geriye uyumluluk icin degistirilmez.
    !! Bu ek entry point ayni verification modelini gercek nonlinear solver ile
    !! cozer ve nonlinear_history_entry_t icindeki authoritative advanced subset'i
    !! disari tasir. Mixed u-p ve Contact bu preset'in parcasi olmadigi icin onlar
    !! icin sahte sifir telemetry uretilmez; availability application katmaninda
    !! Unavailable kalir.
    use, intrinsic :: iso_c_binding, only : c_int, c_double
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT
    use fem_topology, only : TOPOLOGY_HEX8
    use fem_element_registry, only : ELEMENT_TOTAL_LAGRANGIAN_HEX8
    use fem_linear_elastic_material, only : linear_elastic_material_t
    use fem_nonlinear_solver, only : nonlinear_solver_options_t, nonlinear_static_result_t, &
        solve_nonlinear_static
    use fem_linear_solver, only : LINEAR_SOLVER_DENSE_REFERENCE
    use fem_status, only : status_t
    implicit none
    private

    public :: fem_demo_nonlinear_hex8_diagnostics

contains

    integer(c_int) function fem_demo_nonlinear_hex8_diagnostics( &
        young_modulus, poisson_ratio, area, length, force, initial_increment, &
        minimum_increment, maximum_increment, method, line_search_enabled, &
        max_iterations, adaptive_stepping, tip_displacement, completed_load_factor, &
        final_residual_norm, minimum_j, accepted_steps, total_iterations, cutbacks, &
        history_capacity, history_count, history_attempt, history_accepted_step_before, &
        history_iteration, history_load_factor, history_load_increment, &
        history_residual_norm, history_relative_residual, &
        history_displacement_increment_norm, history_relative_displacement, &
        history_alpha, history_minimum_j, history_converged) &
        bind(C, name="fem_demo_nonlinear_hex8_diagnostics")

        real(c_double), value, intent(in) :: young_modulus, poisson_ratio
        real(c_double), value, intent(in) :: area, length, force
        real(c_double), value, intent(in) :: initial_increment, minimum_increment
        real(c_double), value, intent(in) :: maximum_increment
        integer(c_int), value, intent(in) :: method, line_search_enabled
        integer(c_int), value, intent(in) :: max_iterations, adaptive_stepping
        integer(c_int), value, intent(in) :: history_capacity

        real(c_double), intent(out) :: tip_displacement, completed_load_factor
        real(c_double), intent(out) :: final_residual_norm, minimum_j
        integer(c_int), intent(out) :: accepted_steps, total_iterations, cutbacks
        integer(c_int), intent(out) :: history_count
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
        type(nonlinear_static_result_t) :: result
        type(status_t) :: status
        integer(id_kind), parameter :: node_ids(8) = [81_id_kind, 7_id_kind, &
            42_id_kind, 5_id_kind, 900_id_kind, 11_id_kind, 3_id_kind, 77_id_kind]
        integer, parameter :: right_nodes(4) = [2, 3, 6, 7]
        integer, parameter :: left_nodes(4) = [1, 4, 5, 8]
        integer(id_kind) :: right_dofs(4), constraint_id, load_id, dof_id, eq
        integer(index_kind) :: pos
        real(rk) :: e_rk, nu_rk, a_rk, l_rk, f_rk, b, x(3,8), sum_u
        integer :: i, a, c, ncopy

        tip_displacement = 0.0_c_double
        completed_load_factor = 0.0_c_double
        final_residual_norm = 0.0_c_double
        minimum_j = 0.0_c_double
        accepted_steps = 0_c_int
        total_iterations = 0_c_int
        cutbacks = 0_c_int
        history_count = 0_c_int
        fem_demo_nonlinear_hex8_diagnostics = 10_c_int

        if (young_modulus <= 0.0_c_double .or. area <= 0.0_c_double .or. &
            length <= 0.0_c_double .or. poisson_ratio <= -1.0_c_double .or. &
            poisson_ratio >= 0.5_c_double .or. initial_increment <= 0.0_c_double .or. &
            initial_increment > 1.0_c_double .or. minimum_increment <= 0.0_c_double .or. &
            maximum_increment < minimum_increment .or. &
            initial_increment < minimum_increment .or. &
            initial_increment > maximum_increment .or. max_iterations < 1_c_int .or. &
            history_capacity < 0_c_int) return

        e_rk = real(young_modulus, rk)
        nu_rk = real(poisson_ratio, rk)
        a_rk = real(area, rk)
        l_rk = real(length, rk)
        f_rk = real(force, rk)
        b = sqrt(a_rk)

        x(:,1) = [0._rk, 0._rk, 0._rk]
        x(:,2) = [l_rk, 0._rk, 0._rk]
        x(:,3) = [l_rk, b, 0._rk]
        x(:,4) = [0._rk, b, 0._rk]
        x(:,5) = [0._rk, 0._rk, b]
        x(:,6) = [l_rk, 0._rk, b]
        x(:,7) = [l_rk, b, b]
        x(:,8) = [0._rk, b, b]

        do a = 1, 8
            call model%mesh%add_node(node_ids(a), x(:,a), status)
            if (.not. status%is_ok()) goto 920
        end do
        call model%mesh%add_element(600_id_kind, TOPOLOGY_HEX8, node_ids, status)
        if (.not. status%is_ok()) goto 920
        call model%mesh%assign_element_formulation( &
            600_id_kind, ELEMENT_TOTAL_LAGRANGIAN_HEX8, status)
        if (.not. status%is_ok()) goto 920

        material = linear_elastic_material_t( &
            id=9_id_kind, name="GUI Nonlinear Diagnostics StVK", &
            young_modulus=e_rk, poisson_ratio=nu_rk)
        call model%materials%add(material, status)
        if (.not. status%is_ok()) goto 920
        call model%mesh%assign_element_properties( &
            600_id_kind, 9_id_kind, -1_id_kind, status)
        if (.not. status%is_ok()) goto 920
        call model%initialize_standard_registries(status)
        if (.not. status%is_ok()) goto 920
        call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT, status)
        if (.not. status%is_ok()) goto 920

        do a = 1, 8
            do c = 2, 3
                pos = model%dofs%find_by_address( &
                    node_ids(a), FIELD_ID_DISPLACEMENT, c)
                dof_id = model%dofs%dofs(pos)%id
                call model%constraints%add(dof_id, 0._rk, constraint_id, status)
                if (.not. status%is_ok()) goto 920
            end do
        end do

        do i = 1, 4
            a = left_nodes(i)
            pos = model%dofs%find_by_address( &
                node_ids(a), FIELD_ID_DISPLACEMENT, 1)
            dof_id = model%dofs%dofs(pos)%id
            call model%constraints%add(dof_id, 0._rk, constraint_id, status)
            if (.not. status%is_ok()) goto 920
        end do

        do i = 1, 4
            a = right_nodes(i)
            pos = model%dofs%find_by_address( &
                node_ids(a), FIELD_ID_DISPLACEMENT, 1)
            dof_id = model%dofs%dofs(pos)%id
            right_dofs(i) = dof_id
            call model%loads%add(dof_id, f_rk / 4._rk, load_id, status)
            if (.not. status%is_ok()) goto 920
        end do

        options%method = int(method)
        options%line_search = line_search_enabled /= 0_c_int
        options%max_iterations = int(max_iterations)
        options%adaptive_stepping = adaptive_stepping /= 0_c_int
        options%initial_load_increment = real(initial_increment, rk)
        options%minimum_load_increment = real(minimum_increment, rk)
        options%maximum_load_increment = real(maximum_increment, rk)
        options%linear%backend = LINEAR_SOLVER_DENSE_REFERENCE

        call solve_nonlinear_static(model, options, result, status)
        if (.not. status%is_ok()) goto 920

        sum_u = 0._rk
        do i = 1, 4
            eq = model%numbering%equation_of(right_dofs(i))
            sum_u = sum_u + result%active_displacement(int(eq) + 1)
        end do

        tip_displacement = real(sum_u / 4._rk, c_double)
        completed_load_factor = real(result%completed_load_factor, c_double)
        final_residual_norm = real(result%final_residual_norm, c_double)
        minimum_j = real(result%minimum_j, c_double)
        accepted_steps = int(result%accepted_steps, c_int)
        total_iterations = int(result%total_iterations, c_int)
        cutbacks = int(result%cutback_count, c_int)

        if (allocated(result%history)) then
            ncopy = min(int(history_capacity), size(result%history))
            history_count = int(ncopy, c_int)
            do i = 1, ncopy
                history_attempt(i) = int(result%history(i)%attempt, c_int)
                history_accepted_step_before(i) = &
                    int(result%history(i)%accepted_step_before, c_int)
                history_iteration(i) = int(result%history(i)%iteration, c_int)
                history_load_factor(i) = real(result%history(i)%load_factor, c_double)
                history_load_increment(i) = &
                    real(result%history(i)%load_increment, c_double)
                history_residual_norm(i) = &
                    real(result%history(i)%residual_norm, c_double)
                history_relative_residual(i) = &
                    real(result%history(i)%relative_residual, c_double)
                history_displacement_increment_norm(i) = &
                    real(result%history(i)%displacement_increment_norm, c_double)
                history_relative_displacement(i) = &
                    real(result%history(i)%relative_displacement, c_double)
                history_alpha(i) = &
                    real(result%history(i)%line_search_alpha, c_double)
                history_minimum_j(i) = real(result%history(i)%minimum_j, c_double)
                if (result%history(i)%converged) then
                    history_converged(i) = 1_c_int
                else
                    history_converged(i) = 0_c_int
                end if
            end do
        end if

        fem_demo_nonlinear_hex8_diagnostics = 0_c_int
        return

920     continue
        fem_demo_nonlinear_hex8_diagnostics = int(status%code, c_int)
    end function fem_demo_nonlinear_hex8_diagnostics

end module fem_nonlinear_diagnostics_api
