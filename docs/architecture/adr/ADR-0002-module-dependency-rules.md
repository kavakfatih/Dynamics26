# ADR-0002 — Module Dependency Rules

**Karar:** Alt katman ust katmani bilmez. GUI -> C API -> solver -> assembly -> elements/materials -> fields/mesh -> math/foundation yonu korunur. Circular dependency kabul edilmez.
