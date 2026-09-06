# M5 Qualification Gate Matrix

Status: **CONTRACT FROZEN / EXECUTABLE EVIDENCE PENDING**  
Applies to: **D26-M5-FREEZE-1**

| Gate | Frozen requirement | Minimum evidence | Current state |
|---|---|---|---|
| M5-G01 | global size | uniform target produces expected monotone density response | DEFINED / PENDING |
| M5-G02 | GeometryEntityId scoped size | persistent CAD scope affects only intended region | DEFINED / PENDING |
| M5-G03 | Named Selection resolved sizing | Named Selection resolves before kernel entry | DEFINED / PENDING |
| M5-G04 | minimum composition | overlapping rules equal mathematical minimum independent of enumeration | DEFINED / PENDING |
| M5-G05 | curvature predictor | sagitta/normal-angle predictors agree with analytic fixtures | DEFINED / PENDING |
| M5-G06 | actual CAD deviation | accepted mesh satisfies measured 3D CAD deviation policy | DEFINED / PENDING |
| M5-G07 | non-self proximity | owned/incident topology excluded from gap search | DEFINED / PENDING |
| M5-G08 | thin-gap response | meaningful opposite/nonincident geometry drives expected refinement | DEFINED / PENDING |
| M5-G09 | Lipschitz gradation | field satisfies frozen minorant and k-Lipschitz semantics | DEFINED / PENDING |
| M5-G10 | minimum-size limitation reporting | sub-minimum requirement yields MinimumSizeLimited, not success | DEFINED / PENDING |
| M5-G11 | monotonic refinement response | stricter valid target cannot silently coarsen affected region | DEFINED / PENDING |
| M5-G12 | deterministic violation ordering | rule/order permutations reproduce refinement decisions | DEFINED / PENDING |
| M5-G13 | resource termination | budget/resource stop returns truthful typed status | DEFINED / PENDING |
| M5-G14 | settings fingerprint | all topology/termination-affecting settings are fingerprinted | DEFINED / PENDING |
| M5-G15 | uncontrolled point-growth protection | adversarial criteria terminate within configured protection | DEFINED / PENDING |
| M5-G16 | provenance preservation through refinement | constrained CAD provenance survives refinement | DEFINED / PENDING |

## Release rule

`CriteriaSatisfied` is the normal success state. A protected stop or resource/budget limit must remain distinguishable from satisfied geometry/sizing criteria.
