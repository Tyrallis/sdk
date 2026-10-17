// The leaf reads of a corpse: what a runtime supplies for wxl.object's corpse getters.
// Copyright (C) 2026 WarcraftXL
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef WXL_RUNTIME_OBJECTS_CORPSE_HPP
#define WXL_RUNTIME_OBJECTS_CORPSE_HPP

#include <cstdint>

#include "wxl/abi.h"

namespace wxl::runtime
{
    /// The reads a corpse answers.
    class CorpseReads
    {
    public:
        virtual ~CorpseReads() = default;

        virtual bool CorpseOwner(void* object, wxl_guid& out) const        = 0;
        virtual bool CorpseParty(void* object, wxl_guid& out) const        = 0;
        virtual bool CorpseDisplayId(void* object, uint32_t& out) const    = 0;
        virtual bool CorpseGuild(void* object, uint32_t& out) const        = 0;
        virtual bool CorpseFlags(void* object, uint32_t& out) const        = 0;
        virtual bool CorpseDynamicFlags(void* object, uint32_t& out) const = 0;
    };
} // namespace wxl::runtime

#endif // WXL_RUNTIME_OBJECTS_CORPSE_HPP
