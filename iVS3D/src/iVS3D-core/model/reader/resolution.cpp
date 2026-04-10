#include "resolution.h"

#include "stringcontainer.h"

bool Resolution::fromString(const QString& resolution) {
    // remove spaces
    QString resolutionString = resolution.simplified();
    // split at x
    QStringList xSplitList =
        resolutionString.split(stringContainer::ROISpliter);

    int width = -1;
    int height = -1;
    if (xSplitList.size() <= 1) {
        // we dont have a x to split between
    } else {
        bool oneInteger = false;

        // DETERMINE Width
        // create space split list
        QStringList spaceSplitList = xSplitList[0].split(" ");
        // iterate over x split Strings which are split by a space
        for (int n = 0; n < spaceSplitList.size(); n++) {
            // remove all but numbers in string
            spaceSplitList[n].replace(QRegExp("[^\\d]"), "");
            // parse the leftover String
            int parseTemp = spaceSplitList[n].toInt();
            if (parseTemp > 0) {
                if (!oneInteger) {
                    oneInteger = true;
                    width = parseTemp;
                } else {
                    // more than one number in one x-section
                    return false;
                }
            }
        }
        // catch if no number is inside x String
        if (!oneInteger) {
            return false;
        }
        oneInteger = false;
        // DETERMINE Height
        // create space split list
        spaceSplitList = xSplitList[1].split(" ");
        // iterate over x split Strings which are split by a space
        for (int n = 0; n < spaceSplitList.size(); n++) {
            // remove all but numbers in string
            spaceSplitList[n].replace(QRegExp("[^\\d]"), "");
            // parse the leftover String
            int parseTemp = spaceSplitList[n].toInt();
            if (parseTemp > 0) {
                if (!oneInteger) {
                    oneInteger = true;
                    height = parseTemp;
                } else {
                    // more than one number in one y-section
                    return false;
                }
            }
        }
        // catch if no number is inside x String
        if (!oneInteger) {
            return false;
        }
    }
    if (width < 0 || height < 0) return false;

    m_width = uint(width);
    m_height = uint(height);
    return true;
}
