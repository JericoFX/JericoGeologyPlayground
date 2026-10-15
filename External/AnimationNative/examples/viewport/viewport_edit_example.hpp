// SPDX-License-Identifier: MIT
#pragma once
// Original pure authoring example. No renderer, toolkit or native target dependency.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace JE::Research::ViewportEdit {
struct Vec3 {
    double x{},y{},z{};
    bool operator==(const Vec3&) const = default;
};
struct Point {
    std::uint64_t id{};
    Vec3 position;
    bool locked{};
    bool operator==(const Point&) const = default;
};
inline bool Finite(Vec3 value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}
class StaleRevision final:public std::runtime_error {
public:
    StaleRevision():std::runtime_error("Document revision changed during drag") {}
};
class Drag;
class Document {
    struct Entry {std::vector<Point> before,after;};
    std::vector<Point> points_;
    std::vector<Entry> undo_,redo_;
    std::uint64_t revision_{};
    friend class Drag;

    std::size_t index(std::uint64_t id) const {
        const auto found=std::find_if(points_.begin(),points_.end(),[&](const auto& p){return p.id==id;});
        if(found==points_.end())throw std::invalid_argument("Unknown point ID");
        return static_cast<std::size_t>(found-points_.begin());
    }
    void require_revision(std::uint64_t revision) const {
        if(revision_!=revision)throw StaleRevision();
    }
    bool publish(std::uint64_t revision,std::span<const Point> preview) {
        require_revision(revision);
        auto next=points_;
        for(const auto& p:preview) {
            if(!Finite(p.position))throw std::invalid_argument("Nonfinite preview position");
            const auto i=index(p.id);
            if(!points_[i].locked)next[i].position=p.position;
        }
        if(next==points_)return false;
        // All validation and potentially throwing allocation precede publication.
        undo_.push_back({points_,next});
        points_.swap(next);redo_.clear();++revision_;
        return true;
    }
public:
    explicit Document(std::vector<Point> points):points_(std::move(points)) {
        std::unordered_set<std::uint64_t> ids;
        for(const auto& p:points_) {
            if(p.id==0 || !ids.insert(p.id).second || !Finite(p.position))
                throw std::invalid_argument("Document requires unique nonzero IDs and finite positions");
        }
    }
    Document(const Document&)=delete;
    Document& operator=(const Document&)=delete;
    Document(Document&&)=delete;
    Document& operator=(Document&&)=delete;
    const std::vector<Point>& points() const noexcept {return points_;}
    const Point& point(std::uint64_t id) const {return points_[index(id)];}
    std::uint64_t revision() const noexcept {return revision_;}
    std::size_t undo_count() const noexcept {return undo_.size();}
    std::size_t redo_count() const noexcept {return redo_.size();}
    bool undo() {
        if(undo_.empty())return false;
        auto next=undo_.back().before;
        redo_.push_back(undo_.back());
        points_.swap(next);undo_.pop_back();++revision_;
        return true;
    }
    bool redo() {
        if(redo_.empty())return false;
        auto next=redo_.back().after;
        undo_.push_back(redo_.back());
        points_.swap(next);redo_.pop_back();++revision_;
        return true;
    }
};
class Drag {
    Document& document_;
    std::uint64_t revision_;
    std::vector<Point> start_,preview_;
    bool active_{true};
    void require_active() const {
        if(!active_)throw std::invalid_argument("Drag is no longer active");
    }
public:
    // Document must outlive the drag; its single owner serializes all operations.
    Drag(Document& document,std::vector<std::uint64_t> selected):document_(document),revision_(document.revision()) {
        if(selected.empty())throw std::invalid_argument("Drag requires a selection");
        std::unordered_set<std::uint64_t> ids;
        start_.reserve(selected.size());
        for(const auto id:selected) {
            if(!ids.insert(id).second)throw std::invalid_argument("Duplicate selected point ID");
            start_.push_back(document.point(id));
        }
        preview_=start_;
    }
    Drag(const Drag&)=delete;
    Drag& operator=(const Drag&)=delete;
    Drag(Drag&&)=delete;
    Drag& operator=(Drag&&)=delete;
    bool active() const noexcept {return active_;}
    const std::vector<Point>& preview() const noexcept {return preview_;}
    void update_delta(Vec3 delta) {
        require_active();document_.require_revision(revision_);
        if(!Finite(delta))throw std::invalid_argument("Nonfinite drag delta");
        auto next=start_;
        for(auto& p:next)if(!p.locked) {
            p.position={p.position.x+delta.x,p.position.y+delta.y,p.position.z+delta.z};
            if(!Finite(p.position))throw std::invalid_argument("Drag result overflow");
        }
        preview_.swap(next);
    }
    bool commit() {
        require_active();
        const auto changed=document_.publish(revision_,preview_);
        active_=false;return changed;
    }
    void cancel() noexcept {active_=false;}
};
} // namespace JE::Research::ViewportEdit
