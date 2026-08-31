#include "femcae/geometry/OcctStepImporter.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#if defined(FEMCAE_GEOMETRY_HAS_OCCT)
#include <BRepBndLib.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <Poly_Triangulation.hxx>
#include <Precision.hxx>
#include <Quantity_Color.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TCollection_AsciiString.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDataStd_Name.hxx>
#include <TDocStd_Document.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#endif

namespace femcae::geometry {

class OcctStepImporter::Impl {
public:
#if defined(FEMCAE_GEOMETRY_HAS_OCCT)
    std::unordered_map<GeometryEntityId, TopoDS_Shape> shapes;
    std::unordered_map<GeometryEntityId, GeometryEntityId> faceParents;

    // Alpha.3.3: Body altindaki CAD topology kimlikleri explorer tekrarlarina
    // gore degil, TopExp::MapShapes ile uretilen canonical/unique body-local
    // subshape map'lerine gore kaydedilir. Face/Edge/Vertex display provenance
    // bu GeometryEntityId -> TopoDS_Shape bagini kullanir.
    std::unordered_map<GeometryEntityId, std::vector<GeometryEntityId>> bodyFaces;
    std::unordered_map<GeometryEntityId, std::vector<GeometryEntityId>> bodyEdges;
    std::unordered_map<GeometryEntityId, std::vector<GeometryEntityId>> bodyVertices;
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
        impl_->bodyFaces.clear();
        impl_->bodyEdges.clear();
        impl_->bodyVertices.clear();
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

        // Solid ve shell/surface fallback ayni topology registration yolunu
        // kullanir. Boylece bir STEP'in temsil bicimi degisse bile Body altinda
        // Face/Edge/Vertex entity completeness farkli davranmaz.
        const auto registerBodyTopology = [&](const GeometryEntityId bodyId,
                                              const TopoDS_Shape& bodyShape,
                                              const std::string& bodyKey) {
            impl_->shapes.emplace(bodyId, bodyShape);

            TopTools_IndexedMapOfShape faceMap;
            TopTools_IndexedMapOfShape edgeMap;
            TopTools_IndexedMapOfShape vertexMap;
            TopExp::MapShapes(bodyShape, TopAbs_FACE, faceMap);
            TopExp::MapShapes(bodyShape, TopAbs_EDGE, edgeMap);
            TopExp::MapShapes(bodyShape, TopAbs_VERTEX, vertexMap);

            auto& bodyFaces = impl_->bodyFaces[bodyId];
            auto& bodyEdges = impl_->bodyEdges[bodyId];
            auto& bodyVertices = impl_->bodyVertices[bodyId];
            bodyFaces.reserve(static_cast<std::size_t>(faceMap.Extent()));
            bodyEdges.reserve(static_cast<std::size_t>(edgeMap.Extent()));
            bodyVertices.reserve(static_cast<std::size_t>(vertexMap.Extent()));

            for (Standard_Integer ordinal = 1; ordinal <= faceMap.Extent(); ++ordinal) {
                const std::string key = bodyKey + "/face/" + std::to_string(ordinal);
                const auto id = document.addEntity(GeometryEntityKind::Face, bodyId,
                                                   "Face " + std::to_string(ordinal), key, path);
                impl_->shapes.emplace(id, faceMap.FindKey(ordinal));
                impl_->faceParents.emplace(id, bodyId);
                bodyFaces.push_back(id);
                ++result.faceCount;
            }

            for (Standard_Integer ordinal = 1; ordinal <= edgeMap.Extent(); ++ordinal) {
                const std::string key = bodyKey + "/edge/" + std::to_string(ordinal);
                const auto id = document.addEntity(GeometryEntityKind::Edge, bodyId,
                                                   "Edge " + std::to_string(ordinal), key, path);
                impl_->shapes.emplace(id, edgeMap.FindKey(ordinal));
                bodyEdges.push_back(id);
                ++result.edgeCount;
            }

            for (Standard_Integer ordinal = 1; ordinal <= vertexMap.Extent(); ++ordinal) {
                const std::string key = bodyKey + "/vertex/" + std::to_string(ordinal);
                const auto id = document.addEntity(GeometryEntityKind::Vertex, bodyId,
                                                   "Vertex " + std::to_string(ordinal), key, path);
                impl_->shapes.emplace(id, vertexMap.FindKey(ordinal));
                bodyVertices.push_back(id);
                ++result.vertexCount;
            }
        };

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
                ++result.bodyCount;
                registerBodyTopology(bodyId, bodyShape, bodyKey);
            }

            // STEP dosyasi solid yerine shell/surface/face olarak gelebilir.
            // Fallback Body de solid Body ile ayni canonical Face/Edge/Vertex
            // registration kontratini kullanir.
            if (solidOrdinal == 0) {
                const std::string bodyKey = rootKey + "/body/root-shape";
                const auto bodyId = document.addEntity(GeometryEntityKind::Body, assemblyId,
                                                       rootName, bodyKey, path);
                ++result.bodyCount;
                registerBodyTopology(bodyId, rootShape, bodyKey);
            }
        }

        if (result.bodyCount == 0) {
            result.message = "STEP transfer edildi fakat body/surface geometry bulunamadi.";
            return result;
        }

        // Display topology verisi CAD dokumaninin hangi revision'undan
        // uretildigini tasir. Edge/Vertex dahil butun cache'lenmis topology
        // entity'leri ayni import revision'ina baglanir.
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

TopologyTessellation OcctStepImporter::tessellateWithTopology(const GeometryEntityId bodyId,
                                                               const double linearDeflection,
                                                               const double angularDeflectionRad) const {
#if !defined(FEMCAE_GEOMETRY_HAS_OCCT)
    (void)bodyId; (void)linearDeflection; (void)angularDeflectionRad;
    throw std::runtime_error("OCCT bulunamadi; CAD topology tessellation kullanilamaz.");
#else
    if (linearDeflection <= 0.0 || angularDeflectionRad <= 0.0) {
        throw std::invalid_argument("OCCT tessellation toleranslari pozitif olmali.");
    }
    const auto found = impl_->shapes.find(bodyId);
    if (found == impl_->shapes.end()) throw std::invalid_argument("Tessellation body ID importer cache'inde yok.");
    const auto bodyFaces = impl_->bodyFaces.find(bodyId);
    if (bodyFaces == impl_->bodyFaces.end()) {
        throw std::runtime_error("Tessellation body Face provenance cache'inde yok.");
    }

    const TopoDS_Shape& shape = found->second;
    BRepMesh_IncrementalMesh mesher(shape, linearDeflection, Standard_False, angularDeflectionRad, Standard_True);
    (void)mesher;

    TopologyTessellation out;
    out.display.sourceGeometryId = bodyId;
    const auto rev = impl_->sourceRevisions.find(bodyId);
    if (rev != impl_->sourceRevisions.end()) out.display.sourceRevision = rev->second;

    std::unordered_set<GeometryEntityId> visitedFaceIds;
    for (TopExp_Explorer faceIt(shape, TopAbs_FACE); faceIt.More(); faceIt.Next()) {
        const TopoDS_Face face = TopoDS::Face(faceIt.Current());

        // Canonical Face map sirasi ile TopExp_Explorer traversal sirasi ayni
        // olmak zorunda degildir. Face identity ordinal'dan degil IsSame ile
        // bulunur; Alpha.3.2 triangle->Face provenance bu sayede korunur.
        GeometryEntityId faceId = InvalidGeometryId;
        for (const GeometryEntityId candidateId : bodyFaces->second) {
            const auto storedFace = impl_->shapes.find(candidateId);
            if (storedFace != impl_->shapes.end() && storedFace->second.IsSame(face)) {
                faceId = candidateId;
                break;
            }
        }
        if (faceId == InvalidGeometryId) {
            throw std::runtime_error("OCCT tessellation Face'i canonical provenance map'inde bulunamadi.");
        }
        if (!visitedFaceIds.insert(faceId).second) {
            throw std::runtime_error("OCCT tessellation ayni canonical Face'i birden fazla kez traverse etti.");
        }

        TopLoc_Location location;
        const Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);
        if (triangulation.IsNull()) continue;
        const std::uint32_t base = static_cast<std::uint32_t>(out.display.points.size());
        for (Standard_Integer n = 1; n <= triangulation->NbNodes(); ++n) {
            gp_Pnt p = triangulation->Node(n);
            p.Transform(location.Transformation());
            out.display.points.push_back({p.X(), p.Y(), p.Z()});
        }
        for (Standard_Integer t = 1; t <= triangulation->NbTriangles(); ++t) {
            Standard_Integer n1{}, n2{}, n3{};
            triangulation->Triangle(t).Get(n1, n2, n3);
            if (face.Orientation() == TopAbs_REVERSED) std::swap(n2, n3);
            out.display.triangles.push_back({base + static_cast<std::uint32_t>(n1 - 1),
                                             base + static_cast<std::uint32_t>(n2 - 1),
                                             base + static_cast<std::uint32_t>(n3 - 1)});
            out.triangleFaceIds.push_back(faceId);
        }
    }

    if (visitedFaceIds.size() != bodyFaces->second.size()) {
        throw std::runtime_error("OCCT Face provenance map'i ile tessellation traversal'i uyusmuyor.");
    }
    if (!out.hasConsistentProvenance()) {
        throw std::runtime_error("Display triangle / CAD Face provenance boyutlari uyusmuyor.");
    }
    return out;
#endif
}

GeometryTessellation OcctStepImporter::tessellate(const GeometryEntityId bodyId,
                                                   const double linearDeflection,
                                                   const double angularDeflectionRad) const {
    return tessellateWithTopology(bodyId, linearDeflection, angularDeflectionRad).display;
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
