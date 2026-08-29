module fem_linear_assembly
    !! Element-local matris/vector katkilarini global active equation sistemine
    !! scatter eder. Assembly katmani solver backend'ini bilmez.
    use fem_kinds, only : rk, id_kind
    use fem_ids, only : INVALID_ID
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_SIZE_MISMATCH
    use fem_sparse_matrix, only : csr_matrix_t
    use fem_element_dof_map, only : element_dof_map_t
    implicit none
    private

    public :: assemble_matrix_by_equation
    public :: assemble_vector_by_equation
    public :: assemble_stiffness_with_constraints
    public :: add_active_equation_load

contains

    subroutine assemble_matrix_by_equation(global_matrix, equation_ids, local_matrix, status)
        type(csr_matrix_t), intent(inout) :: global_matrix
        integer(id_kind), intent(in) :: equation_ids(:)
        real(rk), intent(in) :: local_matrix(:, :)
        type(status_t), intent(out) :: status
        integer :: a, b

        call status%clear()
        if (size(local_matrix,1) /= size(equation_ids) .or. size(local_matrix,2) /= size(equation_ids)) then
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, "Local matrix boyutu equation map ile uyusmuyor.")
            return
        end if
        do a = 1, size(equation_ids)
            if (equation_ids(a) == INVALID_ID) cycle
            do b = 1, size(equation_ids)
                if (equation_ids(b) == INVALID_ID) cycle
                call global_matrix%add_value(equation_ids(a), equation_ids(b), local_matrix(a,b), status)
                if (.not. status%is_ok()) return
            end do
        end do
    end subroutine assemble_matrix_by_equation

    subroutine assemble_vector_by_equation(global_vector, equation_ids, local_vector, status)
        real(rk), intent(inout) :: global_vector(:)
        integer(id_kind), intent(in) :: equation_ids(:)
        real(rk), intent(in) :: local_vector(:)
        type(status_t), intent(out) :: status
        integer :: a

        call status%clear()
        if (size(local_vector) /= size(equation_ids)) then
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, "Local vector boyutu equation map ile uyusmuyor.")
            return
        end if
        do a = 1, size(equation_ids)
            if (equation_ids(a) == INVALID_ID) cycle
            if (equation_ids(a) < 0_id_kind .or. equation_ids(a) >= int(size(global_vector), id_kind)) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Equation ID global vector araligi disinda.")
                return
            end if
            global_vector(int(equation_ids(a)) + 1) = global_vector(int(equation_ids(a)) + 1) + local_vector(a)
        end do
    end subroutine assemble_vector_by_equation

    subroutine assemble_stiffness_with_constraints(global_matrix, rhs, map, local_stiffness, local_load, status)
        type(csr_matrix_t), intent(inout) :: global_matrix
        real(rk), intent(inout) :: rhs(:)
        type(element_dof_map_t), intent(in) :: map
        real(rk), intent(in) :: local_stiffness(:, :)
        real(rk), intent(in) :: local_load(:)
        type(status_t), intent(out) :: status
        integer :: a, b
        integer(id_kind) :: row_eq, col_eq

        call status%clear()
        if (.not. allocated(map%equation_ids) .or. size(local_stiffness,1) /= size(map%equation_ids) .or. &
            size(local_stiffness,2) /= size(map%equation_ids) .or. size(local_load) /= size(map%equation_ids)) then
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, "Stiffness assembly local boyutlari element DOF map ile uyusmuyor.")
            return
        end if

        do a = 1, size(map%equation_ids)
            row_eq = map%equation_ids(a)
            if (row_eq == INVALID_ID) cycle
            if (row_eq < 0_id_kind .or. row_eq >= int(size(rhs), id_kind)) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Stiffness assembly row equation ID RHS araligi disinda.")
                return
            end if
            rhs(int(row_eq) + 1) = rhs(int(row_eq) + 1) + local_load(a)
            do b = 1, size(map%equation_ids)
                col_eq = map%equation_ids(b)
                if (col_eq == INVALID_ID) then
                    if (map%constrained(b)) then
                        rhs(int(row_eq) + 1) = rhs(int(row_eq) + 1) - &
                            local_stiffness(a,b) * map%prescribed_values(b)
                    end if
                else
                    call global_matrix%add_value(row_eq, col_eq, local_stiffness(a,b), status)
                    if (.not. status%is_ok()) return
                end if
            end do
        end do
    end subroutine assemble_stiffness_with_constraints

    subroutine add_active_equation_load(rhs, equation_id, value, status)
        real(rk), intent(inout) :: rhs(:)
        integer(id_kind), intent(in) :: equation_id
        real(rk), intent(in) :: value
        type(status_t), intent(out) :: status

        call status%clear()
        if (equation_id < 0_id_kind .or. equation_id >= int(size(rhs), id_kind)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Nodal load icin aktif equation ID gecersiz.")
            return
        end if
        rhs(int(equation_id) + 1) = rhs(int(equation_id) + 1) + value
    end subroutine add_active_equation_load

end module fem_linear_assembly
