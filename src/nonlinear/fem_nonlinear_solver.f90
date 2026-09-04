module fem_nonlinear_solver
    !! V0.8 load-controlled nonlinear static solver.
    !!
    !! Temel denklem:
    !!
    !!     K_T(u,lambda) * Delta_u = R(u,lambda)
    !!     R = lambda * f_ext - f_int(u)
    !!
    !! Solver element formulationunu bilmez; V0.7 nonlinear system evaluator
    !! uzerinden residual ve consistent tangent ister. Böylece Newton algoritmasi
    !! ile finite-strain element matematigi birbirinden ayrik kalir.
    use fem_kinds, only : rk, index_kind, id_kind
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT, FIELD_ID_PRESSURE_P0
    use fem_nonlinear_assembly, only : nonlinear_system_t, evaluate_nonlinear_system
    use fem_nonlinear_state, only : nonlinear_displacement_state_t
    use fem_linear_solver, only : linear_solver_options_t, linear_solver_statistics_t, &
        LINEAR_SOLVER_DENSE_REFERENCE, LINEAR_SOLVER_SPARSE_CG, solve_linear_system
    use fem_sparse_matrix, only : csr_matrix_t
    use fem_nonlinear_contracts, only : NONLINEAR_PHASE_NONE, &
        NONLINEAR_PHASE_INPUT_VALIDATION, NONLINEAR_PHASE_LOAD_STEPPING, &
        NONLINEAR_PHASE_NEWTON_ITERATION, NONLINEAR_PHASE_LINE_SEARCH, &
        NONLINEAR_PHASE_LINEAR_SOLVE, NONLINEAR_REASON_NONE, &
        NONLINEAR_REASON_CONVERGED, NONLINEAR_REASON_INVALID_INPUT, &
        NONLINEAR_REASON_NO_ACTIVE_EQUATION, &
        NONLINEAR_REASON_MAXIMUM_STEP_ATTEMPTS_REACHED, &
        NONLINEAR_REASON_MINIMUM_INCREMENT_REACHED, &
        NONLINEAR_REASON_NEWTON_ITERATION_LIMIT, &
        NONLINEAR_REASON_LINE_SEARCH_FAILURE, &
        NONLINEAR_REASON_LINEAR_SOLVER_FAILURE, &
        NONLINEAR_REASON_SINGULAR_OR_ILL_CONDITIONED_TANGENT, &
        NONLINEAR_REASON_INVALID_REFERENCE_JACOBIAN, &
        NONLINEAR_REASON_INVALID_DEFORMATION_JACOBIAN, &
        NONLINEAR_REASON_UNKNOWN_NUMERICAL_FAILURE, &
        ADAPTIVE_EVENT_NONE, ADAPTIVE_EVENT_GROWTH, ADAPTIVE_EVENT_CUTBACK, &
        ADAPTIVE_EVENT_RETRY, ADAPTIVE_REASON_NONE, &
        ADAPTIVE_REASON_FAST_CONVERGENCE, ADAPTIVE_REASON_NEWTON_NONCONVERGENCE, &
        ADAPTIVE_REASON_ITERATION_PREDICTION, &
        ADAPTIVE_REASON_LINEAR_SOLVER_FAILURE, ADAPTIVE_REASON_INVALID_JACOBIAN, &
        ADAPTIVE_REASON_MINIMUM_INCREMENT_LIMIT
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_NUMERICAL_FAILURE
    implicit none
    private

    integer, parameter, public :: NONLINEAR_FULL_NEWTON = 1
    integer, parameter, public :: NONLINEAR_MODIFIED_NEWTON = 2

    type, public :: nonlinear_solver_options_t
        integer :: method = NONLINEAR_FULL_NEWTON
        integer :: max_iterations = 25
        integer :: max_step_attempts = 200
        real(rk) :: initial_load_increment = 0.25_rk
        real(rk) :: minimum_load_increment = 1.0e-4_rk
        real(rk) :: maximum_load_increment = 0.50_rk
        logical :: adaptive_stepping = .true.
        real(rk) :: cutback_factor = 0.50_rk
        real(rk) :: growth_factor = 1.50_rk
        integer :: target_iterations = 6
        logical :: line_search = .true.
        integer :: line_search_max_iterations = 8
        real(rk) :: line_search_reduction = 0.50_rk
        real(rk) :: line_search_min_alpha = 1.0e-4_rk
        logical :: use_residual_criterion = .true.
        logical :: use_displacement_criterion = .true.
        logical :: use_energy_criterion = .false.
        real(rk) :: residual_relative_tolerance = 1.0e-8_rk
        real(rk) :: residual_absolute_tolerance = 1.0e-10_rk
        real(rk) :: pressure_residual_relative_tolerance = 1.0e-8_rk
        real(rk) :: pressure_residual_absolute_tolerance = 1.0e-12_rk
        real(rk) :: displacement_relative_tolerance = 1.0e-8_rk
        real(rk) :: energy_relative_tolerance = 1.0e-10_rk
        type(linear_solver_options_t) :: linear
    contains
        procedure :: validate => nonlinear_options_validate
    end type nonlinear_solver_options_t

    type, public :: nonlinear_history_entry_t
        integer :: attempt = 0
        integer :: accepted_step_before = 0
        integer :: iteration = 0
        real(rk) :: load_factor = 0.0_rk
        real(rk) :: load_increment = 0.0_rk
        real(rk) :: residual_norm = 0.0_rk
        real(rk) :: relative_residual = 0.0_rk
        real(rk) :: displacement_residual_norm = 0.0_rk
        real(rk) :: pressure_residual_norm = 0.0_rk
        real(rk) :: relative_pressure_residual = 0.0_rk
        real(rk) :: displacement_increment_norm = 0.0_rk
        real(rk) :: pressure_increment_norm = 0.0_rk
        real(rk) :: relative_displacement = 0.0_rk
        real(rk) :: energy_measure = 0.0_rk
        real(rk) :: relative_energy = 0.0_rk
        real(rk) :: line_search_alpha = 1.0_rk
        real(rk) :: minimum_j = huge(1.0_rk)
        integer :: active_contact_count = 0
        integer :: stick_contact_count = 0
        integer :: slip_contact_count = 0
        real(rk) :: maximum_penetration = 0.0_rk
        logical :: converged = .false.
        integer :: adaptive_event = ADAPTIVE_EVENT_NONE
        integer :: adaptive_reason = ADAPTIVE_REASON_NONE
    end type nonlinear_history_entry_t

    type, public :: nonlinear_static_result_t
        logical :: converged = .false.
        real(rk) :: completed_load_factor = 0.0_rk
        integer :: accepted_steps = 0
        integer :: step_attempts = 0
        integer :: total_iterations = 0
        integer :: cutback_count = 0
        integer :: termination_phase = NONLINEAR_PHASE_NONE
        integer :: termination_reason = NONLINEAR_REASON_NONE
        real(rk) :: last_attempted_load_factor = 0.0_rk
        real(rk) :: last_load_increment = 0.0_rk
        real(rk) :: final_residual_norm = huge(1.0_rk)
        real(rk) :: minimum_j = huge(1.0_rk)
        integer :: final_active_contact_count = 0
        integer :: final_stick_contact_count = 0
        integer :: final_slip_contact_count = 0
        real(rk) :: maximum_penetration = 0.0_rk
        real(rk), allocatable :: active_displacement(:)
        type(nonlinear_history_entry_t), allocatable :: history(:)
    contains
        procedure :: clear => nonlinear_result_clear
    end type nonlinear_static_result_t

    type, public :: nonlinear_checkpoint_t
        real(rk) :: load_factor = 0.0_rk
        integer :: accepted_steps = 0
        real(rk), allocatable :: active_displacement(:)
    contains
        procedure :: clear => nonlinear_checkpoint_clear
        procedure :: capture => nonlinear_checkpoint_capture
    end type nonlinear_checkpoint_t

    public :: solve_nonlinear_static

contains

    subroutine nonlinear_options_validate(this, status)
        class(nonlinear_solver_options_t), intent(in) :: this
        type(status_t), intent(out) :: status
        call status%clear()
        if (this%method /= NONLINEAR_FULL_NEWTON .and. this%method /= NONLINEAR_MODIFIED_NEWTON) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Nonlinear solver method ID gecersiz."); return
        end if
        if (this%max_iterations < 1 .or. this%max_step_attempts < 1 .or. this%target_iterations < 1) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Nonlinear iteration/step limitleri pozitif olmali."); return
        end if
        if (this%initial_load_increment <= 0.0_rk .or. this%initial_load_increment > 1.0_rk) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Initial load increment 0 < Delta_lambda <= 1 olmali."); return
        end if
        if (this%minimum_load_increment <= 0.0_rk .or. this%maximum_load_increment <= 0.0_rk .or. &
            this%minimum_load_increment > this%maximum_load_increment) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Load increment min/max araligi gecersiz."); return
        end if
        if (this%initial_load_increment < this%minimum_load_increment .or. &
            this%initial_load_increment > this%maximum_load_increment) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Initial load increment min/max sinirlari icinde olmali."); return
        end if
        if (this%cutback_factor <= 0.0_rk .or. this%cutback_factor >= 1.0_rk) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Cutback factor 0 ile 1 arasinda olmali."); return
        end if
        if (this%growth_factor < 1.0_rk) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Growth factor 1 veya daha buyuk olmali."); return
        end if
        if (this%line_search) then
            if (this%line_search_max_iterations < 1 .or. this%line_search_reduction <= 0.0_rk .or. &
                this%line_search_reduction >= 1.0_rk .or. this%line_search_min_alpha <= 0.0_rk .or. &
                this%line_search_min_alpha > 1.0_rk) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Line-search parametreleri gecersiz."); return
            end if
        end if
        if (.not. this%use_residual_criterion .and. .not. this%use_displacement_criterion .and. &
            .not. this%use_energy_criterion) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "En az bir nonlinear convergence kriteri aktif olmali."); return
        end if
        if (this%residual_relative_tolerance < 0.0_rk .or. this%residual_absolute_tolerance < 0.0_rk .or. &
            this%pressure_residual_relative_tolerance < 0.0_rk .or. this%pressure_residual_absolute_tolerance < 0.0_rk .or. &
            this%displacement_relative_tolerance < 0.0_rk .or. this%energy_relative_tolerance < 0.0_rk) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Convergence toleranslari negatif olamaz."); return
        end if
    end subroutine nonlinear_options_validate

    subroutine nonlinear_checkpoint_clear(this)
        class(nonlinear_checkpoint_t), intent(inout) :: this
        this%load_factor = 0.0_rk
        this%accepted_steps = 0
        if (allocated(this%active_displacement)) deallocate(this%active_displacement)
    end subroutine nonlinear_checkpoint_clear

    subroutine nonlinear_checkpoint_capture(this, result, status)
        class(nonlinear_checkpoint_t), intent(inout) :: this
        type(nonlinear_static_result_t), intent(in) :: result
        type(status_t), intent(out) :: status
        call status%clear()
        call this%clear()
        if (.not. allocated(result%active_displacement)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Checkpoint icin nonlinear result displacement mevcut degil.")
            return
        end if
        this%load_factor = result%completed_load_factor
        this%accepted_steps = result%accepted_steps
        allocate(this%active_displacement(size(result%active_displacement)))
        this%active_displacement = result%active_displacement
    end subroutine nonlinear_checkpoint_capture

    subroutine nonlinear_result_clear(this)
        class(nonlinear_static_result_t), intent(inout) :: this
        this%converged = .false.
        this%completed_load_factor = 0.0_rk
        this%accepted_steps = 0
        this%step_attempts = 0
        this%total_iterations = 0
        this%cutback_count = 0
        this%termination_phase = NONLINEAR_PHASE_NONE
        this%termination_reason = NONLINEAR_REASON_NONE
        this%last_attempted_load_factor = 0.0_rk
        this%last_load_increment = 0.0_rk
        this%final_residual_norm = huge(1.0_rk)
        this%minimum_j = huge(1.0_rk)
        this%final_active_contact_count=0;this%final_stick_contact_count=0;this%final_slip_contact_count=0
        this%maximum_penetration=0.0_rk
        if (allocated(this%active_displacement)) deallocate(this%active_displacement)
        if (allocated(this%history)) deallocate(this%history)
    end subroutine nonlinear_result_clear

    subroutine solve_nonlinear_static(model, options, result, status, initial_checkpoint)
        type(model_t), intent(inout) :: model
        type(nonlinear_solver_options_t), intent(in) :: options
        type(nonlinear_static_result_t), intent(inout) :: result
        type(status_t), intent(out) :: status
        type(nonlinear_checkpoint_t), intent(in), optional :: initial_checkpoint
        type(nonlinear_displacement_state_t) :: state
        real(rk) :: current_load, increment, target_load, previous_increment
        logical :: step_converged
        integer :: corrections_used, pending_adaptive_event, pending_adaptive_reason
        integer :: failure_reason

        call status%clear()
        call result%clear()
        result%termination_phase = NONLINEAR_PHASE_INPUT_VALIDATION
        result%termination_reason = NONLINEAR_REASON_INVALID_INPUT
        call options%validate(status)
        if (.not. status%is_ok()) return
        call model%renumber(status)
        if (.not. status%is_ok()) return
        call validate_model_for_load_control(model, status)
        if (.not. status%is_ok()) return
        call model%contacts%prepare(model%mesh,status)
        if(.not.status%is_ok())return
        if (model_has_active_pressure(model) .and. options%linear%backend == LINEAR_SOLVER_SPARSE_CG) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                "Mixed u-p tangent saddle-point/indefinite oldugu icin Conjugate Gradient backend kullanilamaz.")
            return
        end if
        if(model%contacts%count()>0_index_kind.and.options%linear%backend==LINEAR_SOLVER_SPARSE_CG)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                "Contact active-set/friction tangent'i Conjugate Gradient backend ile uyumlu degildir; direct backend secin.")
            return
        end if
        if(present(initial_checkpoint))then
            if(model%contacts%count()>0_index_kind)then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                    "Contact history V1.0 checkpoint schema'sina dahil degildir; contact modelinde checkpoint restart reddedildi.")
                return
            end if
        end if
        if (model%numbering%active_equation_count <= 0_index_kind) then
            result%termination_reason = NONLINEAR_REASON_NO_ACTIVE_EQUATION
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Nonlinear analiz icin en az bir aktif equation gerekli."); return
        end if

        call state%initialize(model%numbering%active_equation_count, status)
        if (.not. status%is_ok()) return
        current_load = 0.0_rk
        increment = options%initial_load_increment
        allocate(result%active_displacement(int(model%numbering%active_equation_count)))
        result%active_displacement = 0.0_rk
        if (present(initial_checkpoint)) then
            if (.not. allocated(initial_checkpoint%active_displacement)) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Nonlinear restart checkpoint displacement icermiyor."); return
            end if
            if (size(initial_checkpoint%active_displacement) /= size(result%active_displacement)) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Nonlinear restart checkpoint DOF boyutu modelle uyusmuyor."); return
            end if
            if (initial_checkpoint%load_factor < 0.0_rk .or. initial_checkpoint%load_factor >= 1.0_rk) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Restart checkpoint load factor [0,1) araliginda olmali."); return
            end if
            call state%set_trial(initial_checkpoint%active_displacement, status); if(.not.status%is_ok())return
            call state%commit(status); if(.not.status%is_ok())return
            current_load = initial_checkpoint%load_factor
            result%completed_load_factor = current_load
            result%accepted_steps = initial_checkpoint%accepted_steps
            result%active_displacement = state%committed
            increment = min(increment, 1.0_rk-current_load)
        end if

        result%termination_phase = NONLINEAR_PHASE_NONE
        result%termination_reason = NONLINEAR_REASON_NONE
        pending_adaptive_event = ADAPTIVE_EVENT_NONE
        pending_adaptive_reason = ADAPTIVE_REASON_NONE

        do while (current_load < 1.0_rk - 100.0_rk*epsilon(1.0_rk))
            if (result%step_attempts >= options%max_step_attempts) then
                result%active_displacement = state%committed
                result%termination_phase = NONLINEAR_PHASE_LOAD_STEPPING
                result%termination_reason = NONLINEAR_REASON_MAXIMUM_STEP_ATTEMPTS_REACHED
                call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, "Nonlinear step-attempt limiti asildi.")
                return
            end if
            result%step_attempts = result%step_attempts + 1
            target_load = min(1.0_rk, current_load + increment)
            result%last_attempted_load_factor = target_load
            result%last_load_increment = increment
            call state%begin_trial(status)
            if (.not. status%is_ok()) return

            call solve_single_load_step(model, state, target_load, increment, options, result, &
                                        pending_adaptive_event, pending_adaptive_reason, &
                                        step_converged, corrections_used, status)
            pending_adaptive_event = ADAPTIVE_EVENT_NONE
            pending_adaptive_reason = ADAPTIVE_REASON_NONE
            if (.not. status%is_ok() .and. status%code /= FEM_STATUS_NUMERICAL_FAILURE) return
            if (.not. status%is_ok() .and. &
                result%termination_reason == NONLINEAR_REASON_INVALID_REFERENCE_JACOBIAN) return

            if (step_converged) then
                call status%clear()
                call state%commit(status)
                if (.not. status%is_ok()) return
                call model%contacts%commit(status)
                if(.not.status%is_ok())return
                current_load = target_load
                result%accepted_steps = result%accepted_steps + 1
                result%completed_load_factor = current_load
                result%active_displacement = state%committed
                result%termination_phase = NONLINEAR_PHASE_NONE
                result%termination_reason = NONLINEAR_REASON_NONE
                previous_increment = increment
                if (options%adaptive_stepping .and. current_load < 1.0_rk) then
                    if (corrections_used <= options%target_iterations) then
                        increment = min(options%maximum_load_increment, increment*options%growth_factor)
                        if (increment > previous_increment*(1.0_rk+100.0_rk*epsilon(1.0_rk))) then
                            pending_adaptive_event = ADAPTIVE_EVENT_GROWTH
                            pending_adaptive_reason = ADAPTIVE_REASON_FAST_CONVERGENCE
                        end if
                    else if (corrections_used > 2*options%target_iterations) then
                        increment = max(options%minimum_load_increment, increment*options%cutback_factor)
                        if (increment < previous_increment*(1.0_rk-100.0_rk*epsilon(1.0_rk))) then
                            pending_adaptive_event = ADAPTIVE_EVENT_CUTBACK
                            pending_adaptive_reason = ADAPTIVE_REASON_ITERATION_PREDICTION
                        end if
                    end if
                end if
                increment = min(increment, 1.0_rk-current_load)
                if (increment <= 0.0_rk .and. current_load < 1.0_rk) increment = options%minimum_load_increment
                if (pending_adaptive_event == ADAPTIVE_EVENT_GROWTH .and. &
                    increment <= previous_increment*(1.0_rk+100.0_rk*epsilon(1.0_rk))) then
                    ! Final load factorune tam oturmak icin yapilan clipping,
                    ! backend tarafinda sahte Growth eventi uretmemelidir.
                    pending_adaptive_event = ADAPTIVE_EVENT_NONE
                    pending_adaptive_reason = ADAPTIVE_REASON_NONE
                end if
            else
                failure_reason = result%termination_reason
                call state%revert(status)
                if (.not. status%is_ok()) return
                call model%contacts%revert(status)
                if(.not.status%is_ok())return
                if (.not. options%adaptive_stepping) then
                    call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, &
                        "Nonlinear load step converge olmadi ve automatic stepping kapali.")
                    return
                end if
                result%cutback_count = result%cutback_count + 1
                result%active_displacement = state%committed
                pending_adaptive_reason = adaptive_reason_from_termination(failure_reason)
                call annotate_last_history_event(result, ADAPTIVE_EVENT_CUTBACK, &
                    pending_adaptive_reason)
                increment = increment*options%cutback_factor
                call status%clear()
                if (increment < options%minimum_load_increment*(1.0_rk-100.0_rk*epsilon(1.0_rk))) then
                    call annotate_last_history_event(result, ADAPTIVE_EVENT_CUTBACK, &
                        ADAPTIVE_REASON_MINIMUM_INCREMENT_LIMIT)
                    result%termination_phase = NONLINEAR_PHASE_LOAD_STEPPING
                    result%termination_reason = NONLINEAR_REASON_MINIMUM_INCREMENT_REACHED
                    call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, &
                        "Nonlinear increment minimum load increment altina dustu; son converged state korunuyor.")
                    return
                end if
                increment = max(increment, options%minimum_load_increment)
                pending_adaptive_event = ADAPTIVE_EVENT_RETRY
            end if
        end do
        result%converged = .true.
        result%completed_load_factor = 1.0_rk
        result%active_displacement = state%committed
        result%termination_phase = NONLINEAR_PHASE_NONE
        result%termination_reason = NONLINEAR_REASON_CONVERGED
        call status%clear()
    end subroutine solve_nonlinear_static

    subroutine solve_single_load_step(model, state, load_factor, load_increment, options, result, &
                                      attempt_adaptive_event, attempt_adaptive_reason, &
                                      converged, corrections_used, status)
        type(model_t), intent(inout) :: model
        type(nonlinear_displacement_state_t), intent(inout) :: state
        real(rk), intent(in) :: load_factor, load_increment
        type(nonlinear_solver_options_t), intent(in) :: options
        type(nonlinear_static_result_t), intent(inout) :: result
        integer, intent(in) :: attempt_adaptive_event, attempt_adaptive_reason
        logical, intent(out) :: converged
        integer, intent(out) :: corrections_used
        type(status_t), intent(out) :: status
        type(nonlinear_system_t) :: system, accepted_system
        type(csr_matrix_t) :: modified_tangent
        type(linear_solver_statistics_t) :: linear_statistics
        real(rk), allocatable :: correction(:), accepted_trial(:), previous_correction(:)
        real(rk) :: residual_norm, residual_scale, relative_residual
        real(rk) :: displacement_residual_norm, pressure_residual_norm, pressure_residual_scale, relative_pressure_residual
        real(rk) :: displacement_norm, pressure_increment_norm, relative_displacement
        real(rk) :: energy_measure, energy_scale, relative_energy
        real(rk) :: alpha, previous_alpha
        logical :: residual_ok, pressure_residual_ok, displacement_ok, energy_ok, all_ok
        integer :: iteration

        call status%clear()
        converged = .false.
        corrections_used = 0
        result%termination_phase = NONLINEAR_PHASE_NONE
        result%termination_reason = NONLINEAR_REASON_NONE
        allocate(previous_correction(size(state%trial)))
        previous_correction = 0.0_rk
        residual_scale = 0.0_rk
        pressure_residual_scale = 0.0_rk
        previous_alpha = 1.0_rk

        do iteration = 1, options%max_iterations + 1
            call evaluate_nonlinear_system(model, state%trial, system, status, load_factor)
            if (.not. status%is_ok()) then
                result%termination_phase = system%termination_phase
                result%termination_reason = system%termination_reason
                if (result%termination_phase == NONLINEAR_PHASE_NONE) then
                    result%termination_phase = NONLINEAR_PHASE_NEWTON_ITERATION
                end if
                if (result%termination_reason == NONLINEAR_REASON_NONE) then
                    result%termination_reason = NONLINEAR_REASON_UNKNOWN_NUMERICAL_FAILURE
                end if
                return
            end if
            result%minimum_j = min(result%minimum_j, system%minimum_j)
            result%final_active_contact_count=system%active_contact_count
            result%final_stick_contact_count=system%stick_contact_count
            result%final_slip_contact_count=system%slip_contact_count
            result%maximum_penetration=max(result%maximum_penetration,system%maximum_penetration)
            call block_norms(model,system%residual,displacement_residual_norm,pressure_residual_norm)
            residual_norm = displacement_residual_norm
            if (iteration == 1) then
                call block_norms(model,system%external_force,residual_scale,pressure_increment_norm)
                residual_scale=max(displacement_residual_norm,residual_scale,tiny(1.0_rk))
            else
                residual_scale=max(residual_scale,displacement_residual_norm,tiny(1.0_rk))
            end if
            pressure_residual_scale=max(pressure_residual_scale,pressure_residual_norm,tiny(1.0_rk))
            relative_residual = displacement_residual_norm / max(residual_scale, tiny(1.0_rk))
            relative_pressure_residual = pressure_residual_norm / max(pressure_residual_scale,tiny(1.0_rk))
            call block_norms(model,previous_correction,displacement_norm,pressure_increment_norm)
            call block_norms(model,state%trial,energy_scale,energy_measure)
            relative_displacement = displacement_norm / max(energy_scale, sqrt(epsilon(1.0_rk)))
            energy_measure = abs(dot_product(previous_correction, system%residual))
            energy_scale = max(abs(dot_product(state%trial, system%external_force)), &
                               residual_scale*max(energy_scale, sqrt(epsilon(1.0_rk))), tiny(1.0_rk))
            relative_energy = energy_measure / energy_scale

            residual_ok = displacement_residual_norm <= options%residual_absolute_tolerance + &
                          options%residual_relative_tolerance*residual_scale
            pressure_residual_ok = (.not.system%has_mixed_pressure) .or. &
                pressure_residual_norm <= options%pressure_residual_absolute_tolerance + &
                                          options%pressure_residual_relative_tolerance*pressure_residual_scale
            displacement_ok = relative_displacement <= options%displacement_relative_tolerance
            energy_ok = relative_energy <= options%energy_relative_tolerance
            if (iteration == 1 .and. residual_ok) then
                displacement_ok = .true.
                pressure_residual_ok = .true.
                energy_ok = .true.
            end if
            all_ok = (.not. options%use_residual_criterion .or. (residual_ok .and. pressure_residual_ok)) .and. &
                     (.not. options%use_displacement_criterion .or. displacement_ok) .and. &
                     (.not. options%use_energy_criterion .or. energy_ok)

            call append_history(result, nonlinear_history_entry_t( &
                attempt=result%step_attempts, accepted_step_before=result%accepted_steps, iteration=iteration, &
                load_factor=load_factor, load_increment=load_increment, residual_norm=residual_norm, &
                relative_residual=relative_residual, displacement_residual_norm=displacement_residual_norm, &
                pressure_residual_norm=pressure_residual_norm, relative_pressure_residual=relative_pressure_residual, &
                displacement_increment_norm=displacement_norm, pressure_increment_norm=pressure_increment_norm, &
                relative_displacement=relative_displacement, energy_measure=energy_measure, &
                relative_energy=relative_energy, line_search_alpha=previous_alpha, minimum_j=system%minimum_j, &
                active_contact_count=system%active_contact_count,stick_contact_count=system%stick_contact_count, &
                slip_contact_count=system%slip_contact_count,maximum_penetration=system%maximum_penetration, &
                converged=all_ok, &
                adaptive_event=merge(attempt_adaptive_event,ADAPTIVE_EVENT_NONE,iteration==1), &
                adaptive_reason=merge(attempt_adaptive_reason,ADAPTIVE_REASON_NONE,iteration==1)))
            result%final_residual_norm = residual_norm
            if (all_ok) then
                converged = .true.
                corrections_used = iteration - 1
                return
            end if
            if (iteration > options%max_iterations) then
                result%termination_phase = NONLINEAR_PHASE_NEWTON_ITERATION
                result%termination_reason = NONLINEAR_REASON_NEWTON_ITERATION_LIMIT
                call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, "Load step nonlinear iteration limitinde converge olmadi.")
                return
            end if

            if (options%method == NONLINEAR_MODIFIED_NEWTON .and. iteration == 1) modified_tangent = system%tangent
            if (options%method == NONLINEAR_MODIFIED_NEWTON) then
                call solve_linear_system(modified_tangent, system%residual, correction, options%linear, linear_statistics, status)
            else
                call solve_linear_system(system%tangent, system%residual, correction, options%linear, linear_statistics, status)
            end if
            if (.not. status%is_ok()) then
                result%termination_phase = NONLINEAR_PHASE_LINEAR_SOLVE
                if (options%linear%backend == LINEAR_SOLVER_DENSE_REFERENCE .and. &
                    status%code == FEM_STATUS_NUMERICAL_FAILURE) then
                    result%termination_reason = NONLINEAR_REASON_SINGULAR_OR_ILL_CONDITIONED_TANGENT
                else
                    result%termination_reason = NONLINEAR_REASON_LINEAR_SOLVER_FAILURE
                end if
                return
            end if
            result%total_iterations = result%total_iterations + 1

            if (options%line_search) then
                call perform_line_search(model, state%trial, correction, load_factor, system%residual, options, &
                                         alpha, accepted_trial, accepted_system, status)
                if (.not. status%is_ok()) then
                    result%termination_phase = NONLINEAR_PHASE_LINE_SEARCH
                    result%termination_reason = NONLINEAR_REASON_LINE_SEARCH_FAILURE
                    return
                end if
                call state%set_trial(accepted_trial, status)
                if (.not. status%is_ok()) return
            else
                alpha = 1.0_rk
                call state%set_trial(state%trial + correction, status)
                if (.not. status%is_ok()) return
            end if
            previous_correction = alpha*correction
            previous_alpha = alpha
        end do
    end subroutine solve_single_load_step

    subroutine perform_line_search(model, base_trial, correction, load_factor, base_residual, options, &
                                   alpha, accepted_trial, accepted_system, status)
        type(model_t), intent(inout) :: model
        real(rk), intent(in) :: base_trial(:), correction(:), load_factor, base_residual(:)
        type(nonlinear_solver_options_t), intent(in) :: options
        real(rk), intent(out) :: alpha
        real(rk), allocatable, intent(out) :: accepted_trial(:)
        type(nonlinear_system_t), intent(out) :: accepted_system
        type(status_t), intent(out) :: status
        type(nonlinear_system_t) :: candidate_system
        real(rk), allocatable :: candidate(:)
        real(rk) :: candidate_norm, base_merit, trial_alpha
        integer :: i

        call status%clear()
        alpha = 0.0_rk
        base_merit = line_search_merit(model,correction,base_residual)
        trial_alpha = 1.0_rk
        do i = 1, options%line_search_max_iterations
            allocate(candidate(size(base_trial)))
            candidate = base_trial + trial_alpha*correction
            call evaluate_nonlinear_system(model, candidate, candidate_system, status, load_factor)
            if (.not. status%is_ok()) then
                call status%clear()
                candidate_norm = huge(1.0_rk)
            else
                candidate_norm = line_search_merit(model,correction,candidate_system%residual)
                if (candidate_norm < base_merit) then
                    alpha = trial_alpha
                    call move_alloc(candidate, accepted_trial)
                    accepted_system = candidate_system
                    return
                end if
            end if
            if (allocated(candidate)) deallocate(candidate)
            trial_alpha = trial_alpha*options%line_search_reduction
            if (trial_alpha < options%line_search_min_alpha) exit
        end do
        allocate(accepted_trial(0))
        call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, "Backtracking line search residual azaltan bir trial state bulamadi.")
    end subroutine perform_line_search

    subroutine validate_model_for_load_control(model, status)
        type(model_t), intent(in) :: model
        type(status_t), intent(out) :: status
        integer :: i
        call status%clear()
        if (allocated(model%constraints%constraints)) then
            do i = 1, size(model%constraints%constraints)
                if (abs(model%constraints%constraints(i)%prescribed_value) > 100.0_rk*epsilon(1.0_rk)) then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                        "Mevcut load-control solver nonzero prescribed-displacement stepping desteklemiyor.")
                    return
                end if
            end do
        end if
    end subroutine validate_model_for_load_control

    pure integer function adaptive_reason_from_termination(termination_reason) result(reason)
        integer, intent(in) :: termination_reason

        select case (termination_reason)
        case (NONLINEAR_REASON_LINEAR_SOLVER_FAILURE, &
              NONLINEAR_REASON_SINGULAR_OR_ILL_CONDITIONED_TANGENT)
            reason = ADAPTIVE_REASON_LINEAR_SOLVER_FAILURE
        case (NONLINEAR_REASON_INVALID_REFERENCE_JACOBIAN, &
              NONLINEAR_REASON_INVALID_DEFORMATION_JACOBIAN)
            reason = ADAPTIVE_REASON_INVALID_JACOBIAN
        case default
            ! Newton iteration veya line-search nonconvergence, bu RC.1
            ! load-step motorunda ayni rejected-attempt cutback sinifidir.
            reason = ADAPTIVE_REASON_NEWTON_NONCONVERGENCE
        end select
    end function adaptive_reason_from_termination

    subroutine annotate_last_history_event(result, event_type, event_reason)
        type(nonlinear_static_result_t), intent(inout) :: result
        integer, intent(in) :: event_type, event_reason
        integer :: last

        if (.not. allocated(result%history)) return
        last = size(result%history)
        if (last < 1) return
        if (result%history(last)%attempt /= result%step_attempts) return
        result%history(last)%adaptive_event = event_type
        result%history(last)%adaptive_reason = event_reason
    end subroutine annotate_last_history_event

    subroutine append_history(result, entry)
        type(nonlinear_static_result_t), intent(inout) :: result
        type(nonlinear_history_entry_t), intent(in) :: entry
        type(nonlinear_history_entry_t), allocatable :: tmp(:)
        integer :: n
        if (.not. allocated(result%history)) then
            allocate(result%history(1))
            result%history(1) = entry
        else
            n = size(result%history)
            allocate(tmp(n+1))
            tmp(1:n) = result%history
            tmp(n+1) = entry
            call move_alloc(tmp, result%history)
        end if
    end subroutine append_history



    subroutine block_norms(model,values,displacement_norm,pressure_norm)
        type(model_t),intent(in)::model
        real(rk),intent(in)::values(:)
        real(rk),intent(out)::displacement_norm,pressure_norm
        integer::i,eq_index
        integer(index_kind)::dof_pos
        real(rk)::u2,p2
        u2=0.0_rk;p2=0.0_rk
        if(.not.allocated(model%numbering%dof_ids))then
            displacement_norm=vector_norm(values);pressure_norm=0.0_rk;return
        end if
        do i=1,size(model%numbering%dof_ids)
            if(model%numbering%equation_ids(i)<0_id_kind)cycle
            eq_index=int(model%numbering%equation_ids(i))+1
            if(eq_index<1.or.eq_index>size(values))cycle
            dof_pos=model%dofs%find_position(model%numbering%dof_ids(i))
            if(dof_pos==0_index_kind)cycle
            select case(model%dofs%dofs(dof_pos)%field_id)
            case(FIELD_ID_PRESSURE_P0);p2=p2+values(eq_index)*values(eq_index)
            case default;u2=u2+values(eq_index)*values(eq_index)
            end select
        end do
        displacement_norm=sqrt(max(0.0_rk,u2));pressure_norm=sqrt(max(0.0_rk,p2))
    end subroutine block_norms

    logical function model_has_active_pressure(model)
        type(model_t),intent(in)::model
        integer::i
        integer(index_kind)::dof_pos
        model_has_active_pressure=.false.
        if(.not.allocated(model%numbering%dof_ids))return
        do i=1,size(model%numbering%dof_ids)
            if(model%numbering%equation_ids(i)<0_id_kind)cycle
            dof_pos=model%dofs%find_position(model%numbering%dof_ids(i))
            if(dof_pos/=0_index_kind)then
                if(model%dofs%dofs(dof_pos)%field_id==FIELD_ID_PRESSURE_P0)then
                    model_has_active_pressure=.true.;return
                end if
            end if
        end do
    end function model_has_active_pressure

    real(rk) function line_search_merit(model,correction,residual) result(value)
        type(model_t),intent(in)::model
        real(rk),intent(in)::correction(:),residual(:)
        real(rk)::u,p
        if(model_has_active_pressure(model))then
            ! correction dot residual = incremental work; u*force ve p*volume
            ! terimleri ayni enerji boyutundadir, mixed unit problemini ortadan kaldirir.
            value=abs(dot_product(correction,residual))
        else
            call block_norms(model,residual,u,p);value=u
        end if
    end function line_search_merit

    pure real(rk) function vector_norm(values) result(value)
        real(rk), intent(in) :: values(:)
        value = sqrt(max(0.0_rk, dot_product(values, values)))
    end function vector_norm

end module fem_nonlinear_solver
