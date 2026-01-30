#pragma once

#include <QString>
#include <QPluginLoader>
#include <QObject>

#include "ibase.h"
#include "ipreview.h"
#include "imask.h"
#include "iselection.h"

struct PluginHandle {
    QPluginLoader* loader;
    IBase* base;
    QObject* qobject;
    IPreview* preview = nullptr;
    IMask* mask = nullptr;
    ISelection* selection = nullptr;

    QString name() const { return base ? base->getName() : QString(); }

    bool hasPreview() const { return preview != nullptr; }

    bool hasMask() const { return mask != nullptr; }

    bool hasSelection() const { return selection != nullptr; }
};

#define TRY_ASSIGN(lhs, expr)                        \
  auto _tmp_##lhs = (expr);                          \
  if (!_tmp_##lhs) return tl::unexpected(_tmp_##lhs.error()); \
  lhs = std::move(*_tmp_##lhs)