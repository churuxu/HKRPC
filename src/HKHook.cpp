#include "HKHook.h"
#include "Utils.h"



static void hook_function_exec(void* ctx, void* udata) {
	HKHook* pThis = (HKHook*)udata;
	pThis->Process(ctx);
}



HKHook::HKHook() {
	target_ = NULL;
	hookctx_ = NULL;	
}
HKHook::~HKHook() {

}


bool HKHook::Attach(void* target) {
	if (target_ || hookctx_)return false;
	target_ = target;
	int iret = Utils::DetourAttachAddress(&target_, hook_function_exec, &hookctx_, this);
	return iret == 0;
}

bool HKHook::Detach() {
	if (!target_ || !hookctx_)return false;
	Utils::DetourDetachAddress(&target_, hookctx_);
	target_ = NULL;
	hookctx_ = NULL;
	return true;
}


void HKHook::Process(void* ctx) {
	std::lock_guard<std::mutex>  lock(mtx_);
	for (auto filter : filters_) {
		filter->Process(ctx);
	}
}

void HKHook::AddFilter(HKHookFilter* filter) {
	std::lock_guard<std::mutex>  lock(mtx_);
	filter->hook_ = this;
	filters_.insert(filter);
}

void  HKHook::RemoveFilter(HKHookFilter* filter) {
	std::lock_guard<std::mutex>  lock(mtx_);
	filters_.erase(filter);
}




HKHookFilter::HKHookFilter() {
	hook_= NULL;
}
HKHookFilter::~HKHookFilter() {
	if (hook_)hook_->RemoveFilter(this);
}

void HKHookFilter::Process(void* ctx) {
	String jsonstr;
	jsonstr += "[";

	//get stack values
	bool first = true;
	for (size_t i = 0; i < paramtypes_.size();i++) {
		if (i>0) jsonstr += ",";	
		intptr_t val = Utils::GetMemoryValue(ctx, paramexps_[i].c_str());
		jsonstr += Utils::ToJsonFormat(val, paramtypes_[i]);
	}
	jsonstr += "]";
	printf("%s\n",jsonstr.c_str());

	if (onresult_)onresult_(jsonstr);
}


void HKHookFilter::AddParamType(const String& type) {
	paramtypes_.push_back(type);
}

void HKHookFilter::AddParamOffset(const String& offset) {
	paramexps_.push_back(offset);
}

void HKHookFilter::MakeDefaultOffset() {
	char buf[32] = {'[','e','s','p','+',0 };
	for (size_t i = 0; i < paramtypes_.size(); i++) {
		int offset = ((i+1) * 4);
		_itoa(offset, &(buf[5]), 10);
		int len = strlen(&(buf[5]));
		buf[len + 5] = ']';
		buf[len + 6] = 0;
		paramexps_.push_back(buf);
	}
}



HKHook* HKHookFilter::GetHook() {
	return hook_;
}

void HKHookFilter::SerResultHandler(FileterResultHandler handler) {
	onresult_ = handler;
}



