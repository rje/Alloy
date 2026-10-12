#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <TopoDS_Shape.hxx>
#include <stdexcept>

// Shape-only STEP exchange. Document ownership and filesystem transactions stay
// with CadDocument and the shared application commands.
class StepExchange final {
public:
    class Error final : public std::runtime_error {
    public:
        explicit Error(const QJsonObject& diagnostics);
        const QJsonObject& diagnostics() const { return diagnostics_; }
    private:
        QJsonObject diagnostics_;
    };
    struct ReadResult {
        TopoDS_Shape shape;
        QJsonObject diagnostics;
    };
    struct WriteResult {
        QByteArray bytes;
        QJsonObject diagnostics;
    };
    static constexpr qsizetype maximumSourceBytes = 32 * 1024 * 1024;
    static ReadResult read(const QByteArray& source);
    static WriteResult write(const TopoDS_Shape& shape, const QString& outputUnits);
};
