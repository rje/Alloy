#include "ControlServer.h"
#include "AlloyWindow.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLocalSocket>
#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>

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
                        response = window_.executeApi(request.value("command").toString(), request.value("args").toObject());
                    }
                    response.insert("id", request.value("id"));
                    const bool traceMesh=qEnvironmentVariableIsSet("ALLOY_TRACE_MESH_COST");QElapsedTimer timer;if(traceMesh)timer.start();
                    const auto bytes=QJsonDocument(response).toJson(QJsonDocument::Compact) + '\n';
                    if(traceMesh)qInfo()<<"mesh_response_serialization_ms"<<request.value("command").toString()<<timer.elapsed()<<"bytes"<<bytes.size();
                    socket->write(bytes);
                    socket->flush();
                }
            });
        }
    });
}
