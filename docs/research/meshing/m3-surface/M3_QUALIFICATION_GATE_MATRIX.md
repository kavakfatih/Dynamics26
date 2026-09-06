# M3 Qualification Gate Matrix

Status: **CONTRACT FROZEN / EXECUTABLE EVIDENCE PENDING**  
Applies to: **D26-M3-FREEZE-1**

| Gate | Frozen requirement | Minimum evidence | Current state |
|---|---|---|---|
| M3-G01 | authoritative B-Rep source | display tessellation cannot alter surface-mesh input | DEFINED / PENDING |
| M3-G02 | canonical shared-edge discretization | one CAD Edge produces one ordered physical 3D node chain | DEFINED / PENDING |
| M3-G03 | adjacent-face physical-node identity | incident Faces consume identical shared-edge MeshNodeIds | DEFINED / PENDING |
| M3-G04 | FaceUse/pcurve separation | orientation/pcurve selected per FaceUse, not per physical edge | DEFINED / PENDING |
| M3-G05 | seam handling | distinct seam chart uses share physical nodes correctly | DEFINED / PENDING |
| M3-G06 | degenerated-edge/pole handling | no invalid repeated-node TRI3 at poles/degenerated edges | DEFINED / PENDING |
| M3-G07 | chart alias -> physical node correctness | many-to-one aliases preserve valid physical topology | DEFINED / PENDING |
| M3-G08 | pcurve endpoint reconciliation | tolerance-consistent non-bit-identical UV endpoints reconcile deterministically | DEFINED / PENDING |
| M3-G09 | constrained triangulation correctness | all wire constraints represented without illegal crossings/gaps | DEFINED / PENDING |
| M3-G10 | triangle orientation / zero-area rejection | deterministic physical validity oracle rejects invalid TRI3 | DEFINED / PENDING |
| M3-G11 | physical CAD surface deviation | measured CAD-vs-TRI3 deviation satisfies policy | DEFINED / PENDING |
| M3-G12 | normal deviation | measured CAD-normal versus TRI3-normal error satisfies policy | DEFINED / PENDING |
| M3-G13 | Face GeometryEntityId provenance | every TRI3 resolves its source CAD Face | DEFINED / PENDING |
| M3-G14 | deterministic fingerprint/replay | same input/settings yields identical topology/provenance fingerprint | DEFINED / PENDING |
| M3-G15 | typed explicit failure | unsupported/inconsistent geometry cannot become partial silent success | DEFINED / PENDING |

## Release rule

M3 is not qualified until all mandatory M3-G01..M3-G15 gates have executable evidence on the exact source HEAD being qualified.

CAD tolerance in M3-G08 is limited to B-Rep reconciliation and must not create a general FEM topology epsilon.
