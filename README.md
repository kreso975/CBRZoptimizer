# CBRZoptimizer

**CBRZoptimizer** is a Windows-native archive extraction and image optimization utility tailored for comic book formats like `.cbz`, `.cbr`, and `.zip`. It handles non-ASCII filenames, nested folders, and includes both internal and external optimization tools.

---

## ✨ Features

- 📦 Extract `.cbz`, `.zip`, and `.cbr` archives
- 🧰 Uses built-in [MiniZ](https://github.com/richgel999/miniz) for ZIP/CBZ handling
- 📂 Automatically flattens nested image folders
- 🖼️ Image optimization with:
  - Internal fallback resizer using [STB Image](https://github.com/nothings/stb)
  - External [ImageMagick](https://imagemagick.org/) if configured
- 🧵 Multithreaded image optimization (STB fallback)
- 🗂️ Handles UTF-8 and special characters in paths
- 🛠️ Configurable via `config.ini`

---

## 🚀 Getting Started

1. **Clone the repository**

   ```bash
   git clone https://github.com/kreso975/CBRZoptimizer.git
