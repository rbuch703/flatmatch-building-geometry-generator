#include <iostream>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "geometryConverter.h"

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
        QJsonObject obj = doc.object();
        parseJsonObject(obj, 1);
        //QJsonValue sp = obj["lon"];

        cout << "Emitting 'done' signal" << endl;
        emit done();
    }

}

