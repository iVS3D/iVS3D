#ifndef KEYFRAMESELECTOR_H
#define KEYFRAMESELECTOR_H

#include <QObject>
#include <QVariant>

/**
 * @class KeyframeSelector
 *
 * @ingroup StationaryCameraPlugin
 *
 * @brief The ImageGatherer class is an interface which defines the structure for the
 *        usecase specific algorithm. The algorithm uses a vector of flow values and indicies
 *        as parameters and selects based on these frames.
 *
 * @author Dominic Zahn
 *
 * @date 2022/07/08
 */
class KeyframeSelector : public QObject
{
    Q_OBJECT
public:
    struct Parameter {
        QString name;
        QString info;
        QVariant value;

        Parameter(QString name, QString info, QVariant value) {
            this->name = name;
            this->info = info;
            this->value = value;
        }

        Parameter(QVariant variantParam) {
            QMap<QString, QVariant> map = variantParam.toMap();
            name = map.value("name").toString();
            info = map.value("info").toString();
            value = map.value("value");
        }

        QVariant toQVariant() {
            QMap<QString, QVariant> map;
            map.insert("name", name);
            map.insert("info", info);
            map.insert("value", value);
            return map;
        }
    };
    typedef QList<Parameter> Settings;

    virtual std::vector<uint> select(std::vector<uint> frameVector, std::vector<double> flowValues, volatile bool *stopped) = 0;
};

#endif // KEYFRAMESELECTOR_H
