# Dynamics26 FEM Research Library

Program: Dynamics26 nonlinear FEM/CAE core
Status: ACTIVE RESEARCH
Date: 2026-09-05

## Purpose

This directory is the permanent research authority for finite-element formulation decisions that are
not yet qualified product capabilities.

The traceability chain is:

    peer-reviewed mechanics/FEM source
    -> Dynamics26 derivation and sign/field convention
    -> discrete approximation spaces
    -> matrix/state architecture
    -> executable verification plan
    -> implementation
    -> solver/system correlation
    -> product capability

Chat history is not the engineering authority.

## Active package

- tet4-nearly-incompressible/ — low-order tetrahedral formulation research for nearly/full
  incompressibility and rubber-like finite strain.

## Boundary with meshing research

Meshing quality and finite-element formulation are independent axes.

    valid/good tetra geometry
    !=
    locking-free / stable TET formulation.

The meshing-side bridge is:
docs/research/meshing/m6-quality/NONLINEAR_RUBBER_MESH_QUALITY.md.

## Clean-room rule

Academic literature supplies mathematics and verification ideas.
Commercial ANSYS/Abaqus/Marc/COMSOL behavior is a product sanity benchmark only.
No proprietary formulation detail is inferred beyond public documentation.
