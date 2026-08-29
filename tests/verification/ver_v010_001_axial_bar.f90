program ver_v010_001_axial_bar
    !! VER-V010-001 — Cebirsel eksenel cubuk verification harness testi.
    !!
    !! Bu program BAR2 FEM elementi DEGILDIR. V0.1.0'da henuz element kernel
    !! bulunmadigi icin analitik olarak bilinen 2x2 stiffness matrisi elle kurulur:
    !!
    !!     K = EA/L [ 1 -1 ]
    !!              [ -1 1 ]
    !!
    !! u1 = 0 essential boundary condition'i uygulandiginda tek bilinmeyen:
    !!
    !!     u2 = F L / (E A)
    !!
    !! olur. Test, verification/CTest zincirinin dogru calistigini kanitlar.
    use fem_kinds, only : rk
    use test_support, only : assert_close
    implicit none

    real(rk), parameter :: young_modulus = 210000.0_rk  ! N/mm^2 = MPa
    real(rk), parameter :: area = 100.0_rk              ! mm^2
    real(rk), parameter :: length = 1000.0_rk           ! mm
    real(rk), parameter :: force = 21000.0_rk           ! N
    real(rk) :: stiffness(2,2), displacement(2), external_force(2)
    real(rk) :: internal_force(2), reaction(2)
    real(rk) :: expected_u2, factor

    factor = young_modulus * area / length
    stiffness = factor * reshape([1.0_rk, -1.0_rk, -1.0_rk, 1.0_rk], [2,2])

    displacement = 0.0_rk
    external_force = [0.0_rk, force]

    ! u1=0 sonrasinda reduced sistem tek denklemdir: K22*u2 = F2.
    displacement(2) = external_force(2) / stiffness(2,2)
    expected_u2 = force * length / (young_modulus * area)

    internal_force = matmul(stiffness, displacement)

    ! Proje residual convention'i R = f_ext - f_int'tir. Mesnet reaksiyonu ise
    ! kisitli DOF'ta dengeyi saglayan kuvvet olarak f_int - f_ext seklinde raporlanir.
    reaction = internal_force - external_force

    call assert_close(displacement(2), expected_u2, 1.0e-12_rk, 1.0e-12_rk, &
        "VER-V010-001 eksenel yer degistirme")
    call assert_close(reaction(1), -force, 1.0e-10_rk, 1.0e-12_rk, &
        "VER-V010-001 mesnet reaksiyonu")

    write(*, '(A)') "PASS VER-V010-001"
    write(*, '(A,ES16.8)') "u2_mm = ", displacement(2)
    write(*, '(A,ES16.8)') "R1_N  = ", reaction(1)
end program ver_v010_001_axial_bar
