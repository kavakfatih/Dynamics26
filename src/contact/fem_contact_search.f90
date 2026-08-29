module fem_contact_search
    !! V0.11 broad-phase AABB + narrow-phase closest-point search.
    use fem_kinds, only : rk, id_kind
    use fem_ids, only : INVALID_ID
    use fem_mesh, only : mesh_t
    use fem_contact_types, only : contact_pair_t
    use fem_contact_geometry, only : facet_aabb, closest_point_on_facet
    use fem_status, only : status_t
    implicit none
    private

    type, public :: contact_search_result_t
        logical :: found = .false.
        integer(id_kind) :: facet_id = INVALID_ID
        real(rk) :: closest_point(3)=0.0_rk
        real(rk) :: normal(3)=0.0_rk
        real(rk) :: gap=huge(1.0_rk)
        real(rk) :: distance=huge(1.0_rk)
        integer :: broad_phase_candidates=0
    end type contact_search_result_t

    public :: search_master_facet
contains
    subroutine search_master_facet(mesh,pair,slave_position,result,status)
        type(mesh_t),intent(in)::mesh
        type(contact_pair_t),intent(in)::pair
        real(rk),intent(in)::slave_position(3)
        type(contact_search_result_t),intent(out)::result
        type(status_t),intent(out)::status
        real(rk)::bmin(3),bmax(3),closest(3),n(3),gap,distance
        integer::i
        call status%clear();result=contact_search_result_t()
        do i=1,size(pair%master_facets)
            call facet_aabb(mesh,pair%master_facets(i),bmin,bmax,status);if(.not.status%is_ok())return
            if(any(slave_position<bmin-pair%search_distance).or.any(slave_position>bmax+pair%search_distance))cycle
            result%broad_phase_candidates=result%broad_phase_candidates+1
            call closest_point_on_facet(mesh,pair%master_facets(i),slave_position,closest,n,gap,distance,status)
            if(.not.status%is_ok())return
            if(distance<result%distance)then
                result%found=.true.;result%facet_id=pair%master_facets(i)%id
                result%closest_point=closest;result%normal=n;result%gap=gap;result%distance=distance
            end if
        end do
    end subroutine search_master_facet
end module fem_contact_search
