program test_v070_error_paths
    use fem_kinds, only : rk,id_kind
    use fem_finite_strain_kinematics, only : deformation_gradient_from_coordinates,second_pk_to_cauchy
    use fem_total_lagrangian_hex8, only : total_lagrangian_hex8_result_t,evaluate_total_lagrangian_hex8
    use fem_linear_elastic_material, only : linear_elastic_material_t
    use fem_status, only : status_t,FEM_STATUS_NUMERICAL_FAILURE
    use test_support, only : assert_true
    implicit none
    real(rk)::current(3,4),grad(3,4),f(3,3),j,s(3,3),sigma(3,3),x(3,8),u(3,8)
    type(linear_elastic_material_t)::mat
    type(total_lagrangian_hex8_result_t)::r
    type(status_t)::status
    current=0._rk;grad=0._rk
    call deformation_gradient_from_coordinates(current,grad,f,j,status)
    call assert_true(status%code==FEM_STATUS_NUMERICAL_FAILURE, "J zero rejected")
    f=0._rk;f(1,1)=-1._rk;f(2,2)=1._rk;f(3,3)=1._rk;s=0._rk
    call second_pk_to_cauchy(f,s,sigma,status)
    call assert_true(status%code==FEM_STATUS_NUMERICAL_FAILURE, "negative J stress rejected")
    x=0._rk;u=0._rk
    mat=linear_elastic_material_t(id=1_id_kind,name="bad hex",young_modulus=1.e6_rk,poisson_ratio=0.3_rk)
    call evaluate_total_lagrangian_hex8(x,u,mat,r,status)
    call assert_true(.not.status%is_ok(), "degenerate reference rejected")
end program test_v070_error_paths
