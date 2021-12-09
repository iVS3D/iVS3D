#include "terminalinteraction.h"



TerminalInteraction::TerminalInteraction()
{
    m_suffixLength = sizeof(SPLITTER) + 3;
}

TerminalInteraction &TerminalInteraction::instance()
{
    static TerminalInteraction INSTANCE;
    return INSTANCE;
}

void TerminalInteraction::slot_displayMessage(QString message)
{
    QMutexLocker locker(&mutex);
    ENABLE_TERMINAL;
    std::cout << message.toStdString();
    // erases last line if neccessary and disables erasing
    if (m_eraseIsActive) {
        for (uint i = message.length(); i < m_lengthToErase; i++) {
            std::cout << " ";
        }
        m_lengthToErase = 0;
        m_eraseIsActive = false;
    }
    std::cout << std::endl;
    DISABLE_TERMINAL;
}

void TerminalInteraction::slot_displayProgress(int progress, QString currentProgress)
{
    QMutexLocker locker(&mutex);
    ENABLE_TERMINAL;
    std::cout << currentProgress.toStdString() << SPLITTER << std::to_string(progress) << "%";
    //add spaces to override the whole last line and activates it for the next line
    for (uint i = currentProgress.length(); i < m_lengthToErase; i++) {
        std::cout << " ";
    }

    std::cout << "\r" << std::flush;

    m_eraseIsActive = true;
    m_lengthToErase = currentProgress.size() + m_suffixLength;
    DISABLE_TERMINAL;
}
