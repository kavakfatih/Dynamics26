#include "SelectionManager.h"

#include <algorithm>
#include <utility>

namespace d26 {

SelectionManager::SelectionManager(QObject *parent) : QObject(parent) {}

int SelectionManager::indexOfIdentity(const SelectionItem &item) const noexcept
{
    for (qsizetype i = 0; i < items_.size(); ++i) {
        if (items_[i].sameIdentity(item)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool SelectionManager::containsIdentity(const SelectionItem &item) const noexcept
{
    return indexOfIdentity(item) >= 0;
}

void SelectionManager::normalizePrimary()
{
    if (items_.isEmpty()) {
        primary_.reset();
        return;
    }
    if (primary_.has_value() && containsIdentity(*primary_)) {
        const int index = indexOfIdentity(*primary_);
        primary_ = items_[index];
        return;
    }
    primary_ = items_.back();
}

void SelectionManager::setPolicy(const SelectionPolicy &policy)
{
    if (policy_ == policy) {
        return;
    }

    const QVector<SelectionItem> oldItems = items_;
    const std::optional<SelectionItem> oldPrimary = primary_;
    const std::optional<SelectionItem> oldPreselection = preselection_;

    policy_ = policy;

    QVector<SelectionItem> filtered;
    filtered.reserve(items_.size());
    for (const SelectionItem &item : items_) {
        if (policy_.accepts(item)) {
            filtered.push_back(item);
        }
    }

    if (!policy_.allowMultiple && filtered.size() > 1) {
        SelectionItem keep = filtered.back();
        if (primary_.has_value()) {
            for (const SelectionItem &candidate : filtered) {
                if (candidate.sameIdentity(*primary_)) {
                    keep = candidate;
                    break;
                }
            }
        }
        filtered = {keep};
    }

    items_ = filtered;
    normalizePrimary();

    if (preselection_.has_value() && !policy_.accepts(*preselection_)) {
        preselection_.reset();
    }

    emit policyChanged();
    if (items_ != oldItems || primary_ != oldPrimary) {
        emit selectionChanged();
    }
    if (preselection_ != oldPreselection) {
        emit preselectionChanged();
    }
}

bool SelectionManager::setPreselection(std::optional<SelectionItem> item)
{
    if (item.has_value() && !policy_.accepts(*item)) {
        item.reset();
    }
    if (preselection_ == item) {
        return false;
    }
    preselection_ = std::move(item);
    emit preselectionChanged();
    return true;
}

void SelectionManager::clearPreselection()
{
    (void)setPreselection(std::nullopt);
}

bool SelectionManager::clear()
{
    if (items_.isEmpty() && !primary_.has_value()) {
        return false;
    }
    items_.clear();
    primary_.reset();
    emit selectionChanged();
    return true;
}

bool SelectionManager::apply(const SelectionItem &item, const SelectionOperation operation)
{
    return apply(QVector<SelectionItem>{item}, operation);
}

bool SelectionManager::apply(const QVector<SelectionItem> &items,
                             const SelectionOperation operation)
{
    if (operation == SelectionOperation::Clear) {
        return clear();
    }

    QVector<SelectionItem> accepted;
    accepted.reserve(items.size());
    for (const SelectionItem &item : items) {
        if (!policy_.accepts(item)) {
            continue;
        }

        int duplicate = -1;
        for (qsizetype i = 0; i < accepted.size(); ++i) {
            if (accepted[i].sameIdentity(item)) {
                duplicate = static_cast<int>(i);
                break;
            }
        }
        if (duplicate >= 0) {
            // Ayni engineering identity area selector'dan birden fazla display
            // primitive ile gelebilir. Kimligi tek tut, metadata'yi tazele.
            accepted[duplicate] = item;
        } else {
            accepted.push_back(item);
        }
    }

    if (accepted.isEmpty()) {
        // Empty window Replace, profesyonel CAE davranisinda selection clear'dir.
        // Add/Remove/Toggle empty hit ise mevcut selection'i korur.
        return operation == SelectionOperation::Replace ? clear() : false;
    }

    const QVector<SelectionItem> oldItems = items_;
    const std::optional<SelectionItem> oldPrimary = primary_;

    switch (operation) {
    case SelectionOperation::Replace:
        if (!policy_.allowMultiple && accepted.size() > 1) {
            items_ = {accepted.back()};
        } else {
            items_ = accepted;
        }
        primary_ = items_.isEmpty() ? std::optional<SelectionItem>{}
                                    : std::optional<SelectionItem>{items_.back()};
        break;

    case SelectionOperation::Add:
        for (const SelectionItem &item : accepted) {
            const int existing = indexOfIdentity(item);
            if (!policy_.allowMultiple) {
                items_ = {item};
            } else if (existing >= 0) {
                items_[existing] = item;
            } else {
                items_.push_back(item);
            }
            primary_ = item;
        }
        break;

    case SelectionOperation::Remove:
        for (const SelectionItem &item : accepted) {
            const int existing = indexOfIdentity(item);
            if (existing < 0) {
                continue;
            }
            items_.removeAt(existing);
            if (primary_.has_value() && primary_->sameIdentity(item)) {
                primary_.reset();
            }
        }
        normalizePrimary();
        break;

    case SelectionOperation::Toggle: {
        std::optional<SelectionItem> lastAdded;
        for (const SelectionItem &item : accepted) {
            const int existing = indexOfIdentity(item);
            if (existing >= 0) {
                items_.removeAt(existing);
                if (primary_.has_value() && primary_->sameIdentity(item)) {
                    primary_.reset();
                }
                continue;
            }
            if (!policy_.allowMultiple) {
                items_ = {item};
            } else {
                items_.push_back(item);
            }
            lastAdded = item;
        }
        if (lastAdded.has_value()) {
            primary_ = lastAdded;
        } else {
            normalizePrimary();
        }
        break;
    }

    case SelectionOperation::Clear:
        break;
    }

    if (items_ == oldItems && primary_ == oldPrimary) {
        return false;
    }
    emit selectionChanged();
    return true;
}

bool SelectionManager::invalidateSourceRevision(const SelectionDomain domain, const quint64 currentRevision)
{
    const QVector<SelectionItem> oldItems = items_;
    const std::optional<SelectionItem> oldPrimary = primary_;
    const std::optional<SelectionItem> oldPreselection = preselection_;

    items_.erase(std::remove_if(items_.begin(), items_.end(), [domain, currentRevision](const SelectionItem &item) {
        return item.domain == domain && item.sourceRevision != currentRevision;
    }), items_.end());
    normalizePrimary();

    if (preselection_.has_value() && preselection_->domain == domain
        && preselection_->sourceRevision != currentRevision) {
        preselection_.reset();
    }

    bool changed = false;
    if (items_ != oldItems || primary_ != oldPrimary) {
        emit selectionChanged();
        changed = true;
    }
    if (preselection_ != oldPreselection) {
        emit preselectionChanged();
        changed = true;
    }
    return changed;
}

bool SelectionManager::invalidateGeometryRevision(const quint64 currentRevision)
{
    return invalidateSourceRevision(SelectionDomain::Geometry, currentRevision);
}

bool SelectionManager::invalidateMeshGeneration(const quint64 currentGeneration)
{
    return invalidateSourceRevision(SelectionDomain::Mesh, currentGeneration);
}

} // namespace d26
