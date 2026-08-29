program ver_v040_001_two_bar_chain
    !! Iki seri TRUSS2 elemanindan olusan eksenel sistem.
    !!
    !! Analitik:
    !!   k = EA/L
    !!   u2 = F/k
    !!   u3 = 2F/k
    !!   R1 = -F
    !!
    !! Bu test mesh -> DOF -> constraint -> numbering -> element DOF map -> sparse
    !! graph -> assembly -> solve -> reaction zincirinin tamamini dogrular.
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_model, only : model_t
    use fem_topology, only : TOPOLOGY_BAR2
    use fem_fields, only : FIELD_ID_DISPLACEMENT
    use fem_element_dof_map, only : element_dof_map_t
    use fem_sparsity_graph, only : sparsity_graph_t
    use fem_sparse_matrix, only : csr_matrix_t, matrix_properties_t, MATRIX_SYMMETRY_SYMMETRIC, &
                                  MATRIX_DEFINITENESS_SPD_EXPECTED
    use fem_linear_assembly, only : assemble_stiffness_with_constraints, add_active_equation_load
    use fem_linear_truss, only : truss2_stiffness_3d
    use fem_linear_solver, only : linear_solver_options_t, linear_solver_statistics_t, solve_linear_system, &
                                  LINEAR_SOLVER_DENSE_REFERENCE, LINEAR_SOLVER_SPARSE_CG
    use fem_reactions, only : reaction_vector_t
    use fem_status, only : status_t
    use test_support, only : assert_true, assert_close, assert_equal_index
    implicit none

    type(model_t) :: model
    type(element_dof_map_t) :: maps(2)
    type(sparsity_graph_t) :: graph
    type(csr_matrix_t) :: stiffness
    type(matrix_properties_t) :: props
    type(linear_solver_options_t) :: options
    type(linear_solver_statistics_t) :: stats
    type(reaction_vector_t) :: reactions
    type(status_t) :: status
    integer(id_kind) :: cid, dof_id, load_dof
    integer(index_kind) :: pos
    real(rk) :: ke(6,6), fe(6), e, area, length, force, expected_u2, expected_u3
    real(rk) :: rhs(2)
    real(rk), allocatable :: solution(:), solution_cg(:)
    integer :: node_index, component, elem
    integer(id_kind), parameter :: node_ids(3) = [100_id_kind, 7_id_kind, 900_id_kind]
    integer(id_kind), parameter :: elem_ids(2) = [50_id_kind, 10_id_kind]

    e = 210.0e9_rk
    area = 1.0e-4_rk
    length = 1.0_rk
    force = 1000.0_rk
    expected_u2 = force * length / (e * area)
    expected_u3 = 2.0_rk * expected_u2

    call model%mesh%add_node(node_ids(1), [0.0_rk, 0.0_rk, 0.0_rk], status)
    call assert_true(status%is_ok(), "node1")
    call model%mesh%add_node(node_ids(2), [1.0_rk, 0.0_rk, 0.0_rk], status)
    call assert_true(status%is_ok(), "node2")
    call model%mesh%add_node(node_ids(3), [2.0_rk, 0.0_rk, 0.0_rk], status)
    call assert_true(status%is_ok(), "node3")
    call model%mesh%add_element(elem_ids(1), TOPOLOGY_BAR2, [node_ids(1),node_ids(2)], status)
    call assert_true(status%is_ok(), "element1")
    call model%mesh%add_element(elem_ids(2), TOPOLOGY_BAR2, [node_ids(2),node_ids(3)], status)
    call assert_true(status%is_ok(), "element2")
    call model%initialize_standard_registries(status)
    call assert_true(status%is_ok(), "registries")
    call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT, status)
    call assert_true(status%is_ok(), "nodal displacement dofs")

    ! Node1 XYZ tamamen sabit; Node2/Node3 YZ sabit. Yalnizca iki axial Ux aktif.
    do node_index = 1, 3
        do component = 1, 3
            if (component == 1 .and. node_index > 1) cycle
            pos = model%dofs%find_by_address(node_ids(node_index), FIELD_ID_DISPLACEMENT, component)
            dof_id = model%dofs%dofs(pos)%id
            call model%constraints%add(dof_id, 0.0_rk, cid, status)
            call assert_true(status%is_ok(), "support constraint")
        end do
    end do
    call model%renumber(status)
    call assert_true(status%is_ok(), "equation numbering")
    call assert_equal_index(model%numbering%active_equation_count, 2_index_kind, "two axial active equations")

    do elem = 1, 2
        call maps(elem)%build_nodal_field(model%mesh, elem_ids(elem), FIELD_ID_DISPLACEMENT, 3, &
            model%dofs, model%constraints, model%numbering, status)
        call assert_true(status%is_ok(), "element dof map")
    end do
    call graph%build(model%numbering%active_equation_count, maps, status)
    call assert_true(status%is_ok(), "sparsity graph")
    props%symmetry = MATRIX_SYMMETRY_SYMMETRIC
    props%definiteness = MATRIX_DEFINITENESS_SPD_EXPECTED
    call stiffness%initialize_from_graph(graph, props, status)
    call assert_true(status%is_ok(), "global stiffness init")
    rhs = 0.0_rk
    fe = 0.0_rk

    do elem = 1, 2
        call truss2_stiffness_3d(model%mesh%nodes(elem)%x, model%mesh%nodes(elem+1)%x, e, area, ke, status)
        call assert_true(status%is_ok(), "truss local stiffness")
        call assemble_stiffness_with_constraints(stiffness, rhs, maps(elem), ke, fe, status)
        call assert_true(status%is_ok(), "global stiffness scatter")
    end do
    call assert_true(stiffness%is_symmetric(1.0e-12_rk), "global stiffness symmetric")

    pos = model%dofs%find_by_address(node_ids(3), FIELD_ID_DISPLACEMENT, 1)
    load_dof = model%dofs%dofs(pos)%id
    call add_active_equation_load(rhs, model%numbering%equation_of(load_dof), force, status)
    call assert_true(status%is_ok(), "tip axial load")

    options%backend = LINEAR_SOLVER_DENSE_REFERENCE
    call solve_linear_system(stiffness, rhs, solution, options, stats, status)
    call assert_true(status%is_ok() .and. stats%converged, "dense assembled solve")
    call assert_close(solution(1), expected_u2, 1.0e-12_rk, 1.0e-10_rk, "middle node displacement")
    call assert_close(solution(2), expected_u3, 1.0e-12_rk, 1.0e-10_rk, "tip displacement")

    options%backend = LINEAR_SOLVER_SPARSE_CG
    options%cg%relative_tolerance = 1.0e-12_rk
    options%cg%absolute_tolerance = 1.0e-14_rk
    call solve_linear_system(stiffness, rhs, solution_cg, options, stats, status)
    call assert_true(status%is_ok() .and. stats%converged, "CG assembled solve")
    call assert_close(solution_cg(2), expected_u3, 1.0e-12_rk, 1.0e-10_rk, "CG equals analytic")

    call reactions%initialize(model%constraints)
    do elem = 1, 2
        call truss2_stiffness_3d(model%mesh%nodes(elem)%x, model%mesh%nodes(elem+1)%x, e, area, ke, status)
        call assert_true(status%is_ok(), "reaction local stiffness")
        call reactions%accumulate_element(maps(elem), ke, fe, solution, status)
        call assert_true(status%is_ok(), "reaction accumulation")
    end do
    pos = model%dofs%find_by_address(node_ids(1), FIELD_ID_DISPLACEMENT, 1)
    dof_id = model%dofs%dofs(pos)%id
    call assert_close(reactions%value_of(dof_id), -force, 1.0e-8_rk, 1.0e-10_rk, "support reaction")
    call assert_close(reactions%value_of(dof_id) + force, 0.0_rk, 1.0e-8_rk, 1.0e-10_rk, "global force equilibrium")

    write(*,'(A)') "PASS VER-V040-001 two-bar assembled axial system"
end program ver_v040_001_two_bar_chain
