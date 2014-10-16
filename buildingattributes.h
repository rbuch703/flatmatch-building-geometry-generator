#ifndef BUILDINGATTRIBUTES_H
#define BUILDINGATTRIBUTES_H

#include "osmtypes.h"
#include <string>
#include "geometry.h"
#include <iostream>
#include <QColor>
//#include <QStringList>

/**
 * Note: the meaning of the height/roof:height and building:levels/roof:levels pairs
 *       is inconsistent within OSM:
 *       - for building:levels/roof:levels, the total number of levels is the sum of both
 *       - for height/roof:height, the total height is 'height' alone, and the height of
 *         the building without the roof is the difference of the two
 */

using namespace std;


//FIXME: add parsing of roof shapes

//FIXME: add parsing of units and unit conversion
static float getLengthInMeters(string s)
{
    return atof(s.c_str());
}

static Vector3 getColorFromString(string col, Vector3 defaultColor)
{

    QColor c(col.c_str());

    if (c.isValid())
        return Vector3( c.redF(), c.greenF(), c.blueF());

    if (col != "")
        cerr << "[WARN] Invalid color '" << col << "' detected. Replacing it by default color" << endl;
    return defaultColor;

    /*return Vector3(
        rand()/(float)RAND_MAX,
        rand()/(float)RAND_MAX,
        rand()/(float)RAND_MAX);*/


}

static Vector3 toneDownColor(Vector3 col, float fac = 0.5, bool brighten = false)
{
    float l = 0.2126*col.x + 0.7152*col.y + 0.0722*col.z;   //luminance
    if (brighten)
        l += 0.25;
    return Vector3( (1-fac)*col.x + fac*l,
                    (1-fac)*col.y + fac*l,
                    (1-fac)*col.z + fac*l);
}

class BuildingAttributes {
public:
    enum RoofShape {FLAT, HIPPED, GABLED, DOME, MANSARD, HALF_HIPPED};

    BuildingAttributes(): numLevels(-1), numRoofLevels(-1), height(-1), roofHeight(-1),
        isJustRoof(false), minHeight(-1), skipLevels(-1), wallColor(0.7,0.7,0.7), roofColor(1,0,0), roofShape(FLAT) {}

    BuildingAttributes(const Tags &tags): numLevels(-1), numRoofLevels(-1), height(-1), roofHeight(-1),
        isJustRoof(false), minHeight(-1), skipLevels(-1), wallColor(), roofColor(), roofShape(FLAT)
    {
        if ( tags.count("building:levels"))
            numLevels = atof( tags.at("building:levels").c_str() );

        if (tags.count("roof:levels"))
            numRoofLevels = atof( tags.at("roof:levels").c_str() );

        if (tags.count("building:min_level"))
            skipLevels = atof( tags.at("building:min_level").c_str() );

        if (tags.count("height"))
            height = getLengthInMeters( tags.at("height").c_str() );

        if (tags.count("roof:height"))
            roofHeight = getLengthInMeters( tags.at("roof:height").c_str() );

        if (tags.count("min_height"))
            minHeight = getLengthInMeters(tags.at("min_height") );

        isJustRoof = (tags.count("building") && tags.at("building") == "roof") ||
                     (tags.count("building:part") && tags.at("building:part") == "roof");

        /* OSM tag names are supposed to use British spelling, but American spelling
         * is incorrectly used sometimes. To compensate, we parse both, and let the
         * British one override the American one if both are present */

        string wallColorStr = tags.count("building:color") ? tags.at("building:color") : "";
        if (tags.count("building:colour"))
            wallColorStr = tags.at("building:colour");

        string roofColorStr = tags.count("roof:color") ? tags.at("roof:color") : "";
        if (tags.count("roof:colour"))
            roofColorStr = tags.at("roof:colour");

        /*
        if (wallColorStr != "")
            cout << "wallColor is '" << wallColorStr << "'" << endl;

        if (roofColorStr != "")
            cout << "roofColor is '" << roofColorStr << "'" << endl;*/

        if (tags.count("roof:shape"))
        {
            string s = tags.at("roof:shape");
            if (s == "flat") roofShape = FLAT;
            if (s == "gabled") roofShape=GABLED;
            if (s == "pyramidal") roofShape= HIPPED;
            if (s == "hipped") roofShape = HIPPED; //is the same thing really
            if (s == "mansard") roofShape = MANSARD;
            if (s == "half-hipped") roofShape= HALF_HIPPED;
        }

        //here start the heuristics
        wallColor = getColorFromString(wallColorStr, Vector3(0.9, 0.9, 0.9));
        //heuristic: slanted roofs are red, flat ones are dark gray
        roofColor = getColorFromString(roofColorStr, getRoofHeight() ? Vector3(1, 0.3, 0.3) : Vector3(0.4, 0.4, 0.4));

        /** tone down the colors and optionally brighten them.
         *
         * heuristic: colors given as rgb triples (#123456) are likely meant to be interpreted  literally
         *             Those given as color names are likely too dark to be realistic.
          **/
        wallColor = toneDownColor(wallColor, 0.5, wallColorStr.length() && wallColorStr[0] != '#');
        roofColor = toneDownColor(roofColor, 0.5, roofColorStr.length() && roofColorStr[0] != '#');

        if (!tags.count("roof:shape") && (numRoofLevels  > 0 || roofHeight > 0))
            roofShape = HIPPED;


        if (roofShape == FLAT)
        {
            if (roofHeight != -1)
                roofHeight = 0; //cannot have a roof height for a flat roof

            if (numRoofLevels != -1 && numLevels != -1)
            {
                numLevels += numRoofLevels;
                numRoofLevels = 0;
            }
        }
    }


    float getMinHeight() const
    {
        if (minHeight != -1) return minHeight;
        if (skipLevels != -1) return skipLevels*3;
        return 0;
    }

    float getHeightWithoutRoof() const
    {
        float rh = roofHeight != -1? roofHeight : (numRoofLevels != -1 ? numRoofLevels*3 : 0);
        if (height != -1) return height - rh;
        if (numLevels != -1) return numLevels * 3;
        return 10; //arbitrary height value
    }

    float getTotalHeight() const
    {
        if (height != -1) return height;
        if (numLevels == -1) return 10; //arbitrary height value

        int levels = numLevels;
        if (this->numRoofLevels != -1)
            levels += numRoofLevels;

        return levels * 3.0;
    }

    float getNumLevels() const
    {
        if (numLevels != -1)
        {
            float nl = numLevels;
            if (skipLevels != -1)
                nl -= skipLevels;
            return nl;
        }

        return (getHeightWithoutRoof()- getMinHeight()) / 3.0;
    }

    float getRoofHeight() const
    {
        if (roofHeight != -1.0) return roofHeight;
        if (roofHeight == -1.0 && (numRoofLevels == 0.0 || numRoofLevels == -1.0)) return 0.0;
        return numRoofLevels * 3.0;
    }

    bool isFreeStandingRoof() const { return isJustRoof;}
    bool heightIsGuessed() const { return (height == -1) && (numLevels == -1);}

    Vector3 getWallColor() const { return wallColor;}
    Vector3 getRoofColor() const { return roofColor;}
private:
    float numLevels;
    float numRoofLevels;
    float height;
    float roofHeight;
    bool isJustRoof;
    float minHeight;
    float skipLevels;
    Vector3 wallColor;
    Vector3 roofColor;
    RoofShape roofShape;
};

#endif // BUILDINGATTRIBUTES_H
