// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

class ScriptingManager
{
public:
	ScriptingManager() = default;
	~ScriptingManager();

	// non-copyable
	ScriptingManager(const ScriptingManager&) = delete;
	ScriptingManager& operator=(const ScriptingManager&) = delete;

	static ScriptingManager& getInstance()
	{
		static ScriptingManager instance;
		return instance;
	}

	bool loadScriptSystems();
};
