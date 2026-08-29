#include "PrePostPanel.h"

#include <femcae/femcae.h>
#include <femcae/meshing/AssignmentResolver.h>

#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QJsonObject>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <vector>

using namespace femcae::meshing;

namespace {
QDoubleSpinBox *spin(double value,double lo,double hi,int decimals,const QString& suffix){auto*s=new QDoubleSpinBox;s->setRange(lo,hi);s->setDecimals(decimals);s->setValue(value);s->setSuffix(suffix);return s;}
constexpr femcae::geometry::GeometryEntityId BodyId=100;
constexpr femcae::geometry::GeometryEntityId XMinId=101;
constexpr femcae::geometry::GeometryEntityId XMaxId=102;
constexpr femcae::geometry::GeometryEntityId YMinId=103;
constexpr femcae::geometry::GeometryEntityId YMaxId=104;
constexpr femcae::geometry::GeometryEntityId ZMinId=105;
constexpr femcae::geometry::GeometryEntityId ZMaxId=106;
}

PrePostPanel::PrePostPanel(QWidget *parent):QWidget(parent)
{
    auto *outer=new QVBoxLayout(this);
    auto *meshGroup=new QGroupBox(tr("Structured HEX8 Mesh — Verified Baseline"),this);
    auto *form=new QFormLayout(meshGroup);
    length_=spin(100.0,0.001,1.0e6,3,tr(" mm"));width_=spin(20.0,0.001,1.0e6,3,tr(" mm"));height_=spin(20.0,0.001,1.0e6,3,tr(" mm"));
    nx_=new QSpinBox;ny_=new QSpinBox;nz_=new QSpinBox;for(auto*s:{nx_,ny_,nz_}){s->setRange(1,100);s->setValue(2);}ny_->setValue(1);nz_->setValue(1);
    auto *generate=new QPushButton(tr("Mesh Oluştur"),meshGroup);meshSummary_=new QLabel(tr("Henüz FEM mesh yok."),meshGroup);meshSummary_->setWordWrap(true);
    form->addRow(tr("Uzunluk"),length_);form->addRow(tr("Genişlik"),width_);form->addRow(tr("Yükseklik"),height_);form->addRow(tr("Nx"),nx_);form->addRow(tr("Ny"),ny_);form->addRow(tr("Nz"),nz_);form->addRow(generate);form->addRow(meshSummary_);

    auto *solveGroup=new QGroupBox(tr("Geometry Assignment → Linear Solve → Post"),this);auto *solveForm=new QFormLayout(solveGroup);
    young_=spin(210.0,0.001,1.0e6,3,tr(" GPa"));poisson_=spin(0.30,-0.99,0.4999,4,QString());force_=spin(1000.0,-1e12,1e12,3,tr(" N"));
    auto *solve=new QPushButton(tr("Mesh Modelini Çöz"),solveGroup);solveSummary_=new QLabel(tr("Henüz çözüm yok."),solveGroup);solveSummary_->setWordWrap(true);
    solveForm->addRow(tr("Young Modülü"),young_);solveForm->addRow(tr("Poisson"),poisson_);solveForm->addRow(tr("X-max Toplam Yük"),force_);solveForm->addRow(solve);solveForm->addRow(solveSummary_);

    auto *exportRow=new QHBoxLayout;auto*csv=new QPushButton(tr("CSV Export"),this);auto*vtk=new QPushButton(tr("VTK Export"),this);auto*cut=new QPushButton(tr("Orta Kesit Probe"),this);exportRow->addWidget(cut);exportRow->addWidget(csv);exportRow->addWidget(vtk);
    auto *note=new QLabel(tr("Bu akış display tessellation kullanmaz. Structured box geometry için gerçek HEX8 mesh üretir; x-min face constraint ve x-max face load provenance üzerinden FEM node'larına çözülür. Genel arbitrary STEP volume meshing henüz production-ready değildir."),this);note->setWordWrap(true);
    cutSummary_=new QLabel(tr("Section cut henüz değerlendirilmedi."),this);cutSummary_->setWordWrap(true);outer->addWidget(meshGroup);outer->addWidget(solveGroup);outer->addLayout(exportRow);outer->addWidget(cutSummary_);outer->addWidget(note);outer->addStretch(1);
    connect(generate,&QPushButton::clicked,this,&PrePostPanel::generateMesh);connect(solve,&QPushButton::clicked,this,&PrePostPanel::solveLinear);connect(cut,&QPushButton::clicked,this,&PrePostPanel::evaluateMidSectionCut);connect(csv,&QPushButton::clicked,this,&PrePostPanel::exportCsv);connect(vtk,&QPushButton::clicked,this,&PrePostPanel::exportVtk);
}


QJsonObject PrePostPanel::projectJson() const
{
    QJsonObject o;
    o["length_mm"]=length_->value();o["width_mm"]=width_->value();o["height_mm"]=height_->value();
    o["nx"]=nx_->value();o["ny"]=ny_->value();o["nz"]=nz_->value();
    o["young_gpa"]=young_->value();o["poisson"]=poisson_->value();o["force_n"]=force_->value();
    o["meshing_contract"]="geometry_provenance_not_display_tessellation";
    return o;
}

void PrePostPanel::loadProjectJson(const QJsonObject &o)
{
    clearProject();
    length_->setValue(o.value("length_mm").toDouble(100.0));width_->setValue(o.value("width_mm").toDouble(20.0));height_->setValue(o.value("height_mm").toDouble(20.0));
    nx_->setValue(o.value("nx").toInt(2));ny_->setValue(o.value("ny").toInt(1));nz_->setValue(o.value("nz").toInt(1));
    young_->setValue(o.value("young_gpa").toDouble(210.0));poisson_->setValue(o.value("poisson").toDouble(0.30));force_->setValue(o.value("force_n").toDouble(1000.0));
}

void PrePostPanel::setViewportConsumer(std::function<void(const SimulationMesh &,const ResultDatabase &)> consumer){viewportConsumer_=std::move(consumer);}
void PrePostPanel::clearProject(){mesh_={};assignments_.clear();results_.clear();meshSummary_->setText(tr("Henüz FEM mesh yok."));solveSummary_->setText(tr("Henüz çözüm yok."));if(cutSummary_)cutSummary_->setText(tr("Section cut henüz değerlendirilmedi."));}

void PrePostPanel::rebuildAssignments(){
    assignments_.clear();GeometryAssignment mat;mat.kind=AssignmentKind::Material;mat.targetGeometryId=BodyId;mat.referencedEntityId=1;mat.name="Default linear material";assignments_.add(mat);
    GeometryAssignment bc;bc.kind=AssignmentKind::Constraint;bc.targetGeometryId=XMinId;bc.constrained={true,true,true};bc.name="Fixed x-min";assignments_.add(bc);
    GeometryAssignment load;load.kind=AssignmentKind::Load;load.targetGeometryId=XMaxId;load.vectorValue={force_->value(),0,0};load.name="X-max total force";assignments_.add(load);
}

void PrePostPanel::generateMesh(){
    StructuredHexMesherOptions o;o.nx=static_cast<std::size_t>(nx_->value());o.ny=static_cast<std::size_t>(ny_->value());o.nz=static_cast<std::size_t>(nz_->value());
    StructuredHexMesher mesher;const BoxBoundaryGeometry g{BodyId,XMinId,XMaxId,YMinId,YMaxId,ZMinId,ZMaxId};
    const double l=length_->value()*1e-3,w=width_->value()*1e-3,h=height_->value()*1e-3;
    mesh_=mesher.meshBox({{0,0,0},{l,w,h}},g,1,o);results_.clear();rebuildAssignments();
    const auto q=evaluateHexMeshQuality(mesh_);
    meshSummary_->setText(tr("Nodes=%1  HEX8=%2  Boundary facets=%3\nmin scaled-J=%4  max aspect=%5  inverted=%6")
        .arg(mesh_.nodes.size()).arg(mesh_.elements.size()).arg(mesh_.boundaryFacets.size()).arg(q.minimumScaledJacobian,0,'g',6).arg(q.maximumAspectRatio,0,'g',6).arg(q.invertedElementCount));
    emit message(tr("V1.0 structured mesh oluşturuldu: %1 node, %2 HEX8").arg(mesh_.nodes.size()).arg(mesh_.elements.size()));
    if(viewportConsumer_)viewportConsumer_(mesh_,results_);
}

bool PrePostPanel::ensureMesh(){if(mesh_.elements.empty()){generateMesh();}return !mesh_.elements.empty();}

void PrePostPanel::solveLinear(){
    if(!ensureMesh())return;rebuildAssignments();const auto resolved=resolveAssignments(mesh_,assignments_);
    std::vector<long long> nids,eids,conn,cn,ln;std::vector<double> xyz,cv,lv;std::vector<int> cc,lc;
    for(const auto&n:mesh_.nodes){nids.push_back(n.id);xyz.insert(xyz.end(),{n.x.x,n.x.y,n.x.z});}
    for(const auto&e:mesh_.elements){eids.push_back(e.id);for(auto id:e.nodeIds)conn.push_back(id);}
    std::vector<MeshEntityId> fixed,loaded;for(const auto&r:resolved){if(r.source.kind==AssignmentKind::Constraint)fixed=r.nodeIds;else if(r.source.kind==AssignmentKind::Load)loaded=r.nodeIds;}
    for(const auto id:fixed)for(int c=1;c<=3;++c){cn.push_back(id);cc.push_back(c);cv.push_back(0.0);}
    if(loaded.empty()){solveSummary_->setText(tr("X-max face provenance node listesi boş."));return;}
    for(const auto id:loaded){ln.push_back(id);lc.push_back(1);lv.push_back(force_->value()/static_cast<double>(loaded.size()));}
    std::vector<double> u(3*mesh_.nodes.size()),reactions(3*mesh_.nodes.size()),vm(mesh_.elements.size());
    const int rc=fem_solve_linear_hex8_mesh(static_cast<int>(mesh_.nodes.size()),nids.data(),xyz.data(),static_cast<int>(mesh_.elements.size()),eids.data(),conn.data(),young_->value()*1e9,poisson_->value(),
        static_cast<int>(cn.size()),cn.data(),cc.data(),cv.data(),static_cast<int>(ln.size()),ln.data(),lc.data(),lv.data(),u.data(),reactions.data(),vm.data());
    if(rc!=0){solveSummary_->setText(tr("Fortran linear mesh solve başarısız. Status=%1").arg(rc));emit message(solveSummary_->text());return;}
    NodeVectorField disp;disp.name="displacement";ElementScalarField stress;stress.name="von_mises";double maxU=0,maxVm=0,reactionX=0;
    for(std::size_t i=0;i<mesh_.nodes.size();++i){disp.values[mesh_.nodes[i].id]={u[3*i],u[3*i+1],u[3*i+2]};maxU=std::max(maxU,std::sqrt(u[3*i]*u[3*i]+u[3*i+1]*u[3*i+1]+u[3*i+2]*u[3*i+2]));reactionX+=reactions[3*i];}
    for(std::size_t i=0;i<mesh_.elements.size();++i){stress.values[mesh_.elements[i].id]=vm[i];maxVm=std::max(maxVm,vm[i]);}
    results_.clear();results_.setDisplacement(std::move(disp));results_.setElementScalar(std::move(stress));
    const auto probe=results_.probeNearestNode(mesh_,{length_->value()*1e-3,width_->value()*1e-3,height_->value()*1e-3});
    solveSummary_->setText(tr("Solve OK\nmax |u| = %1 mm\nmax von Mises = %2 MPa\nΣRx = %3 N\nProbe node=%4, ux=%5 mm")
        .arg(maxU*1e3,0,'g',7).arg(maxVm/1e6,0,'g',7).arg(reactionX,0,'g',7).arg(probe?probe->nodeId:-1).arg(probe?probe->vectorValue.x*1e3:0.0,0,'g',7));
    emit message(tr("V1.0 mesh → Fortran solve → post-processing tamamlandı."));
    emit solveCompleted(maxU*1e3,maxVm/1e6,reactionX,static_cast<qlonglong>(probe?probe->nodeId:-1),probe?probe->vectorValue.x*1e3:0.0);
    if(viewportConsumer_)viewportConsumer_(mesh_,results_);
}

void PrePostPanel::exportCsv(){if(!ensureMesh())return;const auto path=QFileDialog::getSaveFileName(this,tr("Sonuç CSV Kaydet"),"femcae-results.csv",tr("CSV (*.csv)"));if(path.isEmpty())return;try{results_.exportCsv(mesh_,path.toStdString());emit message(tr("CSV export: %1").arg(path));}catch(const std::exception&e){emit message(tr("CSV export başarısız: %1").arg(e.what()));}}
void PrePostPanel::exportVtk(){if(!ensureMesh())return;const auto path=QFileDialog::getSaveFileName(this,tr("Legacy VTK Kaydet"),"femcae-results.vtk",tr("VTK (*.vtk)"));if(path.isEmpty())return;try{results_.exportLegacyVtk(mesh_,path.toStdString(),1.0);emit message(tr("VTK export: %1").arg(path));}catch(const std::exception&e){emit message(tr("VTK export başarısız: %1").arg(e.what()));}}

void PrePostPanel::evaluateMidSectionCut()
{
    if(!ensureMesh()) return;
    const double x=0.5*length_->value()*1e-3;
    const auto ids=results_.cutElements(mesh_,{{x,0,0},{1,0,0},1e-10});
    cutSummary_->setText(tr("x=L/2 section cut: %1 HEX8 kesiliyor. Baseline API element selection döndürür; cut-surface stress interpolation henüz yapılmaz.").arg(ids.size()));
    emit message(cutSummary_->text());
}
