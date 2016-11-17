#include <nan.h>
#include "PCRE2Wrapper.h"

void Init(v8::Local<v8::Object> exports) {
	PCRE2Wrapper::Init("PCRE2", exports);
	PCRE2Wrapper::Init("PCRE2JIT", exports);
}

NODE_MODULE(addon, Init)