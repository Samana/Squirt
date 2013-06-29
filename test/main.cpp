#include "squirrel.h"
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <stdarg.h>
#include <memory>
#include <vector>

void PrintFunc(HSQUIRRELVM vm, const SQChar* fmt, ...)
{
	va_list l;
 	va_start(l, fmt);
	_vtcprintf(fmt, l);
	scprintf(_SC("\n"));
	va_end(l);
}

void CompileErrorHandler(HSQUIRRELVM,const SQChar * desc, const SQChar *source, SQInteger line, SQInteger column)
{
	scprintf(_SC("[COMPILE ERROR] : '%s', in '%s', line %d, col %d\n"), desc, source, line, column);
}

SQInteger OpenSrcFunc(SQUserPointer srcTextOut, SQUserPointer srcLenOut, SQUserPointer srcNameOut, SQInteger index, SQUserPointer up)
{
	SQChar** ppSrc = (SQChar**)up;
	
	FILE* fp = _tfopen(ppSrc[index], _SC("r"));
	if(!fp)
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
	mbstowcs(source, buf, len + 1);

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

//FIXME: Add Close Source Func

int main()
{
	HSQUIRRELVM vm = sq_open(1024);
	sq_setprintfunc(vm, PrintFunc, PrintFunc);
	sq_setcompilererrorhandler(vm, CompileErrorHandler);
	//FILE* fp = fopen("../../../test/test.nut", "r");
	//if(!fp)
	//{
	//	goto _CLEANUP;
	//}
	//fseek(fp, 0, SEEK_END);
	//long len = ftell(fp);
	//fseek(fp, 0, SEEK_SET);
	//char* buf = new char[len + 1];
	//memset(buf, 0, len + 1);
	//fread(buf, 1, len, fp);
	//fclose(fp);
	//SQChar* source = new SQChar[len + 2];
	//memset(source, 0, sizeof(SQChar) * (len + 2));
	//mbstowcs(source, buf, len + 1);
	//delete[] buf;
	//if(SQ_FAILED(sq_compilebuffer(vm, source, _tcslen(source), _SC("test.nut"), SQTrue)))
	//{
	//	delete[] source;
	//	goto _CLEANUP;
	//}
	//delete[] source;

	SQChar* apSrc[] = {
		_SC("../../../test/test.nut"),
	};
	sq_compilestatic(vm, OpenSrcFunc, CloseSrcFunc, 1, apSrc, _SC("testproj"), SQTrue);

	sq_pushroottable(vm);
	if(SQ_SUCCEEDED(sq_call(vm, 1, SQFalse, SQTrue)))
	{
		printf("Run script succeeded.");
	}

_CLEANUP:
	
	sq_close(vm);
	system("pause");
	return 0;
}
