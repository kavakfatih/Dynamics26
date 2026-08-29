module fem_kinds
    !! FEMCAE genelindeki sayisal turleri tek bir noktadan tanimlar.
    !!
    !! Kalici kimlikler ile solver icindeki denklem indeksleri kavramsal olarak
    !! farklidir. V0.1.0'da ikisi de 64 bit tamsayi kullansa bile bu ayrim
    !! ileride mesh yeniden siralama, sparse backend ve restart islemlerinde
    !! korunacaktir.
    use, intrinsic :: iso_fortran_env, only : real64, int32, int64
    implicit none
    private

    integer, parameter, public :: rk = real64
    integer, parameter, public :: small_index_kind = int32
    integer, parameter, public :: id_kind = int64
    integer, parameter, public :: index_kind = int64

end module fem_kinds
