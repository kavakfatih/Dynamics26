module fem_c_api
    !! Qt/C++ uygulama katmaninin kullandigi kararlı C ABI siniri.
    !! GUI, Fortran derived type'larina veya compiler-specific module ABI'sine
    !! dogrudan baglanmaz.
    use, intrinsic :: iso_c_binding, only : c_int, c_double, c_int64_t
    use fem_version, only : VERSION_MAJOR_VALUE => FEM_VERSION_MAJOR, &
                            VERSION_MINOR_VALUE => FEM_VERSION_MINOR, &
                            VERSION_PATCH_VALUE => FEM_VERSION_PATCH, &
                            API_VERSION_VALUE => FEM_C_API_VERSION, &
                            PROJECT_SCHEMA_VALUE => FEM_PROJECT_SCHEMA_VERSION, &
                            RESULT_SCHEMA_VALUE => FEM_RESULT_SCHEMA_VERSION
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_model, only : model_t
    use fem_fields, only : FIELD_ID_DISPLACEMENT, FIELD_ID_PRESSURE_P0
    use fem_topology, only : TOPOLOGY_BAR2, TOPOLOGY_HEX8
    use fem_element_registry, only : ELEMENT_TRUSS2, ELEMENT_SOLID_HEX8, ELEMENT_TOTAL_LAGRANGIAN_HEX8, ELEMENT_MIXED_UP_HEX8_P0
    use fem_linear_elastic_material, only : linear_elastic_material_t
    use fem_hyperelastic_material, only : hyperelastic_material_t, hyperelastic_response, &
        HYPER_NEO_HOOKEAN, HYPER_MOONEY_RIVLIN, HYPER_YEOH, HYPER_OGDEN
    use fem_sections, only : section_t, SECTION_TRUSS
    use fem_linear_static_analysis, only : linear_static_result_t, solve_linear_static
    use fem_linear_solver, only : linear_solver_options_t, LINEAR_SOLVER_DENSE_REFERENCE
    use fem_modal_analysis, only : modal_analysis_options_t, modal_result_t, solve_modal_analysis
    use fem_eigen_solver, only : EIGEN_SOLVER_DENSE_REFERENCE
    use fem_structural_mass, only : MASS_CONSISTENT
    use fem_nonlinear_solver, only : nonlinear_solver_options_t, nonlinear_static_result_t, &
        solve_nonlinear_static
    use fem_nonlinear_assembly, only : nonlinear_system_t, evaluate_nonlinear_system
    use fem_mixed_results, only : element_pressure_results_t, recover_mixed_p0_pressure
    use fem_linear_continuum, only : hex8_recover
    use fem_linear_results, only : von_mises_3d
    use fem_finite_strain_kinematics, only : second_pk_to_first_pk
    use fem_contact_types, only : contact_pair_t, contact_facet_t, CONTACT_ENFORCEMENT_PENALTY, &
        CONTACT_ENFORCEMENT_AUGMENTED_LAGRANGIAN, CONTACT_FRICTIONLESS
    use fem_status, only : status_t
    implicit none
    private

    public :: fem_api_version
    public :: fem_project_schema_version
    public :: fem_result_schema_version
    public :: fem_version_major
    public :: fem_version_minor
    public :: fem_version_patch
    public :: fem_demo_axial_bar
    public :: fem_demo_axial_modal
    public :: fem_demo_nonlinear_hex8
    public :: fem_demo_mixed_up_hex8_shear
    public :: fem_hyperelastic_validate
    public :: fem_hyperelastic_isochoric_uniaxial_preview
    public :: fem_demo_contact_hex8
    public :: fem_solve_linear_hex8_mesh

contains

    integer(c_int) function fem_api_version() bind(C, name="fem_api_version")
        fem_api_version = int(API_VERSION_VALUE, c_int)
    end function fem_api_version

    integer(c_int) function fem_project_schema_version() &
        bind(C, name="fem_project_schema_version")
        fem_project_schema_version = int(PROJECT_SCHEMA_VALUE, c_int)
    end function fem_project_schema_version

    integer(c_int) function fem_result_schema_version() &
        bind(C, name="fem_result_schema_version")
        fem_result_schema_version = int(RESULT_SCHEMA_VALUE, c_int)
    end function fem_result_schema_version

    integer(c_int) function fem_version_major() bind(C, name="fem_version_major")
        fem_version_major = int(VERSION_MAJOR_VALUE, c_int)
    end function fem_version_major

    integer(c_int) function fem_version_minor() bind(C, name="fem_version_minor")
        fem_version_minor = int(VERSION_MINOR_VALUE, c_int)
    end function fem_version_minor

    integer(c_int) function fem_version_patch() bind(C, name="fem_version_patch")
        fem_version_patch = int(VERSION_PATCH_VALUE, c_int)
    end function fem_version_patch


    integer(c_int) function fem_demo_axial_bar(young_modulus, area, length, force, &
                                               tip_displacement, axial_stress, support_reaction) &
        bind(C, name="fem_demo_axial_bar")
        !! GUI smoke/preset yolu analitik formulu dogrudan dondurmez. Bilerek
        !! gercek V0.5 model -> DOF -> sparse assembly -> solve -> reaction
        !! zincirini kurar; boylece Qt/C ABI entegrasyonu solver cekirdegini test eder.
        real(c_double), value, intent(in) :: young_modulus, area, length, force
        real(c_double), intent(out) :: tip_displacement, axial_stress, support_reaction
        type(model_t) :: model
        type(linear_elastic_material_t) :: material
        type(section_t) :: section
        type(linear_static_result_t) :: result
        type(linear_solver_options_t) :: options
        type(status_t) :: status
        integer(index_kind) :: pos
        integer(id_kind) :: constraint_id, load_id, dof_id, tip_x_dof, root_x_dof
        integer :: component
        real(rk) :: e_rk, a_rk, l_rk, f_rk

        tip_displacement = 0.0_c_double
        axial_stress = 0.0_c_double
        support_reaction = 0.0_c_double
        fem_demo_axial_bar = 10_c_int
        if (young_modulus <= 0.0_c_double .or. area <= 0.0_c_double .or. &
            length <= 0.0_c_double) return

        e_rk = real(young_modulus, rk)
        a_rk = real(area, rk)
        l_rk = real(length, rk)
        f_rk = real(force, rk)

        call model%mesh%add_node(100_id_kind, [0.0_rk, 0.0_rk, 0.0_rk], status)
        if (.not. status%is_ok()) goto 900
        call model%mesh%add_node(7_id_kind, [l_rk, 0.0_rk, 0.0_rk], status)
        if (.not. status%is_ok()) goto 900
        call model%mesh%add_element(50_id_kind, TOPOLOGY_BAR2, &
                                    [100_id_kind, 7_id_kind], status)
        if (.not. status%is_ok()) goto 900
        call model%mesh%assign_element_formulation(50_id_kind, ELEMENT_TRUSS2, status)
        if (.not. status%is_ok()) goto 900

        material = linear_elastic_material_t(id=5_id_kind, name="GUI Demo Material", &
                                             young_modulus=e_rk, poisson_ratio=0.30_rk)
        call model%materials%add(material, status)
        if (.not. status%is_ok()) goto 900
        section = section_t(id=9_id_kind, name="GUI Demo Section", &
                            kind=SECTION_TRUSS, area=a_rk)
        call model%sections%add(section, status)
        if (.not. status%is_ok()) goto 900
        call model%mesh%assign_element_properties(50_id_kind, 5_id_kind, 9_id_kind, status)
        if (.not. status%is_ok()) goto 900

        call model%initialize_standard_registries(status)
        if (.not. status%is_ok()) goto 900
        call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT, status)
        if (.not. status%is_ok()) goto 900

        do component = 1, 3
            pos = model%dofs%find_by_address(100_id_kind, FIELD_ID_DISPLACEMENT, component)
            dof_id = model%dofs%dofs(pos)%id
            if (component == 1) root_x_dof = dof_id
            call model%constraints%add(dof_id, 0.0_rk, constraint_id, status)
            if (.not. status%is_ok()) goto 900
        end do
        do component = 2, 3
            pos = model%dofs%find_by_address(7_id_kind, FIELD_ID_DISPLACEMENT, component)
            dof_id = model%dofs%dofs(pos)%id
            call model%constraints%add(dof_id, 0.0_rk, constraint_id, status)
            if (.not. status%is_ok()) goto 900
        end do
        pos = model%dofs%find_by_address(7_id_kind, FIELD_ID_DISPLACEMENT, 1)
        tip_x_dof = model%dofs%dofs(pos)%id
        call model%loads%add(tip_x_dof, f_rk, load_id, status)
        if (.not. status%is_ok()) goto 900

        options%backend = LINEAR_SOLVER_DENSE_REFERENCE
        call solve_linear_static(model, options, result, status)
        if (.not. status%is_ok()) goto 900

        tip_displacement = real(result%value_of_dof(tip_x_dof), c_double)
        axial_stress = force / area
        support_reaction = real(result%reactions%value_of(root_x_dof), c_double)
        fem_demo_axial_bar = 0_c_int
        return

900     continue
        fem_demo_axial_bar = int(status%code, c_int)
    end function fem_demo_axial_bar


    integer(c_int) function fem_demo_axial_modal(young_modulus, density, area, total_length, &
                                                 frequency_1_hz, frequency_2_hz, &
                                                 mid_mode_1, tip_mode_1, mid_mode_2, tip_mode_2) &
        bind(C, name="fem_demo_axial_modal")
        !! Iki esit TRUSS2 elemanli sabit-serbest axial model uzerinden gercek
        !! assembled K/M ve generalized eigenproblem zincirini GUI'ye acar.
        real(c_double), value, intent(in) :: young_modulus, density, area, total_length
        real(c_double), intent(out) :: frequency_1_hz, frequency_2_hz
        real(c_double), intent(out) :: mid_mode_1, tip_mode_1, mid_mode_2, tip_mode_2
        type(model_t) :: model
        type(linear_elastic_material_t) :: material
        type(section_t) :: section
        type(modal_analysis_options_t) :: options
        type(modal_result_t) :: result
        type(status_t) :: status
        integer(index_kind) :: pos
        integer(id_kind) :: constraint_id, dof_id, mid_x_dof, tip_x_dof
        integer :: node_index, component, i
        integer(id_kind), parameter :: node_ids(3)=[100_id_kind,7_id_kind,900_id_kind]
        real(rk) :: e_rk, rho_rk, a_rk, le

        frequency_1_hz=0.0_c_double; frequency_2_hz=0.0_c_double
        mid_mode_1=0.0_c_double; tip_mode_1=0.0_c_double
        mid_mode_2=0.0_c_double; tip_mode_2=0.0_c_double
        fem_demo_axial_modal=10_c_int
        if (young_modulus<=0.0_c_double .or. density<=0.0_c_double .or. area<=0.0_c_double .or. total_length<=0.0_c_double) return
        e_rk=real(young_modulus,rk); rho_rk=real(density,rk); a_rk=real(area,rk); le=0.5_rk*real(total_length,rk)

        call model%mesh%add_node(node_ids(1),[0.0_rk,0.0_rk,0.0_rk],status); if(.not.status%is_ok())goto 910
        call model%mesh%add_node(node_ids(2),[le,0.0_rk,0.0_rk],status); if(.not.status%is_ok())goto 910
        call model%mesh%add_node(node_ids(3),[2.0_rk*le,0.0_rk,0.0_rk],status); if(.not.status%is_ok())goto 910
        call model%mesh%add_element(50_id_kind,TOPOLOGY_BAR2,node_ids(1:2),status); if(.not.status%is_ok())goto 910
        call model%mesh%add_element(10_id_kind,TOPOLOGY_BAR2,node_ids(2:3),status); if(.not.status%is_ok())goto 910
        call model%mesh%assign_element_formulation(50_id_kind,ELEMENT_TRUSS2,status); if(.not.status%is_ok())goto 910
        call model%mesh%assign_element_formulation(10_id_kind,ELEMENT_TRUSS2,status); if(.not.status%is_ok())goto 910
        material=linear_elastic_material_t(id=5_id_kind,name="GUI Modal Material",young_modulus=e_rk,poisson_ratio=0.30_rk,density=rho_rk)
        call model%materials%add(material,status); if(.not.status%is_ok())goto 910
        section=section_t(id=9_id_kind,name="GUI Modal Section",kind=SECTION_TRUSS,area=a_rk)
        call model%sections%add(section,status); if(.not.status%is_ok())goto 910
        call model%mesh%assign_element_properties(50_id_kind,5_id_kind,9_id_kind,status); if(.not.status%is_ok())goto 910
        call model%mesh%assign_element_properties(10_id_kind,5_id_kind,9_id_kind,status); if(.not.status%is_ok())goto 910
        call model%initialize_standard_registries(status); if(.not.status%is_ok())goto 910
        call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status); if(.not.status%is_ok())goto 910
        do node_index=1,3
            do component=1,3
                if(node_index==1 .or. component>1)then
                    pos=model%dofs%find_by_address(node_ids(node_index),FIELD_ID_DISPLACEMENT,component)
                    dof_id=model%dofs%dofs(pos)%id
                    call model%constraints%add(dof_id,0.0_rk,constraint_id,status); if(.not.status%is_ok())goto 910
                end if
            end do
        end do
        pos=model%dofs%find_by_address(node_ids(2),FIELD_ID_DISPLACEMENT,1); mid_x_dof=model%dofs%dofs(pos)%id
        pos=model%dofs%find_by_address(node_ids(3),FIELD_ID_DISPLACEMENT,1); tip_x_dof=model%dofs%dofs(pos)%id
        options%eigen%backend=EIGEN_SOLVER_DENSE_REFERENCE; options%eigen%requested_modes=2; options%mass_kind=MASS_CONSISTENT
        call solve_modal_analysis(model,options,result,status); if(.not.status%is_ok())goto 910
        frequency_1_hz=real(result%frequencies_hz(1),c_double); frequency_2_hz=real(result%frequencies_hz(2),c_double)
        do i=1,size(result%dof_ids)
            if(result%dof_ids(i)==mid_x_dof)then
                mid_mode_1=real(result%dof_modes(i,1),c_double); mid_mode_2=real(result%dof_modes(i,2),c_double)
            else if(result%dof_ids(i)==tip_x_dof)then
                tip_mode_1=real(result%dof_modes(i,1),c_double); tip_mode_2=real(result%dof_modes(i,2),c_double)
            end if
        end do
        fem_demo_axial_modal=0_c_int
        return
910     continue
        fem_demo_axial_modal=int(status%code,c_int)
    end function fem_demo_axial_modal


    integer(c_int) function fem_demo_nonlinear_hex8(young_modulus, poisson_ratio, area, length, force, &
                                                     initial_increment, minimum_increment, maximum_increment, method, line_search_enabled, max_iterations, adaptive_stepping, &
                                                     tip_displacement, completed_load_factor, final_residual_norm, &
                                                     accepted_steps, total_iterations, cutbacks, history_capacity, &
                                                     history_count, history_attempt, history_iteration, &
                                                     history_load_factor, history_relative_residual, &
                                                     history_relative_displacement, history_alpha, history_converged) &
        bind(C, name="fem_demo_nonlinear_hex8")
        !! V0.8 nonlinear GUI/C-ABI preset'i.
        !!
        !! Tek TOTAL_LAGRANGIAN_HEX8 elemani kare kesitli bir prizma olarak kurulur.
        !! y/z displacement'lari kilitlenir, sol yuz x yonunde sabitlenir ve sag
        !! yuze esit nodal nominal kuvvet uygulanir. Cozum genel Newton solver +
        !! load stepping yolundan gecer. Iteration history C ABI'ye kopyalanir.
        real(c_double), value, intent(in) :: young_modulus, poisson_ratio, area, length, force, initial_increment, minimum_increment, maximum_increment
        integer(c_int), value, intent(in) :: method, line_search_enabled, max_iterations, adaptive_stepping, history_capacity
        real(c_double), intent(out) :: tip_displacement, completed_load_factor, final_residual_norm
        integer(c_int), intent(out) :: accepted_steps, total_iterations, cutbacks, history_count
        integer(c_int), intent(out) :: history_attempt(*), history_iteration(*), history_converged(*)
        real(c_double), intent(out) :: history_load_factor(*), history_relative_residual(*)
        real(c_double), intent(out) :: history_relative_displacement(*), history_alpha(*)
        type(model_t) :: model
        type(linear_elastic_material_t) :: material
        type(nonlinear_solver_options_t) :: options
        type(nonlinear_static_result_t) :: result
        type(status_t) :: status
        integer(id_kind), parameter :: node_ids(8)=[81_id_kind,7_id_kind,42_id_kind,5_id_kind, &
                                                     900_id_kind,11_id_kind,3_id_kind,77_id_kind]
        integer, parameter :: right_nodes(4)=[2,3,6,7], left_nodes(4)=[1,4,5,8]
        integer(id_kind) :: right_dofs(4), constraint_id, load_id, dof_id, eq
        integer(index_kind) :: pos
        real(rk) :: e_rk, nu_rk, a_rk, l_rk, f_rk, b, x(3,8), sum_u
        integer :: i, a, c, ncopy

        tip_displacement=0.0_c_double; completed_load_factor=0.0_c_double; final_residual_norm=0.0_c_double
        accepted_steps=0_c_int; total_iterations=0_c_int; cutbacks=0_c_int;history_count=0_c_int
        fem_demo_nonlinear_hex8=10_c_int
        if(young_modulus<=0.0_c_double .or. area<=0.0_c_double .or. length<=0.0_c_double .or. &
           poisson_ratio<=-1.0_c_double .or. poisson_ratio>=0.5_c_double .or. &
           initial_increment<=0.0_c_double .or. initial_increment>1.0_c_double .or. minimum_increment<=0.0_c_double .or. &
           maximum_increment<minimum_increment .or. initial_increment<minimum_increment .or. initial_increment>maximum_increment .or. &
           max_iterations<1_c_int .or. history_capacity<0_c_int) return
        e_rk=real(young_modulus,rk);nu_rk=real(poisson_ratio,rk);a_rk=real(area,rk)
        l_rk=real(length,rk);f_rk=real(force,rk);b=sqrt(a_rk)
        x(:,1)=[0._rk,0._rk,0._rk];x(:,2)=[l_rk,0._rk,0._rk]
        x(:,3)=[l_rk,b,0._rk];x(:,4)=[0._rk,b,0._rk]
        x(:,5)=[0._rk,0._rk,b];x(:,6)=[l_rk,0._rk,b]
        x(:,7)=[l_rk,b,b];x(:,8)=[0._rk,b,b]
        do a=1,8
            call model%mesh%add_node(node_ids(a),x(:,a),status);if(.not.status%is_ok())goto 920
        end do
        call model%mesh%add_element(600_id_kind,TOPOLOGY_HEX8,node_ids,status);if(.not.status%is_ok())goto 920
        call model%mesh%assign_element_formulation(600_id_kind,ELEMENT_TOTAL_LAGRANGIAN_HEX8,status);if(.not.status%is_ok())goto 920
        material=linear_elastic_material_t(id=9_id_kind,name="GUI Nonlinear StVK",young_modulus=e_rk,poisson_ratio=nu_rk)
        call model%materials%add(material,status);if(.not.status%is_ok())goto 920
        call model%mesh%assign_element_properties(600_id_kind,9_id_kind,-1_id_kind,status);if(.not.status%is_ok())goto 920
        call model%initialize_standard_registries(status);if(.not.status%is_ok())goto 920
        call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status);if(.not.status%is_ok())goto 920
        do a=1,8
            do c=2,3
                pos=model%dofs%find_by_address(node_ids(a),FIELD_ID_DISPLACEMENT,c);dof_id=model%dofs%dofs(pos)%id
                call model%constraints%add(dof_id,0._rk,constraint_id,status);if(.not.status%is_ok())goto 920
            end do
        end do
        do i=1,4
            a=left_nodes(i);pos=model%dofs%find_by_address(node_ids(a),FIELD_ID_DISPLACEMENT,1);dof_id=model%dofs%dofs(pos)%id
            call model%constraints%add(dof_id,0._rk,constraint_id,status);if(.not.status%is_ok())goto 920
        end do
        do i=1,4
            a=right_nodes(i);pos=model%dofs%find_by_address(node_ids(a),FIELD_ID_DISPLACEMENT,1);dof_id=model%dofs%dofs(pos)%id
            right_dofs(i)=dof_id
            call model%loads%add(dof_id,f_rk/4._rk,load_id,status);if(.not.status%is_ok())goto 920
        end do
        options%method=int(method)
        options%line_search=line_search_enabled/=0_c_int
        options%max_iterations=int(max_iterations)
        options%adaptive_stepping=adaptive_stepping/=0_c_int
        options%initial_load_increment=real(initial_increment,rk)
        options%minimum_load_increment=real(minimum_increment,rk)
        options%maximum_load_increment=real(maximum_increment,rk)
        options%linear%backend=LINEAR_SOLVER_DENSE_REFERENCE
        call solve_nonlinear_static(model,options,result,status);if(.not.status%is_ok())goto 920
        sum_u=0._rk
        do i=1,4
            eq=model%numbering%equation_of(right_dofs(i));sum_u=sum_u+result%active_displacement(int(eq)+1)
        end do
        tip_displacement=real(sum_u/4._rk,c_double)
        completed_load_factor=real(result%completed_load_factor,c_double)
        final_residual_norm=real(result%final_residual_norm,c_double)
        accepted_steps=int(result%accepted_steps,c_int);total_iterations=int(result%total_iterations,c_int)
        cutbacks=int(result%cutback_count,c_int)
        if(allocated(result%history))then
            ncopy=min(int(history_capacity),size(result%history));history_count=int(ncopy,c_int)
            do i=1,ncopy
                history_attempt(i)=int(result%history(i)%attempt,c_int)
                history_iteration(i)=int(result%history(i)%iteration,c_int)
                history_load_factor(i)=real(result%history(i)%load_factor,c_double)
                history_relative_residual(i)=real(result%history(i)%relative_residual,c_double)
                history_relative_displacement(i)=real(result%history(i)%relative_displacement,c_double)
                history_alpha(i)=real(result%history(i)%line_search_alpha,c_double)
                if(result%history(i)%converged)then;history_converged(i)=1_c_int;else;history_converged(i)=0_c_int;end if
            end do
        end if
        fem_demo_nonlinear_hex8=0_c_int
        return
920     continue
        fem_demo_nonlinear_hex8=int(status%code,c_int)
    end function fem_demo_nonlinear_hex8


    integer(c_int) function fem_demo_mixed_up_hex8_shear(c10, bulk_modulus, shear_gamma, recovered_shear_gamma, &
                                                          element_pressure, completed_load_factor, pressure_residual_norm, &
                                                          total_iterations) bind(C, name="fem_demo_mixed_up_hex8_shear")
        !! V0.10 GUI/C-ABI mixed u-p verification preset'i.
        !! İstenen simple-shear deformation icin hedef internal force hesaplanir,
        !! bu kuvvetler external manufactured load olarak uygulanir ve coupled
        !! Newton solver ayni equilibrium state'ini geri kazanir.
        real(c_double),value,intent(in)::c10,bulk_modulus,shear_gamma
        real(c_double),intent(out)::recovered_shear_gamma,element_pressure,completed_load_factor,pressure_residual_norm
        integer(c_int),intent(out)::total_iterations
        type(model_t)::model
        type(hyperelastic_material_t)::material
        type(nonlinear_system_t)::target_system,final_system
        type(nonlinear_solver_options_t)::options
        type(nonlinear_static_result_t)::result
        type(element_pressure_results_t)::pressure_results
        type(status_t)::status
        integer(id_kind),parameter::node_ids(8)=[17_id_kind,91_id_kind,4_id_kind,250_id_kind,31_id_kind,8_id_kind,77_id_kind,12_id_kind]
        real(rk)::x(3,8),gamma_rk
        real(rk),allocatable::target(:)
        integer(index_kind)::pos
        integer(id_kind)::dof_id,eq,constraint_id,load_id
        integer::a,i

        recovered_shear_gamma=0._c_double;element_pressure=0._c_double;completed_load_factor=0._c_double
        pressure_residual_norm=0._c_double;total_iterations=0_c_int
        fem_demo_mixed_up_hex8_shear=10_c_int
        if(c10<=0._c_double.or.bulk_modulus<=0._c_double.or.abs(shear_gamma)>1._c_double)return
        gamma_rk=real(shear_gamma,rk)
        x(:,1)=[0._rk,0._rk,0._rk];x(:,2)=[1._rk,0._rk,0._rk];x(:,3)=[1._rk,1._rk,0._rk];x(:,4)=[0._rk,1._rk,0._rk]
        x(:,5)=[0._rk,0._rk,1._rk];x(:,6)=[1._rk,0._rk,1._rk];x(:,7)=[1._rk,1._rk,1._rk];x(:,8)=[0._rk,1._rk,1._rk]
        do a=1,8
            call model%mesh%add_node(node_ids(a),x(:,a),status);if(.not.status%is_ok())goto 930
        end do
        call model%mesh%add_element(330_id_kind,TOPOLOGY_HEX8,node_ids,status);if(.not.status%is_ok())goto 930
        call model%mesh%assign_element_formulation(330_id_kind,ELEMENT_MIXED_UP_HEX8_P0,status);if(.not.status%is_ok())goto 930
        material=hyperelastic_material_t(id=44_id_kind,name='C API Mixed NH',model=HYPER_NEO_HOOKEAN, &
            bulk_modulus=real(bulk_modulus,rk),c10=real(c10,rk))
        call model%hyperelastic_materials%add(material,status);if(.not.status%is_ok())goto 930
        call model%mesh%assign_element_properties(330_id_kind,44_id_kind,-1_id_kind,status);if(.not.status%is_ok())goto 930
        call model%initialize_standard_registries(status);if(.not.status%is_ok())goto 930
        call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status);if(.not.status%is_ok())goto 930
        call model%build_element_field_dofs(FIELD_ID_PRESSURE_P0,status);if(.not.status%is_ok())goto 930
        call constrain_component(1,1);if(.not.status%is_ok())goto 930
        call constrain_component(1,2);if(.not.status%is_ok())goto 930
        call constrain_component(1,3);if(.not.status%is_ok())goto 930
        call constrain_component(2,2);if(.not.status%is_ok())goto 930
        call constrain_component(2,3);if(.not.status%is_ok())goto 930
        call constrain_component(4,3);if(.not.status%is_ok())goto 930
        call model%renumber(status);if(.not.status%is_ok())goto 930
        allocate(target(int(model%numbering%active_equation_count)));target=0._rk
        do a=1,8
            pos=model%dofs%find_by_address(node_ids(a),FIELD_ID_DISPLACEMENT,1);dof_id=model%dofs%dofs(pos)%id;eq=model%numbering%equation_of(dof_id)
            if(eq>=0_id_kind)target(int(eq)+1)=gamma_rk*x(2,a)
        end do
        pos=model%dofs%find_by_address(330_id_kind,FIELD_ID_PRESSURE_P0,1);dof_id=model%dofs%dofs(pos)%id;eq=model%numbering%equation_of(dof_id)
        if(eq>=0_id_kind)target(int(eq)+1)=0._rk
        call evaluate_nonlinear_system(model,target,target_system,status);if(.not.status%is_ok())goto 930
        do i=1,size(model%numbering%dof_ids)
            if(model%numbering%equation_ids(i)<0_id_kind)cycle
            pos=model%dofs%find_position(model%numbering%dof_ids(i))
            if(model%dofs%dofs(pos)%field_id/=FIELD_ID_DISPLACEMENT)cycle
            eq=model%numbering%equation_ids(i)
            if(abs(target_system%internal_force(int(eq)+1))>1.e-10_rk)then
                call model%loads%add(model%numbering%dof_ids(i),target_system%internal_force(int(eq)+1),load_id,status)
                if(.not.status%is_ok())goto 930
            end if
        end do
        options%initial_load_increment=0.25_rk;options%maximum_load_increment=0.5_rk;options%minimum_load_increment=1.e-5_rk
        options%max_iterations=30;options%line_search=.true.;options%linear%backend=LINEAR_SOLVER_DENSE_REFERENCE
        call solve_nonlinear_static(model,options,result,status);if(.not.status%is_ok())goto 930
        call evaluate_nonlinear_system(model,result%active_displacement,final_system,status,1._rk);if(.not.status%is_ok())goto 930
        call recover_mixed_p0_pressure(model,result%active_displacement,pressure_results,status);if(.not.status%is_ok())goto 930
        pos=model%dofs%find_by_address(node_ids(3),FIELD_ID_DISPLACEMENT,1);dof_id=model%dofs%dofs(pos)%id;eq=model%numbering%equation_of(dof_id)
        recovered_shear_gamma=real(result%active_displacement(int(eq)+1),c_double)
        if(size(pressure_results%pressure)>0)element_pressure=real(pressure_results%pressure(1),c_double)
        completed_load_factor=real(result%completed_load_factor,c_double)
        pressure_residual_norm=real(final_system%pressure_residual_norm,c_double)
        total_iterations=int(result%total_iterations,c_int)
        fem_demo_mixed_up_hex8_shear=0_c_int
        return
930     continue
        fem_demo_mixed_up_hex8_shear=int(status%code,c_int)
    contains
        subroutine constrain_component(node_index,component)
            integer,intent(in)::node_index,component
            pos=model%dofs%find_by_address(node_ids(node_index),FIELD_ID_DISPLACEMENT,component)
            dof_id=model%dofs%dofs(pos)%id
            call model%constraints%add(dof_id,0._rk,constraint_id,status)
        end subroutine constrain_component
    end function fem_demo_mixed_up_hex8_shear


    integer(c_int) function fem_hyperelastic_validate(model, bulk_modulus, parameter_count, parameters, initial_shear_modulus) &
        bind(C, name="fem_hyperelastic_validate")
        integer(c_int), value, intent(in) :: model, parameter_count
        real(c_double), value, intent(in) :: bulk_modulus
        real(c_double), intent(in) :: parameters(*)
        real(c_double), intent(out) :: initial_shear_modulus
        type(hyperelastic_material_t) :: material
        type(status_t) :: status
        initial_shear_modulus = 0.0_c_double
        call configure_hyperelastic_material(int(model), real(bulk_modulus,rk), int(parameter_count), parameters, material, status)
        if(.not.status%is_ok())then
            fem_hyperelastic_validate=int(status%code,c_int);return
        end if
        initial_shear_modulus=real(material%initial_shear_modulus(),c_double)
        fem_hyperelastic_validate=0_c_int
    end function fem_hyperelastic_validate

    integer(c_int) function fem_hyperelastic_isochoric_uniaxial_preview(model, bulk_modulus, parameter_count, parameters, &
                                                                         stretch, nominal_stress, strain_energy) &
        bind(C, name="fem_hyperelastic_isochoric_uniaxial_preview")
        integer(c_int), value, intent(in) :: model, parameter_count
        real(c_double), value, intent(in) :: bulk_modulus, stretch
        real(c_double), intent(in) :: parameters(*)
        real(c_double), intent(out) :: nominal_stress, strain_energy
        type(hyperelastic_material_t) :: material
        type(status_t) :: status
        real(rk) :: f(3,3),s(3,3),p1(3,3),d(6,6),w,lambda
        nominal_stress=0.0_c_double;strain_energy=0.0_c_double
        if(stretch<=0.0_c_double)then;fem_hyperelastic_isochoric_uniaxial_preview=10_c_int;return;end if
        call configure_hyperelastic_material(int(model), real(bulk_modulus,rk), int(parameter_count), parameters, material, status)
        if(.not.status%is_ok())then
            fem_hyperelastic_isochoric_uniaxial_preview=int(status%code,c_int);return
        end if
        lambda=real(stretch,rk);f=0.0_rk;f(1,1)=lambda;f(2,2)=lambda**(-0.5_rk);f(3,3)=f(2,2)
        call hyperelastic_response(material,f,s,d,w,status)
        if(.not.status%is_ok())then
            fem_hyperelastic_isochoric_uniaxial_preview=int(status%code,c_int);return
        end if
        call second_pk_to_first_pk(f,s,p1)
        nominal_stress=real(p1(1,1),c_double);strain_energy=real(w,c_double)
        fem_hyperelastic_isochoric_uniaxial_preview=0_c_int
    end function fem_hyperelastic_isochoric_uniaxial_preview


    integer(c_int) function fem_demo_contact_hex8(young_modulus,poisson_ratio,normal_penalty,total_force,enforcement, &
                                                   maximum_penetration,total_normal_force,active_contacts,total_iterations) &
        bind(C,name="fem_demo_contact_hex8")
        !! V0.11 rigid-master frictionless contact verification preset.
        real(c_double),value,intent(in)::young_modulus,poisson_ratio,normal_penalty,total_force
        integer(c_int),value,intent(in)::enforcement
        real(c_double),intent(out)::maximum_penetration,total_normal_force
        integer(c_int),intent(out)::active_contacts,total_iterations
        type(model_t)::model
        type(nonlinear_solver_options_t)::options
        type(nonlinear_static_result_t)::result
        type(nonlinear_system_t)::system
        type(status_t)::status
        type(linear_elastic_material_t)::material
        type(contact_pair_t)::pair
        type(contact_facet_t)::facet
        integer(id_kind),parameter::cube_ids(8)=[101_id_kind,102_id_kind,103_id_kind,104_id_kind, &
                                                  105_id_kind,106_id_kind,107_id_kind,108_id_kind]
        integer(id_kind),parameter::master_ids(4)=[201_id_kind,202_id_kind,203_id_kind,204_id_kind]
        real(rk)::x(3,8),xm(3,4)
        integer(index_kind)::pos
        integer(id_kind)::dof_id,constraint_id,load_id
        integer::a,c,enf
        maximum_penetration=0.0_c_double;total_normal_force=0.0_c_double
        active_contacts=0_c_int;total_iterations=0_c_int;fem_demo_contact_hex8=10_c_int
        if(young_modulus<=0.0_c_double.or.poisson_ratio<=-1.0_c_double.or.poisson_ratio>=0.5_c_double.or. &
           normal_penalty<=0.0_c_double.or.total_force<=0.0_c_double)return
        enf=int(enforcement)
        if(enf/=CONTACT_ENFORCEMENT_PENALTY.and.enf/=CONTACT_ENFORCEMENT_AUGMENTED_LAGRANGIAN)return
        x(:,1)=[-0.5_rk,-0.5_rk,0.0_rk];x(:,2)=[0.5_rk,-0.5_rk,0.0_rk]
        x(:,3)=[0.5_rk,0.5_rk,0.0_rk];x(:,4)=[-0.5_rk,0.5_rk,0.0_rk]
        x(:,5)=x(:,1)+[0.0_rk,0.0_rk,1.0_rk];x(:,6)=x(:,2)+[0.0_rk,0.0_rk,1.0_rk]
        x(:,7)=x(:,3)+[0.0_rk,0.0_rk,1.0_rk];x(:,8)=x(:,4)+[0.0_rk,0.0_rk,1.0_rk]
        xm=x(:,1:4)
        do a=1,8;call model%mesh%add_node(cube_ids(a),x(:,a),status);if(.not.status%is_ok())goto 970;end do
        do a=1,4;call model%mesh%add_node(master_ids(a),xm(:,a),status);if(.not.status%is_ok())goto 970;end do
        call model%mesh%add_element(700_id_kind,TOPOLOGY_HEX8,cube_ids,status);if(.not.status%is_ok())goto 970
        call model%mesh%assign_element_formulation(700_id_kind,ELEMENT_TOTAL_LAGRANGIAN_HEX8,status);if(.not.status%is_ok())goto 970
        material=linear_elastic_material_t(id=77_id_kind,name="C API Contact StVK",young_modulus=real(young_modulus,rk), &
                                            poisson_ratio=real(poisson_ratio,rk))
        call model%materials%add(material,status);if(.not.status%is_ok())goto 970
        call model%mesh%assign_element_properties(700_id_kind,77_id_kind,-1_id_kind,status);if(.not.status%is_ok())goto 970
        call model%initialize_standard_registries(status);if(.not.status%is_ok())goto 970
        call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status);if(.not.status%is_ok())goto 970
        do a=1,8
            do c=1,2
                pos=model%dofs%find_by_address(cube_ids(a),FIELD_ID_DISPLACEMENT,c);dof_id=model%dofs%dofs(pos)%id
                call model%constraints%add(dof_id,0.0_rk,constraint_id,status);if(.not.status%is_ok())goto 970
            end do
        end do
        do a=1,4
            do c=1,3
                pos=model%dofs%find_by_address(master_ids(a),FIELD_ID_DISPLACEMENT,c);dof_id=model%dofs%dofs(pos)%id
                call model%constraints%add(dof_id,0.0_rk,constraint_id,status);if(.not.status%is_ok())goto 970
            end do
        end do
        do a=5,8
            pos=model%dofs%find_by_address(cube_ids(a),FIELD_ID_DISPLACEMENT,3);dof_id=model%dofs%dofs(pos)%id
            call model%loads%add(dof_id,-real(total_force,rk)/4.0_rk,load_id,status);if(.not.status%is_ok())goto 970
        end do
        facet=contact_facet_t(id=900_id_kind,node_ids=master_ids)
        pair%id=901_id_kind;allocate(pair%slave_node_ids(4));pair%slave_node_ids=cube_ids(1:4)
        allocate(pair%master_facets(1));pair%master_facets=[facet]
        pair%enforcement=enf;pair%friction_model=CONTACT_FRICTIONLESS
        pair%normal_penalty=real(normal_penalty,rk);pair%search_distance=0.05_rk;pair%activation_tolerance=1.0e-12_rk
        call model%contacts%add(pair,status);if(.not.status%is_ok())goto 970
        options%linear%backend=LINEAR_SOLVER_DENSE_REFERENCE
        options%initial_load_increment=0.25_rk;options%minimum_load_increment=1.0e-5_rk;options%maximum_load_increment=0.5_rk
        options%max_iterations=25;options%line_search=.true.;options%adaptive_stepping=.true.
        call solve_nonlinear_static(model,options,result,status);if(.not.status%is_ok())goto 970
        call evaluate_nonlinear_system(model,result%active_displacement,system,status,1.0_rk);if(.not.status%is_ok())goto 970
        maximum_penetration=real(system%maximum_penetration,c_double)
        total_normal_force=real(system%total_contact_normal_force,c_double)
        active_contacts=int(system%active_contact_count,c_int);total_iterations=int(result%total_iterations,c_int)
        fem_demo_contact_hex8=0_c_int;return
970     continue
        fem_demo_contact_hex8=int(status%code,c_int)
    end function fem_demo_contact_hex8


    subroutine configure_hyperelastic_material(model,bulk_modulus,parameter_count,parameters,material,status)
        integer,intent(in)::model,parameter_count
        real(rk),intent(in)::bulk_modulus
        real(c_double),intent(in)::parameters(*)
        type(hyperelastic_material_t),intent(out)::material
        type(status_t),intent(out)::status
        integer::p,terms
        material=hyperelastic_material_t(id=909_id_kind,name="C API Hyperelastic",model=model,bulk_modulus=bulk_modulus)
        select case(model)
        case(HYPER_NEO_HOOKEAN)
            if(parameter_count/=1)then;call status%set_error(10,"Neo-Hookean C API 1 parameter bekler.");return;end if
            material%c10=real(parameters(1),rk)
        case(HYPER_MOONEY_RIVLIN)
            if(parameter_count/=2)then;call status%set_error(10,"Mooney-Rivlin C API 2 parameter bekler.");return;end if
            material%c10=real(parameters(1),rk);material%c01=real(parameters(2),rk)
        case(HYPER_YEOH)
            if(parameter_count/=3)then;call status%set_error(10,"Yeoh C API 3 parameter bekler.");return;end if
            material%c10=real(parameters(1),rk);material%c20=real(parameters(2),rk);material%c30=real(parameters(3),rk)
        case(HYPER_OGDEN)
            if(parameter_count<2.or.parameter_count>6.or.mod(parameter_count,2)/=0)then
                call status%set_error(10,"Ogden C API mu/alpha ciftleri halinde 2,4 veya 6 parameter bekler.");return
            end if
            terms=parameter_count/2;material%ogden_term_count=terms
            do p=1,terms
                material%ogden_mu(p)=real(parameters(2*p-1),rk);material%ogden_alpha(p)=real(parameters(2*p),rk)
            end do
        case default
            call status%set_error(10,"C API hyperelastic model ID gecersiz.");return
        end select
        call material%validate(status)
    end subroutine configure_hyperelastic_material


    integer(c_int) function fem_solve_linear_hex8_mesh(node_count, node_ids, coordinates_xyz, &
                                                       element_count, element_ids, connectivity8, &
                                                       young_modulus, poisson_ratio, &
                                                       constraint_count, constraint_node_ids, constraint_components, constraint_values, &
                                                       load_count, load_node_ids, load_components, load_values, &
                                                       displacements_xyz, reactions_xyz, element_von_mises) &
        bind(C, name="fem_solve_linear_hex8_mesh")
        !! V1.0 arbitrary lineer HEX8 mesh C ABI.
        !!
        !! Bu yordam mesher/preprocessor katmaninin urettigi node/connectivity verisini
        !! preset geometriye cevirmeden gercek model -> DOF -> sparse assembly -> solve
        !! zincirine tasir. C dizileri node/element sirasi ile sonuc dizilerini eslestirir;
        !! fiziksel Node/Element ID'ler array index olarak kullanilmaz.
        integer(c_int), value, intent(in) :: node_count, element_count, constraint_count, load_count
        integer(c_int64_t), intent(in) :: node_ids(*), element_ids(*), connectivity8(*)
        real(c_double), intent(in) :: coordinates_xyz(*)
        real(c_double), value, intent(in) :: young_modulus, poisson_ratio
        integer(c_int64_t), intent(in) :: constraint_node_ids(*), load_node_ids(*)
        integer(c_int), intent(in) :: constraint_components(*), load_components(*)
        real(c_double), intent(in) :: constraint_values(*), load_values(*)
        real(c_double), intent(out) :: displacements_xyz(*), reactions_xyz(*), element_von_mises(*)
        type(model_t) :: model
        type(linear_elastic_material_t) :: material
        type(linear_static_result_t) :: result
        type(linear_solver_options_t) :: options
        type(status_t) :: status
        integer(id_kind), allocatable :: conn(:)
        integer(id_kind) :: node_id, element_id, dof_id, constraint_id, load_id
        integer(index_kind) :: pos, npos
        real(rk) :: coords(3), x(3,8), u(24), vm_sum
        real(rk), allocatable :: strain(:,:), stress(:,:), points(:,:)
        integer :: i, e, a, c, pidx

        fem_solve_linear_hex8_mesh = 10_c_int
        if (node_count < 8_c_int .or. element_count < 1_c_int .or. &
            young_modulus <= 0.0_c_double .or. poisson_ratio <= -1.0_c_double .or. poisson_ratio >= 0.5_c_double .or. &
            constraint_count < 0_c_int .or. load_count < 0_c_int) return

        do i=1,int(node_count)
            node_id=int(node_ids(i),id_kind)
            coords=[real(coordinates_xyz(3*i-2),rk),real(coordinates_xyz(3*i-1),rk),real(coordinates_xyz(3*i),rk)]
            call model%mesh%add_node(node_id,coords,status); if(.not.status%is_ok()) goto 990
        end do
        allocate(conn(8))
        do e=1,int(element_count)
            element_id=int(element_ids(e),id_kind)
            do a=1,8
                conn(a)=int(connectivity8(8*(e-1)+a),id_kind)
            end do
            call model%mesh%add_element(element_id,TOPOLOGY_HEX8,conn,status); if(.not.status%is_ok()) goto 990
            call model%mesh%assign_element_formulation(element_id,ELEMENT_SOLID_HEX8,status); if(.not.status%is_ok()) goto 990
        end do
        material=linear_elastic_material_t(id=1_id_kind,name="V1.0 PrePost Linear Material", &
            young_modulus=real(young_modulus,rk),poisson_ratio=real(poisson_ratio,rk))
        call model%materials%add(material,status); if(.not.status%is_ok()) goto 990
        do e=1,int(element_count)
            call model%mesh%assign_element_properties(int(element_ids(e),id_kind),1_id_kind,-1_id_kind,status)
            if(.not.status%is_ok()) goto 990
        end do
        call model%initialize_standard_registries(status); if(.not.status%is_ok()) goto 990
        call model%build_nodal_field_dofs(FIELD_ID_DISPLACEMENT,status); if(.not.status%is_ok()) goto 990

        do i=1,int(constraint_count)
            c=int(constraint_components(i))
            if(c<1 .or. c>3) goto 990
            pos=model%dofs%find_by_address(int(constraint_node_ids(i),id_kind),FIELD_ID_DISPLACEMENT,c)
            if(pos==0_index_kind) goto 990
            dof_id=model%dofs%dofs(pos)%id
            call model%constraints%add(dof_id,real(constraint_values(i),rk),constraint_id,status); if(.not.status%is_ok()) goto 990
        end do
        do i=1,int(load_count)
            c=int(load_components(i))
            if(c<1 .or. c>3) goto 990
            pos=model%dofs%find_by_address(int(load_node_ids(i),id_kind),FIELD_ID_DISPLACEMENT,c)
            if(pos==0_index_kind) goto 990
            dof_id=model%dofs%dofs(pos)%id
            call model%loads%add(dof_id,real(load_values(i),rk),load_id,status); if(.not.status%is_ok()) goto 990
        end do
        options%backend=LINEAR_SOLVER_DENSE_REFERENCE
        call solve_linear_static(model,options,result,status); if(.not.status%is_ok()) goto 990

        do i=1,int(node_count)
            node_id=int(node_ids(i),id_kind)
            do c=1,3
                pos=model%dofs%find_by_address(node_id,FIELD_ID_DISPLACEMENT,c)
                if(pos==0_index_kind) goto 990
                dof_id=model%dofs%dofs(pos)%id
                displacements_xyz(3*i-3+c)=real(result%value_of_dof(dof_id),c_double)
                reactions_xyz(3*i-3+c)=real(result%reactions%value_of(dof_id),c_double)
            end do
        end do

        do e=1,int(element_count)
            element_id=int(element_ids(e),id_kind)
            do a=1,8
                npos=model%mesh%find_node_position(int(connectivity8(8*(e-1)+a),id_kind))
                if(npos==0_index_kind) goto 990
                x(:,a)=model%mesh%nodes(npos)%x
                do c=1,3
                    pos=model%dofs%find_by_address(model%mesh%nodes(npos)%id,FIELD_ID_DISPLACEMENT,c)
                    dof_id=model%dofs%dofs(pos)%id
                    u(3*a-3+c)=result%value_of_dof(dof_id)
                end do
            end do
            call hex8_recover(x,material,u,strain,stress,points,status); if(.not.status%is_ok()) goto 990
            vm_sum=0.0_rk
            do pidx=1,size(stress,2)
                vm_sum=vm_sum+von_mises_3d(stress(:,pidx))
            end do
            element_von_mises(e)=real(vm_sum/real(size(stress,2),rk),c_double)
            deallocate(strain,stress,points)
        end do
        fem_solve_linear_hex8_mesh=0_c_int
        return
990     continue
        if(allocated(strain)) deallocate(strain)
        if(allocated(stress)) deallocate(stress)
        if(allocated(points)) deallocate(points)
        if(status%code/=0) then
            fem_solve_linear_hex8_mesh=int(status%code,c_int)
        else
            fem_solve_linear_hex8_mesh=10_c_int
        end if
    end function fem_solve_linear_hex8_mesh

end module fem_c_api
