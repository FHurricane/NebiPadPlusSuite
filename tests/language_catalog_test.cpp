#include "language.h"

#include <QCollator>
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    const auto &catalog = languages();
    if (catalog.size() < 110) return 1;
    const int arduino = detectLanguage("controller.ino", {});
    if (arduino < 0 || catalog[arduino].name != "Arduino Files" ||
        !catalog[arduino].extensions.contains("ino") || !catalog[arduino].shebang.isEmpty() ||
        catalog[arduino].indentStyle != IndentStyle::Braces) return 5;
    for (int i = 0; i < catalog.size(); ++i) {
        const auto &language = catalog[i];
        if (i > 0 && collator.compare(catalog[i - 1].name, language.name) >= 0) {
            qCritical() << "Catalog not sorted:" << language.name;
            return 2;
        }
        if (!language.extensions.isEmpty() &&
            detectLanguage("example." + language.extensions.first(), {}) != i) {
            qCritical() << "Extension detection failed:" << language.name;
            return 3;
        }
        if (!language.shebang.isEmpty() &&
            detectLanguage({}, language.shebang + "\nprint") != i) {
            qCritical() << "Shebang detection failed:" << language.name;
            return 4;
        }
    }
    return 0;
}
