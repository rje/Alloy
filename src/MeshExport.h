#pragma once
#include "GeometryKernel.h"
#include <condition_variable>
#include <functional>
#include <mutex>
#include <QByteArray>
#include <memory>

// Export tessellation is detached from the document and viewport caches.
class MeshExport final {
public:
    struct PreparedArtifact {
        QByteArray bytes;
        std::vector<gp_Pnt> facetsMm; // Three serialized vertices per final triangle, world millimeters.
        QJsonObject metadata;
    };
    static std::shared_ptr<const PreparedArtifact> prepare(const EvaluatedBody& body,
        const gp_Trsf& placement,const QString& format,const QString& units,
        double surfaceDeviationMm,double normalDeviationDeg);
    static QJsonObject publish(const PreparedArtifact& artifact,const QString& path);
    // Value-only handshake: approval is the publication linearization point.
    // The GUI never waits; the worker owns staging and atomic file publication.
    class PublicationGate final {
    public:
        enum class State { Preparing, AwaitingApproval, Approved, Denied };
        State state() const { std::lock_guard lock(mutex_); return state_; }
        bool approve() {
            std::lock_guard lock(mutex_);
            if(state_!=State::AwaitingApproval)return false;
            state_=State::Approved;changed_.notify_all();return true;
        }
        bool deny() {
            std::lock_guard lock(mutex_);
            if(state_==State::Approved)return false;
            state_=State::Denied;changed_.notify_all();return true;
        }
        bool request() {
            std::unique_lock lock(mutex_);
            if(state_==State::Preparing)state_=State::AwaitingApproval;
            changed_.wait(lock,[this]{return state_==State::Approved || state_==State::Denied;});
            return state_==State::Approved;
        }
    private:
        mutable std::mutex mutex_;
        std::condition_variable changed_;
        State state_=State::Preparing;
    };
    // Inputs are committed world geometry; each supplied placement is removed
    // from mesh coordinates and emitted exactly once on its Core build item.
    static QJsonObject write3mf(const std::vector<EvaluatedBody>& bodies,
        const std::vector<gp_Trsf>& buildTransforms, const QString& path,
        double surfaceDeviationMm, double normalDeviationDeg,
        const std::function<bool()>& approvePublication = {});
    // Deterministic material-free triangles, separate groups, world Z-up.
    static QJsonObject writeObj(const std::vector<EvaluatedBody>& bodies, const QString& path,
        const QString& units, double surfaceDeviationMm, double normalDeviationDeg,
        const std::function<bool()>& approvePublication = {});
    static QJsonObject writeStl(const std::vector<EvaluatedBody>& bodies, const QString& path,
        const QString& units, bool binary, double surfaceDeviationMm,
        double normalDeviationDeg, const std::function<bool()>& approvePublication = {});
    static QJsonObject writeStl(const EvaluatedBody& body, const QString& path,
        const QString& units, bool binary, double surfaceDeviationMm,
        double normalDeviationDeg, const std::function<bool()>& approvePublication = {});
};
