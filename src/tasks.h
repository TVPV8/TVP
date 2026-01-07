// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include <condition_variable>
#include "thread_holder_base.h"
#include "enums.h"

#ifdef STATS_ENABLED
#define createTask(function) createTaskWithStats(function, #function, __FUNCTION__)
#define createTimedTask(delay, function) createTaskWithStats(delay, function, #function, __FUNCTION__)
#define createSchedulerTask(delay, function) createSchedulerTaskWithStats(delay, function, #function, __FUNCTION__)
#define addGameTask(function, ...) addGameTaskWithStats(function, #function, __FUNCTION__, __VA_ARGS__)
#define addGameTaskTimed(delay, function, ...) addGameTaskTimedWithStats(delay, function, #function, __FUNCTION__, __VA_ARGS__)
#else
#define createTask(function) createTaskWithStats(function, "", "")
#define createTimedTask(delay, function) createTaskWithStats(delay, function, "", "")
#define createSchedulerTask(delay, function) createSchedulerTaskWithStats(delay, function, "", "")
#define addGameTask(function, ...) addGameTaskWithStats(function, "", "", __VA_ARGS__)
#define addGameTaskTimed(delay, function, ...) addGameTaskTimedWithStats(delay, function, "", "", __VA_ARGS__)
#endif

using TaskFunc = std::function<void(void)>;
const int DISPATCHER_TASK_EXPIRATION = 2000;
const auto SYSTEM_TIME_ZERO = std::chrono::system_clock::time_point(std::chrono::milliseconds(0));

class Task
{
	public:
		// DO NOT allocate this class on the stack
		explicit Task(TaskFunc&& f, const std::string& _description, const std::string& _extraDescription) :
		func(std::move(f)), description(_description), extraDescription(_extraDescription) {}
		Task(uint32_t ms, TaskFunc&& f, const std::string& _description, const std::string& _extraDescription) :
		expiration(std::chrono::system_clock::now() + std::chrono::milliseconds(ms)), func(std::move(f)), description(_description), extraDescription(_extraDescription) {}

		virtual ~Task() = default;
		void operator()() {
			func();
		}

		void setDontExpire() {
			expiration = SYSTEM_TIME_ZERO;
		}

		bool hasExpired() const {
			if (expiration == SYSTEM_TIME_ZERO) {
				return false;
			}
			return expiration < std::chrono::system_clock::now();
		}

		std::chrono::system_clock::time_point expiration = SYSTEM_TIME_ZERO;
		// Expiration has another meaning for scheduler tasks,
		// then it is the time the task should be added to the
		// dispatcher
		TaskFunc func;
		const std::string description;
		const std::string extraDescription;
		uint64_t executionTime = 0;
};

Task* createTaskWithStats(TaskFunc&& f, const std::string& description, const std::string& extraDescription);
Task* createTaskWithStats(uint32_t expiration, TaskFunc&& f, const std::string& description, const std::string& extraDescription);

class Dispatcher : public ThreadHolder<Dispatcher> {
	public:
		Dispatcher() : ThreadHolder() {
			static int id = 0;
			dispatcherId = id;
			id += 1;
		}
		void addTask(Task* task);

		void shutdown();

		uint64_t getDispatcherCycle() const {
			return dispatcherCycle;
		}

		void threadMain();

		bool beatIOSync = false;

	private:
		std::mutex taskLock;
		std::condition_variable taskSignal;

		std::vector<Task*> taskList;
		uint64_t dispatcherCycle = 0;
		int dispatcherId = 0;
};

extern Dispatcher g_dispatcher;
