program test_tensor_notation
    use fem_kinds, only : rk
    use fem_tensor_notation, only : stress_tensor_to_voigt, stress_voigt_to_tensor, &
                                    strain_tensor_to_voigt, strain_voigt_to_tensor, &
                                    tensor_trace
    use test_support, only : assert_close
    implicit none

    real(rk) :: tensor(3,3), reconstructed(3,3), voigt(6)

    tensor = reshape([1.0_rk, 4.0_rk, 6.0_rk, &
                      4.0_rk, 2.0_rk, 5.0_rk, &
                      6.0_rk, 5.0_rk, 3.0_rk], [3,3])

    call stress_tensor_to_voigt(tensor, voigt)
    call assert_close(voigt(4), 4.0_rk, 0.0_rk, 0.0_rk, "stress xy tensorial kalmali")
    call assert_close(voigt(5), 5.0_rk, 0.0_rk, 0.0_rk, "stress yz sirasi")
    call assert_close(voigt(6), 6.0_rk, 0.0_rk, 0.0_rk, "stress xz sirasi")
    call stress_voigt_to_tensor(voigt, reconstructed)
    call assert_close(maxval(abs(reconstructed-tensor)), 0.0_rk, 0.0_rk, 0.0_rk, "stress roundtrip")

    call strain_tensor_to_voigt(tensor, voigt)
    call assert_close(voigt(4), 8.0_rk, 0.0_rk, 0.0_rk, "engineering gamma_xy = 2 epsilon_xy")
    call strain_voigt_to_tensor(voigt, reconstructed)
    call assert_close(maxval(abs(reconstructed-tensor)), 0.0_rk, 0.0_rk, 0.0_rk, "strain roundtrip")
    call assert_close(tensor_trace(tensor), 6.0_rk, 0.0_rk, 0.0_rk, "tensor trace")
    write(*, '(A)') "PASS unit_tensor_notation"
end program test_tensor_notation
