# Changelog

## [Unreleased] – v1.1.2

### Added
- Online update check on startup.
- Buttons to open TMP and Output folders directly in File Explorer.

### Changed
- Full rewrite of image button creation logic for better structure and reuse.

### Fixed
- **GUI Selection Fixes**  
  Resolved issues related to inconsistent or incorrect GUI element selections.
- TMP and Output folder paths are now validated on startup.  
  If missing in `.ini`, default Windows paths are assigned via `g_config`.

---

