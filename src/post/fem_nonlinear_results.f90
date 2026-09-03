module fem_nonlinear_results
    !! Final converged Total-Lagrangian sonucunu product result alanlarina tasir.
    !!
    !! Newton assembly yalniz aktif equation uzayini saklar. Mesnet reaksiyonu
    !! icin element ic kuvvetleri final konfigürasyonda tum nodal DOF'lara tekrar
    !! assemble edilir ve constrained DOF'larda
    !!
    !!     R_support = f_int - lambda * f_ext
    !!
    !! dengesi kullanilir. Equivalent stress, her HEX8'in sekiz Gauss noktasinda
    !! hesaplanan final Cauchy von Mises degerlerinin aritmetik ortalamasidir.
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_ids, only : INVALID_ID
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT
    use fem_element_registry, only : ELEMENT_TOTAL_LAGRANGIAN_HEX8
    use fem_total_lagrangian_hex8, only : total_lagrangian_hex8_result_t, &
        evaluate_total_lagrangian_hex8
    use fem_linear_results, only : von_mises_3d
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, &
        FEM_STATUS_SIZE_MISMATCH, FEM_STATUS_NOT_INITIALIZED
    implicit none
    private

    type, public :: nonlinear_final_results_t
        real(rk), allocatable :: displacement_xyz(:, :)
        real(rk), allocatable :: reaction_xyz(:, :)
        real(rk), allocatable :: element_equivalent_cauchy(:)
    contains
        procedure :: clear => nonlinear_final_results_clear
    end type nonlinear_final_results_t

    public :: recover_nonlinear_final_results

contains

    subroutine nonlinear_final_results_clear(this)
        class(nonlinear_final_results_t), intent(inout) :: this
        if (allocated(this%displacement_xyz)) deallocate(this%displacement_xyz)
        if (allocated(this%reaction_xyz)) deallocate(this%reaction_xyz)
        if (allocated(this%element_equivalent_cauchy)) then
            deallocate(this%element_equivalent_cauchy)
        end if
    end subroutine nonlinear_final_results_clear

    subroutine recover_nonlinear_final_results(model, active_displacement, &
                                                load_factor, results, status)
        type(model_t), intent(inout) :: model
        real(rk), intent(in) :: active_displacement(:)
        real(rk), intent(in) :: load_factor
        type(nonlinear_final_results_t), intent(inout) :: results
        type(status_t), intent(out) :: status
        type(total_lagrangian_hex8_result_t) :: element_result
        real(rk), allocatable :: internal_xyz(:, :), external_xyz(:, :)
        real(rk) :: reference_coords(3,8), local_u(3,8), vm_sum
        integer :: i, e, a, component, p
        integer(id_kind) :: dof_id, equation_id
        integer(index_kind) :: dof_pos, node_pos, constraint_pos, material_pos

        call status%clear()
        call results%clear()
        if (.not. allocated(model%mesh%nodes) .or. &
            .not. allocated(model%mesh%elements) .or. &
            .not. allocated(model%dofs%dofs)) then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED, &
                "Nonlinear final result recovery icin mesh/DOF sistemi hazir degil.")
            return
        end if
        if (load_factor < 0.0_rk .or. load_factor > 1.0_rk + &
            100.0_rk*epsilon(1.0_rk)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                "Nonlinear result recovery load factor [0,1] araliginda olmali.")
            return
        end if

        call model%renumber(status)
        if (.not. status%is_ok()) return
        if (size(active_displacement) /= &
            int(model%numbering%active_equation_count)) then
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, &
                "Final active displacement boyutu equation count ile uyusmuyor.")
            return
        end if

        allocate(results%displacement_xyz(3,size(model%mesh%nodes)))
        allocate(results%reaction_xyz(3,size(model%mesh%nodes)))
        allocate(results%element_equivalent_cauchy(size(model%mesh%elements)))
        allocate(internal_xyz(3,size(model%mesh%nodes)))
        allocate(external_xyz(3,size(model%mesh%nodes)))
        results%displacement_xyz = 0.0_rk
        results%reaction_xyz = 0.0_rk
        results%element_equivalent_cauchy = 0.0_rk
        internal_xyz = 0.0_rk
        external_xyz = 0.0_rk

        ! Active ve prescribed DOF'lari tek, tam nodal displacement alaninda
        ! birlestir. Node ID hicbir zaman array index olarak yorumlanmaz.
        do i = 1, size(model%mesh%nodes)
            do component = 1, 3
                dof_pos = model%dofs%find_by_address(model%mesh%nodes(i)%id, &
                    FIELD_ID_DISPLACEMENT, component)
                if (dof_pos == 0_index_kind) then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                        "Nonlinear result recovery nodal displacement DOF'u bulamadi.")
                    return
                end if
                dof_id = model%dofs%dofs(dof_pos)%id
                constraint_pos = model%constraints%find_position_by_dof(dof_id)
                if (constraint_pos /= 0_index_kind) then
                    results%displacement_xyz(component,i) = &
                        model%constraints%constraints(constraint_pos)%prescribed_value
                else
                    equation_id = model%numbering%equation_of(dof_id)
                    if (equation_id == INVALID_ID .or. equation_id < 0_id_kind .or. &
                        int(equation_id) + 1 > size(active_displacement)) then
                        call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                            "Nonlinear result recovery aktif equation ID'yi bulamadi.")
                        return
                    end if
                    results%displacement_xyz(component,i) = &
                        active_displacement(int(equation_id)+1)
                end if
            end do
        end do

        do e = 1, size(model%mesh%elements)
            if (model%mesh%elements(e)%formulation_id /= &
                ELEMENT_TOTAL_LAGRANGIAN_HEX8) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                    "Beta.3 nonlinear result recovery yalniz TL HEX8 destekler.")
                return
            end if
            if (.not. allocated(model%mesh%elements(e)%node_ids)) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                    "TL HEX8 final result connectivity allocate edilmemis.")
                return
            end if
            if (size(model%mesh%elements(e)%node_ids) /= 8) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                    "TL HEX8 final result sekiz node gerektirir.")
                return
            end if
            material_pos = model%materials%find_position( &
                model%mesh%elements(e)%material_id)
            if (material_pos == 0_index_kind) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                    "TL HEX8 final result icin Linear Elastic/StVK material bulunamadi.")
                return
            end if

            do a = 1, 8
                node_pos = model%mesh%find_node_position( &
                    model%mesh%elements(e)%node_ids(a))
                if (node_pos == 0_index_kind) then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                        "TL HEX8 final result connectivity Node ID'si bulunamadi.")
                    return
                end if
                reference_coords(:,a) = model%mesh%nodes(node_pos)%x
                local_u(:,a) = results%displacement_xyz(:,node_pos)
            end do

            call evaluate_total_lagrangian_hex8(reference_coords, local_u, &
                model%materials%materials(material_pos), element_result, status)
            if (.not. status%is_ok()) return

            vm_sum = 0.0_rk
            do p = 1, size(element_result%cauchy,2)
                vm_sum = vm_sum + von_mises_3d(element_result%cauchy(:,p))
            end do
            results%element_equivalent_cauchy(e) = &
                vm_sum / real(size(element_result%cauchy,2),rk)

            do a = 1, 8
                node_pos = model%mesh%find_node_position( &
                    model%mesh%elements(e)%node_ids(a))
                do component = 1, 3
                    internal_xyz(component,node_pos) = &
                        internal_xyz(component,node_pos) + &
                        element_result%internal_force(3*(a-1)+component)
                end do
            end do
        end do

        ! Reference-configuration equivalent nodal loads final load factor ile
        ! olceklenir. Constrained DOF'a dogrudan load verilmesi de kaybolmaz.
        if (allocated(model%loads%loads)) then
            do i = 1, size(model%loads%loads)
                dof_pos = model%dofs%find_position(model%loads%loads(i)%dof_id)
                if (dof_pos == 0_index_kind) then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                        "Nonlinear final reaction load DOF'u gecersiz.")
                    return
                end if
                if (model%dofs%dofs(dof_pos)%field_id /= &
                    FIELD_ID_DISPLACEMENT) then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                        "Nonlinear final reaction yalniz displacement load'u destekler.")
                    return
                end if
                node_pos = model%mesh%find_node_position( &
                    model%dofs%dofs(dof_pos)%entity_id)
                component = model%dofs%dofs(dof_pos)%component
                if (node_pos == 0_index_kind .or. component < 1 .or. component > 3) then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                        "Nonlinear final reaction load adresi gecersiz.")
                    return
                end if
                external_xyz(component,node_pos) = &
                    external_xyz(component,node_pos) + &
                    load_factor*model%loads%loads(i)%value
            end do
        end if

        if (allocated(model%constraints%constraints)) then
            do i = 1, size(model%constraints%constraints)
                dof_pos = model%dofs%find_position( &
                    model%constraints%constraints(i)%dof_id)
                if (dof_pos == 0_index_kind) then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                        "Nonlinear final reaction constraint DOF'u gecersiz.")
                    return
                end if
                if (model%dofs%dofs(dof_pos)%field_id /= &
                    FIELD_ID_DISPLACEMENT) then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                        "Nonlinear final reaction yalniz displacement constraint'i destekler.")
                    return
                end if
                node_pos = model%mesh%find_node_position( &
                    model%dofs%dofs(dof_pos)%entity_id)
                component = model%dofs%dofs(dof_pos)%component
                if (node_pos == 0_index_kind .or. component < 1 .or. component > 3) then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                        "Nonlinear final reaction constraint adresi gecersiz.")
                    return
                end if
                results%reaction_xyz(component,node_pos) = &
                    internal_xyz(component,node_pos) - external_xyz(component,node_pos)
            end do
        end if
    end subroutine recover_nonlinear_final_results

end module fem_nonlinear_results
