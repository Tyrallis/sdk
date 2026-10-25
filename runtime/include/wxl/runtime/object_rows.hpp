// SPDX-License-Identifier: MIT
// Copyright (c) 2026 WarcraftXL
//
// Helpers for reading object_reads.def: a row's `source` column picks what a consumer emits.
//
//     #define WXL_OBJECT_READ(kinds, member, Method, type, source, Cxx, cxx, doc) \
//         WXL_OBJECT_BINDS(source, virtual bool Method(void*, type&) const = 0;)
//
// WXL_OBJECT_BINDS emits its text for a row a runtime answers (field, client, cache),
// WXL_OBJECT_SHARES for a row the shared runtime answers (shared), WXL_OBJECT_FIELDS for a
// field row alone. WXL_OBJECT_MISS is what the getter answers when its read says false.

#ifndef WXL_RUNTIME_OBJECT_ROWS_HPP
#define WXL_RUNTIME_OBJECT_ROWS_HPP

#define WXL_OBJECT_BINDS_field(...)  __VA_ARGS__
#define WXL_OBJECT_BINDS_client(...) __VA_ARGS__
#define WXL_OBJECT_BINDS_cache(...)  __VA_ARGS__
#define WXL_OBJECT_BINDS_shared(...)
#define WXL_OBJECT_BINDS(source, ...) WXL_OBJECT_BINDS_##source(__VA_ARGS__)

#define WXL_OBJECT_SHARES_field(...)
#define WXL_OBJECT_SHARES_client(...)
#define WXL_OBJECT_SHARES_cache(...)
#define WXL_OBJECT_SHARES_shared(...) __VA_ARGS__
#define WXL_OBJECT_SHARES(source, ...) WXL_OBJECT_SHARES_##source(__VA_ARGS__)

#define WXL_OBJECT_FIELDS_field(...)  __VA_ARGS__
#define WXL_OBJECT_FIELDS_client(...)
#define WXL_OBJECT_FIELDS_cache(...)
#define WXL_OBJECT_FIELDS_shared(...)
#define WXL_OBJECT_FIELDS(source, ...) WXL_OBJECT_FIELDS_##source(__VA_ARGS__)

/// The client written for each runtime: not a field, not shared.
#define WXL_OBJECT_WRITTEN_field(...)
#define WXL_OBJECT_WRITTEN_client(...) __VA_ARGS__
#define WXL_OBJECT_WRITTEN_cache(...)  __VA_ARGS__
#define WXL_OBJECT_WRITTEN_shared(...)
#define WXL_OBJECT_WRITTEN(source, ...) WXL_OBJECT_WRITTEN_##source(__VA_ARGS__)

#define WXL_OBJECT_MISS_field  WXL_E_INVALID
#define WXL_OBJECT_MISS_client WXL_E_INVALID
#define WXL_OBJECT_MISS_cache  WXL_E_FAILED
#define WXL_OBJECT_MISS_shared WXL_E_INVALID
#define WXL_OBJECT_MISS(source) WXL_OBJECT_MISS_##source

#endif // WXL_RUNTIME_OBJECT_ROWS_HPP
