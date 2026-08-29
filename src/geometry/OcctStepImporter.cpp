#include "femcae/geometry/OcctStepImporter.h"

#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <utility>

#if defined(FEMCAE_GEOMETRY_HAS_OCCT)
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRep_Tool.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <Poly_Triangulation.hxx>
#include <Precision.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TCollection_AsciiString.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDataStd_Name.hxx>
#include <TDocStd_Document.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <Quantity_Color.hxx>
#endif

namespace femcae::geometry {

class OcctStepImporter::Impl {
public:
#if defined(FEMCAE_GEOMETRY_HAS_OCCT)
    std::unordered_map<GeometryEntityId, TopoDS_Shape> shapes;
    std::unordered_map<GeometryEntityId, GeometryEntityId> faceParents;
    std::unordered_map<GeometryEntityId, std::uint64_t> sourceRevisions;
#endif
};

OcctStepImporter::OcctStepImporter() : impl_(std::make_unique<Impl>()) {}
OcctStepImporter::~OcctStepImporter() = default;
OcctStepImporter::OcctStepImporter(OcctStepImporter&&) noexcept = default;
OcctStepImporter& OcctStepImporter::operator=(OcctStepImporter&&) noexcept = default;

bool OcctStepImporter::available() noexcept {
#if defined(FEMCAE_GEOMETRY_HAS_OCCT)
    return true;
#else
    return false;
#endif
}

StepImportResult OcctStepImporter::importFile(const std::string& path, GeometryDocument& document) {
    StepImportResult result;
#if !defined(FEMCAE_GEOMETRY_HAS_OCCT)
    (void)path;
    (void)document;
    result.message = "OCCT bulunamadi; STEP import adapter'i bu build'de devre disi.";
    return result;
#else
    try {
        impl_->shapes.clear();
        impl_->faceParents.clear();
        impl_->sourceRevisions.clear();
        Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
        Handle(TDocStd_Document) xdeDoc;
        app->NewDocument("MDTV-XCAF", xdeDoc);

        STEPCAFControl_Reader reader;
        reader.SetNameMode(Standard_True);
        reader.SetColorMode(Standard_True);
        reader.SetLayerMode(Standard_True);
        if (reader.ReadFile(path.c_str()) != IFSelect_RetDone) {
            result.message = "STEP dosyasi OCCT tarafindan okunamadi.";
            return result;
        }
        if (!reader.Transfer(xdeDoc)) {
            result.message = "STEP XDE/OCAF transfer basarisiz.";
            return result;
        }

        auto shapeTool = XCAFDoc_DocumentTool::ShapeTool(xdeDoc->Main());
        auto colorTool = XCAFDoc_DocumentTool::ColorTool(xdeDoc->Main());
        TDF_LabelSequence roots;
        shapeTool->GetFreeShapes(roots);

        document.clear();
        for (Standard_Integer rootIndex = 1; rootIndex <= roots.Length(); ++rootIndex) {
            const TDF_Label& rootLabel = roots.Value(rootIndex);
            TopoDS_Shape rootShape;
            if (!shapeTool->GetShape(rootLabel, rootShape) || rootShape.IsNull()) continue;

            std::string rootName = "STEP Root " + std::to_string(rootIndex);
            Handle(TDataStd_Name) nameAttr;
            if (rootLabel.FindAttribute(TDataStd_Name::GetID(), nameAttr)) {
                TCollection_AsciiString asciiName(nameAttr->Get(), '?');
                if (!asciiName.IsEmpty()) rootName = asciiName.ToCString();
            }

            const std::string rootKey = "step/root/" + std::to_string(rootIndex);
            const auto assemblyId = document.addEntity(GeometryEntityKind::Assembly, InvalidGeometryId,
                                                       rootName, rootKey, path);
            ++result.assemblyCount;

            Quantity_Color rootColor;
            if (colorTool->GetColor(rootLabel, XCAFDoc_ColorGen, rootColor)) {
                if (auto* e = document.findMutable(assemblyId)) {
                    e->color = RgbaColor{rootColor.Red(), rootColor.Green(), rootColor.Blue(), 1.0};
                }
            }

            std::size_t solidOrdinal = 0;
            for (TopExp_Explorer solidIt(rootShape, TopAbs_SOLID); solidIt.More(); solidIt.Next()) {
                const TopoDS_Shape bodyShape = solidIt.Current();
                ++solidOrdinal;
                const std::string bodyKey = rootKey + "/body/" + std::to_string(solidOrdinal);
                const auto bodyId = document.addEntity(GeometryEntityKind::Body, assemblyId,
                                                       "Body " + std::to_string(solidOrdinal), bodyKey, path);
                impl_->shapes.emplace(bodyId, bodyShape);
                ++result.bodyCount;

                std::size_t faceOrdinal = 0;
                for (TopExp_Explorer faceIt(bodyShape, TopAbs_FACE); faceIt.More(); faceIt.Next()) {
                    ++faceOrdinal;
                    const std::string faceKey = bodyKey + "/face/" + std::to_string(faceOrdinal);
                    const auto faceId = document.addEntity(GeometryEntityKind::Face, bodyId,
                                       "Face " + std::to_string(faceOrdinal), faceKey, path);
                    impl_->shapes.emplace(faceId, faceIt.Current());
                    impl_->faceParents.emplace(faceId, bodyId);
                    ++result.faceCount;
                }
                std::size_t edgeOrdinal = 0;
                for (TopExp_Explorer edgeIt(bodyShape, TopAbs_EDGE); edgeIt.More(); edgeIt.Next()) {
                    ++edgeOrdinal;
                    const std::string edgeKey = bodyKey + "/edge/" + std::to_string(edgeOrdinal);
                    document.addEntity(GeometryEntityKind::Edge, bodyId,
                                       "Edge " + std::to_string(edgeOrdinal), edgeKey, path);
                    ++result.edgeCount;
                }
                std::size_t vertexOrdinal = 0;
                for (TopExp_Explorer vertexIt(bodyShape, TopAbs_VERTEX); vertexIt.More(); vertexIt.Next()) {
                    ++vertexOrdinal;
                    const std::string vertexKey = bodyKey + "/vertex/" + std::to_string(vertexOrdinal);
                    document.addEntity(GeometryEntityKind::Vertex, bodyId,
                                       "Vertex " + std::to_string(vertexOrdinal), vertexKey, path);
                    ++result.vertexCount;
                }
            }

            // STEP dosyasi shell/surface olarak gelebilir; solid yoksa root'u body olarak sakla.
            if (solidOrdinal == 0) {
                const std::string bodyKey = rootKey + "/body/root-shape";
                const auto bodyId = document.addEntity(GeometryEntityKind::Body, assemblyId, rootName, bodyKey, path);
                impl_->shapes.emplace(bodyId, rootShape);
                ++result.bodyCount;
                std::size_t faceOrdinal = 0;
                for (TopExp_Explorer faceIt(rootShape, TopAbs_FACE); faceIt.More(); faceIt.Next()) {
                    ++faceOrdinal;
                    const auto faceId = document.addEntity(GeometryEntityKind::Face, bodyId,
                                       "Face " + std::to_string(faceOrdinal),
                                       bodyKey + "/face/" + std::to_string(faceOrdinal), path);
                    impl_->shapes.emplace(faceId, faceIt.Current());
                    impl_->faceParents.emplace(faceId, bodyId);
                    ++result.faceCount;
                }
            }
        }

        if (result.bodyCount == 0) {
            result.message = "STEP transfer edildi fakat body/surface geometry bulunamadi.";
            return result;
        }
        // Display tessellation CAD dokumaninin hangi revision'undan uretildigini
        // tasir. Boylece GUI eski triangulation'i solver mesh veya guncel CAD
        // sanmaz; provenance yalniz CAD katmaninda kalir.
        const auto importedRevision = document.revision();
        for (const auto& [geometryId, ignoredShape] : impl_->shapes) {
            (void)ignoredShape;
            impl_->sourceRevisions[geometryId] = importedRevision;
        }
        result.success = true;
        result.message = "OK";
        return result;
    } catch (const Standard_Failure& failure) {
        result.message = failure.GetMessageString() ? failure.GetMessageString() : "OCCT Standard_Failure";
        return result;
    } catch (const std::exception& ex) {
        result.message = ex.what();
        return result;
    }
#endif
}

GeometryTessellation OcctStepImporter::tessellate(const GeometryEntityId bodyId,
                                                   const double linearDeflection,
                                                   const double angularDeflectionRad) const {
#if !defined(FEMCAE_GEOMETRY_HAS_OCCT)
    (void)bodyId; (void)linearDeflection; (void)angularDeflectionRad;
    throw std::runtime_error("OCCT bulunamadi; CAD tessellation kullanilamaz.");
#else
    if (linearDeflection <= 0.0 || angularDeflectionRad <= 0.0) {
        throw std::invalid_argument("OCCT tessellation toleranslari pozitif olmali.");
    }
    const auto found = impl_->shapes.find(bodyId);
    if (found == impl_->shapes.end()) throw std::invalid_argument("Tessellation body ID importer cache'inde yok.");

    const TopoDS_Shape& shape = found->second;
    BRepMesh_IncrementalMesh mesher(shape, linearDeflection, Standard_False, angularDeflectionRad, Standard_True);
    (void)mesher;

    GeometryTessellation out;
    out.sourceGeometryId = bodyId;
    const auto rev = impl_->sourceRevisions.find(bodyId);
    if (rev != impl_->sourceRevisions.end()) out.sourceRevision = rev->second;
    for (TopExp_Explorer faceIt(shape, TopAbs_FACE); faceIt.More(); faceIt.Next()) {
        const TopoDS_Face face = TopoDS::Face(faceIt.Current());
        TopLoc_Location location;
        const Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);
        if (triangulation.IsNull()) continue;
        const std::uint32_t base = static_cast<std::uint32_t>(out.points.size());
        for (Standard_Integer n = 1; n <= triangulation->NbNodes(); ++n) {
            gp_Pnt p = triangulation->Node(n);
            p.Transform(location.Transformation());
            out.points.push_back({p.X(), p.Y(), p.Z()});
        }
        for (Standard_Integer t = 1; t <= triangulation->NbTriangles(); ++t) {
            Standard_Integer n1{}, n2{}, n3{};
            triangulation->Triangle(t).Get(n1, n2, n3);
            if (face.Orientation() == TopAbs_REVERSED) std::swap(n2, n3);
            out.triangles.push_back({base + static_cast<std::uint32_t>(n1 - 1),
                                     base + static_cast<std::uint32_t>(n2 - 1),
                                     base + static_cast<std::uint32_t>(n3 - 1)});
        }
    }
    return out;
#endif
}


std::optional<StepAxisAlignedBoxDescriptor> OcctStepImporter::axisAlignedBoxDescriptor(const GeometryEntityId bodyId,
                                                                                        const double tolerance) const {
#if !defined(FEMCAE_GEOMETRY_HAS_OCCT)
    (void)bodyId; (void)tolerance;
    return std::nullopt;
#else
    if (tolerance <= 0.0) throw std::invalid_argument("Axis-aligned box tolerance pozitif olmali.");
    const auto bodyIt=impl_->shapes.find(bodyId);
    if(bodyIt==impl_->shapes.end()) return std::nullopt;
    Bnd_Box bodyBox;BRepBndLib::Add(bodyIt->second,bodyBox);
    Standard_Real xmin{},ymin{},zmin{},xmax{},ymax{},zmax{};bodyBox.Get(xmin,ymin,zmin,xmax,ymax,zmax);
    const double scale=std::max({1.0,std::abs(xmax-xmin),std::abs(ymax-ymin),std::abs(zmax-zmin)});
    // BRepBndLib kutulari STEP topoloji toleransini da kapsar. Kullanici toleransi
    // Precision::Confusion() degerinin altinda kalsa bile sifir kalinlikli duzlem
    // yuzlerini sayisal bbox genislemesi nedeniyle reddetmemeliyiz.
    const double tol=std::max(tolerance*scale, 10.0*static_cast<double>(Precision::Confusion()));
    StepAxisAlignedBoxDescriptor d;d.bodyId=bodyId;d.min={xmin,ymin,zmin};d.max={xmax,ymax,zmax};
    auto assign=[&](GeometryEntityId& slot,const GeometryEntityId id){if(slot!=InvalidGeometryId) return false;slot=id;return true;};
    for(const auto& [faceId,parent] : impl_->faceParents){
        if(parent!=bodyId)continue;const auto fit=impl_->shapes.find(faceId);if(fit==impl_->shapes.end())continue;
        Bnd_Box fb;BRepBndLib::Add(fit->second,fb);Standard_Real fx0{},fy0{},fz0{},fx1{},fy1{},fz1{};fb.Get(fx0,fy0,fz0,fx1,fy1,fz1);
        if(std::abs(fx1-fx0)<=tol){const double x=0.5*(fx0+fx1);if(std::abs(x-xmin)<=tol){if(!assign(d.xMinFace,faceId))return std::nullopt;continue;}if(std::abs(x-xmax)<=tol){if(!assign(d.xMaxFace,faceId))return std::nullopt;continue;}}
        if(std::abs(fy1-fy0)<=tol){const double y=0.5*(fy0+fy1);if(std::abs(y-ymin)<=tol){if(!assign(d.yMinFace,faceId))return std::nullopt;continue;}if(std::abs(y-ymax)<=tol){if(!assign(d.yMaxFace,faceId))return std::nullopt;continue;}}
        if(std::abs(fz1-fz0)<=tol){const double z=0.5*(fz0+fz1);if(std::abs(z-zmin)<=tol){if(!assign(d.zMinFace,faceId))return std::nullopt;continue;}if(std::abs(z-zmax)<=tol){if(!assign(d.zMaxFace,faceId))return std::nullopt;continue;}}
        return std::nullopt; // box disi veya axis-aligned olmayan face
    }
    if(d.xMinFace==InvalidGeometryId||d.xMaxFace==InvalidGeometryId||d.yMinFace==InvalidGeometryId||d.yMaxFace==InvalidGeometryId||d.zMinFace==InvalidGeometryId||d.zMaxFace==InvalidGeometryId)return std::nullopt;
    return d;
#endif
}

} // namespace femcae::geometry
