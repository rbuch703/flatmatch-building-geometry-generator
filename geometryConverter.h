#ifndef GEOMETRYCONVERTER_H
#define GEOMETRYCONVERTER_H

#include <QObject>

using namespace std;

class GeometryConverter: public QObject {
    Q_OBJECT

public slots:
    void onDownloadFinished();

signals:
    void done();
};


#endif // GEOMETRYCONVERTER_H
