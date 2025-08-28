#include "BlurAlgorithm.h"

// Global CUDA switch controlled by Blur::sampleImages(useCuda)
bool g_useCuda = false;

#if defined(BLUR_PLUGIN_OPENCV_HAS_CUDA_LINK)
#include <opencv2/core/cuda.hpp>
#endif

double BlurAlgorithm::calcOneBluriness(Reader *images, int index)
{
    cv::Mat mat = images->getPic(index);

#if defined(BLUR_PLUGIN_OPENCV_HAS_CUDA_LINK)
    if (g_useCuda) {
        static thread_local bool t_cudaInited = false;
        if (!t_cudaInited) {
            // Validate device count to avoid silent failures
            if (cv::cuda::getCudaEnabledDeviceCount() <= 0) {
                throw std::runtime_error("No CUDA devices available");
            }
            cv::cuda::setDevice(0);
            t_cudaInited = true;
        }
    }
#endif

    return singleCalculation(mat);
}

std::vector<double> BlurAlgorithm::calcFullBluriness(Reader *images,
                                                     Progressable *reciever,
                                                     volatile bool *stopped,
                                                     int start,
                                                     int end,
                                                     std::vector<double> blurValues)
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
// Per-thread CUDA init if requested (no error masking)
#if defined(BLUR_PLUGIN_OPENCV_HAS_CUDA_LINK)
        if (g_useCuda) {
            static thread_local bool t_cudaInited = false;
            if (!t_cudaInited) {
                cv::cuda::setDevice(0);
                t_cudaInited = true;
            }
        }
#endif

        while (true) {
            if (*stopped)
                return;

            cv::Mat mat;
            uint idx;
            int progress;
            if (!seqImages->getNext(mat, idx, progress)) {
                return;
            }

            double value = parallelCalculation(mat, blurValues, idx, progress, reciever);
            blurValues[idx] = value;
        }
    };

    QElapsedTimer timer;
    timer.start();

    int workers = QThread::idealThreadCount();  

    QFutureSynchronizer<void> synchronizer;
    for (int i = 0; i < workers; i++) {
        synchronizer.addFuture(QtConcurrent::run(getBlur));
    }
    synchronizer.waitForFinished();
    qDebug() << "Calculating all blur values took:" << timer.elapsed() << "ms";

    delete seqImages;
    return blurValues;
}

double BlurAlgorithm::parallelCalculation(const cv::Mat &image,
                                          const std::vector<double> &blurValues,
                                          const uint &idx,
                                          const int &progress,
                                          Progressable *receiver)
{
    if (blurValues[idx] != 0) {
        return blurValues[idx]; // reuse from buffer
    }

    // report the progress to the user
    if (receiver != nullptr && idx > uint(m_currentProgress)) {
        m_currentProgress = idx;
        QString currentProgress = tr("Calculate blur for frame ") + QString::number(idx);
        QMetaObject::invokeMethod(receiver,
                                  "slot_makeProgress",
                                  Qt::DirectConnection,
                                  Q_ARG(int, progress),
                                  Q_ARG(QString, currentProgress));
    }

    // compute blur value (no error masking)
    return singleCalculation(image);
}
