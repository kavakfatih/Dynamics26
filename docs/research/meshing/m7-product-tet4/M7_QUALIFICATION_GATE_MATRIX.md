# M7 Qualification Gate Matrix

Status: **CONTRACT FROZEN / EXECUTABLE EVIDENCE PENDING**  
Applies to: **D26-M7-FREEZE-1**

| Gate | Frozen requirement | Minimum evidence | Current state |
|---|---|---|---|
| M7-G01 | TRI3/TET4 topology metadata | topology/connectivity/reference metadata first-class and versioned | DEFINED / PENDING |
| M7-G02 | mesh storage migration | SimulationMesh safely stores required boundary/volume topology | DEFINED / PENDING |
| M7-G03 | AnalysisSnapshot migration | immutable snapshot represents TET4 without HEX8 assumptions | DEFINED / PENDING |
| M7-G04 | solver-input topology contract | flattening carries explicit topology/formulation identities | DEFINED / PENDING |
| M7-G05 | C ABI/schema versioning | additive/versioned ABI and persistence migration evidence | DEFINED / PENDING |
| M7-G06 | Body/Region provenance | every volume TET4 resolves persistent Body/Region identity | DEFINED / PENDING |
| M7-G07 | Face/TRI3 provenance | every boundary TRI3 resolves persistent Face identity | DEFINED / PENDING |
| M7-G08 | Edge/Vertex node provenance | constrained nodes retain Edge/Vertex GeometryEntityId | DEFINED / PENDING |
| M7-G09 | GeometryAssociationMap integrity | mapping complete, non-coordinate-derived and generation-consistent | DEFINED / PENDING |
| M7-G10 | geometry Named Selection remesh resolution | geometry scopes re-resolve through provenance after remesh | DEFINED / PENDING |
| M7-G11 | mesh-scope stale-generation behavior | old mesh-generation scopes become STALE without silent rebind | DEFINED / PENDING |
| M7-G12 | TRI3 support/load integration | surface supports/loads integrate correctly on TRI3 facets | DEFINED / PENDING |
| M7-G13 | save/reopen lifecycle | TET4 topology/provenance/scopes survive persistence round-trip | DEFINED / PENDING |
| M7-G14 | TET4 patch-test handoff | product contract feeds dedicated solver patch-test qualification | DEFINED / PENDING |
| M7-G15 | convergence handoff | data contract supports mesh-refinement convergence studies | DEFINED / PENDING |
| M7-G16 | M6 quality x solver correlation | quality telemetry correlates with solver outcomes without conflation | DEFINED / PENDING |
| M7-G17 | nonlinear distortion reporting | runtime distortion stays separate from initial mesh quality | DEFINED / PENDING |
| M7-G18 | truthful formulation capability language | capability matrix separates mesh/compressible/near-incompressible/rubber states | DEFINED / PENDING |

## Solver gate boundary

Do not duplicate the dedicated TET4 formulation qualification under `docs/research/fem/tet4-nearly-incompressible/`. M7-G14..G18 are product/handoff gates, not formulation qualification.
