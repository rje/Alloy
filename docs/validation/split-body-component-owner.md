# Split Body in an active local component

SOL-052 keeps the authored owner of a new Split Body with the active component in both the direct command and the preview/commit path. Editing an existing Split retains its owner. The existing body and plane ownership checks still reject a root-owned Split against a component-owned body and plane.

The focused real-app probe opened the accepted port/card enclosure, activated Case, created its retained top/bottom Midplane, and checked root-active refusal without an authored change. Case-active dialog preview produced two bodies; Cancel restored the exact prior authored document. A direct Split then produced closed, valid negative and positive Case bodies of 29,166.648508 and 30,473.185483 mm³, respectively. Their sum equals the source body's 59,639.833991 mm³ within 1e-4 mm³. An edit kept the Split feature and owner IDs; save/reopen kept both derived body IDs and the authored state. Evidence: `../Alloy-reference-records/deliverables/enclosure-snap-2026-10-13/split-probe-02/summary.json`, `split-preview.png`, and `ported-case-split.alloy` in that directory.

The existing `split_body_plane` core test passed. The native `split_body_controls` test now authors its Box, Midplane and Split inside a local Case; actual body click and Browser plane click drove preview, Cancel and OK, then edit preview retained Case ownership. It passed with captures under `../Alloy-reference-records/deliverables/enclosure-snap-2026-10-13/split-native-02/`. The first capture attempt lacked its output directory and failed only at screenshot writing; the rerun with that directory present passed.

This is owner routing for new local Split features. Cross-component targets remain unsupported, and this receipt does not claim the later snap geometry or timeline reorder.

Astra independently accepted the final source, actual-model ledger and native
captures. Integrated as 59952ad; the main application build passes.
