#include "ControlServer.h"
#include "AlloyWindow.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLocalSocket>
#include <QTimer>

ControlServer::ControlServer(AlloyWindow& window, QObject* parent)
    : QLocalServer(parent), window_(window) {
    setSocketOptions(QLocalServer::UserAccessOption);
    connect(this, &QLocalServer::newConnection, this, [this] {
        while (auto* socket = nextPendingConnection()) {
            socket->setReadBufferSize(65537);
            auto* timeout = new QTimer(socket);
            timeout->setSingleShot(true);
            timeout->start(10000);
            connect(timeout, &QTimer::timeout, socket, &QLocalSocket::disconnectFromServer);
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            connect(socket, &QLocalSocket::readyRead, this, [this, socket, timeout] {
                timeout->start(10000);
                if (socket->bytesAvailable() > 65536) {
                    socket->write("{\"ok\":false,\"completion\":\"failed\",\"error\":{\"code\":\"request_too_large\",\"message\":\"Maximum request size is 64 KiB\"}}\n");
                    socket->disconnectFromServer();
                    return;
                }
                while (socket->canReadLine()) {
                    QJsonParseError parse;
                    const auto document = QJsonDocument::fromJson(socket->readLine(), &parse);
                    const auto request = document.object();
                    QJsonObject response;
                    if (parse.error != QJsonParseError::NoError || !document.isObject()
                        || !request.value("command").isString()
                        || (request.contains("args") && !request.value("args").isObject())) {
                        response = {{"ok", false}, {"completion", "failed"},
                            {"error", QJsonObject{{"code", "invalid_request"},
                            {"message", "Expected JSON object with command string and optional args object"}}}};
                    } else {
                        response = window_.execute(request.value("command").toString(), request.value("args").toObject());
                    }
                    response.insert("id", request.value("id"));
                    socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + '\n');
                    socket->flush();
                }
            });
        }
    });
}
