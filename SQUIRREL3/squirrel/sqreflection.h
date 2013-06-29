#ifndef _SQREFLECTION_H_
#define _SQREFLECTION_H_

struct SQType;

struct SQAssembly
{
	SQString* _name;
	SQTable* _references;
	SQTable* _types;

	SQObjectPtr FindType(SQString* name);
};

struct SQNamespace
{
	SQTable* _subnamespaces;
	SQTable* _klasses;

	void AddClass(SQObjectPtr name);
	SQType* GetClass(SQObjectPtr name) const;
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
};

struct SQType //: public CHAINABLE_OBJ
{
	SQMetaType m_eMetaType;
	SQObjectPtr m_Name;

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
