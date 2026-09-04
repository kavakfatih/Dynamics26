module fem_nonlinear_contracts
    !! RC.1 nonlinear termination sözleşmesi.
    !!
    !! Bu integer değerler public C ABI ile bire bir sabittir; yeniden
    !! sıralanamaz. status_t coarse library status'ünü taşımaya devam ederken
    !! phase/reason çözümün nerede ve neden sonlandığını string parsing olmadan
    !! açıklar. Pivot sonucu rigid-body rank veya definiteness hükmü değildir.
    implicit none
    private

    integer, parameter, public :: NONLINEAR_PHASE_NONE = 0
    integer, parameter, public :: NONLINEAR_PHASE_INPUT_VALIDATION = 1
    integer, parameter, public :: NONLINEAR_PHASE_LOAD_STEPPING = 2
    integer, parameter, public :: NONLINEAR_PHASE_NEWTON_ITERATION = 3
    integer, parameter, public :: NONLINEAR_PHASE_LINE_SEARCH = 4
    integer, parameter, public :: NONLINEAR_PHASE_LINEAR_SOLVE = 5
    integer, parameter, public :: NONLINEAR_PHASE_ELEMENT_KINEMATICS = 6
    integer, parameter, public :: NONLINEAR_PHASE_RESULT_RECOVERY = 7
    integer, parameter, public :: NONLINEAR_PHASE_CANCELLATION = 8

    integer, parameter, public :: NONLINEAR_REASON_NONE = 0
    integer, parameter, public :: NONLINEAR_REASON_CONVERGED = 1
    integer, parameter, public :: NONLINEAR_REASON_INVALID_INPUT = 2
    integer, parameter, public :: NONLINEAR_REASON_NO_ACTIVE_EQUATION = 3
    integer, parameter, public :: NONLINEAR_REASON_MAXIMUM_STEP_ATTEMPTS_REACHED = 4
    integer, parameter, public :: NONLINEAR_REASON_MINIMUM_INCREMENT_REACHED = 5
    integer, parameter, public :: NONLINEAR_REASON_NEWTON_ITERATION_LIMIT = 6
    integer, parameter, public :: NONLINEAR_REASON_LINE_SEARCH_FAILURE = 7
    integer, parameter, public :: NONLINEAR_REASON_LINEAR_SOLVER_FAILURE = 8
    integer, parameter, public :: NONLINEAR_REASON_SINGULAR_OR_ILL_CONDITIONED_TANGENT = 9
    integer, parameter, public :: NONLINEAR_REASON_INVALID_REFERENCE_JACOBIAN = 10
    integer, parameter, public :: NONLINEAR_REASON_INVALID_DEFORMATION_JACOBIAN = 11
    integer, parameter, public :: NONLINEAR_REASON_RESULT_RECOVERY_FAILURE = 12
    integer, parameter, public :: NONLINEAR_REASON_CANCELLED = 13
    integer, parameter, public :: NONLINEAR_REASON_UNKNOWN_NUMERICAL_FAILURE = 14

end module fem_nonlinear_contracts
