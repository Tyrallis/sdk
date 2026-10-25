// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// The generated methods of one table, inside its wxl::api::facade::Methods specialization:
//
//     template <>
//     struct Methods<wxl_data_api>
//     {
//     #define WXL_API_TABLE "wxl/services/data.def"
//     #include "wxl/services/methods.inc"
//     };
//
// Each row whose `cxx` column is `generated` becomes a static member called like a method
// (wxl/services/rows.hpp, "a generated facade method"); the others are the facade's own.

#ifndef WXL_API_TABLE
#error "define WXL_API_TABLE as the table to read before including methods.inc"
#endif

#define WXL_API_SERVICE(svc, name, version, since, access, doc)
#define WXL_API_RESOURCE(svc, Name, ctype, none_value)
#define WXL_API_CLOSE(svc, member, Name, answer, Resource, when, doc)
#define WXL_API_SINCE(svc, member, since)
#define WXL_API_FIELD(type, member, kind)
#define WXL_API(svc, member, Name, answer, params, lua, cxx, doc) \
    WXL_API_CXX_##cxx(svc, member, Name, answer, params)
#include WXL_API_TABLE
#undef WXL_API
#undef WXL_API_FIELD
#undef WXL_API_SINCE
#undef WXL_API_CLOSE
#undef WXL_API_RESOURCE
#undef WXL_API_SERVICE
#undef WXL_API_TABLE
