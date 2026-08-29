program ver_v090_001_hyperelastic_tangent
    use fem_kinds, only : rk, id_kind
    use fem_hyperelastic_material
    use fem_finite_strain_kinematics, only : green_lagrange_strain
    use fem_tensor_notation, only : strain_tensor_to_voigt, strain_voigt_to_tensor, stress_tensor_to_voigt
    use fem_matrix_math, only : identity_matrix_3x3
    use fem_status, only : status_t
    use test_support, only : assert_true
    implicit none
    type(hyperelastic_material_t) :: mats(4)
    type(status_t) :: status
    real(rk) :: f0(3,3),e0(3,3),ev(6),ep(6),em(6),cp(3,3),cm(3,3),fp(3,3),fm(3,3)
    real(rk) :: s(3,3),sp(3,3),sm(3,3),d(6,6),fd(6,6),svp(6),svm(6),w,h,err,scale
    integer :: m,col

    mats(1)=hyperelastic_material_t(id=1_id_kind,name='NH',model=HYPER_NEO_HOOKEAN,bulk_modulus=35.0e6_rk,c10=1.2e6_rk)
    mats(2)=hyperelastic_material_t(id=2_id_kind,name='MR',model=HYPER_MOONEY_RIVLIN,bulk_modulus=35.0e6_rk,c10=0.8e6_rk,c01=0.4e6_rk)
    mats(3)=hyperelastic_material_t(id=3_id_kind,name='Yeoh',model=HYPER_YEOH,bulk_modulus=35.0e6_rk,c10=1.2e6_rk,c20=0.15e6_rk,c30=0.03e6_rk)
    mats(4)=hyperelastic_material_t(id=4_id_kind,name='Ogden',model=HYPER_OGDEN,bulk_modulus=35.0e6_rk,ogden_term_count=2, &
        ogden_mu=[1.6e6_rk,0.8e6_rk,0.0_rk],ogden_alpha=[1.8_rk,-2.2_rk,0.0_rk])

    f0=0.0_rk;f0(1,1)=1.18_rk;f0(1,2)=0.08_rk;f0(2,2)=0.93_rk;f0(2,3)=0.04_rk;f0(3,3)=1.07_rk
    call green_lagrange_strain(f0,e0);call strain_tensor_to_voigt(e0,ev)
    h=2.0e-7_rk
    do m=1,4
        call hyperelastic_response(mats(m),f0,s,d,w,status);call assert_true(status%is_ok(),'Hyperelastic base response')
        do col=1,6
            ep=ev;em=ev;ep(col)=ep(col)+h;em(col)=em(col)-h
            call strain_voigt_to_tensor(ep,cp);cp=identity_matrix_3x3()+2.0_rk*cp
            call strain_voigt_to_tensor(em,cm);cm=identity_matrix_3x3()+2.0_rk*cm
            call cholesky_upper(cp,fp,status);call assert_true(status%is_ok(),'C+ Cholesky')
            call cholesky_upper(cm,fm,status);call assert_true(status%is_ok(),'C- Cholesky')
            call hyperelastic_response(mats(m),fp,sp,fd,w,status);call assert_true(status%is_ok(),'plus response')
            call stress_tensor_to_voigt(sp,svp)
            call hyperelastic_response(mats(m),fm,sm,fd,w,status);call assert_true(status%is_ok(),'minus response')
            call stress_tensor_to_voigt(sm,svm);fd(:,col)=(svp-svm)/(2.0_rk*h)
        end do
        err=maxval(abs(d-fd));scale=max(1.0_rk,maxval(abs(fd)))
        if(err/scale>2.5e-5_rk)then
            write(*,'(A,I0,A,ES12.4,A,ES12.4)')'model=',m,' tangent relative err=',err/scale,' abs=',err
        end if
        call assert_true(err/scale<2.5e-5_rk,'Analytic hyperelastic tangent finite-difference ile uyusmali')
        call assert_true(maxval(abs(d-transpose(d)))/max(1.0_rk,maxval(abs(d)))<2.0e-10_rk,'Hyperelastic tangent symmetric olmali')
    end do
contains
    subroutine cholesky_upper(c,f,status)
        real(rk),intent(in)::c(3,3);real(rk),intent(out)::f(3,3);type(status_t),intent(out)::status
        real(rk)::l(3,3),sumv;integer::i,j,k
        call status%clear();l=0.0_rk
        do i=1,3
            do j=1,i
                sumv=c(i,j)
                do k=1,j-1;sumv=sumv-l(i,k)*l(j,k);end do
                if(i==j)then
                    if(sumv<=0.0_rk)then;call status%set_error(30,'Cholesky SPD failure');return;end if
                    l(i,j)=sqrt(sumv)
                else
                    l(i,j)=sumv/l(j,j)
                end if
            end do
        end do
        f=transpose(l)
    end subroutine cholesky_upper
end program ver_v090_001_hyperelastic_tangent
