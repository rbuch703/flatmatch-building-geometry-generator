#! /bin/bash

for x in `seq 8717 8725` 
do
    mkdir -p geoTiles/$x
    for y in `seq 5395 5407`
    do
        ./GeometryConversion $x $y 2> geoTiles/$x/$y.json
    done
done

    #x:  -> 9 tiles
    ##//y:  -> 13 tiles

