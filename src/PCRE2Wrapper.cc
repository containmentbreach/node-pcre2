#include "PCRE2Wrapper.h"

Nan::Persistent<v8::Function> PCRE2Wrapper::constructor;

PCRE2Wrapper::PCRE2Wrapper() {
	lastIndex = 0;
	global = false;
}

PCRE2Wrapper::~PCRE2Wrapper() {
}

void PCRE2Wrapper::Init(std::string name, v8::Local<v8::Object> exports) {
	Nan::HandleScope scope;

	v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
	tpl->SetClassName(Nan::New(name).ToLocalChecked());
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	
	Nan::SetPrototypeMethod(tpl, "test", PCRE2Wrapper::Test);
	Nan::SetPrototypeMethod(tpl, "exec", PCRE2Wrapper::Exec);
	Nan::SetPrototypeMethod(tpl, "match", PCRE2Wrapper::Match);
	Nan::SetPrototypeMethod(tpl, "replace", PCRE2Wrapper::Replace);
	
	Nan::SetPrototypeMethod(tpl, "toString", PCRE2Wrapper::ToString);
	
	v8::Local<v8::ObjectTemplate> proto = tpl->PrototypeTemplate();
	Nan::SetAccessor(proto, Nan::New("source").ToLocalChecked(), PCRE2Wrapper::PropertyGetter);
	Nan::SetAccessor(proto, Nan::New("flags").ToLocalChecked(), PCRE2Wrapper::PropertyGetter);
	Nan::SetAccessor(proto, Nan::New("lastIndex").ToLocalChecked(), PCRE2Wrapper::PropertyGetter, PCRE2Wrapper::PropertySetter);
	Nan::SetAccessor(proto, Nan::New("global").ToLocalChecked(), PCRE2Wrapper::PropertyGetter);
	Nan::SetAccessor(proto, Nan::New("ignoreCase").ToLocalChecked(), PCRE2Wrapper::PropertyGetter);
	Nan::SetAccessor(proto, Nan::New("multiline").ToLocalChecked(), PCRE2Wrapper::PropertyGetter);
	Nan::SetAccessor(proto, Nan::New("sticky").ToLocalChecked(), PCRE2Wrapper::PropertyGetter);

	constructor.Reset(tpl->GetFunction());
	exports->Set(Nan::New(name).ToLocalChecked(), tpl->GetFunction());
}

void PCRE2Wrapper::New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	if ( !info.IsConstructCall() ) {
		Nan::ThrowError("Use `new` to create instances of this object.");
		return;
	}
	
	PCRE2Wrapper* obj = new PCRE2Wrapper();
	obj->Wrap(info.This());
	
	std::string pattern;
	std::string flags;
	
	if ( info[0]->IsString() ) {
		pattern = std::string(*v8::String::Utf8Value(info[0]->ToString()));
		flags = std::string(info[1]->IsUndefined() ? "" : *v8::String::Utf8Value(info[1]->ToString()));
	}
	else if ( info[0]->IsRegExp() ) {
		v8::Local<v8::RegExp> v8Regexp = info[0].As<v8::RegExp>();
		v8::Local<v8::String> v8Source = v8Regexp->GetSource();
		v8::RegExp::Flags v8Flags = v8Regexp->GetFlags();
		
		pattern = std::string(*v8::String::Utf8Value(v8Source));
		
		if ( bool(v8Flags & v8::RegExp::kIgnoreCase) ) flags += "i";
		if ( bool(v8Flags & v8::RegExp::kMultiline) ) flags += "m";
		if ( bool(v8Flags & v8::RegExp::kGlobal) ) flags += "g";
	}

	if( strchr(flags.c_str(), 'g') ) obj->global = true;
	
	std::string constructorName(*v8::String::Utf8Value(info.This()->GetConstructorName()));
	
	if( constructorName == "PCRE2JIT" ) {
		flags += "S";
	}
	
	obj->re.setPattern(pattern.c_str());
	obj->re.addModifier(flags.c_str());
	obj->re.compile();
	
	if ( !obj->re ) {	
		Nan::ThrowError(obj->re.getErrorMessage().c_str());
		return;
	}

	info.GetReturnValue().Set(info.This());
}

void PCRE2Wrapper::Test(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	PCRE2Wrapper *obj = ObjectWrap::Unwrap<PCRE2Wrapper>(info.This());
	
	const char* subject = *(v8::String::Utf8Value(info[0]));
	jp::RegexMatch& matcher = obj->re.getMatchObject();
	
	matcher.setSubject(subject);
	matcher.setModifier("");
	
	matcher.setNumberedSubstringVector(NULL);
	matcher.setNamedSubstringVector(NULL);
	matcher.setNameToNumberMapVector(NULL);
	matcher.setBoundsVector(NULL);
	
	matcher.setStartOffset(0);
	
	bool result = (matcher.match() > 0);
	
	info.GetReturnValue().Set(result ? Nan::True() : Nan::False());
}

void PCRE2Wrapper::Exec(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	PCRE2Wrapper *obj = ObjectWrap::Unwrap<PCRE2Wrapper>(info.This());
	
	const char* subject = *(v8::String::Utf8Value(info[0]));
	jp::RegexMatch& matcher = obj->re.getMatchObject();
	
	matcher.setSubject(subject);
	matcher.setModifier("");
	
	matcher.setNumberedSubstringVector(&obj->vec_num);
	matcher.setNamedSubstringVector(&obj->vec_nas);
	matcher.setNameToNumberMapVector(&obj->vec_ntn);
	matcher.setBoundsVector(NULL);
	
	if ( obj->global ) matcher.setStartOffset(obj->lastIndex);
	
	size_t result_count = matcher.match();
	
	if ( result_count == 0 ) {
		obj->lastIndex = 0;
	
		info.GetReturnValue().Set(Nan::Null());
	}
	else {
		size_t finish_offset = matcher.getEndOffset();
	
		if ( obj->global ) obj->lastIndex = finish_offset;
	
		v8::Local<v8::Array> result = Nan::New<v8::Array>(result_count);
		
		std::string whole_match;
		
		for ( size_t i=0; i<obj->vec_num[0].size(); ++i ) {
			std::string matched_str = obj->vec_num[0][i];
			if ( i == 0 ) whole_match = matched_str;
			
			result->Set(i, Nan::New(matched_str).ToLocalChecked());
		}
				
		bool hasNames = false;
		v8::Local<v8::Object> named = Nan::New<v8::Object>();
	
		for ( auto const& ent : obj->vec_nas[0] ) {
			named->Set(Nan::New(ent.first).ToLocalChecked(), Nan::New(ent.second).ToLocalChecked());
			hasNames = true;
		}
		
		if ( hasNames ) result->Set(Nan::New("named").ToLocalChecked(), named);

		int indexParam = finish_offset - whole_match.length();
		
		result->Set(Nan::New("index").ToLocalChecked(), Nan::New((int32_t)indexParam));
		result->Set(Nan::New("input").ToLocalChecked(), Nan::New(subject).ToLocalChecked());
		
		info.GetReturnValue().Set(result);
	}
}

void PCRE2Wrapper::Match(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	PCRE2Wrapper *obj = ObjectWrap::Unwrap<PCRE2Wrapper>(info.This());
	
	if ( !obj->global ) {
		PCRE2Wrapper::Exec(info);
		return;
	}
	
	const char* subject = *(v8::String::Utf8Value(info[0]));
	jp::RegexMatch& matcher = obj->re.getMatchObject();
	
	matcher.setSubject(subject);
	matcher.setModifier("g");
	
	matcher.setNumberedSubstringVector(&obj->vec_num);
	matcher.setNamedSubstringVector(NULL);
	matcher.setNameToNumberMapVector(NULL);
	matcher.setBoundsVector(NULL);
	
	matcher.setStartOffset(0);
	
	size_t result_count = matcher.match();
	
	if ( result_count == 0 ) {
		obj->lastIndex = 0;
	
		info.GetReturnValue().Set(Nan::Null());
	}
	else {
		v8::Local<v8::Array> result = Nan::New<v8::Array>(result_count);
		
		for ( size_t i=0; i<obj->vec_num.size(); ++i ) {
			result->Set(i, Nan::New(obj->vec_num[i][0]).ToLocalChecked());
		}
		
		info.GetReturnValue().Set(result);
	}
}

void PCRE2Wrapper::Replace(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	PCRE2Wrapper *obj = ObjectWrap::Unwrap<PCRE2Wrapper>(info.This());
	
	std::string subject(*(v8::String::Utf8Value(info[0])));
	bool withCallback = info[1]->IsFunction();
	
	if ( !withCallback ) {
		const char* replacement = *(v8::String::Utf8Value(info[1]));
		
		std::string result = obj->re.replace(subject, replacement, obj->global ? "g" : "");
		info.GetReturnValue().Set(Nan::New(result).ToLocalChecked());
		return;
	}

	v8::Local<v8::Function> callback = info[1].As<v8::Function>();

	jp::RegexMatch& matcher = obj->re.getMatchObject();
	
	matcher.setSubject(subject);
	matcher.setModifier(obj->global ? "g" : "");
	
	matcher.setNumberedSubstringVector(NULL);
	matcher.setNamedSubstringVector(&obj->vec_nas);
	matcher.setNameToNumberMapVector(NULL);
	matcher.setBoundsVector(&obj->vec_mbs);
	
	matcher.setStartOffset(0);
	
	size_t result_count = matcher.match();
	
	if ( result_count == 0 ) {
		info.GetReturnValue().Set(Nan::New(subject).ToLocalChecked());
		return;
	}

	std::string replaced_subject = "";
	
	size_t lastEnd = 0;
	
	for ( size_t i=0; i<obj->vec_mbs.size(); ++i ) {
		bool hasNames = (obj->vec_nas[i].size() > 0); //vec_nas[i] !!! wrong design.. but quicker than edit jpcre to my needs, shoudln't be buggy.
	
		const unsigned argc = obj->vec_mbs[i].size()/2 + 2 + (hasNames ? 1 : 0);
		v8::Local<v8::Value> *argv = new v8::Local<v8::Value>[argc];
	
		for ( size_t j=0; j<obj->vec_mbs[i].size()/2; j++ ) {
			size_t jj = j*2;
		
			size_t sublen = obj->vec_mbs[i][jj+1] - obj->vec_mbs[i][jj];
			
			if ( j == 0 ) {
				replaced_subject += subject.substr(lastEnd, obj->vec_mbs[i][jj]-lastEnd);
				lastEnd = obj->vec_mbs[i][jj+1];
			}
			
			argv[j] = Nan::New(subject.substr(obj->vec_mbs[i][jj], sublen)).ToLocalChecked();
		}
		
		argv[argc-(2 + (hasNames ? 1:0))] = Nan::New((uint32_t)obj->vec_mbs[i][0]);
		argv[argc-(1 + (hasNames ? 1:0))] = Nan::New(subject).ToLocalChecked();
		
		if ( hasNames ) {
			v8::Local<v8::Object> named = Nan::New<v8::Object>();
	
			for ( auto const& ent : obj->vec_nas[i] ) {
				named->Set(Nan::New(ent.first).ToLocalChecked(), Nan::New(ent.second).ToLocalChecked());
			}
		
			argv[argc-1] = named;
		}
						
		v8::Local<v8::Value> returned = callback->Call(Nan::GetCurrentContext()->Global(), argc, argv);
		replaced_subject += *(v8::String::Utf8Value(returned));
				
		delete[] argv;
	}
	
	replaced_subject += subject.substr(lastEnd, std::string::npos);
	
	info.GetReturnValue().Set(Nan::New(replaced_subject).ToLocalChecked());
}

void PCRE2Wrapper::ToString(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	PCRE2Wrapper *obj = ObjectWrap::Unwrap<PCRE2Wrapper>(info.This());
	
	std::string flags = obj->re.getModifier();
	if ( obj->global ) flags += "g";
	
	std::string result = "/" + obj->re.getPattern() + "/" + flags;
	
	info.GetReturnValue().Set(Nan::New(result).ToLocalChecked());
}

void PCRE2Wrapper::HasModifier(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	PCRE2Wrapper *obj = ObjectWrap::Unwrap<PCRE2Wrapper>(info.This());
	
	const char* subject = *(v8::String::Utf8Value(info[0]));
	
	bool result = obj->hasModifier(subject);
	
	info.GetReturnValue().Set(result ? Nan::True() : Nan::False());
}

void PCRE2Wrapper::PropertyGetter(v8::Local<v8::String> property, const Nan::PropertyCallbackInfo<v8::Value>& info) {
	PCRE2Wrapper *obj = ObjectWrap::Unwrap<PCRE2Wrapper>(info.This());
	
	std::string name(*(v8::String::Utf8Value(property)));
	
	if ( name == "source" ) {
		info.GetReturnValue().Set(Nan::New(obj->re.getPattern()).ToLocalChecked());
	}
	else if ( name == "flags" ) {
		std::string modifier = obj->re.getModifier();
		if ( obj->global ) modifier += "g";
		
		info.GetReturnValue().Set(Nan::New(modifier).ToLocalChecked());
	}
	else if ( name == "lastIndex" ) {
		info.GetReturnValue().Set(Nan::New<v8::Integer>((uint32_t)obj->lastIndex));
	}
	else {
		bool result = false;
		
		if ( name == "global" ) result = obj->global;
		else if ( name == "sticky" ) result = false;
		else if ( name == "ignoreCase" ) result = obj->hasModifier("i");
		else if ( name == "multiline" ) result = obj->hasModifier("m");
		
		info.GetReturnValue().Set(Nan::New(result));
	}
}

NAN_SETTER(PCRE2Wrapper::PropertySetter) {
	PCRE2Wrapper *obj = ObjectWrap::Unwrap<PCRE2Wrapper>(info.This());
	
	std::string name(*(v8::String::Utf8Value(property)));
	
	if ( name == "lastIndex" ) {
		int32_t val = value->Int32Value();
		obj->lastIndex = val < 0 ? 0 : val;
		info.GetReturnValue().Set(Nan::New<v8::Integer>((uint32_t)obj->lastIndex));
	}
}

bool PCRE2Wrapper::hasModifier(std::string mods) {
	std::string modifier = re.getModifier();
	
	for ( size_t i=0; i<mods.length(); i++ ) {
		char mod = mods[i];
		
		if ( mod == 'g' ) {
			if ( !global ) return false;
		}
		else if ( modifier.find(mod) == std::string::npos ) return false;
	}
	
	return true;
}