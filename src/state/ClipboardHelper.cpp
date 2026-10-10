#include "ClipboardHelper.h"
#include <QClipboard>
#include <QGuiApplication>

void ClipboardHelper::copyToClipboard(const QString &text) {
    if (auto *clipboard = QGuiApplication::clipboard())
        clipboard->setText(text);
}
