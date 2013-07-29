// sq.net.h

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <tchar.h>
#include <vector>
#include <map>
#include <gcroot.h>

#include "squirrel.h"
#include "../squirrel/squirt.h"

using namespace System;

namespace sqnet {

	class StringToNative
	{
		System::IntPtr hGlobal;
	public:
		StringToNative(System::String^ str)
		{
			hGlobal = System::Runtime::InteropServices::Marshal::StringToHGlobalUni(str);
		}
		const SQChar* Value()
		{
			return (SQChar*)hGlobal.ToPointer();
		}
		~StringToNative()
		{
			System::Runtime::InteropServices::Marshal::FreeHGlobal(hGlobal);
		}
	};

	public ref class Cons
	{
	public:
		property static System::Windows::Media::Color ForegroundColor;
		property static System::Windows::Media::Color BackgroundColor;

		static Cons()
		{
			ForegroundColor = System::Windows::Media::Colors::Gray;
			BackgroundColor = System::Windows::Media::Colors::Black;
		}
	};

	int Print(const SQChar* str);

#pragma managed(push, off)
	
	int StaticPrintFunc(const SQChar* fmt, ...);
	void PrintFunc(HSQUIRRELVM vm, const SQChar* fmt, ...);
	void DebugHookFuncNative(HSQUIRRELVM v, SQInteger type, const SQChar * sourcename, SQInteger line, const SQChar * funcname);

#pragma managed(pop)

	void CompileErrorHandler(HSQUIRRELVM,const SQChar * desc, const SQChar *source, SQInteger line, SQInteger column)
	{
		auto str = System::String::Format(_SC("[COMPILING ERROR] : {0} in '{1}', line {2}, col {3}"), 
			gcnew System::String(desc),
			gcnew System::String(source),
			line,
			column);
		Cons::ForegroundColor = System::Windows::Media::Colors::Red;
		Console::WriteLine(str);
	}

	SQInteger OpenSrcFunc(SQUserPointer srcTextOut, SQUserPointer srcLenOut, SQUserPointer srcNameOut, SQInteger index, SQUserPointer up)
	{
		SQChar** ppSrc = (SQChar**)up;
	
		FILE* fp = nullptr;
		if(0 != _tfopen_s(&fp, ppSrc[index], _SC("r")))
		{
			return SQ_ERROR;
		}
		fseek(fp, 0, SEEK_END);
		long len = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		char* buf = new char[len + 1];
		memset(buf, 0, len + 1);
		fread(buf, 1, len, fp);
		fclose(fp);
		SQChar* source = new SQChar[len + 2];
		memset(source, 0, sizeof(SQChar) * (len + 2));
		size_t nConv;
		mbstowcs_s(&nConv, source, len + 2, buf, len + 1);

		*(SQChar**)srcTextOut = source;
		*(SQInteger*)srcLenOut = len;
		*(SQChar**)srcNameOut = ppSrc[index];

		return SQ_OK;
	}

	void CloseSrcFunc(SQUserPointer srcText, SQUserPointer srcLen, SQInteger index, SQUserPointer up)
	{
		SQChar* source = *(SQChar**)srcText;
		if(source)
		{
			delete[] source;
			*(SQChar**)srcText = NULL;
		}
	}

	public enum class ERuntimeType
	{
		Jit_llvm,
		Interpreter,
	};

	public enum class DebugHookType
	{
		Line,
		Call,
		Return,
	};

	public ref class SQVM
	{
		static const int DEFAULT_INIT_STACK_SIZE = 1024;
		HSQUIRRELVM m_VM;
		bool m_bDebugInfoEnabled;
		System::Delegate^ m_DebugHookDelegate;

		[System::Runtime::InteropServices::UnmanagedFunctionPointerAttribute(System::Runtime::InteropServices::CallingConvention::Cdecl)]
		delegate void DebugHookFuncNative(HSQUIRRELVM v, SQInteger type, const SQChar * sourcename, SQInteger line, const SQChar * funcname);

	public:
		delegate void DebugHookFunc(DebugHookType type, String^ sourceName, String^ funcName, int line);
		event DebugHookFunc^ OnDebugCallback;

	public:
		static SQVM()
		{
			sq_setstaticprintffunc(&StaticPrintFunc);
		}

		SQVM();
		SQVM(int initStackSize);
		~SQVM();

	protected:
		!SQVM();

	internal:

		void OnDebugHook(HSQUIRRELVM v, SQInteger type, const SQChar * sourcename, SQInteger line, const SQChar * funcname);

	public:

		property ERuntimeType RuntimeType
		{
			ERuntimeType get() { return sq_getjitenabled(m_VM) ? ERuntimeType::Jit_llvm : ERuntimeType::Interpreter; }
			void set(ERuntimeType value) { sq_enablejit(m_VM, value == ERuntimeType::Jit_llvm ? SQTrue : SQFalse); }
		}

		property bool DebugInfoEnabled
		{
			bool get() { return m_bDebugInfoEnabled; }
			void set(bool value) { m_bDebugInfoEnabled = value; sq_enabledebuginfo(m_VM, m_bDebugInfoEnabled ? SQTrue : SQFalse); }
		}

		bool compilestatic( array<String^>^ arrSrc, String^ projName, bool bRaiseError )
		{
			std::vector<std::scstring> vecSrc(arrSrc->Length);
			std::vector<const SQChar*> vecSrcPtr(arrSrc->Length);
			for(int i=0; i<arrSrc->Length; i++)
			{
				StringToNative ntstr(arrSrc[i]);
				vecSrc[i] = std::scstring(ntstr.Value());
				vecSrcPtr[i] = vecSrc[i].c_str();
			}
			StringToNative ntProjName(projName);
			return SQ_SUCCEEDED(sq_compilestatic(m_VM, OpenSrcFunc, CloseSrcFunc, arrSrc->Length, &vecSrcPtr[0], ntProjName.Value(), bRaiseError));
			
		}

		void pushroottable()
		{
			sq_pushroottable(m_VM);
		}

		bool call(int params, bool retval, bool raiseerror)
		{
			return SQ_SUCCEEDED(sq_call(m_VM, params, retval, raiseerror));
		}
	};
}
