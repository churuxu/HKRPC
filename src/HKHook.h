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

	void AddParamOffset(const String& offset);

	void MakeDefaultOffset();

	HKHook* GetHook();

	void SerResultHandler(FileterResultHandler handler);
protected:
	std::vector<String> paramtypes_;  //数据类型
	std::vector<String> paramexps_;  //数据寄存器表达式 例如esp+4
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



