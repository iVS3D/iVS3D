#pragma once
#include <QMutex>
#include <QObject>
#include <QThread>
#include <QWaitCondition>
#include <functional>
#include <optional>

#include "opencv2/core/mat.hpp"

struct ImageRequest {
    uint idx;
};
Q_DECLARE_METATYPE(ImageRequest)

struct ImageResult {
    uint idx;
    cv::Mat img;
};
Q_DECLARE_METATYPE(ImageResult)

class AsyncImageLoader : public QObject {
    Q_OBJECT
   public:
    using WorkFunction = std::function<ImageResult(const ImageRequest&)>;

    AsyncImageLoader(WorkFunction fn, QObject* parent = nullptr)
        : QObject(parent), m_fn(fn) {
        static bool registered = false;
        if (!registered) {
            qRegisterMetaType<ImageRequest>("ImageRequest");
            qRegisterMetaType<ImageResult>("ImageResult");
            registered = true;
        }
        m_thread = new QThread(this);
        connect(m_thread, &QThread::started, this,
                &AsyncImageLoader::processLoop);
        this->moveToThread(m_thread);
        m_thread->start();
    }

    ~AsyncImageLoader() {
        {
            QMutexLocker lock(&m_mutex);
            m_quit = true;
            m_wait.wakeAll();
        }
        m_thread->quit();
        m_thread->wait();
    }

    // GUI thread calls this
    void request(const ImageRequest req) {
        QMutexLocker lock(&m_mutex);
        m_latestRequest = req;
        m_hasNewRequest = true;
        m_wait.wakeAll();
    }

   signals:
    void finished(const ImageRequest req, const ImageResult res);

   private slots:
    void processLoop() {
        while (true) {
            ImageRequest work;

            {
                QMutexLocker lock(&m_mutex);
                while (!m_hasNewRequest && !m_quit) m_wait.wait(&m_mutex);

                if (m_quit) return;

                // take the latest request, discard others
                work = *m_latestRequest;
                m_hasNewRequest = false;
            }

            // do work without holding the mutex
            ImageResult result = m_fn(work);

            emit finished(work, result);
        }
    }

   private:
    WorkFunction m_fn;

    QThread* m_thread;
    QMutex m_mutex;
    QWaitCondition m_wait;

    std::optional<ImageRequest> m_latestRequest;
    bool m_hasNewRequest = false;
    bool m_quit = false;
};
