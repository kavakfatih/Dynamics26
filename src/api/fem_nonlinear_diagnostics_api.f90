module fem_nonlinear_diagnostics_api
    !! Beta.2 B2.5 advanced solver diagnostics C ABI.
    !!
    !! Mevcut V0.8/V0.10/V0.11 C ABI fonksiyonlari geriye uyumluluk icin
    !! degistirilmez. Bu additive entry point'ler ayni gercek verification
    !! modellerini solve_nonlinear_static ile cozer ve nonlinear_history_entry_t
    !! icindeki authoritative telemetry'yi disari tasir. Bir preset'in uretmedigi
    !! mixed/contact metrikleri application katmaninda Unavailable kalir.
    use, intrinsic :: iso_c_binding, only : c_int, c_double
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT, FIELD_ID_PRESSURE_P0
    use fem_topology, only : TOPOLOGY_HEX8
    use fem_element_registry, only : ELEMENT_TOTAL_LAGRANGIAN_HEX8, &
        ELEMENT_MIXED_UP_HEX8_P0
    use fem_linear_elastic_material, only : linear_elastic_material_t
    use fem_hyperelastic_material, only : hyperelastic_material_t, HYPER_NEO_HOOKEAN
    use fem_nonlinear_solver, only : nonlinear_solver_options_t, &
        nonlinear_static_result_t, solve_nonlinear_static
    use fem_nonlinear_assembly, only : nonlinear_system_t, evaluate_nonlinear_system
    use fem_mixed_results, only : element_pressure_results_t, recover_mixed_p0_pressure
    use fem_linear_solver, only : LINEAR_SOLVER_DENSE_REFERENCE
    use fem_contact_types, only : contact_pair_t, contact_facet_t, &
        CONTACT_ENFORCEMENT_PENALTY, CONTACT_ENFORCEMENT_AUGMENTED_LAGRANGIAN, &
        CONTACT_FRICTIONLESS
    use fem_status, only : status_t
    implicit none
    private

    public :: fem_demo_nonlinear_hex8_diagnostics
    public :: fem_demo_mixed_up_hex8_shear_diagnostics
    public :: fem_demo_contact_hex8_diagnostics

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
                pos = model%dofs%find_by_address(node_ids(a), FIELD_ID_DISPLACEMENT, c)
                dof_id = model%dofs%dofs(pos)%id
                call model%constraints%add(dof_id, 0._rk, constraint_id, status)
                if (.not. status%is_ok()) goto 920
            end do
        end do

        do i = 1, 4
            a = left_nodes(i)
            pos = model%dofs%find_by_address(node_ids(a), FIELD_ID_DISPLACEMENT, 1)
            dof_id = model%dofs%dofs(pos)%id
            call model%constraints%add(dof_id, 0._rk, constraint_id, status)
            if (.not. status%is_ok()) goto 920
        end do

        do i = 1, 4
            a = right_nodes(i)
            pos = model%dofs%find_by_address(node_ids(a), FIELD_ID_DISPLACEMENT, 1)
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
                history_load_increment(i) = real(result%history(i)%load_increment, c_double)
                history_residual_norm(i) = real(result%history(i)%residual_norm, c_double)
                history_relative_residual(i) = real(result%history(i)%relative_residual, c_double)
                history_displacement_increment_norm(i) = &
                    real(result%history(i)%displacement_increment_norm, c_double)
                history_relative_displacement(i) = &
                    real(result%history(i)%relative_displacement, c_double)
                history_alpha(i) = real(result%history(i)%line_search_alpha, c_double)
                history_minimum_j(i) = real(result%history(i)%minimum_j, c_double)
                history_converged(i) = merge(1_c_int, 0_c_int, result%history(i)%converged)
            end do
        end if

        fem_demo_nonlinear_hex8_diagnostics = 0_c_int
        return
920     continue
        fem_demo_nonlinear_hex8_diagnostics = int(status%code, c_int)
    end function fem_demo_nonlinear_hex8_diagnostics


    integer(c_int) function fem_demo_mixed_up_hex8_shear_diagnostics( &
        c10, bulk_modulus, shear_gamma, recovered_shear_gamma, element_pressure, &
        completed_load_factor, final_residual_norm, final_pressure_residual_norm, &
        minimum_j, accepted_steps, total_iterations, cutbacks, history_capacity, &
        history_count, history_attempt, history_accepted_step_before, history_iteration, &
        history_load_factor, history_load_increment, history_residual_norm, &
        history_relative_residual, history_displacement_increment_norm, &
        history_relative_displacement, history_pressure_residual_norm, &
        history_relative_pressure_residual, history_pressure_increment_norm, &
        history_alpha, history_minimum_j, history_converged) &
        bind(C, name="fem_demo_mixed_up_hex8_shear_diagnostics")
        !! V0.10 mixed u-p verification modelinin additive telemetry yolu.
        real(c_double), value, intent(in) :: c10, bulk_modulus, shear_gamma
        real(c_double), intent(out) :: recovered_shear_gamma, element_pressure
        real(c_double), intent(out) :: completed_load_factor, final_residual_norm
        real(c_double), intent(out) :: final_pressure_residual_norm, minimum_j
        integer(c_int), intent(out) :: accepted_steps, total_iterations, cutbacks
        integer(c_int), value, intent(in) :: history_capacity
        integer(c_int), intent(out) :: history_count
        integer(c_int), intent(out) :: history_attempt(*), history_accepted_step_before(*)
        integer(c_int), intent(out) :: history_iteration(*), history_converged(*)
        real(c_double), intent(out) :: history_load_factor(*), history_load_increment(*)
        real(c_double), intent(out) :: history_residual_norm(*), history_relative_residual(*)
        real(c_double), intent(out) :: history_displacement_increment_norm(*)
        real(c_double), intent(out) :: history_relative_displacement(*)
        real(c_double), intent(out) :: history_pressure_residual_norm(*)
        real(c_double), intent(out) :: history_relative_pressure_residual(*)
        real(c_double), intent(out) :: history_pressure_increment_norm(*)
        real(c_double), intent(out) :: history_alpha(*), history_minimum_j(*)

        type(model_t) :: model
        type(hyperelastic_material_t) :: material
        type(nonlinear_system_t) :: target_system, final_system
        type(nonlinear_solver_options_t) :: options
        type(nonlinear_static_result_t) :: result
        type(element_pressure_results_t) :: pressure_results
        type(status_t) :: status
        integer(id_kind), parameter :: node_ids(8) = [17_id_kind, 91_id_kind, &
            4_id_kind, 250_id_kind, 31_id_kind, 8_id_kind, 77_id_kind, 12_id_kind]
        real(rk) :: x(3,8), gamma_rk
        real(rk), allocatable :: target(:)
        integer(index_kind) :: pos
        integer(id_kind) :: dof_id, eq, constraint_id, load_id
        integer :: a, i, ncopy

        recovered_shear_gamma = 0._c_double
        element_pressure = 0._c_double
        completed_load_factor = 0._c_double
        final_residual_norm = 0._c_double
        final_pressure_residual_norm = 0._c_double
        minimum_j = 0._c_double
        accepted_steps = 0_c_int
        total_iterations = 0_c_int
        cutbacks = 0_c_int
        history_count = 0_c_int
        fem_demo_mixed_up_hex8_shear_diagnostics = 10_c_int
        if (c10 <= 0._c_double .or. bulk_modulus <= 0._c_double .or. &
            abs(shear_gamma) > 1._c_double .or. history_capacity < 0_c_int) return

        gamma_rk = real(shear_gamma, rk)
        x(:,1) = [0._rk,0._rk,0._rk]
        x(:,2) = [1._rk,0._rk,0._rk]
        x(:,3) = [1._rk,1._rk,0._rk]
        x(:,4) = [0._rk,1._rk,0._rk]
        x(:,5) = [0._rk,0._rk,1._rk]
        x(:,6) = [1._rk,0._rk,1._rk]
        x(:,7) = [1._rk,1._rk,1._rk]
        x(:,8) = [0._rk,1._rk,1._rk]
        do a = 1, 8
            call model%mesh%add_node(node_ids(a), x(:,a), status)
            if (.not. status%is_ok()) goto 930
        end do
        call model%mesh%add_element(330_id_kind, TOPOLOGY_HEX8, node_ids, status)
        if (.not. status%is_ok()) goto 930
        call model%mesh%assign_element_formulation( &
            330_id_kind, ELEMENT_MIXED_UP_HEX8_P0, status)
        if (.not. status%is_ok()) goto 930
        material = hyperelastic_material_t(id=44_id_kind, name='C API Mixed Diagnostics NH', &
            model=HYPER_NEO_HOOKEAN, bulk_modulus=real(bulk_modulus,rk), &
            c10=real(c10,rk))
        call model%hyperelastic_materials%add(material, status)
        if (.not. status%is_ok()) goto 930
        call model%mesh%assign_element_properties(330_id_kind, 44_id_kind, -1_id_kind, status)
        if (.not. status%is_ok()) goto 930
        call model%initialize_standard_registries(status)
        if (.not. status%is_ok()) goto 930
        call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT, status)
        if (.not. status%is_ok()) goto 930
        call model%build_element_field_dofs(FIELD_ID_PRESSURE_P0, status)
        if (.not. status%is_ok()) goto 930
        call constrain_component(1,1); if (.not. status%is_ok()) goto 930
        call constrain_component(1,2); if (.not. status%is_ok()) goto 930
        call constrain_component(1,3); if (.not. status%is_ok()) goto 930
        call constrain_component(2,2); if (.not. status%is_ok()) goto 930
        call constrain_component(2,3); if (.not. status%is_ok()) goto 930
        call constrain_component(4,3); if (.not. status%is_ok()) goto 930
        call model%renumber(status)
        if (.not. status%is_ok()) goto 930

        allocate(target(int(model%numbering%active_equation_count)))
        target = 0._rk
        do a = 1, 8
            pos = model%dofs%find_by_address(node_ids(a), FIELD_ID_DISPLACEMENT, 1)
            dof_id = model%dofs%dofs(pos)%id
            eq = model%numbering%equation_of(dof_id)
            if (eq >= 0_id_kind) target(int(eq)+1) = gamma_rk * x(2,a)
        end do
        pos = model%dofs%find_by_address(330_id_kind, FIELD_ID_PRESSURE_P0, 1)
        dof_id = model%dofs%dofs(pos)%id
        eq = model%numbering%equation_of(dof_id)
        if (eq >= 0_id_kind) target(int(eq)+1) = 0._rk
        call evaluate_nonlinear_system(model, target, target_system, status)
        if (.not. status%is_ok()) goto 930
        do i = 1, size(model%numbering%dof_ids)
            if (model%numbering%equation_ids(i) < 0_id_kind) cycle
            pos = model%dofs%find_position(model%numbering%dof_ids(i))
            if (model%dofs%dofs(pos)%field_id /= FIELD_ID_DISPLACEMENT) cycle
            eq = model%numbering%equation_ids(i)
            if (abs(target_system%internal_force(int(eq)+1)) > 1.e-10_rk) then
                call model%loads%add(model%numbering%dof_ids(i), &
                    target_system%internal_force(int(eq)+1), load_id, status)
                if (.not. status%is_ok()) goto 930
            end if
        end do

        options%initial_load_increment = 0.25_rk
        options%maximum_load_increment = 0.5_rk
        options%minimum_load_increment = 1.e-5_rk
        options%max_iterations = 30
        options%line_search = .true.
        options%adaptive_stepping = .true.
        options%linear%backend = LINEAR_SOLVER_DENSE_REFERENCE
        call solve_nonlinear_static(model, options, result, status)
        if (.not. status%is_ok()) goto 930
        call evaluate_nonlinear_system( &
            model, result%active_displacement, final_system, status, 1._rk)
        if (.not. status%is_ok()) goto 930
        call recover_mixed_p0_pressure( &
            model, result%active_displacement, pressure_results, status)
        if (.not. status%is_ok()) goto 930

        pos = model%dofs%find_by_address(node_ids(3), FIELD_ID_DISPLACEMENT, 1)
        dof_id = model%dofs%dofs(pos)%id
        eq = model%numbering%equation_of(dof_id)
        recovered_shear_gamma = real(result%active_displacement(int(eq)+1), c_double)
        if (size(pressure_results%pressure) > 0) then
            element_pressure = real(pressure_results%pressure(1), c_double)
        end if
        completed_load_factor = real(result%completed_load_factor, c_double)
        final_residual_norm = real(result%final_residual_norm, c_double)
        final_pressure_residual_norm = real(final_system%pressure_residual_norm, c_double)
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
                history_load_increment(i) = real(result%history(i)%load_increment, c_double)
                history_residual_norm(i) = real(result%history(i)%residual_norm, c_double)
                history_relative_residual(i) = real(result%history(i)%relative_residual, c_double)
                history_displacement_increment_norm(i) = &
                    real(result%history(i)%displacement_increment_norm, c_double)
                history_relative_displacement(i) = &
                    real(result%history(i)%relative_displacement, c_double)
                history_pressure_residual_norm(i) = &
                    real(result%history(i)%pressure_residual_norm, c_double)
                history_relative_pressure_residual(i) = &
                    real(result%history(i)%relative_pressure_residual, c_double)
                history_pressure_increment_norm(i) = &
                    real(result%history(i)%pressure_increment_norm, c_double)
                history_alpha(i) = real(result%history(i)%line_search_alpha, c_double)
                history_minimum_j(i) = real(result%history(i)%minimum_j, c_double)
                history_converged(i) = merge(1_c_int, 0_c_int, result%history(i)%converged)
            end do
        end if

        fem_demo_mixed_up_hex8_shear_diagnostics = 0_c_int
        return
930     continue
        fem_demo_mixed_up_hex8_shear_diagnostics = int(status%code, c_int)
    contains
        subroutine constrain_component(node_index, component)
            integer, intent(in) :: node_index, component
            pos = model%dofs%find_by_address( &
                node_ids(node_index), FIELD_ID_DISPLACEMENT, component)
            dof_id = model%dofs%dofs(pos)%id
            call model%constraints%add(dof_id, 0._rk, constraint_id, status)
        end subroutine constrain_component
    end function fem_demo_mixed_up_hex8_shear_diagnostics


    integer(c_int) function fem_demo_contact_hex8_diagnostics( &
        young_modulus, poisson_ratio, normal_penalty, total_force, enforcement, &
        maximum_penetration, total_normal_force, active_contacts, stick_contacts, &
        slip_contacts, completed_load_factor, final_residual_norm, minimum_j, &
        accepted_steps, total_iterations, cutbacks, history_capacity, history_count, &
        history_attempt, history_accepted_step_before, history_iteration, &
        history_load_factor, history_load_increment, history_residual_norm, &
        history_relative_residual, history_displacement_increment_norm, &
        history_relative_displacement, history_alpha, history_minimum_j, &
        history_active_contacts, history_stick_contacts, history_slip_contacts, &
        history_maximum_penetration, history_converged) &
        bind(C, name="fem_demo_contact_hex8_diagnostics")
        !! V0.11 rigid-master frictionless verification modelinin telemetry yolu.
        !! Bu fonksiyon general GUI Contact solve desteği iddiasi degildir.
        real(c_double), value, intent(in) :: young_modulus, poisson_ratio
        real(c_double), value, intent(in) :: normal_penalty, total_force
        integer(c_int), value, intent(in) :: enforcement, history_capacity
        real(c_double), intent(out) :: maximum_penetration, total_normal_force
        real(c_double), intent(out) :: completed_load_factor, final_residual_norm, minimum_j
        integer(c_int), intent(out) :: active_contacts, stick_contacts, slip_contacts
        integer(c_int), intent(out) :: accepted_steps, total_iterations, cutbacks
        integer(c_int), intent(out) :: history_count
        integer(c_int), intent(out) :: history_attempt(*), history_accepted_step_before(*)
        integer(c_int), intent(out) :: history_iteration(*), history_converged(*)
        real(c_double), intent(out) :: history_load_factor(*), history_load_increment(*)
        real(c_double), intent(out) :: history_residual_norm(*), history_relative_residual(*)
        real(c_double), intent(out) :: history_displacement_increment_norm(*)
        real(c_double), intent(out) :: history_relative_displacement(*)
        real(c_double), intent(out) :: history_alpha(*), history_minimum_j(*)
        integer(c_int), intent(out) :: history_active_contacts(*)
        integer(c_int), intent(out) :: history_stick_contacts(*), history_slip_contacts(*)
        real(c_double), intent(out) :: history_maximum_penetration(*)

        type(model_t) :: model
        type(nonlinear_solver_options_t) :: options
        type(nonlinear_static_result_t) :: result
        type(nonlinear_system_t) :: system
        type(status_t) :: status
        type(linear_elastic_material_t) :: material
        type(contact_pair_t) :: pair
        type(contact_facet_t) :: facet
        integer(id_kind), parameter :: cube_ids(8) = [101_id_kind,102_id_kind, &
            103_id_kind,104_id_kind,105_id_kind,106_id_kind,107_id_kind,108_id_kind]
        integer(id_kind), parameter :: master_ids(4) = [201_id_kind,202_id_kind, &
            203_id_kind,204_id_kind]
        real(rk) :: x(3,8), xm(3,4)
        integer(index_kind) :: pos
        integer(id_kind) :: dof_id, constraint_id, load_id
        integer :: a, c, enf, i, ncopy

        maximum_penetration = 0._c_double
        total_normal_force = 0._c_double
        completed_load_factor = 0._c_double
        final_residual_norm = 0._c_double
        minimum_j = 0._c_double
        active_contacts = 0_c_int
        stick_contacts = 0_c_int
        slip_contacts = 0_c_int
        accepted_steps = 0_c_int
        total_iterations = 0_c_int
        cutbacks = 0_c_int
        history_count = 0_c_int
        fem_demo_contact_hex8_diagnostics = 10_c_int
        if (young_modulus <= 0._c_double .or. poisson_ratio <= -1._c_double .or. &
            poisson_ratio >= 0.5_c_double .or. normal_penalty <= 0._c_double .or. &
            total_force <= 0._c_double .or. history_capacity < 0_c_int) return
        enf = int(enforcement)
        if (enf /= CONTACT_ENFORCEMENT_PENALTY .and. &
            enf /= CONTACT_ENFORCEMENT_AUGMENTED_LAGRANGIAN) return

        x(:,1) = [-0.5_rk,-0.5_rk,0._rk]
        x(:,2) = [ 0.5_rk,-0.5_rk,0._rk]
        x(:,3) = [ 0.5_rk, 0.5_rk,0._rk]
        x(:,4) = [-0.5_rk, 0.5_rk,0._rk]
        x(:,5) = x(:,1) + [0._rk,0._rk,1._rk]
        x(:,6) = x(:,2) + [0._rk,0._rk,1._rk]
        x(:,7) = x(:,3) + [0._rk,0._rk,1._rk]
        x(:,8) = x(:,4) + [0._rk,0._rk,1._rk]
        xm = x(:,1:4)
        do a = 1, 8
            call model%mesh%add_node(cube_ids(a), x(:,a), status)
            if (.not. status%is_ok()) goto 970
        end do
        do a = 1, 4
            call model%mesh%add_node(master_ids(a), xm(:,a), status)
            if (.not. status%is_ok()) goto 970
        end do
        call model%mesh%add_element(700_id_kind, TOPOLOGY_HEX8, cube_ids, status)
        if (.not. status%is_ok()) goto 970
        call model%mesh%assign_element_formulation( &
            700_id_kind, ELEMENT_TOTAL_LAGRANGIAN_HEX8, status)
        if (.not. status%is_ok()) goto 970
        material = linear_elastic_material_t(id=77_id_kind, &
            name="C API Contact Diagnostics StVK", young_modulus=real(young_modulus,rk), &
            poisson_ratio=real(poisson_ratio,rk))
        call model%materials%add(material, status)
        if (.not. status%is_ok()) goto 970
        call model%mesh%assign_element_properties(700_id_kind,77_id_kind,-1_id_kind,status)
        if (.not. status%is_ok()) goto 970
        call model%initialize_standard_registries(status)
        if (.not. status%is_ok()) goto 970
        call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT, status)
        if (.not. status%is_ok()) goto 970
        do a = 1, 8
            do c = 1, 2
                pos = model%dofs%find_by_address(cube_ids(a),FIELD_ID_DISPLACEMENT,c)
                dof_id = model%dofs%dofs(pos)%id
                call model%constraints%add(dof_id,0._rk,constraint_id,status)
                if (.not. status%is_ok()) goto 970
            end do
        end do
        do a = 1, 4
            do c = 1, 3
                pos = model%dofs%find_by_address(master_ids(a),FIELD_ID_DISPLACEMENT,c)
                dof_id = model%dofs%dofs(pos)%id
                call model%constraints%add(dof_id,0._rk,constraint_id,status)
                if (.not. status%is_ok()) goto 970
            end do
        end do
        do a = 5, 8
            pos = model%dofs%find_by_address(cube_ids(a),FIELD_ID_DISPLACEMENT,3)
            dof_id = model%dofs%dofs(pos)%id
            call model%loads%add(dof_id,-real(total_force,rk)/4._rk,load_id,status)
            if (.not. status%is_ok()) goto 970
        end do
        facet = contact_facet_t(id=900_id_kind,node_ids=master_ids)
        pair%id = 901_id_kind
        allocate(pair%slave_node_ids(4)); pair%slave_node_ids = cube_ids(1:4)
        allocate(pair%master_facets(1)); pair%master_facets = [facet]
        pair%enforcement = enf
        pair%friction_model = CONTACT_FRICTIONLESS
        pair%normal_penalty = real(normal_penalty,rk)
        pair%search_distance = 0.05_rk
        pair%activation_tolerance = 1.0e-12_rk
        call model%contacts%add(pair,status)
        if (.not. status%is_ok()) goto 970

        options%linear%backend = LINEAR_SOLVER_DENSE_REFERENCE
        options%initial_load_increment = 0.25_rk
        options%minimum_load_increment = 1.0e-5_rk
        options%maximum_load_increment = 0.5_rk
        options%max_iterations = 25
        options%line_search = .true.
        options%adaptive_stepping = .true.
        call solve_nonlinear_static(model,options,result,status)
        if (.not. status%is_ok()) goto 970
        call evaluate_nonlinear_system( &
            model,result%active_displacement,system,status,1._rk)
        if (.not. status%is_ok()) goto 970

        maximum_penetration = real(system%maximum_penetration,c_double)
        total_normal_force = real(system%total_contact_normal_force,c_double)
        active_contacts = int(system%active_contact_count,c_int)
        stick_contacts = int(system%stick_contact_count,c_int)
        slip_contacts = int(system%slip_contact_count,c_int)
        completed_load_factor = real(result%completed_load_factor,c_double)
        final_residual_norm = real(result%final_residual_norm,c_double)
        minimum_j = real(result%minimum_j,c_double)
        accepted_steps = int(result%accepted_steps,c_int)
        total_iterations = int(result%total_iterations,c_int)
        cutbacks = int(result%cutback_count,c_int)

        if (allocated(result%history)) then
            ncopy = min(int(history_capacity),size(result%history))
            history_count = int(ncopy,c_int)
            do i = 1, ncopy
                history_attempt(i) = int(result%history(i)%attempt,c_int)
                history_accepted_step_before(i) = &
                    int(result%history(i)%accepted_step_before,c_int)
                history_iteration(i) = int(result%history(i)%iteration,c_int)
                history_load_factor(i) = real(result%history(i)%load_factor,c_double)
                history_load_increment(i) = real(result%history(i)%load_increment,c_double)
                history_residual_norm(i) = real(result%history(i)%residual_norm,c_double)
                history_relative_residual(i) = real(result%history(i)%relative_residual,c_double)
                history_displacement_increment_norm(i) = &
                    real(result%history(i)%displacement_increment_norm,c_double)
                history_relative_displacement(i) = &
                    real(result%history(i)%relative_displacement,c_double)
                history_alpha(i) = real(result%history(i)%line_search_alpha,c_double)
                history_minimum_j(i) = real(result%history(i)%minimum_j,c_double)
                history_active_contacts(i) = &
                    int(result%history(i)%active_contact_count,c_int)
                history_stick_contacts(i) = int(result%history(i)%stick_contact_count,c_int)
                history_slip_contacts(i) = int(result%history(i)%slip_contact_count,c_int)
                history_maximum_penetration(i) = &
                    real(result%history(i)%maximum_penetration,c_double)
                history_converged(i) = merge(1_c_int,0_c_int,result%history(i)%converged)
            end do
        end if

        fem_demo_contact_hex8_diagnostics = 0_c_int
        return
970     continue
        fem_demo_contact_hex8_diagnostics = int(status%code,c_int)
    end function fem_demo_contact_hex8_diagnostics

end module fem_nonlinear_diagnostics_api
