# Changelog
## [2025-07-28] – v1.2.0

### Added
- Support for webP image format. Using libwebp library
  - Added in STB image Optimization workflow - works fast because of STB multithread
  - Added when Image Optimization is not selected - Works in multithread
  - Added to ImageMagick workflow
  - Added to Cover Extract workflow
- GUI is done
- Config and ini are done

### Changed
- Full rewrite of CheckBoxes and LabelProc handle.
- Created elements Grouping

## [2025-07-26] – v1.1.2

### Added
- Online update check on startup.
- Buttons to open TMP and Output folders directly in File Explorer.

### Changed
- Full rewrite of image button creation logic for better structure and reuse.
- Added 16x16 Menu icon

### Fixed
- **GUI Selection Fixes**  
  Resolved issues related to inconsistent or incorrect GUI element selections.
- TMP and Output folder paths are now validated on startup.  
  If missing in `.ini`, default Windows paths are assigned via `g_config`.

---

