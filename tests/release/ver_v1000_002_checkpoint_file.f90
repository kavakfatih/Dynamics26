program ver_v1000_002_checkpoint_file
    use fem_kinds, only : rk, id_kind
    use fem_model, only : model_t
    use fem_nonlinear_solver, only : nonlinear_solver_options_t, nonlinear_static_result_t, &
        nonlinear_checkpoint_t, solve_nonlinear_static
    use fem_checkpoint_io, only : write_nonlinear_checkpoint, read_nonlinear_checkpoint, &
        FEM_CHECKPOINT_SCHEMA_VERSION
    use fem_status, only : status_t, FEM_STATUS_NUMERICAL_FAILURE
    use nonlinear_test_models, only : build_uniaxial_stvk_hex8
    use test_support, only : assert_true, assert_close
    implicit none

    type(model_t) :: model
    type(nonlinear_solver_options_t) :: options
    type(nonlinear_static_result_t) :: partial, restarted
    type(nonlinear_checkpoint_t) :: checkpoint, loaded
    type(status_t) :: status
    integer(id_kind) :: dofs(4), eq
    real(rk) :: e, nu, l, area, stretch, g, lam, e11, force, expected
    integer :: i, unit
    character(len=*), parameter :: path = 'v1000_checkpoint.chk'
    character(len=*), parameter :: corrupt_path = 'v1000_checkpoint_corrupt.chk'
    character(len=*), parameter :: checksum_path = 'v1000_checkpoint_bad_checksum.chk'

    call assert_true(FEM_CHECKPOINT_SCHEMA_VERSION == 1, 'checkpoint schema V1')
    e=5.0e6_rk; nu=.27_rk; l=1.0_rk; area=1.0_rk; stretch=1.12_rk
    g=e/(2.0_rk*(1.0_rk+nu)); lam=e*nu/((1.0_rk+nu)*(1.0_rk-2.0_rk*nu))
    e11=.5_rk*(stretch**2-1.0_rk); force=area*stretch*(lam+2.0_rk*g)*e11
    expected=(stretch-1.0_rk)*l

    call build_uniaxial_stvk_hex8(model,e,nu,l,area,force,dofs,status)
    call assert_true(status%is_ok(),'V1 checkpoint model')
    options%initial_load_increment=.25_rk; options%minimum_load_increment=.01_rk
    options%maximum_load_increment=.25_rk; options%adaptive_stepping=.false.; options%max_step_attempts=1
    call solve_nonlinear_static(model,options,partial,status)
    call assert_true(status%code==FEM_STATUS_NUMERICAL_FAILURE,'partial run stops at first accepted step')
    call checkpoint%capture(partial,status); call assert_true(status%is_ok(),'checkpoint capture')

    call write_nonlinear_checkpoint(path,checkpoint,status)
    call assert_true(status%is_ok(),'checkpoint file write')
    call read_nonlinear_checkpoint(path,loaded,status)
    call assert_true(status%is_ok(),'checkpoint file read')
    call assert_close(loaded%load_factor,checkpoint%load_factor,0.0_rk,0.0_rk,'checkpoint exact load factor')
    call assert_true(loaded%accepted_steps==checkpoint%accepted_steps,'checkpoint accepted steps')
    call assert_true(size(loaded%active_displacement)==size(checkpoint%active_displacement),'checkpoint DOF count')
    do i=1,size(loaded%active_displacement)
        call assert_close(loaded%active_displacement(i),checkpoint%active_displacement(i),0.0_rk,0.0_rk,'checkpoint exact displacement bits')
    end do

    options%max_step_attempts=20
    call solve_nonlinear_static(model,options,restarted,status,loaded)
    call assert_true(status%is_ok(),'disk checkpoint restart solve')
    call assert_true(restarted%converged,'disk checkpoint reaches final load')
    do i=1,4
        eq=model%numbering%equation_of(dofs(i))
        call assert_close(restarted%active_displacement(int(eq)+1),expected,2.e-9_rk,2.e-9_rk,'disk restart displacement')
    end do

    open(newunit=unit,file=corrupt_path,status='replace',action='write')
    write(unit,'(A)') 'FEMCAE_NONLINEAR_CHECKPOINT'
    write(unit,'(A)') '1'
    write(unit,'(A)') '0000000000000000'
    write(unit,'(A)') '1'
    write(unit,'(A)') '4'
    write(unit,'(A)') '0000000000000000' ! intentionally truncated; checksum/END absent
    close(unit)
    call read_nonlinear_checkpoint(corrupt_path,loaded,status)
    call assert_true(.not.status%is_ok(),'truncated checkpoint rejected')
    call assert_true(.not.allocated(loaded%active_displacement),'corrupt checkpoint leaves no partial state')

    ! Structurally complete but intentionally wrong checksum.
    open(newunit=unit,file=checksum_path,status='replace',action='write')
    write(unit,'(A)') 'FEMCAE_NONLINEAR_CHECKPOINT'
    write(unit,'(A)') '1'
    write(unit,'(A)') '0000000000000000'
    write(unit,'(A)') '0'
    write(unit,'(A)') '1'
    write(unit,'(A)') '0000000000000000'
    write(unit,'(A)') '0000000000000000'
    write(unit,'(A)') 'END'
    close(unit)
    call read_nonlinear_checkpoint(checksum_path,loaded,status)
    call assert_true(.not.status%is_ok(),'checkpoint checksum corruption rejected')
    call assert_true(.not.allocated(loaded%active_displacement),'bad checksum leaves no partial state')

    open(newunit=unit,file=path,status='old'); close(unit,status='delete')
    open(newunit=unit,file=corrupt_path,status='old'); close(unit,status='delete')
    open(newunit=unit,file=checksum_path,status='old'); close(unit,status='delete')
end program ver_v1000_002_checkpoint_file
