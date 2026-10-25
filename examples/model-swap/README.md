# model-swap

A C++ mod that changes how the engine works: it attaches to the point `CM2Cache::CreateShared`, the
function every model of the client goes through by file name, logs each model the first time it
loads, and draws every murloc as a gnoll. The walkthrough is "Changing the engine: points" in
[`docs/building-a-mod.md`](../../docs/building-a-mod.md).

Build it like any mod (`cmake -S . -B build && cmake --build build`), copy
`build/dist/wxl.model-swap/` into the client's `Mods/`, and watch `Logs\WarcraftXL.log` for
`[model-swap]` lines as you walk around.
