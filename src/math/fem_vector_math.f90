module fem_vector_math
    !! FEM element ve solver katmanlarinin kullanacagi temel vector islemleri.
    use fem_kinds, only : rk
    use fem_status, only : status_t, FEM_STATUS_SIZE_MISMATCH, &
                           FEM_STATUS_NUMERICAL_FAILURE
    implicit none
    private

    public :: vector_dot, vector_norm2, normalize_vector

contains

    subroutine vector_dot(a, b, value, status)
        real(rk), intent(in) :: a(:), b(:)
        real(rk), intent(out) :: value
        type(status_t), intent(out) :: status

        call status%clear()
        value = 0.0_rk
        if (size(a) /= size(b)) then
            call status%set_error(FEM_STATUS_SIZE_MISMATCH, &
                "Dot product icin vector boyutlari ayni olmalidir.")
            return
        end if
        value = dot_product(a, b)
    end subroutine vector_dot

    pure real(rk) function vector_norm2(a) result(value)
        real(rk), intent(in) :: a(:)
        value = sqrt(dot_product(a, a))
    end function vector_norm2

    subroutine normalize_vector(a, normalized, status)
        real(rk), intent(in) :: a(:)
        real(rk), allocatable, intent(out) :: normalized(:)
        type(status_t), intent(out) :: status
        real(rk) :: magnitude

        call status%clear()
        magnitude = vector_norm2(a)
        if (magnitude <= tiny(1.0_rk)) then
            allocate(normalized(size(a)))
            normalized = 0.0_rk
            call status%set_error(FEM_STATUS_NUMERICAL_FAILURE, &
                "Sifir buyuklukte vector normalize edilemez.")
            return
        end if
        normalized = a / magnitude
    end subroutine normalize_vector

end module fem_vector_math
