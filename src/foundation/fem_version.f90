module fem_version
    !! Uygulama, proje semasi, sonuc semasi ve C API surumleri birbirinden
    !! bagimsizdir. Bu ayrim gelecekte eski proje dosyalarini acarken gereklidir.
    implicit none
    private

    integer, parameter, public :: FEM_VERSION_MAJOR = 1
    integer, parameter, public :: FEM_VERSION_MINOR = 0
    integer, parameter, public :: FEM_VERSION_PATCH = 2
    character(len=*), parameter, public :: FEM_VERSION_STRING = "1.0.2"

    integer, parameter, public :: FEM_PROJECT_SCHEMA_VERSION = 1
    integer, parameter, public :: FEM_RESULT_SCHEMA_VERSION = 1
    !! Beta.2 B2.5 diagnostics entry point additive bir C ABI genislemesidir.
    !! Mevcut V1 fonksiyon imzalari ve davranis contract'i degismedigi icin API
    !! major/capability contract'i 1 olarak korunur.
    integer, parameter, public :: FEM_C_API_VERSION = 1

end module fem_version
