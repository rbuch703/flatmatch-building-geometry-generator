#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <iostream>

//#include "main.moc"
#include "geometryConverter.h"

using namespace std;

QNetworkReply *reply = NULL;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString s("Hello World");
    cout << s.toStdString() << endl;

    QNetworkAccessManager *manager = new QNetworkAccessManager();

    //QObject::connect(manager, SIGNAL(finished(QNetworkReply *)),
    //                             SLOT(slotRequestFinished(QNetworkReply *)));

    QNetworkRequest request;
    request.setUrl(QUrl("http://overpass-api.de/api/interpreter?data=%5Bout%3Ajson%5D%5Btimeout%3A25%5D%3B(way%5B%22building%22%5D(52.12674216000133%2C11.630952718968814%2C52.144708569956215%2C11.66022383857208)%3Bway%5B%22building%3Apart%22%5D(52.12674216000133%2C11.630952718968814%2C52.144708569956215%2C11.66022383857208)%3Brelation%5B%22building%22%5D(52.12674216000133%2C11.630952718968814%2C52.144708569956215%2C11.66022383857208))%3Bout%20body%3B%3E%3Bout%20skel%20qt%3B"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    reply = manager->get(request);
    GeometryConverter * dummy = new GeometryConverter();

    QObject::connect(reply, SIGNAL(finished()), dummy, SLOT(onDownloadFinished()));
    //                           SLOT(slotProgress(qint64,qint64)));

    QObject::connect(dummy, SIGNAL(done()), &app, SLOT(quit()) );
    //                           SLOT(slotProgress(qint64,qint64)));

    return app.exec();

}
