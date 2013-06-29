// sq.net.h

#pragma once

#include "squirrel.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <tchar.h>
#include <vector>

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

	void Print(const SQChar* str)
	{
		System::Console::WriteLine(gcnew System::String(str));
	}

#pragma managed(push, off)
	
	void PrintFunc(HSQUIRRELVM vm, const SQChar* fmt, ...)
	{
		static const int MAX_OUTPUT_STRING = 65536;
		std::vector<SQChar> buf(512);
		while(buf.size() < MAX_OUTPUT_STRING)
		{
			va_list l;
 			va_start(l, fmt);
			if(_vsntprintf_s(&buf[0], buf.size(), buf.size() - 1, fmt, l) > 0)
			{
				Print(&buf[0]);
				break;
			}
			va_end(l);
			buf.resize(buf.size() * 2);
		}
	}

#pragma managed(pop)

	void CompileErrorHandler(HSQUIRRELVM,const SQChar * desc, const SQChar *source, SQInteger line, SQInteger column)
	{
		auto str = System::String::Format(_SC("[COMPILING ERROR] : {0} in '{1}', line {2}, col {3}"), 
			gcnew System::String(desc),
			gcnew System::String(source),
			line,
			column);
		Console::ForegroundColor = System::ConsoleColor::Red;
		Console::WriteLine(str);
		Console::ResetColor();
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

	public ref class SQVM
	{
		static const int DEFAULT_INIT_STACK_SIZE = 1024;
		HSQUIRRELVM m_VM;

	public:
		SQVM()
			: m_VM(nullptr)
		{
			m_VM = sq_open(DEFAULT_INIT_STACK_SIZE);
			if(!m_VM)
				throw gcnew System::InvalidOperationException("sq_open returns nullptr");
			sq_setprintfunc(m_VM, &PrintFunc, &PrintFunc);
			sq_setcompilererrorhandler(m_VM, &CompileErrorHandler);
		}
		SQVM(int initStackSize)
			: m_VM(nullptr)
		{
			m_VM = sq_open(initStackSize);
			if(!m_VM)
				throw gcnew System::InvalidOperationException("sq_open returns nullptr");
			sq_setprintfunc(m_VM, &PrintFunc, &PrintFunc);
			sq_setcompilererrorhandler(m_VM, &CompileErrorHandler);
		}
		~SQVM()
		{
			this->!SQVM();
		}

	protected:
		!SQVM()
		{
			if(m_VM)
			{
				sq_close(m_VM);
				m_VM = nullptr;
			}
		}

	public:

		bool compilestatic( array<String^>^ arrSrc, String^ projName, bool bRaiseError )
		{
			std::vector<std::basic_string<SQChar>> vecSrc(arrSrc->Length);
			std::vector<const SQChar*> vecSrcPtr(arrSrc->Length);
			for(int i=0; i<arrSrc->Length; i++)
			{
				StringToNative ntstr(arrSrc[i]);
				vecSrc[i] = std::basic_string<SQChar>(ntstr.Value());
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
