# Build, sign and publish Alloy candidates

GitHub Actions is the build infrastructure. The existing
[native workflow](../.github/workflows/native-build.yml) already has macOS
arm64/Intel, Windows x64 and Linux x64 jobs, triggered only by a `v*` tag or
manual dispatch. Its current outputs are unbundled, temporary compilation
artifacts. They are not downloadable application packages. GitHub supplies
[hosted macOS, Windows and Linux runners](https://docs.github.com/en/actions/concepts/runners/github-hosted-runners);
[private-repository usage](https://docs.github.com/en/billing/concepts/product-billing/github-actions)
uses the account's included allowance and then billed minutes. A separate
build service is not needed for the first candidate.

## Candidate flow

1. Build the exact selected commit on all declared architectures using pinned,
   fingerprinted Qt/OCCT/solver inputs. Keep tag/manual dispatch, not CI for
   every commit or pull request.
2. Bundle the application, local control client, Qt plugins, OCCT resources,
   solver libraries, notices and source/replacement instructions. Produce a
   relocatable `.app` ZIP for each declared Mac architecture, Windows ZIP and
   explicitly scoped Linux tarball. Preserve executable modes and symlinks.
3. Sign platform executables, notarize the Mac app, then create the final
   archives and checksums. Record source, dependency, toolchain, build and
   package identity in a manifest.
4. Test **those final archive bytes** on clean/extracted hosts. Local Mac
   qualification and external Windows/Linux testers must report OS/GPU,
   display scale, physical input, local control, sketch-to-solid editing,
   save/reopen and printable output. Hosted compilation or software rendering
   alone does not qualify a desktop target.
5. Promote the same hashes through an explicit `gh`-driven GitHub Release with
   capability/known-limit notes, notices and `SHA256SUMS`; do not rebuild during
   promotion. [Actions artifacts](https://docs.github.com/en/actions/tutorials/store-and-share-data)
   are short-lived CI evidence, while
   [release assets](https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases)
   are the download channel.

## Platform signing

- **macOS:** The current local package is arm64, Homebrew-based and ad-hoc
  signed. Generalize its dependency manifest and paths before CI use. For a
  public direct download, sign nested code and the `.app` with Developer ID,
  hardened runtime and timestamp; submit a ZIP through `notarytool`, inspect
  the result, staple the app, then make the final ZIP. Apple requires
  [Developer Program membership](https://developer.apple.com/programs/whats-included)
  for this distribution path; the current listed membership is US$99/year.
  Follow Apple's [notarization requirements](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution)
  and [scripted workflow](https://developer.apple.com/documentation/security/customizing-the-notarization-workflow).
- **Windows:** Package the owned EXE/client and required Qt/OCCT/solver DLLs
  first. Timestamped Authenticode signing of owned binaries is the direct ZIP
  path; an installer can follow. [Azure Artifact Signing](https://learn.microsoft.com/en-us/azure/artifact-signing/quickstart)
  (formerly Trusted Signing) has an
  [official GitHub Action](https://github.com/Azure/artifact-signing-action)
  and OIDC federation, avoiding a certificate private key in CI. Eligibility,
  identity validation and a paid Azure subscription must be resolved before
  selecting it. The current Basic price is
  [US$9.99/month for 5,000 signatures](https://learn.microsoft.com/en-us/azure/artifact-signing/how-to-change-sku).
  Signing does not itself guarantee immediate SmartScreen reputation. An
  approved [SignPath Foundation](https://signpath.org/) application is a
  possible later alternative, not a prerequisite for an unsigned test archive.
- **Linux:** Define a distro/ABI baseline and graphics/display requirements;
  start with a tarball and SHA-256 manifest. There is no universal notarization
  equivalent. A detached release signature or distribution-specific signing
  can be added after the packaging baseline is qualified. AppImage requires a
  sufficiently old [build baseline](https://docs.appimage.org/introduction/concepts.html);
  Flatpak adds a separate [publication and sandbox](https://docs.flatpak.org/en/latest/publishing.html)
  decision.

Use a protected GitHub release environment for Apple signing secrets, a
temporary runner keychain and cleanup; use narrow OIDC role assignment for
Windows. Keep `contents: write` only in the explicit promotion job, pin
third-party actions, and do not expose signing jobs to untrusted PR code.
[GitHub environments](https://docs.github.com/en/actions/how-tos/deploy/configure-and-manage-deployments/manage-environments)
provide approval and secret scoping.

The unresolved release inputs are the legal publisher identity, Apple Developer
membership/credentials, Windows signing eligibility/provider, minimum Mac and
Windows versions, Linux distro/ABI baseline and whether Intel Mac is in the
first preview. These are release choices, not reasons to delay unsigned
relocatable archives for external qualification. Existing CI-002–009 and
GATE-005/REL-018 own bundling and runtime evidence; CI-010/011 own the bounded
signing follow-ups.
