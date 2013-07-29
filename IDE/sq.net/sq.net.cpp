// This is the main DLL file.

#include "stdafx.h"

#include "sq.net.h"

static std::map<HSQUIRRELVM, gcroot<sqnet::SQVM^>*> m_GlobalVMMap;

namespace sqnet
{
	int Print(const SQChar* str)
	{
		String^ tmp = gcnew System::String(str);
		Cons::ForegroundColor = System::Windows::Media::Colors::Green;
		System::Console::Write(tmp);
		return tmp->Length;
	}

#pragma managed(push, off)
	
	int StaticPrintFunc(const SQChar* fmt, ...)
	{
		static const int MAX_OUTPUT_STRING = 65536;
		std::vector<SQChar> buf(512);
		while(buf.size() < MAX_OUTPUT_STRING)
		{
			va_list l;
 			va_start(l, fmt);
			if(_vsntprintf_s(&buf[0], buf.size(), buf.size() - 1, fmt, l) > 0)
			{
				return Print(&buf[0]);
				break;
			}
			va_end(l);
			buf.resize(buf.size() * 2);
		}
		return -1;
	}

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

	void DebugHookFuncNative(HSQUIRRELVM v, SQInteger type, const SQChar * sourcename, SQInteger line, const SQChar * funcname)
	{
		auto itor = m_GlobalVMMap.find(v);
		if(itor != m_GlobalVMMap.end())
		{
			//(*(itor->second))->OnDebugHook(type, sourcename, line, funcname);
		}
	}

#pragma managed(pop)

	SQVM::SQVM()
		: m_VM(nullptr)
		, m_bDebugInfoEnabled(false)
	{
		m_VM = sq_open(DEFAULT_INIT_STACK_SIZE);
		if(!m_VM)
			throw gcnew System::InvalidOperationException("sq_open returns nullptr");
		sq_setprintfunc(m_VM, &PrintFunc, &PrintFunc);
		sq_setcompilererrorhandler(m_VM, &CompileErrorHandler);

		m_DebugHookDelegate = gcnew DebugHookFuncNative(this, &SQVM::OnDebugHook);
		System::IntPtr pFunc = System::Runtime::InteropServices::Marshal::GetFunctionPointerForDelegate(m_DebugHookDelegate);
		sq_setnativedebughook(m_VM, (SQDEBUGHOOK)pFunc.ToPointer());
	}
	
	SQVM::SQVM(int initStackSize)
		: m_VM(nullptr)
	{
		m_VM = sq_open(initStackSize);
		if(!m_VM)
			throw gcnew System::InvalidOperationException("sq_open returns nullptr");
		sq_setprintfunc(m_VM, &PrintFunc, &PrintFunc);
		sq_setcompilererrorhandler(m_VM, &CompileErrorHandler);
		m_DebugHookDelegate = gcnew DebugHookFuncNative(this, &SQVM::OnDebugHook);
		System::IntPtr pFunc = System::Runtime::InteropServices::Marshal::GetFunctionPointerForDelegate(m_DebugHookDelegate);
		sq_setnativedebughook(m_VM, (SQDEBUGHOOK)pFunc.ToPointer());
	}

	SQVM::~SQVM()
	{
		this->!SQVM();
	}

	SQVM::!SQVM()
	{
		if(m_VM)
		{
			sq_close(m_VM);
			m_VM = nullptr;
		}
	}

	void SQVM::OnDebugHook(HSQUIRRELVM v, SQInteger type, const SQChar * sourcename, SQInteger line, const SQChar * funcname)
	{
		if(v != m_VM)
		{
			throw gcnew System::InvalidOperationException("Debug hook called on a wrong object. How can this happen!");
		}

		DebugHookType eType;
		switch(type)
		{
		case _SC('l'): eType = DebugHookType::Line; break;
		case _SC('c'): eType = DebugHookType::Call; break;
		case _SC('r'): eType = DebugHookType::Return; break;
		};

		OnDebugCallback(
			eType, 
			sourcename ? gcnew String(sourcename) : nullptr,
			funcname ? gcnew String(funcname) : nullptr,
			line);
	}
}