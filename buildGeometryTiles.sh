#! /bin/bash

X_MD=`seq 8717 8725` # x range for Magdeburg
Y_MD=`seq 5395 5407` # y range for Magdeburg

X_HN=`seq 8731 8741` # x range for Halle-Neustadt
Y_HN=`seq 5446 5455` # y range for Halle-Neustadt



for x in $X_MD
do
    mkdir -p geoTiles/$x
    for y in $Y_MD
    do
        ./GeometryConversion $x $y > geoTiles/$x/$y.json
    done
done

    #x:  -> 9 tiles
    ##//y:  -> 13 tiles

