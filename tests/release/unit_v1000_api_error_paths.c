#include <femcae/femcae.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int call_mesh(const long long *nodes,const long long *conn,double e,int cc,const int *components){
    const double xyz[24]={0,0,0, 1,0,0, 1,1,0, 0,1,0, 0,0,1, 1,0,1, 1,1,1, 0,1,1};
    const long long eid[1]={20};
    const long long cn[3]={1,1,1}; const double cv[3]={0,0,0};
    const long long ln[1]={2}; const int lc[1]={1}; const double lv[1]={1};
    double u[24],r[24],vm[1]; memset(u,0,sizeof(u));memset(r,0,sizeof(r));vm[0]=0;
    return fem_solve_linear_hex8_mesh(8,nodes,xyz,1,eid,conn,e,0.3,cc,cn,components,cv,1,ln,lc,lv,u,r,vm);
}
int main(void){
    const long long nodes[8]={1,2,3,4,5,6,7,8};
    const long long conn[8]={1,2,3,4,5,6,7,8};
    const int good_components[3]={1,2,3};
    assert(call_mesh(nodes,conn,-1.0,3,good_components)!=0);

    long long duplicate[8]={1,2,3,4,5,6,7,7};
    assert(call_mesh(duplicate,conn,1000.0,3,good_components)!=0);

    long long missing[8]={1,2,3,4,5,6,7,999};
    assert(call_mesh(nodes,missing,1000.0,3,good_components)!=0);

    const int bad_component[3]={1,2,4};
    assert(call_mesh(nodes,conn,1000.0,3,bad_component)!=0);

    puts("PASS V1.0 public C API invalid-input paths");
    return 0;
}
