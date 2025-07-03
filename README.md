# CBRZoptimizer

**CBRZoptimizer** is a Windows-native archive extraction and image optimization utility tailored for comic book formats like `.cbz`, `.cbr`. It includes both internal and external optimization tools.

> **Note:** This application is developed as a hobby project.  
> Use at your own risk and for personal/non-commercial purposes.

---

## ✨ Features

- 📦 Extract `.cbz`, `.zip`, and `.cbr`, `.rar` archives
- 🧰 Uses built-in [MiniZ](https://github.com/richgel999/miniz) for ZIP/CBZ handling
- 📂 Automatically flattens nested image folders
- 🖼️ Image optimization with:
  - Internal [STB Image](https://github.com/nothings/stb)
  - External [ImageMagick](https://imagemagick.org/) if configured
- 🧵 Multithreaded image optimization (STB internal)
- 🗂️ Handles UTF-8 and special characters in paths
- 🛠️ Configurable via `config.ini`

---

## 🚀 Getting Started

1. **Clone the repository**

   ```bash
   git clone https://github.com/kreso975/CBRZoptimizer.git
   ```
2. **Build the project** in Visual Studio

3. **Configure your settings** in `config.ini`:

   - Input/output folders
   - Optional tools like WinRAR and ImageMagick

---

## ⚙️ Requirements

- Windows 10 or newer
- Optional: [WinRAR](https://www.win-rar.com/) for `.cbr`, `.rar` support
- Optional: [ImageMagick CLI](https://imagemagick.org/script/download.php) for advanced image optimization

---

## 🧪 Usage Notes

- Extracted archives are unpacked to the `TMP_FOLDER`
- Final output files are saved in the `OUTPUT_FOLDER`
- STB fallback is used automatically when ImageMagick is unavailable

---

## 🧾 License

This project is licensed under the [Apache License 2.0](LICENSE).

---

> 🖼️ Extract. Optimize. Rebuild.  
> **CBRZoptimizer** makes comic archives clean, fast, and Unicode-proof.

---

> **Disclaimer:**  
> This software is provided as-is, with no warranty.  
> It is intended for personal and non-commercial purposes.

## Screenshots of GUI  
  
**Dashboard**  
  
<div style="display: flex; justify-content: center;"> <img src="./img/CBRZoptimizer.png?raw=true" alt="CBR & CBZ Optimizer" title="CBR & CBZ Optimizer" style="width: 100%;"> </div>