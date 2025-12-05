#pragma once
#include <QPluginLoader>
#include "ipreview.h"
#include "ibase.h"
#include <vector>

class PreviewManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief instance returns the singelton of this class
     * @return previewmanager instance
     */
    static PreviewManager& instance();

    int getPreviewCount() {
        return static_cast<int>(m_previewList.size());
    }

    Visualization getVisualization(uint idx, PreviewData data) {
        if (idx >= m_previewList.size()) {
            return Visualization{};
        }
        // TODO: move this to a worker thread if necessary
        return m_previewList[idx].preview->generatePreview(data);
    }

private:

    PreviewManager();

    struct PreviewPlugin {
        IPreview *preview;
        IBase *base;
        QPluginLoader *loader;
    };

    std::vector<PreviewPlugin> m_previewList;

    void loadPlugins();

private slots:
    void updatePreviewRequest();
};