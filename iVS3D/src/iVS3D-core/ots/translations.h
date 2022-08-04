#ifndef LIB3D_OTS_TRANSLATIONS_H
#define LIB3D_OTS_TRANSLATIONS_H

// Qt
#include <QTranslator>
#include <QString>

namespace lib3d {
namespace ots {

/**
 * @brief Class to support different translations and provide routines to incorporate these into
 * the program.
 */
class Translations
{

  public:

    /**
     * @brief Function to load translation into given translator.
     * @param[in] ipTranslator Pointer to translator.
     * @param[in] iLocale Locale to be loaded.
     * @return True. if translation is loaded successfully. False, otherwise.
     */
    static bool load(QTranslator *ipTranslator, const QString& iLocale = "en");

};

} // namespace ots
} // namespace libvis3d

#endif // LIB3D_OTS_TRANSLATIONS_H
