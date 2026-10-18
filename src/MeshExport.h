#pragma once
#include "GeometryKernel.h"
#include <condition_variable>
#include <functional>
#include <mutex>

// Export tessellation is detached from the document and viewport caches.
class MeshExport final {
public:
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
    static QJsonObject writeStl(const std::vector<EvaluatedBody>& bodies, const QString& path,
        const QString& units, bool binary, double surfaceDeviationMm,
        double normalDeviationDeg, const std::function<bool()>& approvePublication = {});
    static QJsonObject writeStl(const EvaluatedBody& body, const QString& path,
        const QString& units, bool binary, double surfaceDeviationMm,
        double normalDeviationDeg, const std::function<bool()>& approvePublication = {});
};
