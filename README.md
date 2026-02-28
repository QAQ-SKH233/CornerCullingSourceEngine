# CornerCullingSourceEngine

### Introduction
This is the latest in a long line of occlusion culling / anti-wallhack systems, **now migrated to Counter-Strike 2** using Metamod:Source 2.0.

There are a few features that make this implementation great for competitive Counter-Strike:
- Open source
- Good performance (1-2% of frame time for 10 v 10 on Dust2)
- Strict culling with ray casts
- Guaranteed to be optimistic (no popping) for players under the latency threshold
- Compatible with CS2 via Metamod:Source 2.0 and SwiftlyS2

The main caveat is that occluders are placed manually, so we do not automatically support community maps.  
All feedback is welcome!

### Requirements
- Counter-Strike 2 dedicated server
- [Metamod:Source 2.0](https://github.com/alliedmodders/metamod-source) (commit `176bee4a8debb7625ab82c4059ac518` or later)
- [HL2SDK (CS2)](https://github.com/alliedmodders/hl2sdk/tree/cs2)
- [SwiftlyS2](https://github.com/swiftly-solution/swiftlys2) (for plugin-side scripting)
- [AMBuild](https://github.com/alliedmodders/ambuild) (build system)

### Building
```bash
# Set environment variables (or use command-line options)
export MMSOURCE20=/path/to/metamod-source
export HL2SDKCS2=/path/to/hl2sdk-cs2
export HL2SDKMANIFESTS=/path/to/hl2sdk-manifests

# Create build directory
mkdir build && cd build

# Configure (targets x86_64 by default for CS2)
python3 ../configure.py --sdks cs2

# Build
ambuild
```

Alternatively, use command-line options:
```bash
python3 ../configure.py \
  --mms_path /path/to/metamod-source \
  --hl2sdk-root /path/to/hl2sdk-root \
  --hl2sdk-manifests /path/to/hl2sdk-manifests \
  --sdks cs2
```

### Installation
1. Build the plugin (see above) or download pre-built binaries
2. Copy the packaged output to your CS2 server's `game/csgo/` directory:
   ```
   game/csgo/addons/culling/          # Plugin binary
   game/csgo/addons/metamod/culling.vdf  # Metamod VDF
   ```
3. Copy the map culling data files to `game/csgo/maps/`:
   ```
   game/csgo/maps/culling_de_dust2.txt
   game/csgo/maps/culling_de_mirage.txt
   ...
   ```
4. Install Metamod:Source 2.0 if not already installed
5. Restart the server

### Adding Occluders to Custom Maps
- Create a file for your custom map: `game/csgo/maps/culling_<MAPNAME>.txt`
  - To prevent crashes, you may need a placeholder occluder (AABB from 0 0 0 to 1 1 1) until you add more
- An axis-aligned bounding box is declared by "AABB" and defined by the coordinates of two opposite vertices
- A cuboid is declared by "cuboid" and defined by:
  - offset
  - scale
  - rotation
  - 8 vertices in the order below
- A cuboid is usually best defined with 8 raw vertex coordinates, "0 0 0" offset, "1 1 1" scale, and "0 0 0" rotation
- The user must ensure that the vertices of a cuboid's faces are coplanar. Failure will cause undefined behavior

```  
   .1------0
 .' |    .'|
2---+--3'  |
|   |  |   |
|  .5--+---4
|.'    | .'
6------7'
```

### Migration Notes (CS:GO → CS2)
- **Build system**: Migrated from SourceMod AMBuild to Metamod:Source 2.0 AMBuild
- **Architecture**: Changed from 32-bit to 64-bit (CS2 is 64-bit only)
- **Plugin framework**: Replaced SourceMod extension with standalone Metamod:Source 2.0 plugin
- **Plugin scripting**: SwiftlyS2 replaces SourcePawn for plugin-side scripting
- **Map paths**: Changed from `csgo/maps/` to `game/csgo/maps/`
- **C++ standard**: Upgraded from C++14 to C++17
- **Tick rate**: Default changed from 128 to 64 (CS2 default)

### Experimental
- Automatic mesh generation for an alternative occlusion culling algorithm
- While the mesh generation is mostly fine, save for a few transparency issues, the necessary triangle intersection and other integration code will take a fair bit of work.

### Future Work
- Polish lookahead logic  
- Find and fill "missed spots" in maps  
- Calculate lookahead with velocity instead of speed  
- Join occluders, preventing "leaks" through thin corners  
- Smoke occlusion  

### Technical details
- You can find more details with the original UE4 implementation:
https://github.com/87andrewh/CornerCulling

### Special Thanks
Paul "arkem" Chamberlain  
Garrett Weinzierl at PlayWin  
DJPlaya, lekobyroxa, and the AlliedModders community  
Challengermode esports platform  
TURF! community servers  
