#include "translations.h"

namespace lib3d {
namespace ots {

//==================================================================================================
bool Translations::load(QTranslator *ipTranslator, const QString &iLocale)
{
  QString translationFile = ":/translations/lib3D_ots_" + iLocale + ".qm";
  return ipTranslator->load(translationFile);
}

} // namespace ots
} // namespace lib3d
