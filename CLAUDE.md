# MapGyver Development Guide

## Building the Project

### Prerequisites
- JUCE Framework (clone `juce8_local` branch from https://github.com/benkuper/JUCE)
- NDI SDK (https://ndi.video/for-developers/ndi-sdk/)
- Platform-specific build tools

### Build Commands

#### Linux
```bash
cd Builds/LinuxMakefile
make -j8
```

#### Windows
Open `Builds/VisualStudio2022/MapGyver.sln` in Visual Studio and build.

#### macOS
Open `Builds/MacOSX_CI/MapGyver.xcodeproj` in Xcode and build.

### Running (Linux)
```bash
LD_LIBRARY_PATH=../../External/servus/lib/linux:../../External/vlc/lib/linux:$LD_LIBRARY_PATH build/MapGyver
```

## Testing the Mesh Warping Feature

### Manual Testing Steps

1. **Launch MapGyver** after building

2. **Create or load a composition**:
   - Add a media layer (image, video, or shader)

3. **Enable mesh warping on a layer**:
   - Select the layer in the composition
   - Find the "Mesh Warping" section in the layer's properties
   - Enable "Enable Mesh" to activate mesh warping

4. **Test mesh controls**:
   - **Subdivisions**: Use the "+" and "-" buttons to add/remove grid subdivisions
   - **Show Grid**: Toggle grid visibility with "Show Grid" checkbox
   - **Lock Mesh**: Toggle mesh editing lock with "Lock Mesh" checkbox
   - **Warp Mode**: Switch between Bilinear, Perspective, and Bezier modes
   - **Reset Mesh**: Click to reset to default 2x2 configuration

5. **Test point manipulation** (when UI editor is implemented):
   - Click on control points to select them
   - Drag points to deform the mesh
   - Use Shift+click for multi-selection
   - Press 'B' on selected point for bezier mode

6. **Test persistence**:
   - Save the project
   - Close and reopen
   - Verify mesh configuration is preserved

### Testing Surfaces
The mesh warping is also available on Screen Surfaces:
- Create a new Screen
- Add a Surface
- Find "Mesh Warping" section in Surface properties
- Same controls as composition layers

## Code Structure

### Mesh Warping Files
- `Source/Common/Mesh/MeshGrid.h/.cpp` - Core mesh data structure
- `Source/Common/Mesh/MeshWarper.h/.cpp` - GPU rendering
- `Source/Common/MeshIncludes.h` - Include file

### Integration Points
- `Source/Media/medias/composition/CompositionLayer/CompositionLayer.h/.cpp`
- `Source/Media/medias/composition/CompositionMedia.cpp`
- `Source/Screen/Surface/Surface.h/.cpp`

## Code Style
- Follow existing MapGyver conventions
- Use JUCE framework patterns
- Maintain cross-platform compatibility (Windows, macOS, Linux)
