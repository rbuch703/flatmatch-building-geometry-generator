
"GeometryConversion": generates tile-based 3D building models based on OpenStreetMap building data.


Dependencies:
- gcc (C++ frontends)
- GNU make
- headers and libraries for the Qt5 component core, network and gui
- qmake (Ubuntu package qt5-qmake)
- CGAL (Ubuntu package libcgal-dev)

Build instructions:
- "qmake ."
- "make" (for GPU-based computations)
- Note: this project mixes C and C++ code. This is not a good coding style, but happened when adding the C API-based OpenCL renderer

Compilation Output:
- executable "GeometryConversion"

Usage:
- "./GeometryConversion <tileX> <tileY>"

, where <tileX> and <tileY> are level 14 tile indices in Web Mercator projection, following the conventions from [https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames] . (If no parameters are passed then default tile indices for a tile in Magdeburg, Germany will be used). This outputs the building information for that tile on the console in a custom JSON format that's used by FlatMatch.

Alternatively, the provided script buildGeometryTiles.sh can be modified and run. It uses GeometryConversion internally to generate the buiding geometries for a set of tiles and writes them to separate files.

Note:
By default, this tool uses http://overpass-api.de/api/ to retrieve the OSM data. Batch usage of this API is discouraged and is throttled to one request per IP and second (access from a given IP can even be blocked if it uses the service excessively). If batch usage is desired, please look into setting up your own instance of an Overpass API (aka OSM3S) server and changing the URL close to the end of geometryConverter.cpp to point to your server.

