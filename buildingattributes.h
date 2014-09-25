#ifndef BUILDINGATTRIBUTES_H
#define BUILDINGATTRIBUTES_H

#include "osmtypes.h"

/**
 * Note: the meaning of the height/roof:height and building:levels/roof:levels pairs
 *       is inconsistent within OSM:
 *       - for building:levels/roof:levels, the total number of levels is the sum of both
 *       - for height/roof:height, the total height is 'height' alone, and the height of
 *         the building without the roof is the difference of the two
 */

//FIXME: add parsing of roof shapes

//FIXME: add parsing of units and unit conversion
static float getLengthInMeters(string s)
{
    return atof(s.c_str());
}

class BuildingAttributes {
public:

    BuildingAttributes(): numLevels(-1), numRoofLevels(-1), height(-1), roofHeight(-1),
        isJustRoof(false), minHeight(-1), skipLevels(-1) {}

    BuildingAttributes(const Tags &tags): numLevels(-1), numRoofLevels(-1), height(-1), roofHeight(-1),
        isJustRoof(false), minHeight(-1), skipLevels(-1)
    {
        if ( tags.count("building:levels"))
            numLevels = atof( tags.at("building:levels").c_str() );

        if (tags.count("roof:levels"))
            numRoofLevels = atof( tags.at("roof:levels").c_str() );

        if (tags.count("building:min_levels"))
            skipLevels = atof( tags.at("building:min_levels").c_str() );

        if (tags.count("height"))
            height = getLengthInMeters( tags.at("height").c_str() );

        if (tags.count("roof:height"))
            roofHeight = getLengthInMeters( tags.at("roof:height").c_str() );

        if (tags.count("min_height"))
            minHeight = getLengthInMeters(tags.at("min_height") );

        isJustRoof = tags.count("building") && tags.at("building") == "roof";
    }

    float getMinHeight() const
    {
        if (minHeight != -1) return minHeight;
        if (skipLevels != -1) return skipLevels*3;
        return 0;
    }

    float getHeightWithoutRoof() const
    {
        if (height != -1) return height - roofHeight;
        if (numLevels) return numLevels * 3;
        return 10; //arbitrary height value
    }

    float getRoofHeight() const
    {
        if (roofHeight != 0.0) return roofHeight;
        if (roofHeight == 0.0 && numRoofLevels == 0.0) return 0.0;
        return numRoofLevels * 3.0;
    }

    bool isFreeStandingRoof() const { return isJustRoof;}
    bool heightIsGuessed() const { return (height == -1) && (numLevels == 0);}
private:
    float numLevels;
    float numRoofLevels;
    float height;
    float roofHeight;
    bool isJustRoof;
    float minHeight;
    float skipLevels;
};

#endif // BUILDINGATTRIBUTES_H
