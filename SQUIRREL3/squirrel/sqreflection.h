#ifndef _SQREFLECTION_H_
#define _SQREFLECTION_H_

#include "squirt.h"

struct SQType;

struct SQAssembly
{
	SQObjectPtr _name;
	SQObjectPtr _types;
	bool _isloaded;

	SQObjectPtr FindType(SQString* name);
};

enum SQMetaType
{
	SQ_TYPE_DYNAMIC = 0,
	SQ_TYPE_VOID,
	SQ_TYPE_BOOLEAN,
	SQ_TYPE_CHAR8,
	SQ_TYPE_BYTE8,
	SQ_TYPE_INT16,
	SQ_TYPE_UINT16,
	SQ_TYPE_INT32,
	SQ_TYPE_UINT32,
	SQ_TYPE_INT64,
	SQ_TYPE_UINT64,
	SQ_TYPE_FLOAT32,
	SQ_TYPE_DOUBLE64,
	SQ_TYPE_STRING,
	SQ_TYPE_OBJECT,
	SQ_TYPE_CLOSURE,
	SQ_TYPE_NATIVECLOSURE,
	SQ_TYPE_NATIVEPTR,
	SQ_TYPE_INT_ANY,
	SQ_TYPE_FLOAT_ANY,
};

struct SQTypeDesc
{
	SQType* _resolved;
	SQObjectPtr _unresolved;
	struct SQAstNode_FunctionDef* _parentfunc;

	static const SQChar* GetMetaTypeLiterals(SQMetaType metaType);

	SQTypeDesc()
		: _resolved(NULL)
		, _unresolved((SQInteger)SQ_TYPE_DYNAMIC)
		, _parentfunc(NULL)
	{
	}

	SQTypeDesc(SQMetaType metaType)
		: _resolved(NULL)
		, _unresolved((SQInteger)metaType)
		, _parentfunc(NULL)
	{
	}

	SQTypeDesc(SQObjectPtr typeName)
		: _resolved(NULL)
		, _unresolved(typeName)
		, _parentfunc(NULL)
	{
		assert(sq_isstring(typeName));
	}

	const SQChar* ToString(std::basic_string<SQChar>& buf) const;
};

struct SQType //: public CHAINABLE_OBJ
{
	SQMetaType m_eMetaType;
	SQObjectPtr m_Name;
	SQAssembly* m_pOwnerAssembly;

	SQType()
		: m_eMetaType(SQ_TYPE_DYNAMIC)
		, m_pOwnerAssembly(NULL)
	{
	}

	SQObjectPtr ToString() const
	{
		return m_Name;
	}

	static void InitTypes(HSQUIRRELVM vm);
};

struct SQMemberDesc
{
	SQType m_Type;

	SQType GetMemberType() const { return m_Type; }
};

struct SQFieldDesc : public SQMemberDesc
{
};

struct SQMethodDesc : public SQMemberDesc
{
};

#endif//_SQREFLECTION_H_
