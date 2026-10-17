// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL

#include "wxl/runtime/handles.hpp"

namespace wxl::runtime
{
    HandleTable::HandleTable()
    {
        // Slot 0 exists but is never handed out, so WXL_HANDLE_NONE cannot resolve.
        slots_.emplace_back();
    }

    wxl_handle HandleTable::Track(void* object)
    {
        if (!object) return WXL_HANDLE_NONE;

        const auto existing = byObject_.find(object);
        if (existing != byObject_.end()) return existing->second;

        std::size_t index;
        if (!free_.empty())
        {
            index = free_.back();
            free_.pop_back();
        }
        else
        {
            index = slots_.size();
            slots_.emplace_back();
        }

        Slot& slot  = slots_[index];
        slot.object = object;

        const wxl_handle handle = Pack(index, slot.generation);
        byObject_.emplace(object, handle);
        ++live_;
        return handle;
    }

    void HandleTable::Untrack(void* object)
    {
        const auto existing = byObject_.find(object);
        if (existing == byObject_.end()) return;

        const std::size_t index = IndexOf(existing->second);
        byObject_.erase(existing);

        if (index == 0 || index >= slots_.size()) return;

        Slot& slot  = slots_[index];
        slot.object = nullptr;

        // Bumping past the handed-out generation is what makes the old handle stale. Wrapping
        // back to 0 would make it live again, so skip 0 on the wrap.
        if (++slot.generation == 0) slot.generation = 1;

        free_.push_back(index);
        --live_;
    }

    void HandleTable::Clear()
    {
        for (std::size_t index = 1; index < slots_.size(); ++index)
        {
            Slot& slot = slots_[index];
            if (!slot.object) continue;

            slot.object = nullptr;
            if (++slot.generation == 0) slot.generation = 1;
            free_.push_back(index);
        }

        byObject_.clear();
        live_ = 0;
    }

    void* HandleTable::Resolve(wxl_handle handle) const
    {
        const std::size_t index = IndexOf(handle);
        if (index == 0 || index >= slots_.size()) return nullptr;

        const Slot& slot = slots_[index];
        if (slot.generation != GenerationOf(handle)) return nullptr;
        return slot.object;
    }

    wxl_handle HandleTable::HandleOf(void* object) const
    {
        const auto existing = byObject_.find(object);
        return existing == byObject_.end() ? WXL_HANDLE_NONE : existing->second;
    }
} // namespace wxl::runtime
