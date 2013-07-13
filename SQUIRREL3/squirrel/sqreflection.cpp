#include "sqpcheader.h"
#include "sqtable.h"
#include "sqreflection.h"
#include "sqstaticanalyser.h"
#include "sqvm.h"


const SQChar* SQTypeDesc::GetMetaTypeLiterals(SQMetaType metaType)
{
	static const SQChar* literals[] =
	{
		_SC("dynamic"),//SQ_TYPE_DYNAMIC = 0,
		_SC("void"),	//SQ_TYPE_VOID,
		_SC("bool"),//SQ_TYPE_BOOLEAN,
		_SC("char"),//SQ_TYPE_CHAR8,
		_SC("byte"),//SQ_TYPE_BYTE8,
		_SC("int16"),//SQ_TYPE_INT16,
		_SC("uint16"),//SQ_TYPE_UINT16,
		_SC("int"),//SQ_TYPE_INT32,
		_SC("uint"),//SQ_TYPE_UINT32,
		_SC("int64"),//SQ_TYPE_INT64,
		_SC("uint64"),//SQ_TYPE_UINT64,
		_SC("float"),//SQ_TYPE_FLOAT32,
		_SC("double"),//SQ_TYPE_DOUBLE64,
		_SC("string"),//SQ_TYPE_STRING,
		_SC("object"),//SQ_TYPE_OBJECT,
		_SC("function"),//SQ_TYPE_CLOSURE,
		_SC("nativefunc"),//SQ_TYPE_NATIVECLOSURE,
		_SC("nativeptr"),//SQ_TYPE_NATIVEPTR,
		_SC("int_any"),
		_SC("float_any"),
		NULL,
	};

	return literals[metaType];
}

const SQChar* SQTypeDesc::ToString(std::scstring& buf) const
{
	if(_resolved)
	{
		return _string(_resolved->m_Name)->_val;
	}
	else
	{
		if(sq_isstring(_unresolved))
		{
			return _string(_unresolved)->_val;
		}
		else if(sq_isinteger(_unresolved))
		{
			SQMetaType metaType = (SQMetaType)_integer(_unresolved);
			if(metaType == SQ_TYPE_CLOSURE && _parentfunc != NULL)
			{
				_parentfunc->GetSignatureString(buf);
				return buf.c_str();
			}
			else
			{
				return GetMetaTypeLiterals(metaType);
			}
		}
		else
		{
			return _SC("");
		}
	}
}

void SQType::InitTypes(HSQUIRRELVM vm)
{
	SQType* pType = NULL;

	for(SQInteger i=SQ_TYPE_DYNAMIC; i<=SQ_TYPE_STRING; i++)
	{
		sq_new(pType, SQType);
		pType->m_eMetaType = (SQMetaType)i;
		pType->m_Name = SQString::Create(_ss(vm), SQTypeDesc::GetMetaTypeLiterals((SQMetaType)i));

		_table(vm->_roottable)->NewSlot(pType->m_Name, pType);
	}
}