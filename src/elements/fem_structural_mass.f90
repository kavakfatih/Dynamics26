module fem_structural_mass
    !! V0.6.0 structural mass matrix library.
    !!
    !! Consistent mass keeps the interpolation coupling produced by \int rho N^T N dV.
    !! Lumped mass is obtained by row-sum for continuum/truss families; the 2B beam
    !! uses a positive diagonal engineering lump that preserves translational mass.
    use fem_kinds, only : rk
    use fem_topology, only : TOPOLOGY_QUAD4, TOPOLOGY_HEX8
    use fem_quadrature, only : quadrature_rule_t, standard_quadrature_rule
    use fem_element_kernel, only : element_geometry_point_t, evaluate_geometry_point
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_NUMERICAL_FAILURE
    implicit none
    private

    integer, parameter, public :: MASS_CONSISTENT = 1
    integer, parameter, public :: MASS_LUMPED = 2

    public :: truss2_mass_3d, beam2_frame_mass_2d
    public :: quad4_mass_plane, quad4_mass_axisymmetric, hex8_mass
    public :: lump_by_row_sum

contains

    subroutine validate_mass_inputs(density, mass_kind, status)
        real(rk), intent(in) :: density
        integer, intent(in) :: mass_kind
        type(status_t), intent(out) :: status
        call status%clear()
        if (density <= 0.0_rk) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Modal mass icin density pozitif olmali.")
            return
        end if
        if (mass_kind /= MASS_CONSISTENT .and. mass_kind /= MASS_LUMPED) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "Bilinmeyen mass matrix turu.")
        end if
    end subroutine validate_mass_inputs

    subroutine truss2_mass_3d(x1, x2, density, area, mass_kind, me, status)
        real(rk), intent(in) :: x1(3), x2(3), density, area
        integer, intent(in) :: mass_kind
        real(rk), intent(out) :: me(6,6)
        type(status_t), intent(out) :: status
        real(rk) :: length, coefficient
        integer :: c

        me = 0.0_rk
        call validate_mass_inputs(density, mass_kind, status)
        if (.not. status%is_ok()) return
        if (area <= 0.0_rk) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "TRUSS2 mass icin area pozitif olmali.")
            return
        end if
        length = sqrt(dot_product(x2-x1, x2-x1))
        if (length <= sqrt(tiny(1.0_rk))) then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, "TRUSS2 mass icin element uzunlugu sifira cok yakin.")
            return
        end if

        if (mass_kind == MASS_CONSISTENT) then
            coefficient = density * area * length / 6.0_rk
            do c = 1, 3
                me(c,c) = 2.0_rk*coefficient
                me(c,c+3) = coefficient
                me(c+3,c) = coefficient
                me(c+3,c+3) = 2.0_rk*coefficient
            end do
        else
            coefficient = density * area * length / 2.0_rk
            do c = 1, 3
                me(c,c) = coefficient
                me(c+3,c+3) = coefficient
            end do
        end if
    end subroutine truss2_mass_3d

    subroutine beam2_frame_mass_2d(x1, x2, density, area, mass_kind, me, status)
        !! Euler-Bernoulli 2B frame DOF order: [u1,v1,rz1,u2,v2,rz2].
        real(rk), intent(in) :: x1(2), x2(2), density, area
        integer, intent(in) :: mass_kind
        real(rk), intent(out) :: me(6,6)
        type(status_t), intent(out) :: status
        real(rk) :: dx, dy, length, c, s, total_mass
        real(rk) :: local(6,6), transform(6,6), factor

        me = 0.0_rk
        call validate_mass_inputs(density, mass_kind, status)
        if (.not. status%is_ok()) return
        if (area <= 0.0_rk) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "BEAM2 mass icin area pozitif olmali.")
            return
        end if
        dx = x2(1)-x1(1); dy = x2(2)-x1(2)
        length = sqrt(dx*dx + dy*dy)
        if (length <= sqrt(tiny(1.0_rk))) then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, "BEAM2 mass icin element uzunlugu sifira cok yakin.")
            return
        end if
        c = dx/length; s = dy/length
        total_mass = density*area*length
        local = 0.0_rk

        if (mass_kind == MASS_CONSISTENT) then
            ! Axial consistent part m/6 [2 1; 1 2]
            local(1,1)=total_mass/3.0_rk; local(1,4)=total_mass/6.0_rk
            local(4,1)=local(1,4); local(4,4)=total_mass/3.0_rk
            ! Euler-Bernoulli bending consistent mass m/420.
            factor = total_mass/420.0_rk
            local(2,2)=156.0_rk*factor
            local(2,3)=22.0_rk*length*factor
            local(2,5)=54.0_rk*factor
            local(2,6)=-13.0_rk*length*factor
            local(3,2)=local(2,3)
            local(3,3)=4.0_rk*length*length*factor
            local(3,5)=13.0_rk*length*factor
            local(3,6)=-3.0_rk*length*length*factor
            local(5,2)=local(2,5)
            local(5,3)=local(3,5)
            local(5,5)=156.0_rk*factor
            local(5,6)=-22.0_rk*length*factor
            local(6,2)=local(2,6)
            local(6,3)=local(3,6)
            local(6,5)=local(5,6)
            local(6,6)=4.0_rk*length*length*factor
        else
            ! Positive diagonal engineering lump. Translational inertia in x/y is
            ! m/2 per node; rz gets m L^2 / 24 per node to avoid a singular beam mass.
            local(1,1)=0.5_rk*total_mass; local(2,2)=0.5_rk*total_mass
            local(4,4)=0.5_rk*total_mass; local(5,5)=0.5_rk*total_mass
            local(3,3)=total_mass*length*length/24.0_rk
            local(6,6)=local(3,3)
        end if

        transform = 0.0_rk
        transform(1,1)=c; transform(1,2)=s
        transform(2,1)=-s; transform(2,2)=c; transform(3,3)=1.0_rk
        transform(4,4)=c; transform(4,5)=s
        transform(5,4)=-s; transform(5,5)=c; transform(6,6)=1.0_rk
        me = matmul(transpose(transform), matmul(local, transform))
    end subroutine beam2_frame_mass_2d

    subroutine quad4_mass_plane(coords, density, thickness, mass_kind, me, status)
        real(rk), intent(in) :: coords(2,4), density, thickness
        integer, intent(in) :: mass_kind
        real(rk), intent(out) :: me(8,8)
        type(status_t), intent(out) :: status
        type(quadrature_rule_t) :: rule
        type(element_geometry_point_t) :: gp
        integer :: p, a, b, c, ia, ib
        real(rk) :: coeff

        me = 0.0_rk
        call validate_mass_inputs(density, mass_kind, status)
        if (.not. status%is_ok()) return
        if (thickness <= 0.0_rk) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, "QUAD4 mass icin thickness pozitif olmali.")
            return
        end if
        call standard_quadrature_rule(TOPOLOGY_QUAD4, rule, status)
        if (.not. status%is_ok()) return
        do p=1,rule%point_count
            call evaluate_geometry_point(TOPOLOGY_QUAD4, coords, rule%points(:,p), rule%weights(p), gp, status)
            if (.not. status%is_ok()) return
            coeff=density*thickness*gp%integration_measure
            do a=1,4
                do b=1,4
                    do c=1,2
                        ia=2*(a-1)+c; ib=2*(b-1)+c
                        me(ia,ib)=me(ia,ib)+coeff*gp%shape(a)*gp%shape(b)
                    end do
                end do
            end do
        end do
        if (mass_kind==MASS_LUMPED) call lump_by_row_sum(me)
    end subroutine quad4_mass_plane

    subroutine quad4_mass_axisymmetric(coords, density, mass_kind, me, status)
        real(rk), intent(in) :: coords(2,4), density
        integer, intent(in) :: mass_kind
        real(rk), intent(out) :: me(8,8)
        type(status_t), intent(out) :: status
        type(quadrature_rule_t) :: rule
        type(element_geometry_point_t) :: gp
        integer :: p, a, b, c, ia, ib
        real(rk) :: coeff

        me=0.0_rk
        call validate_mass_inputs(density,mass_kind,status); if (.not.status%is_ok()) return
        call standard_quadrature_rule(TOPOLOGY_QUAD4,rule,status); if (.not.status%is_ok()) return
        do p=1,rule%point_count
            call evaluate_geometry_point(TOPOLOGY_QUAD4,coords,rule%points(:,p),rule%weights(p),gp,status,.true.)
            if (.not.status%is_ok()) return
            coeff=density*gp%integration_measure
            do a=1,4; do b=1,4; do c=1,2
                ia=2*(a-1)+c; ib=2*(b-1)+c
                me(ia,ib)=me(ia,ib)+coeff*gp%shape(a)*gp%shape(b)
            end do; end do; end do
        end do
        if (mass_kind==MASS_LUMPED) call lump_by_row_sum(me)
    end subroutine quad4_mass_axisymmetric

    subroutine hex8_mass(coords, density, mass_kind, me, status)
        real(rk), intent(in) :: coords(3,8), density
        integer, intent(in) :: mass_kind
        real(rk), intent(out) :: me(24,24)
        type(status_t), intent(out) :: status
        type(quadrature_rule_t) :: rule
        type(element_geometry_point_t) :: gp
        integer :: p, a, b, c, ia, ib
        real(rk) :: coeff

        me=0.0_rk
        call validate_mass_inputs(density,mass_kind,status); if (.not.status%is_ok()) return
        call standard_quadrature_rule(TOPOLOGY_HEX8,rule,status); if (.not.status%is_ok()) return
        do p=1,rule%point_count
            call evaluate_geometry_point(TOPOLOGY_HEX8,coords,rule%points(:,p),rule%weights(p),gp,status)
            if (.not.status%is_ok()) return
            coeff=density*gp%integration_measure
            do a=1,8; do b=1,8; do c=1,3
                ia=3*(a-1)+c; ib=3*(b-1)+c
                me(ia,ib)=me(ia,ib)+coeff*gp%shape(a)*gp%shape(b)
            end do; end do; end do
        end do
        if (mass_kind==MASS_LUMPED) call lump_by_row_sum(me)
    end subroutine hex8_mass

    subroutine lump_by_row_sum(matrix)
        real(rk), intent(inout) :: matrix(:,:)
        real(rk), allocatable :: diagonal(:)
        integer :: i
        allocate(diagonal(size(matrix,1)))
        diagonal=sum(matrix,dim=2)
        matrix=0.0_rk
        do i=1,size(diagonal)
            matrix(i,i)=diagonal(i)
        end do
    end subroutine lump_by_row_sum

end module fem_structural_mass
