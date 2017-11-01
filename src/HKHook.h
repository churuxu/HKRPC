#pragma once

#include "pch.h"


class HKHook;

class HKHookFilter {
	friend class HKHook;
public:
	HKHookFilter();
	~HKHookFilter();
	typedef std::function<void(const String&)> FileterResultHandler;

	void Process(void* ctx);
	
	void AddParamType(const String& type);

	void AddParamOffset(int offset);

	void MakeDefaultOffset();

	HKHook* GetHook();

	void SerResultHandler(FileterResultHandler handler);
protected:
	std::vector<String> paramtypes_;
	std::vector<int> paramoffsets_;
	FileterResultHandler onresult_;
	HKHook* hook_;
};


class HKHook {
public:

	HKHook();
	~HKHook();

	bool Attach(void* target);
	bool Detach();

	void Process(void* ctx);

	void AddFilter(HKHookFilter* filter);

	void RemoveFilter(HKHookFilter* filter);

protected:
	void* target_;
	void* hookctx_;
	std::mutex mtx_;
	std::set<HKHookFilter*> filters_;
};



