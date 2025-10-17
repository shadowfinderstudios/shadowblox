#pragma once

#include <cstdint>

struct lua_State;
class ThreadHandle;

class LuauRuntime {
public:
	enum VMType {
		CoreVM = 0,
		UserVM,
		VM_MAX
	};

	LuauRuntime(void (*initCallback)(lua_State *));
	~LuauRuntime();

	ThreadHandle getVM(VMType type);

	void gcStep(const uint32_t *step, double delta);
	void gcSize(int32_t *outBuffer);

private:
	void (*initCallback)(lua_State *);
	lua_State *vms[VM_MAX];
	void initVM(lua_State *L);
};

class ThreadHandle {
public:
	ThreadHandle(lua_State *L);
	~ThreadHandle();
	operator lua_State *() const;

private:
	lua_State *L;
};
