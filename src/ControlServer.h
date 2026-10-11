#pragma once
#include <QLocalServer>
class AlloyWindow;

// One JSON request per line, one explicit completion response per line.
class ControlServer final : public QLocalServer {
public:
    explicit ControlServer(AlloyWindow& window, QObject* parent = nullptr);
private:
    AlloyWindow& window_;
};
