module fem_contact_types
    !! V0.11 node-to-rigid-facet contact veri modeli ve history state'i.
    !! Master facetler rigid kabul edilir; slave taraf nodal displacement alanidir.
    use fem_kinds, only : rk, id_kind, index_kind
    use fem_ids, only : INVALID_ID, id_is_valid
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT, FEM_STATUS_NOT_INITIALIZED
    use fem_mesh, only : mesh_t
    implicit none
    private

    integer, parameter, public :: CONTACT_ENFORCEMENT_PENALTY = 1
    integer, parameter, public :: CONTACT_ENFORCEMENT_AUGMENTED_LAGRANGIAN = 2
    integer, parameter, public :: CONTACT_FRICTIONLESS = 0
    integer, parameter, public :: CONTACT_FRICTION_COULOMB = 1
    integer, parameter, public :: CONTACT_STATE_OPEN = 0
    integer, parameter, public :: CONTACT_STATE_STICK = 1
    integer, parameter, public :: CONTACT_STATE_SLIP = 2

    type, public :: contact_facet_t
        integer(id_kind) :: id = INVALID_ID
        integer(id_kind) :: node_ids(4) = INVALID_ID
    end type contact_facet_t

    type, public :: contact_point_state_t
        integer(id_kind) :: committed_master_facet_id = INVALID_ID
        integer(id_kind) :: trial_master_facet_id = INVALID_ID
        integer :: committed_status = CONTACT_STATE_OPEN
        integer :: trial_status = CONTACT_STATE_OPEN
        real(rk) :: committed_normal_multiplier = 0.0_rk
        real(rk) :: trial_normal_multiplier = 0.0_rk
        real(rk) :: committed_gap = 0.0_rk
        real(rk) :: trial_gap = 0.0_rk
        real(rk) :: committed_tangential_traction(3) = 0.0_rk
        real(rk) :: trial_tangential_traction(3) = 0.0_rk
        real(rk) :: committed_position(3) = 0.0_rk
        real(rk) :: trial_position(3) = 0.0_rk
        logical :: initialized = .false.
    contains
        procedure :: begin_trial => contact_point_begin_trial
        procedure :: commit => contact_point_commit
        procedure :: revert => contact_point_revert
    end type contact_point_state_t

    type, public :: contact_pair_t
        integer(id_kind) :: id = INVALID_ID
        integer(id_kind), allocatable :: slave_node_ids(:)
        type(contact_facet_t), allocatable :: master_facets(:)
        integer :: enforcement = CONTACT_ENFORCEMENT_PENALTY
        integer :: friction_model = CONTACT_FRICTIONLESS
        real(rk) :: normal_penalty = 0.0_rk
        real(rk) :: tangential_penalty = 0.0_rk
        real(rk) :: friction_coefficient = 0.0_rk
        real(rk) :: search_distance = 0.0_rk
        real(rk) :: activation_tolerance = 1.0e-12_rk
        type(contact_point_state_t), allocatable :: states(:)
    contains
        procedure :: validate => contact_pair_validate
        procedure :: prepare => contact_pair_prepare
        procedure :: begin_trial => contact_pair_begin_trial
        procedure :: commit => contact_pair_commit
        procedure :: revert => contact_pair_revert
        procedure :: clear => contact_pair_clear
    end type contact_pair_t

    type, public :: contact_registry_t
        type(contact_pair_t), allocatable :: pairs(:)
    contains
        procedure :: clear => contact_registry_clear
        procedure :: add => contact_registry_add
        procedure :: validate => contact_registry_validate
        procedure :: prepare => contact_registry_prepare
        procedure :: begin_trial => contact_registry_begin_trial
        procedure :: commit => contact_registry_commit
        procedure :: revert => contact_registry_revert
        procedure :: count => contact_registry_count
        procedure :: has_friction => contact_registry_has_friction
    end type contact_registry_t

contains

    subroutine contact_point_begin_trial(this)
        class(contact_point_state_t), intent(inout) :: this
        this%trial_master_facet_id = this%committed_master_facet_id
        this%trial_status = this%committed_status
        this%trial_normal_multiplier = this%committed_normal_multiplier
        this%trial_gap = this%committed_gap
        this%trial_tangential_traction = this%committed_tangential_traction
        this%trial_position = this%committed_position
    end subroutine contact_point_begin_trial

    subroutine contact_point_commit(this)
        class(contact_point_state_t), intent(inout) :: this
        this%committed_master_facet_id = this%trial_master_facet_id
        this%committed_status = this%trial_status
        this%committed_normal_multiplier = this%trial_normal_multiplier
        this%committed_gap = this%trial_gap
        this%committed_tangential_traction = this%trial_tangential_traction
        this%committed_position = this%trial_position
        this%initialized = .true.
    end subroutine contact_point_commit

    subroutine contact_point_revert(this)
        class(contact_point_state_t), intent(inout) :: this
        call this%begin_trial()
    end subroutine contact_point_revert

    subroutine contact_pair_clear(this)
        class(contact_pair_t), intent(inout) :: this
        this%id=INVALID_ID
        if(allocated(this%slave_node_ids))deallocate(this%slave_node_ids)
        if(allocated(this%master_facets))deallocate(this%master_facets)
        if(allocated(this%states))deallocate(this%states)
        this%enforcement=CONTACT_ENFORCEMENT_PENALTY
        this%friction_model=CONTACT_FRICTIONLESS
        this%normal_penalty=0.0_rk;this%tangential_penalty=0.0_rk
        this%friction_coefficient=0.0_rk;this%search_distance=0.0_rk
        this%activation_tolerance=1.0e-12_rk
    end subroutine contact_pair_clear

    subroutine contact_pair_validate(this,mesh,status)
        class(contact_pair_t),intent(in)::this
        type(mesh_t),intent(in)::mesh
        type(status_t),intent(out)::status
        integer::i,j
        call status%clear()
        if(.not.id_is_valid(this%id))then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact pair ID gecersiz.");return
        end if
        if(.not.allocated(this%slave_node_ids).or.size(this%slave_node_ids)<1)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact pair en az bir slave node gerektirir.");return
        end if
        if(.not.allocated(this%master_facets).or.size(this%master_facets)<1)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact pair en az bir master QUAD4 facet gerektirir.");return
        end if
        if(this%normal_penalty<=0.0_rk.or.this%search_distance<=0.0_rk.or.this%activation_tolerance<0.0_rk)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact penalty/search parametreleri pozitif olmali.");return
        end if
        if(this%enforcement/=CONTACT_ENFORCEMENT_PENALTY.and. &
           this%enforcement/=CONTACT_ENFORCEMENT_AUGMENTED_LAGRANGIAN)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact enforcement tipi desteklenmiyor.");return
        end if
        if(this%friction_model/=CONTACT_FRICTIONLESS.and.this%friction_model/=CONTACT_FRICTION_COULOMB)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact friction modeli desteklenmiyor.");return
        end if
        if(this%friction_model==CONTACT_FRICTION_COULOMB)then
            if(this%friction_coefficient<0.0_rk.or.this%tangential_penalty<=0.0_rk)then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Coulomb contact icin mu>=0 ve tangential penalty>0 olmali.");return
            end if
        end if
        do i=1,size(this%slave_node_ids)
            if(mesh%find_node_position(this%slave_node_ids(i))==0_index_kind)then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact slave Node ID mesh icinde bulunamadi.");return
            end if
            if(i>1.and.any(this%slave_node_ids(1:i-1)==this%slave_node_ids(i)))then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact slave Node ID tekrarli olamaz.");return
            end if
        end do
        do i=1,size(this%master_facets)
            if(.not.id_is_valid(this%master_facets(i)%id))then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact facet ID gecersiz.");return
            end if
            if(i>1)then
                do j=1,i-1
                    if(this%master_facets(j)%id==this%master_facets(i)%id)then
                        call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact facet ID tekrarli olamaz.");return
                    end if
                end do
            end if
            do j=1,4
                if(mesh%find_node_position(this%master_facets(i)%node_ids(j))==0_index_kind)then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact master facet Node ID mesh icinde bulunamadi.");return
                end if
                if(j>1.and.any(this%master_facets(i)%node_ids(1:j-1)==this%master_facets(i)%node_ids(j)))then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact facet ayni Node ID'yi tekrar kullanamaz.");return
                end if
            end do
        end do
    end subroutine contact_pair_validate

    subroutine contact_pair_prepare(this,mesh,status)
        class(contact_pair_t),intent(inout)::this
        type(mesh_t),intent(in)::mesh
        type(status_t),intent(out)::status
        integer::i
        integer(index_kind)::pos
        call this%validate(mesh,status);if(.not.status%is_ok())return
        if(allocated(this%states))then
            if(size(this%states)==size(this%slave_node_ids).and.all(this%states%initialized))return
            deallocate(this%states)
        end if
        allocate(this%states(size(this%slave_node_ids)))
        do i=1,size(this%states)
            pos=mesh%find_node_position(this%slave_node_ids(i))
            this%states(i)%committed_position=mesh%nodes(pos)%x
            this%states(i)%trial_position=mesh%nodes(pos)%x
            this%states(i)%initialized=.true.
        end do
        call status%clear()
    end subroutine contact_pair_prepare

    subroutine contact_pair_begin_trial(this,status)
        class(contact_pair_t),intent(inout)::this
        type(status_t),intent(out)::status
        integer::i
        call status%clear()
        if(.not.allocated(this%states))then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED,"Contact state prepare edilmedi.");return
        end if
        do i=1,size(this%states);call this%states(i)%begin_trial();end do
    end subroutine contact_pair_begin_trial

    subroutine contact_pair_commit(this,status)
        class(contact_pair_t),intent(inout)::this
        type(status_t),intent(out)::status
        integer::i
        call status%clear()
        if(.not.allocated(this%states))then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED,"Contact state prepare edilmedi.");return
        end if
        do i=1,size(this%states);call this%states(i)%commit();end do
    end subroutine contact_pair_commit

    subroutine contact_pair_revert(this,status)
        class(contact_pair_t),intent(inout)::this
        type(status_t),intent(out)::status
        integer::i
        call status%clear()
        if(.not.allocated(this%states))then
            call status%set_error(FEM_STATUS_NOT_INITIALIZED,"Contact state prepare edilmedi.");return
        end if
        do i=1,size(this%states);call this%states(i)%revert();end do
    end subroutine contact_pair_revert

    subroutine contact_registry_clear(this)
        class(contact_registry_t),intent(inout)::this
        if(allocated(this%pairs))deallocate(this%pairs)
    end subroutine contact_registry_clear

    subroutine contact_registry_add(this,pair,status)
        class(contact_registry_t),intent(inout)::this
        type(contact_pair_t),intent(in)::pair
        type(status_t),intent(out)::status
        type(contact_pair_t),allocatable::tmp(:)
        integer::n,i
        call status%clear()
        if(.not.id_is_valid(pair%id))then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact registry pair ID gecersiz.");return
        end if
        if(allocated(this%pairs))then
            do i=1,size(this%pairs)
                if(this%pairs(i)%id==pair%id)then
                    call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact pair ID registry'de tekrarli olamaz.");return
                end if
            end do
            n=size(this%pairs);allocate(tmp(n+1));tmp(1:n)=this%pairs;tmp(n+1)=pair;call move_alloc(tmp,this%pairs)
        else
            allocate(this%pairs(1));this%pairs(1)=pair
        end if
    end subroutine contact_registry_add

    subroutine contact_registry_validate(this,mesh,status)
        class(contact_registry_t),intent(in)::this
        type(mesh_t),intent(in)::mesh
        type(status_t),intent(out)::status
        integer::i
        call status%clear();if(.not.allocated(this%pairs))return
        do i=1,size(this%pairs);call this%pairs(i)%validate(mesh,status);if(.not.status%is_ok())return;end do
    end subroutine contact_registry_validate

    subroutine contact_registry_prepare(this,mesh,status)
        class(contact_registry_t),intent(inout)::this
        type(mesh_t),intent(in)::mesh
        type(status_t),intent(out)::status
        integer::i
        call status%clear();if(.not.allocated(this%pairs))return
        do i=1,size(this%pairs);call this%pairs(i)%prepare(mesh,status);if(.not.status%is_ok())return;end do
    end subroutine contact_registry_prepare

    subroutine contact_registry_begin_trial(this,status)
        class(contact_registry_t),intent(inout)::this
        type(status_t),intent(out)::status
        integer::i
        call status%clear();if(.not.allocated(this%pairs))return
        do i=1,size(this%pairs);call this%pairs(i)%begin_trial(status);if(.not.status%is_ok())return;end do
    end subroutine contact_registry_begin_trial

    subroutine contact_registry_commit(this,status)
        class(contact_registry_t),intent(inout)::this
        type(status_t),intent(out)::status
        integer::i
        call status%clear();if(.not.allocated(this%pairs))return
        do i=1,size(this%pairs);call this%pairs(i)%commit(status);if(.not.status%is_ok())return;end do
    end subroutine contact_registry_commit

    subroutine contact_registry_revert(this,status)
        class(contact_registry_t),intent(inout)::this
        type(status_t),intent(out)::status
        integer::i
        call status%clear();if(.not.allocated(this%pairs))return
        do i=1,size(this%pairs);call this%pairs(i)%revert(status);if(.not.status%is_ok())return;end do
    end subroutine contact_registry_revert

    pure integer(index_kind) function contact_registry_count(this)
        class(contact_registry_t),intent(in)::this
        if(allocated(this%pairs))then;contact_registry_count=int(size(this%pairs),index_kind);else;contact_registry_count=0_index_kind;end if
    end function contact_registry_count

    pure logical function contact_registry_has_friction(this)
        class(contact_registry_t),intent(in)::this
        integer::i
        contact_registry_has_friction=.false.;if(.not.allocated(this%pairs))return
        do i=1,size(this%pairs)
            if(this%pairs(i)%friction_model==CONTACT_FRICTION_COULOMB)then;contact_registry_has_friction=.true.;return;end if
        end do
    end function contact_registry_has_friction

end module fem_contact_types
