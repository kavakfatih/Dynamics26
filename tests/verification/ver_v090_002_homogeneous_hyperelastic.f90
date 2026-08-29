program ver_v090_002_homogeneous_hyperelastic
    use fem_kinds, only : rk, id_kind
    use, intrinsic :: ieee_arithmetic, only : ieee_is_finite
    use fem_hyperelastic_material
    use fem_finite_strain_kinematics, only : second_pk_to_first_pk
    use fem_status, only : status_t
    use test_support, only : assert_true
    implicit none
    type(hyperelastic_material_t) :: mats(4)
    type(status_t) :: status
    real(rk) :: f(3,3),s(3,3),p(3,3),d(6,6),w,lambda
    integer :: mode, im
    mats(1)=hyperelastic_material_t(id=1_id_kind,name='NH verify',model=HYPER_NEO_HOOKEAN,bulk_modulus=50.0e6_rk,c10=1.25e6_rk)
    mats(2)=hyperelastic_material_t(id=2_id_kind,name='MR verify',model=HYPER_MOONEY_RIVLIN,bulk_modulus=50.0e6_rk,c10=0.9e6_rk,c01=0.35e6_rk)
    mats(3)=hyperelastic_material_t(id=3_id_kind,name='Yeoh verify',model=HYPER_YEOH,bulk_modulus=50.0e6_rk,c10=1.25e6_rk,c20=0.1e6_rk,c30=0.02e6_rk)
    mats(4)=hyperelastic_material_t(id=4_id_kind,name='Ogden verify',model=HYPER_OGDEN,bulk_modulus=50.0e6_rk,ogden_term_count=2,ogden_mu=[1.7e6_rk,0.8e6_rk,0._rk],ogden_alpha=[2._rk,-2._rk,0._rk])
    lambda=1.25_rk
    do im=1,4
    do mode=1,4
        f=0.0_rk
        select case(mode)
        case(1) ! isochoric uniaxial
            f(1,1)=lambda;f(2,2)=lambda**(-0.5_rk);f(3,3)=f(2,2)
        case(2) ! planar tension, lambda3 chosen for J=1
            f(1,1)=lambda;f(2,2)=1.0_rk;f(3,3)=1.0_rk/lambda
        case(3) ! equibiaxial
            f(1,1)=lambda;f(2,2)=lambda;f(3,3)=lambda**(-2.0_rk)
        case(4) ! simple shear
            f(1,1)=1.0_rk;f(2,2)=1.0_rk;f(3,3)=1.0_rk;f(1,2)=0.35_rk
        end select
        call hyperelastic_response(mats(im),f,s,d,w,status);call assert_true(status%is_ok(),'Homogeneous hyperelastic response')
        call assert_true(w>0.0_rk,'Deformed homogeneous state positive energy vermeli')
        call second_pk_to_first_pk(f,s,p)
        call assert_true(all(ieee_is_finite(p)),'Nominal stress finite olmali')
    end do
    end do
end program ver_v090_002_homogeneous_hyperelastic
