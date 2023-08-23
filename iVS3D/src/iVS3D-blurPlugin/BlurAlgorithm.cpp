#include "BlurAlgorithm.h"


double BlurAlgorithm::calcOneBluriness(Reader *images, int index)
{
    cv::Mat mat = images->getPic(index);
    return singleCalculation(mat);
}

std::vector<double> BlurAlgorithm::calcFullBluriness(Reader *images, Progressable *reciever, volatile bool *stopped, int start, int end, std::vector<double> blurValues)
{
    m_currentProgress = 0;
    int picCount = end - start + 1;

    //Define index list of images in range [start,end]
    std::vector<uint> index;
    index.reserve(picCount);
    for (int i = start; i <= end; i++) {
        index.push_back(i);
    }

    // create a sequential reader for accessing the images more efficiently
    SequentialReader *seqImages = images->createSequentialReader(index);

    // define lambda function to calulate blurValue for multiple images sequentially
    std::function<void()> getBlur = [seqImages, reciever, &blurValues, stopped, this]() {
        // loop until the computation gets stopped or all images are processed
        while(true){
            if (*stopped) {
                return; // user stopped the computation -> return
            }

            // get the next image, along with its index and progress so far
            cv::Mat mat;
            uint idx;
            int progress;

            if(!seqImages->getNext(mat, idx, progress)) {
                return; // no images left to process -> return
            }

            // now we do the calculation of the blur value for the image we just got
            double value = parallelCalculation(mat, blurValues, idx, progress, reciever);
            blurValues[idx] = value;
        }

    };

    QElapsedTimer timer;
    timer.start();
    // start the computation in multiple worker threads
    QFutureSynchronizer<void> synchronizer;
    // use all available threads for now
    for(int i=0; i<QThread::idealThreadCount(); i++){
        synchronizer.addFuture(QtConcurrent::run(getBlur));
    }
    synchronizer.waitForFinished();
    qDebug() << "Calculating all blur values took:" << timer.elapsed() << "ms";
    // cleanup, we have to delete the sequential reader manually
    delete seqImages;
    return blurValues;
}

double BlurAlgorithm::parallelCalculation(const cv::Mat &image, const std::vector<double> &blurValues, const uint &idx, const int &progress, Progressable *receiver)
{
    if (blurValues[idx] != 0) {
      return blurValues[idx];   // we have the blur value for this image in our buffer from previous computation -> reuse
    }

    // report the progress to the user
    if (receiver != nullptr && idx > uint(m_currentProgress)) {
        m_currentProgress = idx;
        QString currentProgress = tr("Calculate blur for frame ") + QString::number(idx);
        QMetaObject::invokeMethod(
                    receiver,
                    "slot_makeProgress",
                    Qt::DirectConnection,
                    Q_ARG(int, progress),
                    Q_ARG(QString, currentProgress));
    }

    // actually compute the blur value
    return singleCalculation(image);
}

