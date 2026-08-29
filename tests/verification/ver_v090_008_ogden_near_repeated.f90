program ver_v090_008_ogden_near_repeated
    use fem_kinds, only : rk, id_kind
    use fem_hyperelastic_material
    use fem_finite_strain_kinematics, only : green_lagrange_strain
    use fem_tensor_notation, only : strain_tensor_to_voigt, strain_voigt_to_tensor, stress_tensor_to_voigt
    use fem_matrix_math, only : identity_matrix_3x3
    use fem_status, only : status_t
    use test_support, only : assert_true
    implicit none
    type(hyperelastic_material_t) :: m
    type(status_t) :: status
    real(rk) :: f0(3,3),e0(3,3),ev(6),ep(6),em(6),cp(3,3),cm(3,3),fp(3,3),fm(3,3)
    real(rk) :: s(3,3),sp(3,3),sm(3,3),d(6,6),scratch(6,6),fd(6,6),svp(6),svm(6),w,h,err,scale
    integer :: col

    ! Birbirine cok yakin ilk iki principal stretch, spectral tangent'in
    ! repeated-eigenvalue limit kolunu zorlar. Isaretli Ogden mu terimi de
    ! gercek fitting senaryolarina daha yakin bir regression kapsami verir.
    m=hyperelastic_material_t(id=1_id_kind,name='Ogden near repeated',model=HYPER_OGDEN,bulk_modulus=30.0e6_rk,ogden_term_count=2, &
        ogden_mu=[2.4e6_rk,-0.7e6_rk,0.0_rk],ogden_alpha=[1.7_rk,-2.4_rk,0.0_rk])
    call m%validate(status);call assert_true(status%is_ok(),'Signed-term Ogden material should validate')

    f0=0.0_rk
    f0(1,1)=1.10000000001_rk
    f0(2,2)=1.10000000000_rk
    f0(3,3)=0.91_rk
    call green_lagrange_strain(f0,e0);call strain_tensor_to_voigt(e0,ev)
    call hyperelastic_response(m,f0,s,d,w,status);call assert_true(status%is_ok(),'Near-repeated Ogden base response')

    h=1.0e-7_rk
    do col=1,6
        ep=ev;em=ev;ep(col)=ep(col)+h;em(col)=em(col)-h
        call strain_voigt_to_tensor(ep,cp);cp=identity_matrix_3x3()+2.0_rk*cp
        call strain_voigt_to_tensor(em,cm);cm=identity_matrix_3x3()+2.0_rk*cm
        call cholesky_upper(cp,fp,status);call assert_true(status%is_ok(),'C+ Cholesky')
        call cholesky_upper(cm,fm,status);call assert_true(status%is_ok(),'C- Cholesky')
        call hyperelastic_response(m,fp,sp,scratch,w,status);call assert_true(status%is_ok(),'Ogden plus response')
        call stress_tensor_to_voigt(sp,svp)
        call hyperelastic_response(m,fm,sm,scratch,w,status);call assert_true(status%is_ok(),'Ogden minus response')
        call stress_tensor_to_voigt(sm,svm)
        fd(:,col)=(svp-svm)/(2.0_rk*h)
    end do
    err=maxval(abs(d-fd));scale=max(1.0_rk,maxval(abs(fd)))
    if(err/scale>1.0e-4_rk)write(*,'(A,ES12.4,A,ES12.4)')'Ogden repeated-eigen tangent rel err=',err/scale,' abs=',err
    call assert_true(err/scale<1.0e-4_rk,'Near-repeated Ogden tangent finite-difference ile uyusmali')
    call assert_true(maxval(abs(d-transpose(d)))/max(1.0_rk,maxval(abs(d)))<2.0e-9_rk,'Near-repeated Ogden tangent symmetric olmali')
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
end program ver_v090_008_ogden_near_repeated
