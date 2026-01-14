#pragma once
#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <functional>
#include <optional>
#include "opencv2/core.hpp"
#include "visualization.h"


struct PreviewRequest {
    uint idx;
    cv::Mat img;
};
Q_DECLARE_METATYPE(PreviewRequest)

struct PreviewResult {
    uint idx;
    VisualizationResult visualization;
};
Q_DECLARE_METATYPE(PreviewResult)

class AsyncPreview : public QObject
{
    Q_OBJECT
public:
    using WorkFunction = std::function<PreviewResult(const PreviewRequest&)>;

    AsyncPreview(WorkFunction fn, QObject* parent = nullptr)
        : QObject(parent), m_fn(fn) 
    {
        static bool registered = false;
        if (!registered) {
            qRegisterMetaType<PreviewRequest>("PreviewRequest");
            qRegisterMetaType<PreviewResult>("PreviewResult");
            registered = true;
        }
        m_thread = new QThread(this);
        connect(m_thread, &QThread::started, this, &AsyncPreview::processLoop);
        this->moveToThread(m_thread);
        m_thread->start();
    }

    ~AsyncPreview() {
        {
            QMutexLocker lock(&m_mutex);
            m_quit = true;
            m_wait.wakeAll();
        }
        m_thread->quit();
        m_thread->wait();
    }

    // GUI thread calls this
    void request(const PreviewRequest& req) {
        QMutexLocker lock(&m_mutex);
        m_latestRequest = req;
        m_hasNewRequest = true;
        m_wait.wakeAll();
    }

signals:
    void finished(const PreviewRequest& req, const PreviewResult& res);

private slots:
    void processLoop() {
        while (true) {
            PreviewRequest work;

            {
                QMutexLocker lock(&m_mutex);
                while (!m_hasNewRequest && !m_quit)
                    m_wait.wait(&m_mutex);

                if (m_quit)
                    return;

                // take the latest request, discard others
                work = *m_latestRequest;
                m_hasNewRequest = false;
            }

            // do work without holding the mutex
            PreviewResult result = m_fn(work);

            emit finished(work, result);
        }
    }

private:
    WorkFunction m_fn;

    QThread* m_thread;
    QMutex m_mutex;
    QWaitCondition m_wait;

    std::optional<PreviewRequest> m_latestRequest;
    bool m_hasNewRequest = false;
    bool m_quit = false;
};