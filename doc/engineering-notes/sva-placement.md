# SVA Placement for MVP

- **Status:** accepted
- **Date:** 2026-10-30
- **Context:** The fixture corpus is being extended toward end-to-end formal
  tests, and external property-file attachment was considered while `bind` and
  hierarchy remain outside the MVP.
- **Decision:** MVP properties are written inline in the selected top module.
  Do not invent a Lemma-specific sidecar attachment mechanism. Standalone
  assertion/checker files and external attachment are deferred.
- **Rationale:** This keeps SVA placement within ordinary SystemVerilog source
  structure while the MVP remains a single flat top. Standard external
  attachment mechanisms require additional specified RTL features.
- **References:** [SVA support](../sva-support.md), [tool flow](../tool-flow.md),
  and [MVP](../mvp.md).
