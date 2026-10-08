# Notes for Claude

## Versioning

Version numbers follow semantic versioning strictly. Never bump a version on
your own initiative:

- no bump for CI, packaging or build-system changes
- patch for bug fixes
- minor for backwards-compatible features
- major only for breaking changes

If a change seems to warrant a bump, propose it and let the maintainer decide.

## Format

- Always three parts, `MAJOR.MINOR.PATCH`, and the same number everywhere: the git tag
  (`vMAJOR.MINOR.PATCH`), `version` in `meson.build` (and with it the wheel name) and the
  release title.
- Where the code registers a VapourSynth plugin, `VS_MAKE_VERSION(major, minor)` in
  `configPlugin` carries MAJOR and MINOR; a patch release leaves it unchanged.
- Published releases are never renamed, retagged or rebuilt: their download URLs and SHA-256
  sums are pinned by Hybrid's build scripts. Older tags with one or two parts (`v5`, `v1.1`)
  stay as published; the next release continues from the current version with three parts,
  e.g. `5` -> `5.0.1`, `1.1` -> `1.1.1`.
