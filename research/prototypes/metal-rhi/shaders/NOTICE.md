# Pinned Qt example shaders

These four files are copied unchanged from Qtbase 6.11.2's
`examples/widgets/rhi/simplerhiwidget/shaders/` and `shader_assets/`, source commit
`ef55f427f2c8b410d34f8a7681020a3000cf6866` (the installed source archive's `.tag`).
Retrieved from the existing local pinned Qt source on 2026-10-13; no shader tool
or new dependency was downloaded. Upstream source:
https://code.qt.io/cgit/qt/qtbase.git/tree/examples/widgets/rhi/simplerhiwidget?h=ef55f427f2c8b410d34f8a7681020a3000cf6866

The upstream root `REUSE.toml` covers `examples/**` with
`SPDX-FileCopyrightText = "Copyright (C) The Qt Company Ltd."` and
`SPDX-License-Identifier = "LicenseRef-Qt-Commercial OR BSD-3-Clause"`.
The spike uses the BSD-3-Clause option; complete terms are adjacent in
`LICENSE.BSD-3-Clause`. Surrounding upstream example C++ additionally records
Copyright (C) 2023 The Qt Company Ltd. This license applies to these Qt-derived
assets, separately from Alloy's MIT code. No Qt Shader Tools/qsb binary is shipped.

SHA-256:

- color.vert: `224e95fd2f7848c7de35bc6fcd69e3e3cfd92307913d714680656a22bda2118d`
- color.frag: `b531646343545c098eaae46b34d2c9ee7b5df6c6105fe6e8be7704afb5759a27`
- color.vert.qsb: `5ecccd1b00fe14fb12d863fd07386f98f4c1f98032820740afa495bb555f25b4`
- color.frag.qsb: `a2505b3d92fc7371ec7ff4632e3858ce5e9c803e0771dfc2940fe0d8a21eac6e`

Runtime initialization requires valid QShader bundles and an existing Metal
variant. The supplied shaders use an MVP matrix, vec4 position and vec3 color.
The experiment supplies 3D positions and constant per-face colors/encoded IDs;
it does not claim integer shader outputs, authored lighting or production CAD
materials. Future modified shaders require a separately evaluated pinned tool.
