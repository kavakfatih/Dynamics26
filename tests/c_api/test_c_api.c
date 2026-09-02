#include "femcae/femcae.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static int require_equal(const char *name, int actual, int expected)
{
    if (actual != expected) {
        fprintf(stderr, "FAIL %s: actual=%d expected=%d\n", name, actual, expected);
        return 1;
    }
    return 0;
}

int main(void)
{
    int failed = 0;
    failed |= require_equal("api", fem_api_version(), 1);
    failed |= require_equal("project_schema", fem_project_schema_version(), 1);
    failed |= require_equal("result_schema", fem_result_schema_version(), 1);
    failed |= require_equal("version_major", fem_version_major(), 1);
    failed |= require_equal("version_minor", fem_version_minor(), 0);

    {
        const double params[2] = {1.0e6, 0.4e6};
        double g0 = 0.0, p11 = 0.0, energy = 0.0;
        failed |= require_equal("hyperelastic_validate", fem_hyperelastic_validate(2, 30.0e6, 2, params, &g0), 0);
        if (g0 <= 0.0) { fprintf(stderr, "FAIL hyperelastic G0\n"); failed = 1; }
        failed |= require_equal("hyperelastic_preview", fem_hyperelastic_isochoric_uniaxial_preview(2, 30.0e6, 2, params, 1.2, &p11, &energy), 0);
        if (!(p11 > 0.0 && energy > 0.0)) { fprintf(stderr, "FAIL hyperelastic preview values\n"); failed = 1; }
    }
    failed |= require_equal("version_patch", fem_version_patch(), 2);

    {
        double u = 0.0, s = 0.0, r = 0.0;
        int rc = fem_demo_axial_bar(200.0e9, 1.0e-4, 2.0, 1000.0, &u, &s, &r);
        if (rc != 0 || u < 9.999e-5 || u > 1.0001e-4 || s != 1.0e7 || r != -1000.0) {
            fprintf(stderr, "FAIL axial demo rc=%d u=%.17g s=%.17g r=%.17g\n", rc, u, s, r);
            failed = 1;
        }
    }

    {
        double f1=0.0,f2=0.0,m11=0.0,m12=0.0,m21=0.0,m22=0.0;
        int rc=fem_demo_axial_modal(210.0e9,7800.0,1.0e-4,2.0,&f1,&f2,&m11,&m12,&m21,&m22);
        if(rc!=0 || !(f1>0.0) || !(f2>f1) || m12==0.0 || m22==0.0){
            fprintf(stderr,"FAIL modal demo rc=%d f1=%.17g f2=%.17g\n",rc,f1,f2);
            failed=1;
        }
    }

    {
        const double e=6.0e6, nu=0.29, area=1.0, length=1.0, stretch=1.10;
        const double g=e/(2.0*(1.0+nu));
        const double lam=e*nu/((1.0+nu)*(1.0-2.0*nu));
        const double e11=0.5*(stretch*stretch-1.0);
        const double force=area*stretch*(lam+2.0*g)*e11;
        double u=0.0,lf=0.0,rn=0.0; int steps=0,iters=0,cutbacks=0,hcount=0;
        int hatt[64]={0},hiter[64]={0},hconv[64]={0};
        double hload[64]={0.0},hres[64]={0.0},hdisp[64]={0.0},halpha[64]={0.0};
        int rc=fem_demo_nonlinear_hex8(e,nu,area,length,force,0.25,0.01,0.5,1,1,25,1,&u,&lf,&rn,&steps,&iters,&cutbacks,
                                       64,&hcount,hatt,hiter,hload,hres,hdisp,halpha,hconv);
        if(rc!=0 || u<0.099999 || u>0.100001 || lf!=1.0 || steps<1 || iters<1 || hcount<1){
            fprintf(stderr,"FAIL nonlinear demo rc=%d u=%.17g lf=%.17g steps=%d iters=%d cutbacks=%d rn=%.17g\n",
                    rc,u,lf,steps,iters,cutbacks,rn);
            failed=1;
        }
    }

    {
        const double e=6.0e6, nu=0.29, area=1.0, length=1.0, stretch=1.10;
        const double g=e/(2.0*(1.0+nu));
        const double lam=e*nu/((1.0+nu)*(1.0-2.0*nu));
        const double e11=0.5*(stretch*stretch-1.0);
        const double force=area*stretch*(lam+2.0*g)*e11;
        double u=0.0,lf=0.0,rn=0.0,minj=0.0;
        int steps=0,iters=0,cutbacks=0,hcount=0;
        int hatt[64]={0},hstep[64]={0},hiter[64]={0},hconv[64]={0};
        double hload[64]={0.0},hinc[64]={0.0},habsr[64]={0.0},hrelr[64]={0.0};
        double habsdu[64]={0.0},hreldu[64]={0.0},halpha[64]={0.0},hminj[64]={0.0};
        int rc=fem_demo_nonlinear_hex8_diagnostics(
            e,nu,area,length,force,0.25,0.01,0.5,1,1,25,1,
            &u,&lf,&rn,&minj,&steps,&iters,&cutbacks,64,&hcount,
            hatt,hstep,hiter,hload,hinc,habsr,hrelr,habsdu,hreldu,halpha,hminj,hconv);
        if(rc!=0 || u<0.099999 || u>0.100001 || lf!=1.0 || steps<1 || iters<1 || hcount<1 || minj<=0.0){
            fprintf(stderr,"FAIL nonlinear diagnostics rc=%d u=%.17g lf=%.17g minJ=%.17g steps=%d iters=%d hcount=%d\n",
                    rc,u,lf,minj,steps,iters,hcount);
            failed=1;
        } else {
            int i;
            for(i=0;i<hcount;i++){
                if(hatt[i]<1 || hstep[i]<0 || hiter[i]<1 || hinc[i]<=0.0 || habsr[i]<0.0 || habsdu[i]<0.0 || hminj[i]<=0.0){
                    fprintf(stderr,"FAIL nonlinear diagnostics row=%d att=%d step=%d iter=%d dload=%.17g R=%.17g du=%.17g minJ=%.17g\n",
                            i,hatt[i],hstep[i],hiter[i],hinc[i],habsr[i],habsdu[i],hminj[i]);
                    failed=1;
                    break;
                }
            }
        }
    }

    {
        double gamma=0.0,p=0.0,lf=0.0,pr=0.0; int iters=0;
        int rc=fem_demo_mixed_up_hex8_shear(0.9e6,2.0e9,0.12,&gamma,&p,&lf,&pr,&iters);
        if(rc!=0 || gamma<0.119999 || gamma>0.120001 || fabs(p)>1.0e-4 || lf!=1.0 || pr>1.0e-10 || iters<1){
            fprintf(stderr,"FAIL mixed u-p demo rc=%d gamma=%.17g p=%.17g lf=%.17g pr=%.17g iters=%d\n",rc,gamma,p,lf,pr,iters);
            failed=1;
        }
    }

    {
        double pen=0.0,nf=0.0; int active=0,iters=0;
        int rc=fem_demo_contact_hex8(1.0e6,0.30,1.0e8,1000.0,1,&pen,&nf,&active,&iters);
        if(rc!=0 || active!=4 || pen<=0.0 || pen>1.0e-4 || fabs(nf-1000.0)>1.0e-2 || iters<1){
            fprintf(stderr,"FAIL contact demo rc=%d pen=%.17g nf=%.17g active=%d iters=%d\n",rc,pen,nf,active,iters);
            failed=1;
        }
    }

    if (failed) {
        return EXIT_FAILURE;
    }

    puts("PASS c_api_smoke");
    return EXIT_SUCCESS;
}
