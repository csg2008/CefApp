#ifndef __V8HANDLER_H__
#define __V8HANDLER_H__

#pragma once
#include <map>
#include <functional>
#include "libcef/include/cef_v8.h"
#include "libcef/include/cef_app.h"
#include "libcef/include/cef_base.h"
#include "misc.h"

typedef std::function<bool(CefString, CefRefPtr<CefListValue>)> CallBackJsCall;
typedef std::function<CefRefPtr<CefValue> (CefRefPtr<CefListValue>)> CustomFunction;

class MyV8Accessor : public CefV8Accessor {
public:
	MyV8Accessor() {}

	virtual bool Get(const CefString& name,
		const CefRefPtr<CefV8Value> object,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) override {
		if (name == "myval") {
			// Return the value.
			retval = CefV8Value::CreateString(myval_);
			return true;
		}

		// Value does not exist.
		return false;
	}

	virtual bool Set(const CefString& name,const CefRefPtr<CefV8Value> object,const CefRefPtr<CefV8Value> value,CefString& exception) override {
		if (name == "myval") {
			if (value->IsString()) {
				// Store the value.
				myval_ = value->GetStringValue();
			} else {
				// Throw an exception.
				exception = "Invalid value type";
			}
			return true;
		}

		// Value does not exist.
		return false;
	}

	// Variable used for storing the value.
	CefString myval_;

	// Provide the reference counting implementation for this class.
	IMPLEMENT_REFCOUNTING(MyV8Accessor);
};

class V8HandlerEx : public CefV8Handler {
public:
	V8HandlerEx(CefRefPtr<CefApp> app, const std::string& path, const bool debug) : isDebug(debug), app_(app), appPath(path), dataPath(path + "\\data")  { }
	std::string getDateFromMacro(char const *date, char const *time);

public:
	virtual bool Execute(const CefString& name,
	CefRefPtr<CefV8Value> object,
	const CefV8ValueList& arguments,
	CefRefPtr<CefV8Value>& retval,
	CefString& exception) override;

private:
	bool isDebug;
    std::string appPath;
	std::string dataPath;
	// Map of message callbacks.
	typedef std::map<std::pair<std::string, int>, std::pair<CefRefPtr<CefV8Context>, CefRefPtr<CefV8Value> > >CallbackMap;
	CallbackMap callback_map_;

	CefRefPtr<CefApp> app_;

	std::map<CefString, CustomFunction> function_map_;
public:
	IMPLEMENT_REFCOUNTING(V8HandlerEx);
};

#endif