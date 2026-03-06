#pragma once

#include <QString>
#include <QPluginLoader>
#include <QObject>
#include <memory>
#include <optional>

#include "ibase.h"
#include "ipreview.h"
#include "imask.h"
#include "iselection.h"

struct PluginHandle {
  QPluginLoader* loader = nullptr;
  PLUG::IBase* base = nullptr;
  QObject* qobject = nullptr;
    PLUG::IPreview* preview = nullptr;
    PLUG::IMask* mask = nullptr;
    PLUG::ISelection* selection = nullptr;
    std::shared_ptr<QWidget> settingsWidget = nullptr;
    std::optional<PLUG::Error> settingsWidgetError = std::nullopt;

    QString name() const { return base ? base->getName() : QString(); }

    bool hasPreview() const { return preview != nullptr; }

    bool hasMask() const { return mask != nullptr; }

    bool hasSelection() const { return selection != nullptr; }
};

#define TRY_ASSIGN(lhs, expr)                        \
  auto _tmp_##lhs = (expr);                          \
  if (!_tmp_##lhs) return tl::unexpected(_tmp_##lhs.error()); \
  lhs = std::move(*_tmp_##lhs)
