# Dynamics26 Meshing Exact Oracle

This directory contains **test-only** exact arithmetic used to verify future Dynamics26 robust geometric predicates.

It is not a production meshing runtime dependency.

## Commands

Generate the committed predicate corpus into a directory:

\`\`\`bash
python3 tools/meshing_oracle/generate_predicate_corpus.py \
  --output-dir /tmp/d26-predicate-fixtures
\`\`\`

Run the executable oracle verification through CTest after configuring/building the repository:

\`\`\`bash
ctest --test-dir build -L robust-geometry --output-on-failure
\`\`\`

## Independence rule

- Oracle A uses exact \`Fraction\` arithmetic over exact binary64 ratios.
- Oracle B decomposes binary64 values into dyadic integers and evaluates an independent homogeneous determinant.

Fixtures are emitted only when both paths agree.

No Netgen, Gmsh, TetGen, CGAL or commercial CAE source code is used by this oracle.
