#include "Sbx/Runtime/LuauRuntime.hpp"

#include <cstdint>
#include <mutex>

#include "Luau/CodeGen.h"
#include "lua.h"
#include "lualib.h"

#include "Sbx/Runtime/Base.hpp"

ThreadHandle::ThreadHandle(lua_State *L) :
		L(L) {
	if (std::mutex *mut = luaSBX_getthreaddata(L)->mutex.get())
		mut->lock();
}

ThreadHandle::~ThreadHandle() {
	if (std::mutex *mut = luaSBX_getthreaddata(L)->mutex.get())
		mut->unlock();
}

ThreadHandle::operator lua_State *() const { return L; }

LuauRuntime::LuauRuntime(void (*initCallback)(lua_State *)) :
		initCallback(initCallback) {
	vms[CoreVM] = luaSBX_newstate(CoreVM, ElevatedGameScriptIdentity);
	initVM(vms[CoreVM]);

	vms[UserVM] = luaSBX_newstate(UserVM, GameScriptIdentity);
	initVM(vms[UserVM]);
}

LuauRuntime::~LuauRuntime() {
	for (lua_State *&L : vms) {
		luaSBX_close(L);
		L = nullptr;
	}
}

void LuauRuntime::initVM(lua_State *L) {
	if (initCallback)
		initCallback(L);

	// Seal main global state
	luaL_sandbox(L);

	if (Luau::CodeGen::isSupported())
		Luau::CodeGen::create(L);
}

ThreadHandle LuauRuntime::getVM(VMType type) {
	return vms[type];
}

void LuauRuntime::gcStep(const uint32_t *step, double delta) {
	for (int i = 0; i < VM_MAX; i++) {
		ThreadHandle L = getVM(VMType(i));
		lua_gc(L, LUA_GCSTEP, step[i] * delta);
	}
}

void LuauRuntime::gcSize(int32_t *outBuffer) {
	for (int i = 0; i < VM_MAX; i++) {
		ThreadHandle L = getVM(VMType(i));
		outBuffer[i] = lua_gc(L, LUA_GCCOUNT, 0);
	}
}
