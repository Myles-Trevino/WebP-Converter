# WebP Converter
Faster and easier WebP conversion.

## Features
- Automatically searches the input folder for PNG, JPG/JPEG, and TIF/TIFF images, then converts them to WebP.
- Saves the converted images to the output folder, retaining the directory structure and naming found in the input folder.
- Uses all available CPU cores to perform the conversion.
- Encoding can be customized with the [cwebp parameters](https://developers.google.com/speed/webp/docs/cwebp).

## Usage
- Compile.
- Download the [WebP distribution](https://storage.googleapis.com/downloads.webmproject.org/releases/webp/index.html).
- Place cwebp.exe next to the converter executable.
- Create a folder named "Input" next to the executable.
- Place your images in the input folder. For organization, you can create subfolders within the input folder. This structure will be recreated in the output folder.
- Run the converter executable, optionally passing cwebp arguments.
- Converted files will be saved to a folder named "Output".
