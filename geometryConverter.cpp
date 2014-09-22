#include <iostream>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "geometryConverter.h"

#include <stdint.h>
#include <assert.h>
#include <map>

extern QNetworkReply *reply;


QString getDepthString(int depth)
{
    QString s = "";
    while (depth--)
        s+="  ";
    return s;
}

void parseJsonObject(QJsonObject object, int depth);
void parseJsonValue(QJsonValue val, int depth);

#define INDENT getDepthString(depth).toStdString()

void parseJsonArray(QJsonArray array, int depth = 0)
{
    for (QJsonArray::const_iterator it = array.begin(); it != array.end(); it++)
    {
        cout << INDENT;
        parseJsonValue(*it, depth+1);
    }

}

void parseJsonObject(QJsonObject object, int depth = 0)
{
    QStringList keys = object.keys();
    for (QStringList::const_iterator it = keys.begin(); it != keys.end(); it++)
    {
        cout << INDENT << (*it).toStdString() << " = ";
        QJsonValue val = object.value(*it);
        parseJsonValue(val, depth+1);
    }
}

void parseJsonValue(QJsonValue val, int depth = 0)
{
    if (val.isArray())
    {
        cout << "[" << endl;
        parseJsonArray(val.toArray(), depth+1);
        cout << INDENT << "]" << endl;
    }

    if (val.isBool())
        cout << (val.toBool() ? "true":"false") << endl;

    if (val.isDouble())
        cout << val.toDouble() << endl;

    if (val.isNull())
        cout << "<NULL>" << endl;

    if (val.isObject())
    {
        cout << "  {" << endl;
        parseJsonObject(val.toObject(), depth+1);
        cout << INDENT << "}" << endl;
    }

    if (val.isString())
    {
        cout << "'" << val.toString().toStdString() << "'" << endl;
    }

}

map<uint64_t, pair<double, double> > getNodes(QJsonArray elements)
{
    map<uint64_t, pair<double, double> > nodes;
    for (QJsonArray::const_iterator el = elements.begin(); el != elements.end(); el++)
    {
        QJsonObject obj = (*el).toObject();
        if (obj["type"].toString() != "node")
            continue;

        uint64_t id = obj["id"].toDouble();
        double lat = obj["lat"].toDouble();
        double lng = obj["lon"].toDouble();

        if (!nodes.count(id))
            nodes.insert( pair<uint64_t, pair<double, double> >(id, pair<double, double>(lat, lng) ) );
    }
    return nodes;
}

void GeometryConverter::onDownloadFinished()
{
    cout << "Finished" << endl;


    if (reply->error() > 0) {
        cout << "Error" << endl;
        cout << reply->errorString().toStdString();
    }
    else {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        cout << ( doc.isNull() ? "invalid":"valid") << endl;
        //QJsonObject obj = doc.object();
        QJsonArray elements = doc.object()["elements"].toArray();
        map<uint64_t, pair<double, double> > nodes = getNodes(elements);
        cout << "parsed " << nodes.size() << " nodes" << endl;

        for (QJsonArray::const_iterator el = elements.begin(); el != elements.end(); el++)
        {
            QJsonObject obj = (*el).toObject();
            QString type = obj["type"].toString();
            //cout << type.toStdString() << endl;
            if (type != "way")
                continue;
            //parseJsonObject( obj, 1);

            uint64_t id = obj["id"].toDouble();
            list<pair<double, double> > wayNodes;
            QJsonArray jNodes = obj["nodes"].toArray();
            for (QJsonArray::const_iterator it = jNodes.begin(); it != jNodes.end(); it++)
            {
                assert ( (*it).isDouble() == true);
                uint64_t nodeId = (*it).toDouble();
                assert( nodes.count(nodeId) > 0);
                wayNodes.push_back( nodes[nodeId]);
            }
            cout << "way " << id << " has " << wayNodes.size() << " nodes" << endl;

        }
        //QJsonValue sp = obj["lon"];

        cout << "Emitting 'done' signal" << endl;
        emit done();
    }

}

