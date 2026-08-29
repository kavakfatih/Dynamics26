module fem_contact_geometry
    !! Rigid QUAD4 facet geometry yardimcilari. Facet ordering master normalini belirler.
    use fem_kinds, only : rk, index_kind
    use fem_contact_types, only : contact_facet_t
    use fem_mesh, only : mesh_t
    use fem_status, only : status_t, FEM_STATUS_INVALID_ARGUMENT
    implicit none
    private
    public :: facet_coordinates, facet_normal, facet_aabb, closest_point_on_facet

contains

    subroutine facet_coordinates(mesh,facet,x,status)
        type(mesh_t),intent(in)::mesh
        type(contact_facet_t),intent(in)::facet
        real(rk),intent(out)::x(3,4)
        type(status_t),intent(out)::status
        integer::i
        integer(index_kind)::pos
        call status%clear();x=0.0_rk
        do i=1,4
            pos=mesh%find_node_position(facet%node_ids(i))
            if(pos==0_index_kind)then
                call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact facet node mesh icinde bulunamadi.");return
            end if
            x(:,i)=mesh%nodes(pos)%x
        end do
    end subroutine facet_coordinates

    subroutine facet_normal(mesh,facet,n,status)
        type(mesh_t),intent(in)::mesh
        type(contact_facet_t),intent(in)::facet
        real(rk),intent(out)::n(3)
        type(status_t),intent(out)::status
        real(rk)::x(3,4),a(3),b(3),c(3),normn,scale,planarity
        call facet_coordinates(mesh,facet,x,status);if(.not.status%is_ok())return
        a=x(:,2)-x(:,1);b=x(:,4)-x(:,1)
        n=cross3(a,b);normn=sqrt(dot_product(n,n))
        scale=max(1.0_rk,sqrt(dot_product(a,a)),sqrt(dot_product(b,b)))
        if(normn<=100.0_rk*epsilon(1.0_rk)*scale*scale)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Contact facet alani sifira yakin.");return
        end if
        n=n/normn
        c=x(:,3)-x(:,1);planarity=abs(dot_product(c,n))
        if(planarity>1.0e-10_rk*scale)then
            call status%set_error(FEM_STATUS_INVALID_ARGUMENT,"Rigid QUAD4 contact facet planar olmali.");return
        end if
    end subroutine facet_normal

    subroutine facet_aabb(mesh,facet,bmin,bmax,status)
        type(mesh_t),intent(in)::mesh
        type(contact_facet_t),intent(in)::facet
        real(rk),intent(out)::bmin(3),bmax(3)
        type(status_t),intent(out)::status
        real(rk)::x(3,4)
        integer::j
        call facet_coordinates(mesh,facet,x,status);if(.not.status%is_ok())return
        do j=1,3;bmin(j)=minval(x(j,:));bmax(j)=maxval(x(j,:));end do
    end subroutine facet_aabb

    subroutine closest_point_on_facet(mesh,facet,p,closest,n,gap,distance,status)
        type(mesh_t),intent(in)::mesh
        type(contact_facet_t),intent(in)::facet
        real(rk),intent(in)::p(3)
        real(rk),intent(out)::closest(3),n(3),gap,distance
        type(status_t),intent(out)::status
        real(rk)::x(3,4),c1(3),c2(3),d1,d2
        call facet_coordinates(mesh,facet,x,status);if(.not.status%is_ok())return
        call facet_normal(mesh,facet,n,status);if(.not.status%is_ok())return
        c1=closest_point_triangle(p,x(:,1),x(:,2),x(:,3));c2=closest_point_triangle(p,x(:,1),x(:,3),x(:,4))
        d1=dot_product(p-c1,p-c1);d2=dot_product(p-c2,p-c2)
        if(d1<=d2)then;closest=c1;distance=sqrt(max(0.0_rk,d1));else;closest=c2;distance=sqrt(max(0.0_rk,d2));end if
        gap=dot_product(p-closest,n)
    end subroutine closest_point_on_facet

    pure function closest_point_triangle(p,a,b,c) result(q)
        real(rk),intent(in)::p(3),a(3),b(3),c(3)
        real(rk)::q(3),ab(3),ac(3),ap(3),bp(3),cp(3),bc(3)
        real(rk)::d1,d2,d3,d4,d5,d6,vc,vb,va,v,w,denom
        ab=b-a;ac=c-a;ap=p-a;d1=dot_product(ab,ap);d2=dot_product(ac,ap)
        if(d1<=0.0_rk.and.d2<=0.0_rk)then;q=a;return;end if
        bp=p-b;d3=dot_product(ab,bp);d4=dot_product(ac,bp)
        if(d3>=0.0_rk.and.d4<=d3)then;q=b;return;end if
        vc=d1*d4-d3*d2
        if(vc<=0.0_rk.and.d1>=0.0_rk.and.d3<=0.0_rk)then;v=d1/(d1-d3);q=a+v*ab;return;end if
        cp=p-c;d5=dot_product(ab,cp);d6=dot_product(ac,cp)
        if(d6>=0.0_rk.and.d5<=d6)then;q=c;return;end if
        vb=d5*d2-d1*d6
        if(vb<=0.0_rk.and.d2>=0.0_rk.and.d6<=0.0_rk)then;w=d2/(d2-d6);q=a+w*ac;return;end if
        va=d3*d6-d5*d4
        bc=c-b
        if(va<=0.0_rk.and.(d4-d3)>=0.0_rk.and.(d5-d6)>=0.0_rk)then
            w=(d4-d3)/((d4-d3)+(d5-d6));q=b+w*bc;return
        end if
        denom=1.0_rk/(va+vb+vc);v=vb*denom;w=vc*denom;q=a+ab*v+ac*w
    end function closest_point_triangle

    pure function cross3(a,b) result(c)
        real(rk),intent(in)::a(3),b(3);real(rk)::c(3)
        c=[a(2)*b(3)-a(3)*b(2),a(3)*b(1)-a(1)*b(3),a(1)*b(2)-a(2)*b(1)]
    end function cross3
end module fem_contact_geometry
