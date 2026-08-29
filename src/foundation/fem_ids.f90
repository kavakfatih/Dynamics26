module fem_ids
    !! Kalici model kimlikleri icin ortak kurallar.
    !!
    !! Node ID, element ID, DOF ID ve equation ID ayni fiziksel kavram degildir.
    !! Bu moduldaki ortak invalid degeri yalnizca "atanmamis kimlik" durumunu
    !! belirtir; kimlik uzaylarini birbirine esitlemez.
    use fem_kinds, only : id_kind
    implicit none
    private

    integer(id_kind), parameter, public :: INVALID_ID = -1_id_kind

    public :: id_is_valid

contains

    pure logical function id_is_valid(value)
        integer(id_kind), intent(in) :: value
        id_is_valid = value >= 0_id_kind
    end function id_is_valid

end module fem_ids
