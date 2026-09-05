# Meshing Clean-Room / Source-Boundary Policy

This policy exists to protect Dynamics26 originality and keep meshing research auditable.

## 1. Principle

Open source is a valuable research resource, but source availability does not erase copyright or license obligations.

Dynamics26 meshing work follows:

\`\`\`text
study theory and behavior
→ write an independent Dynamics26 specification
→ implement from the specification and mathematical references
→ verify independently
\`\`\`

It does **not** follow:

\`\`\`text
read source
→ rename symbols
→ translate language
→ rearrange code
→ commit as Dynamics26
\`\`\`

## 2. Allowed research use

Allowed:

- read public papers,
- read official documentation,
- inspect public repository structure,
- identify algorithm families,
- identify architecture boundaries,
- inspect test categories and benchmark ideas,
- record failure modes at a conceptual level,
- compare public output/capability behavior,
- reproduce equations and algorithms from properly cited mathematical literature in an original implementation.

## 3. Prohibited actions

Do not:

- copy source code,
- paste source snippets into Dynamics26 source or research files,
- line-by-line translate C/C++/Fortran/Python,
- preserve distinctive function/class naming from another project without an independent Dynamics26 reason,
- reproduce rule tables, constants or heuristic sequences from source merely by changing notation,
- mechanically rewrite GPL/AGPL/LGPL code and claim the license problem is removed,
- use AI to "wash" third-party implementation code into superficially different code,
- copy commercial UI text/layout as a product identity,
- infer proprietary commercial algorithms beyond public documentation.

## 4. Clean implementation workflow

For each algorithm:

### Researcher record

Write:

- engineering problem,
- mathematical definitions,
- primary papers,
- known algorithm families,
- invariants,
- failure modes,
- verification cases,
- performance expectations.

### Dynamics26 specification

Create an implementation-neutral contract:

- inputs,
- outputs,
- ownership,
- units,
- tolerances,
- deterministic behavior,
- failure codes,
- complexity goals,
- tests.

### Implementation

Write new code from the specification using Dynamics26 naming, data structures and architecture.

### Review

Before merge:

- confirm no external source fragment was copied,
- confirm source comments cite theory where useful,
- confirm tests are independently authored,
- confirm license-sensitive source was not imported.

## 5. Source-specific caution

### Netgen
LGPL-2.1 project. Study is permitted; production integration or code reuse still requires explicit review. Current strategy does not integrate it.

### Gmsh
GPL-family licensing and bundled/contributed components require particular caution. Source is architecture research only.

### MMG
LGPL project. Research source for future adaptation architecture; no code import in the original mesher track.

### TetGen
Current repository documents AGPLv3/commercial dual-license paths for recent versions. No source code reuse.

### CGAL
Licensing varies by package. Mesh_3 is a theory/architecture reference; no source reuse without a separate future review.

### Robust-predicate implementations
The mathematical methods are essential, but code-license provenance differs among published copies and ports. Dynamics26 should either:
- implement the required arithmetic/predicates independently from papers, or
- adopt a separately approved, clearly licensed micro-dependency through an explicit ADR.

The default research plan is independent implementation.

## 6. Commercial products

ANSYS, COMSOL and Marc/Mentat are closed-source product benchmarks.

Allowed:

- official manuals,
- published theory manuals,
- observable workflow semantics,
- publicly documented capability limits.

Not allowed:

- reverse engineering protected binaries,
- decompilation,
- pretending undocumented behavior is known source truth.

## 7. Documentation requirement

Any source-level study that materially influences an ADR must list:

- repository,
- file/directory studied at a high level,
- date,
- license observed,
- abstract engineering lesson,
- statement that no code was copied.

## 8. Legal escalation

If a future work package proposes incorporating third-party code, stop the original-engine assumption and create a dedicated dependency/license ADR before implementation.

This policy is an engineering governance rule, not a substitute for legal counsel.
