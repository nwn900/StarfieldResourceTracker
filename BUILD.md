# Building ResourceTracker.dll

## Option 1: Get a pre-built DLL (no compiler needed)

1. **Push this project to GitHub**
   - Create a new repo on GitHub, then in this folder run:
   - `git init`
   - `git add .`
   - `git commit -m "Initial commit"`
   - `git remote add origin https://github.com/YOUR_USERNAME/YOUR_REPO.git`
   - `git push -u origin main` (or `master`)

2. **Run the build workflow**
   - Open your repo on GitHub → **Actions** → **Build ResourceTracker DLL**
   - Click **Run workflow** (or push triggers it automatically)
   - When the job finishes, open the run and download the **ResourceTracker-DLL** artifact (contains `ResourceTracker.dll`)

3. **Install**
   - Copy `ResourceTracker.dll` into `Starfield\Data\SFSE\Plugins\`
   - Ensure [SFSE](https://www.nexusmods.com/starfield/mods/106) and [Address Library for SFSE Plugins](https://www.nexusmods.com/starfield/mods/3256) are installed

---

## Option 2: Build on your PC

You need:

- **Visual Studio 2022** (Community is free) or **Visual Studio 2022 Build Tools**  
  - Install workload: **Desktop development with C++**
- **CMake** (e.g. from [cmake.org](https://cmake.org) or `winget install Kitware.CMake`)
- **vcpkg**  
  - Clone: `git clone https://github.com/microsoft/vcpkg.git C:\vcpkg`  
  - Run: `C:\vcpkg\bootstrap-vcpkg.bat`  
  - Set environment variable: **VCPKG_ROOT** = `C:\vcpkg` (or your vcpkg path)

### Steps

1. **Dependencies (one-time)**  
   In this project folder, ensure CommonLibSF and DKUtil are in `extern\`:
   - If you use git: `git submodule update --init --recursive`
   - If not: clone [CommonLibSF](https://github.com/Starfield-Reverse-Engineering/CommonLibSF) into `extern\CommonLibSF` and [DKUtil](https://github.com/gottyduke/DKUtil) into `extern\DKUtil`

2. **Vcpkg packages**
   ```bat
   %VCPKG_ROOT%\vcpkg install spdlog nlohmann-json xbyak --triplet x64-windows-static-md
   ```

3. **Configure & build**
   ```bat
   cd "c:\Users\micha\Desktop\Starfield Resource Tracker"
   build-msvc.bat
   ```
   Or open **x64 Native Tools Command Prompt for VS 2022** and run:
   ```bat
   cd "c:\Users\micha\Desktop\Starfield Resource Tracker"
   cmake -B build -S Plugin -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static-md -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release
   ```

4. **Output**
   - DLL: `build\Release\ResourceTracker.dll`  
   - Copy it to `Starfield\Data\SFSE\Plugins\`
