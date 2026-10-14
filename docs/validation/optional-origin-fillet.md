# Optional origin and support diagnostic corrections

REL-008, 2026-10-13. A non-mutating lookup of the optional origin reference
prevents general sketch Fillet from inserting a null originPointId. The previous
read made the existing face_support fixture fail its first rounded source solve.
A redundant linked-Project support preflight is also removed: the established
support checks already reject missing, forward, unsupported and consumed sources
with their original structured diagnostics. Project source preflight remains.

Astra independently diagnosed and source-reviewed both corrections. Existing
shared_line_fillets, rounded_rectangle_fillets and linked_planar_project targeted
core checks pass. The face_support fixture advances past the corrected origin
failure but intermittently fails its initial region union with `Ambiguous
extrusion edge provenance`; an independently debugged run reached the later
support check before the diagnostic correction. GEO-006 owns that separate
geometry failure. The fixture now reports its actual commit error. No clean
full-suite or new visual-workflow claim is made, and no assertions were weakened.
