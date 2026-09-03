#pragma once

#include <QObject>

namespace fasthub::core {

/// @brief Base class for ViewModels providing property notification support.
class ViewModelBase : public QObject {
    Q_OBJECT
public:
    explicit ViewModelBase(QObject* parent = nullptr) : QObject(parent) {}
    ~ViewModelBase() override = default;
};

} // namespace fasthub::core
