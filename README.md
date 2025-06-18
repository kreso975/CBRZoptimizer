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
2. **Build the project** in Visual Studio (tested with MSVC)

3. **Configure your settings** in `config.ini`:

   - Input/output folders
   - Optional tools like WinRAR and ImageMagick

---

## ⚙️ Requirements

- Windows 10 or newer
- Optional: [WinRAR](https://www.win-rar.com/) for `.cbr` support
- Optional: [ImageMagick CLI](https://imagemagick.org/script/download.php) for advanced image optimization
---

## 📝 Configuration

Edit `config.ini` to define key folders and tools:

```ini  
[Paths]
TMP_FOLDER=D:\TMPVideo
OUTPUT_FOLDER=D:\CBRZ_Output
WINRAR_PATH=C:\Program Files\WinRAR\WinRAR.exe
IMAGEMAGICK_PATH=C:\Tools\ImageMagick\magick.exe
```  

## 🧪 Usage Notes

- Extracted archives are unpacked to the `TMP_FOLDER`
- Final output files are saved in the `OUTPUT_FOLDER`
- STB fallback is used automatically when ImageMagick is unavailable
- Logs are written to `stb_image_debug.log` for image processing

---

## 🧾 License

This project is licensed under the [Apache License 2.0](LICENSE).

---

## 🤝 Contributions

Pull requests, feature suggestions, and bug reports are welcome!  
If it breaks on your favorite `.cbz`, feel free to open an issue.  

> 🖼️ Extract. Optimize. Rebuild.  
> **CBRZoptimizer** makes comic archives clean, fast, and Unicode-proof.

