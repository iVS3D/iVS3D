#include "BlurAlgorithm.h"


double BlurAlgorithm::calcOneBluriness(Reader *images, int index)
{
    return singleCalculation(images->getPic(index));
}

std::vector<double> BlurAlgorithm::calcFullBluriness(Reader *images, Progressable *reciever, volatile bool *stopped, int start, int end, std::vector<double> blurValues)
{
    images->enableMultithreading();
    m_currentProgress = 0;
    int picCount = end - start + 1;
    //Define lambda function to calulate blurValues
    std::function<void(const int)> getBlur = [images, start, picCount, reciever, &blurValues, stopped, this](const int &n) {
        if (*stopped) {
            return;
        }
        cv::Mat mat;
        while (mat.empty()) {
           mat = images->getPic(n, true);
        }
        double value = parallelCalculation(mat, blurValues, n, reciever, start, picCount);
        blurValues[n] = value;

    };
    //Define index of images
    QVector<uint> index;
    for (int i = start; i <= end; i++) {
        index.append(i);
    }
    images->initMultipleAccess(index.toStdVector());
    //calculate blurValues on multiple threads
    QtConcurrent::blockingMap(index, getBlur);
    qDebug() << "STOPED";
    return blurValues;

}

double BlurAlgorithm::parallelCalculation(cv::Mat image, std::vector<double> blurValues, int n, Progressable *receiver, int start, int picCount)
{
    if (blurValues[n] != 0) {
      return blurValues[n];
    }

    if (receiver != nullptr && n > m_currentProgress) {
        m_currentProgress = n;
        int progress = ((n - start) * 100 / picCount);
        QString currentProgress = "Calculate blur of frame number " + QString::number(n - start) + " of " + QString::number(picCount) + " total frames";
        QMetaObject::invokeMethod(
                    receiver,
                    "slot_makeProgress",
                    Qt::DirectConnection,
                    Q_ARG(int, progress),
                    Q_ARG(QString, currentProgress));
    }
    return singleCalculation(image);
}

