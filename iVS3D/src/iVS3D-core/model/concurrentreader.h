#ifndef CONCURRENTREADER_H
#define CONCURRENTREADER_H

#include <QObject>
#include <QThread>
#include <QTimer>

#include "reader.h"
#include "opencv2/core.hpp"

/**
 * @class ConcurrentReader
 *
 * @ingroup Model
 *
 * @brief The ConcurrentReader class is a wrapper for a Reader object. The ConcurrentReader can be moved to a separate thread to
 * read images in parallel. The images can be requested using ConcurrentReader::slot_read. The reader sends the signal
 * ConcurrentReader::sig_imageReady if the image is loaded. Since usually requests for images from the gui thread comi in much
 * faster than they can be handled by the Reader, the ConcurrentReader collects blocks incoming requests and only passes the latest
 * read request to the Reader. This ensures that on slower hardware and with higher resolution images always the most recent image
 * is loaded and send to the gui.
 *
 * @author Dominik Wüst
 *
 * @date 2021/04/13
 */
class ConcurrentReader : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief ConcurrentReader creates a wrapper for the given Reader object
     * @param reader The object to use for reading images.
     */
    explicit ConcurrentReader(Reader *reader);
    ~ConcurrentReader();

public slots:
    /**
     * @brief [slot] slot_read adds the requested idx to the waiting-queue or starts the reader if the queue is empty.
     * @param idx The index to read from
     */
    void slot_read(uint idx);

private slots:
    /**
     * @brief slot_pull takes the most recent index from queue and reads the image from.
     */
    void slot_pull();

signals:
    /**
     * @brief [signal] sig_imageReady is emitted after image is loaded.
     * @param idx The index of the image that has been loaded
     * @param img The image that has been loaded
     */
    void sig_imageReady(uint idx, const cv::Mat &img);

private:
    Reader *m_reader;
    uint m_next_idx;
    QThread m_readerThread;
    QTimer *m_timer;
};

#endif // CONCURRENTREADER_H
