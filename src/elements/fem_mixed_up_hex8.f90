module fem_mixed_up_hex8
    !! V0.10 mixed displacement-pressure HEX8/Q1-P0 baseline.
    !!
    !! Local unknown sirasi:
    !!   q_e = [u_1x,u_1y,u_1z,...,u_8x,u_8y,u_8z,p_e]
    !!
    !! Perturbed-Lagrangian referans enerji yogunlugu:
    !!   Psi(u,p) = W_iso(Fbar) + p (J-1) - p^2/(2 K)
    !!
    !! Burada p positive-tension isaret sozlesmesindedir. Pressure P0 oldugu
    !! icin element icinde sabittir. Bu baseline, V0.10 locking benchmark ve
    !! coupled block-assembly dogrulamasi icindir; genel inf-sup stabilitesi
    !! iddia edilmez.
    use fem_kinds, only : rk
    use fem_topology, only : TOPOLOGY_HEX8
    use fem_quadrature, only : quadrature_rule_t, standard_quadrature_rule
    use fem_element_kernel, only : element_geometry_point_t, evaluate_geometry_point
    use fem_finite_strain_kinematics, only : deformation_gradient_from_coordinates
    use fem_hyperelastic_material, only : hyperelastic_material_t, hyperelastic_isochoric_response
    use fem_total_lagrangian_hex8, only : build_green_lagrange_b_matrix, build_geometric_stiffness_block
    use fem_matrix_math, only : determinant_3x3
    use fem_tensor_notation, only : stress_tensor_to_voigt, strain_voigt_to_tensor
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_NUMERICAL_FAILURE
    implicit none
    private

    type, public :: mixed_up_hex8_result_t
        real(rk) :: internal_force(25) = 0.0_rk
        real(rk) :: tangent(25,25) = 0.0_rk
        real(rk) :: kuu(24,24) = 0.0_rk
        real(rk) :: kup(24,1) = 0.0_rk
        real(rk) :: kpu(1,24) = 0.0_rk
        real(rk) :: kpp(1,1) = 0.0_rk
        real(rk) :: isochoric_energy = 0.0_rk
        real(rk) :: mixed_energy = 0.0_rk
        real(rk) :: minimum_j = huge(1.0_rk)
        real(rk) :: integrated_volume_change = 0.0_rk
        real(rk) :: reference_volume = 0.0_rk
    end type mixed_up_hex8_result_t

    public :: evaluate_mixed_up_hex8
    public :: pressure_second_pk_and_tangent
    public :: determinant_gradient_wrt_u

contains

    subroutine evaluate_mixed_up_hex8(reference_coords, displacement, pressure, material, result, status)
        real(rk), intent(in) :: reference_coords(3,8)
        real(rk), intent(in) :: displacement(3,8)
        real(rk), intent(in) :: pressure
        type(hyperelastic_material_t), intent(in) :: material
        type(mixed_up_hex8_result_t), intent(out) :: result
        type(status_t), intent(out) :: status
        type(quadrature_rule_t) :: rule
        type(element_geometry_point_t) :: gp
        real(rk) :: current(3,8), f(3,3), j
        real(rk) :: s_iso(3,3), c_iso(6,6), w_iso
        real(rk) :: s_p(3,3), c_p(6,6), s_total(3,3), c_total(6,6)
        real(rk) :: b(6,24), kg(24,24), svec(6), sp_unit(3,3), sp_unit_vec(6)
        real(rk) :: gj(24), dv0, constraint_value
        integer :: q

        call status%clear()
        result = mixed_up_hex8_result_t()
        call material%validate(status)
        if(.not.status%is_ok())return
        if(material%bulk_modulus<=0.0_rk)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Mixed u-p bulk modulus K pozitif olmali.")
            return
        end if

        current = reference_coords + displacement
        call standard_quadrature_rule(TOPOLOGY_HEX8, rule, status)
        if(.not.status%is_ok())return

        do q=1,rule%point_count
            call evaluate_geometry_point(TOPOLOGY_HEX8,reference_coords,rule%points(:,q),rule%weights(q),gp,status)
            if(.not.status%is_ok())return
            call deformation_gradient_from_coordinates(current,gp%dshape_dphysical,f,j,status)
            if(.not.status%is_ok())return
            if(j<=0.0_rk)then
                call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"Mixed u-p HEX8 J>0 gerektirir.")
                return
            end if

            call hyperelastic_isochoric_response(material,f,s_iso,c_iso,w_iso,status)
            if(.not.status%is_ok())return
            call pressure_second_pk_and_tangent(f,pressure,s_p,c_p,status)
            if(.not.status%is_ok())return
            s_total=s_iso+s_p
            c_total=c_iso+c_p

            call build_green_lagrange_b_matrix(f,gp%dshape_dphysical,b,status)
            if(.not.status%is_ok())return
            call build_geometric_stiffness_block(s_total,gp%dshape_dphysical,kg,status)
            if(.not.status%is_ok())return
            call determinant_gradient_wrt_u(f,j,gp%dshape_dphysical,gj,status)
            if(.not.status%is_ok())return

            call stress_tensor_to_voigt(s_total,svec)
            call unit_pressure_second_pk(f,j,sp_unit,status)
            if(.not.status%is_ok())return
            call stress_tensor_to_voigt(sp_unit,sp_unit_vec)
            dv0=gp%integration_measure

            result%internal_force(1:24)=result%internal_force(1:24)+matmul(transpose(b),svec)*dv0
            constraint_value=(j-1.0_rk)-pressure/material%bulk_modulus
            result%internal_force(25)=result%internal_force(25)+constraint_value*dv0

            result%kuu=result%kuu+(matmul(transpose(b),matmul(c_total,b))+kg)*dv0
            result%kup(:,1)=result%kup(:,1)+matmul(transpose(b),sp_unit_vec)*dv0
            result%kpu(1,:)=result%kpu(1,:)+gj*dv0
            result%kpp(1,1)=result%kpp(1,1)-dv0/material%bulk_modulus

            result%isochoric_energy=result%isochoric_energy+w_iso*dv0
            result%mixed_energy=result%mixed_energy+(w_iso+pressure*(j-1.0_rk)-0.5_rk*pressure*pressure/material%bulk_modulus)*dv0
            result%minimum_j=min(result%minimum_j,j)
            result%integrated_volume_change=result%integrated_volume_change+(j-1.0_rk)*dv0
            result%reference_volume=result%reference_volume+dv0
        end do

        result%tangent(1:24,1:24)=result%kuu
        result%tangent(1:24,25)=result%kup(:,1)
        result%tangent(25,1:24)=result%kpu(1,:)
        result%tangent(25,25)=result%kpp(1,1)
    end subroutine evaluate_mixed_up_hex8

    subroutine unit_pressure_second_pk(f,j,s_unit,status)
        real(rk),intent(in)::f(3,3),j
        real(rk),intent(out)::s_unit(3,3)
        type(status_t),intent(out)::status
        real(rk)::c(3,3),cinv(3,3),detc
        call status%clear();s_unit=0.0_rk
        c=matmul(transpose(f),f)
        call inverse3(c,cinv,detc)
        if(detc<=tiny(1.0_rk))then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"Mixed u-p C tensor singular.");return
        end if
        s_unit=j*cinv
    end subroutine unit_pressure_second_pk

    subroutine pressure_second_pk_and_tangent(f,pressure,second_pk,tangent,status)
        !! S_p = p J C^{-1}; tangent = dS_p/dE at fixed p.
        real(rk),intent(in)::f(3,3),pressure
        real(rk),intent(out)::second_pk(3,3),tangent(6,6)
        type(status_t),intent(out)::status
        real(rk)::c(3,3),cinv(3,3),detc,j,dj
        real(rk)::v(6),de(3,3),dcinv(3,3),ds(3,3)
        integer::col
        call status%clear();second_pk=0.0_rk;tangent=0.0_rk
        j=determinant_3x3(f)
        if(j<=0.0_rk)then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"Pressure stress J>0 gerektirir.");return
        end if
        c=matmul(transpose(f),f);call inverse3(c,cinv,detc)
        if(detc<=tiny(1.0_rk))then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"Pressure stress C tensor singular.");return
        end if
        second_pk=pressure*j*cinv
        do col=1,6
            v=0.0_rk;v(col)=1.0_rk;call strain_voigt_to_tensor(v,de)
            dj=j*sum(cinv*transpose(de))
            dcinv=-2.0_rk*matmul(cinv,matmul(de,cinv))
            ds=pressure*(dj*cinv+j*dcinv)
            call stress_tensor_to_voigt(ds,tangent(:,col))
        end do
    end subroutine pressure_second_pk_and_tangent

    subroutine determinant_gradient_wrt_u(f,j,dshape_dreference,gradient,status)
        real(rk),intent(in)::f(3,3),j
        real(rk),intent(in)::dshape_dreference(3,8)
        real(rk),intent(out)::gradient(24)
        type(status_t),intent(out)::status
        real(rk)::finv(3,3),detf,grad(3)
        integer::a,i
        call status%clear();gradient=0.0_rk
        call inverse3(f,finv,detf)
        if(detf<=tiny(1.0_rk))then
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE,"dJ/du icin F singular veya inverted.");return
        end if
        do a=1,8
            grad=j*matmul(transpose(finv),dshape_dreference(:,a))
            do i=1,3
                gradient(3*(a-1)+i)=grad(i)
            end do
        end do
    end subroutine determinant_gradient_wrt_u

    pure subroutine inverse3(a,ainv,det)
        real(rk),intent(in)::a(3,3)
        real(rk),intent(out)::ainv(3,3),det
        det=determinant_3x3(a);ainv=0.0_rk
        if(abs(det)<=tiny(1.0_rk))return
        ainv(1,1)=(a(2,2)*a(3,3)-a(2,3)*a(3,2))/det
        ainv(1,2)=-(a(1,2)*a(3,3)-a(1,3)*a(3,2))/det
        ainv(1,3)=(a(1,2)*a(2,3)-a(1,3)*a(2,2))/det
        ainv(2,1)=-(a(2,1)*a(3,3)-a(2,3)*a(3,1))/det
        ainv(2,2)=(a(1,1)*a(3,3)-a(1,3)*a(3,1))/det
        ainv(2,3)=-(a(1,1)*a(2,3)-a(1,3)*a(2,1))/det
        ainv(3,1)=(a(2,1)*a(3,2)-a(2,2)*a(3,1))/det
        ainv(3,2)=-(a(1,1)*a(3,2)-a(1,2)*a(3,1))/det
        ainv(3,3)=(a(1,1)*a(2,2)-a(1,2)*a(2,1))/det
    end subroutine inverse3

end module fem_mixed_up_hex8
