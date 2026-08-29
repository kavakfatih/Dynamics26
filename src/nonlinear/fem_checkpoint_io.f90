module fem_checkpoint_io
    !! V1.0 nonlinear checkpoint disk I/O.
    !!
    !! Dosya formati bilerek kucuk, surumlu ve checksum kontrolludur. IEEE real64
    !! degerler decimal metne donusturulmek yerine bit-pattern olarak hex yazilir;
    !! boylece round-trip sirasinda sayisal yuvarlama kaynakli restart farki olusmaz.
    !! Contact history V1.0 checkpoint schema'sina dahil degildir; solver mevcut
    !! contact-restart kisitini korur.
    use, intrinsic :: iso_fortran_env, only : int64
    use, intrinsic :: ieee_arithmetic, only : ieee_is_finite
    use fem_kinds, only : rk
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT
    use fem_nonlinear_solver, only : nonlinear_checkpoint_t
    implicit none
    private

    character(len=*), parameter :: CHECKPOINT_MAGIC = 'FEMCAE_NONLINEAR_CHECKPOINT'
    integer, parameter, public :: FEM_CHECKPOINT_SCHEMA_VERSION = 1

    public :: write_nonlinear_checkpoint
    public :: read_nonlinear_checkpoint

contains

    pure integer(int64) function checkpoint_checksum(checkpoint) result(value)
        type(nonlinear_checkpoint_t), intent(in) :: checkpoint
        integer(int64) :: bits
        integer :: i, shift

        value = int(z'46454D4341451001', int64)
        bits = transfer(checkpoint%load_factor, bits)
        value = ieor(value, bits)
        value = ieor(value, ishftc(int(checkpoint%accepted_steps, int64), 7))
        if (allocated(checkpoint%active_displacement)) then
            value = ieor(value, ishftc(int(size(checkpoint%active_displacement), int64), 17))
            do i = 1, size(checkpoint%active_displacement)
                bits = transfer(checkpoint%active_displacement(i), bits)
                shift = modulo(11 * i, 64)
                value = ieor(value, ishftc(bits, shift))
            end do
        end if
    end function checkpoint_checksum

    subroutine validate_checkpoint(checkpoint, status)
        type(nonlinear_checkpoint_t), intent(in) :: checkpoint
        type(status_t), intent(out) :: status
        integer :: i

        call status%clear()
        if (.not. allocated(checkpoint%active_displacement)) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                'Checkpoint active displacement dizisi icermiyor.')
            return
        end if
        if (size(checkpoint%active_displacement) < 1) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                'Checkpoint en az bir aktif DOF icermeli.')
            return
        end if
        if (.not. ieee_is_finite(checkpoint%load_factor) .or. &
            checkpoint%load_factor < 0.0_rk .or. checkpoint%load_factor >= 1.0_rk) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                'Checkpoint load factor sonlu ve [0,1) araliginda olmali.')
            return
        end if
        if (checkpoint%accepted_steps < 0) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                'Checkpoint accepted step sayisi negatif olamaz.')
            return
        end if
        do i = 1, size(checkpoint%active_displacement)
            if (.not. ieee_is_finite(checkpoint%active_displacement(i))) then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                    'Checkpoint displacement dizisi NaN/Inf iceremez.')
                return
            end if
        end do
    end subroutine validate_checkpoint

    subroutine write_nonlinear_checkpoint(path, checkpoint, status)
        character(len=*), intent(in) :: path
        type(nonlinear_checkpoint_t), intent(in) :: checkpoint
        type(status_t), intent(out) :: status
        integer :: unit, ios, i
        integer(int64) :: bits, checksum
        character(len=256) :: iomsg

        call validate_checkpoint(checkpoint, status)
        if (.not. status%is_ok()) return
        if (len_trim(path) == 0) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, 'Checkpoint dosya yolu bos olamaz.')
            return
        end if

        open(newunit=unit, file=trim(path), status='replace', action='write', form='formatted', &
             iostat=ios, iomsg=iomsg)
        if (ios /= 0) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                'Checkpoint dosyasi yazma icin acilamadi: '//trim(iomsg))
            return
        end if

        checksum = checkpoint_checksum(checkpoint)
        write(unit,'(A)',iostat=ios) CHECKPOINT_MAGIC
        if (ios == 0) write(unit,'(I0)',iostat=ios) FEM_CHECKPOINT_SCHEMA_VERSION
        bits = transfer(checkpoint%load_factor, bits)
        if (ios == 0) write(unit,'(Z16.16)',iostat=ios) bits
        if (ios == 0) write(unit,'(I0)',iostat=ios) checkpoint%accepted_steps
        if (ios == 0) write(unit,'(I0)',iostat=ios) size(checkpoint%active_displacement)
        do i = 1, size(checkpoint%active_displacement)
            if (ios /= 0) exit
            bits = transfer(checkpoint%active_displacement(i), bits)
            write(unit,'(Z16.16)',iostat=ios) bits
        end do
        if (ios == 0) write(unit,'(Z16.16)',iostat=ios) checksum
        if (ios == 0) write(unit,'(A)',iostat=ios) 'END'
        close(unit)
        if (ios /= 0) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, 'Checkpoint dosyasi yazilirken I/O hatasi olustu.')
            return
        end if
        call status%clear()
    end subroutine write_nonlinear_checkpoint

    subroutine read_nonlinear_checkpoint(path, checkpoint, status)
        character(len=*), intent(in) :: path
        type(nonlinear_checkpoint_t), intent(inout) :: checkpoint
        type(status_t), intent(out) :: status
        integer :: unit, ios, i, schema, n
        integer(int64) :: bits, expected_checksum, actual_checksum
        character(len=256) :: line, iomsg

        call status%clear()
        call checkpoint%clear()
        if (len_trim(path) == 0) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, 'Checkpoint dosya yolu bos olamaz.')
            return
        end if
        open(newunit=unit, file=trim(path), status='old', action='read', form='formatted', &
             iostat=ios, iomsg=iomsg)
        if (ios /= 0) then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
                'Checkpoint dosyasi okunamadi: '//trim(iomsg))
            return
        end if

        read(unit,'(A)',iostat=ios) line
        if (ios /= 0 .or. trim(line) /= CHECKPOINT_MAGIC) goto 900
        read(unit,*,iostat=ios) schema
        if (ios /= 0 .or. schema /= FEM_CHECKPOINT_SCHEMA_VERSION) goto 900
        read(unit,'(Z16)',iostat=ios) bits
        if (ios /= 0) goto 900
        checkpoint%load_factor = transfer(bits, checkpoint%load_factor)
        read(unit,*,iostat=ios) checkpoint%accepted_steps
        if (ios /= 0) goto 900
        read(unit,*,iostat=ios) n
        if (ios /= 0 .or. n < 1 .or. n > 100000000) goto 900
        allocate(checkpoint%active_displacement(n))
        do i = 1, n
            read(unit,'(Z16)',iostat=ios) bits
            if (ios /= 0) goto 900
            checkpoint%active_displacement(i) = transfer(bits, checkpoint%active_displacement(i))
        end do
        read(unit,'(Z16)',iostat=ios) expected_checksum
        if (ios /= 0) goto 900
        read(unit,'(A)',iostat=ios) line
        if (ios /= 0 .or. trim(line) /= 'END') goto 900
        close(unit)

        call validate_checkpoint(checkpoint, status)
        if (.not. status%is_ok()) then
            call checkpoint%clear()
            return
        end if
        actual_checksum = checkpoint_checksum(checkpoint)
        if (actual_checksum /= expected_checksum) then
            call checkpoint%clear()
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT, 'Checkpoint checksum dogrulamasi basarisiz.')
            return
        end if
        call status%clear()
        return

900     continue
        close(unit)
        call checkpoint%clear()
        call status%set_error(FEM_STATUS_INVALID_ARGUMENT, &
            'Checkpoint dosyasi bozuk, eksik veya desteklenmeyen schema iceriyor.')
    end subroutine read_nonlinear_checkpoint

end module fem_checkpoint_io
