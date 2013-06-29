
using System;
using System.Collections;

namespace TestNamespace
{
	class DummyClass
	{
		static function foo(p)
		{
			return p + 1;
		}
	};


	class SqClass
	{
		<|attr0 = true, attr1 = "..."|>
		m_nontyped = 0;

		int m_typed = 27;
		float m_typed2 = 1.5;
		string m_typed3 = "##";
		m_typed4 = DummyClass();

		function Foo()
		{
			local a = 1;
			local b = 2;
			local c, d;
			::print("executing SqClass::Foo");
			::print("local a + local b = " + (a + b));
			::print("m_typed = " + m_typed);
			::print("m_nontyped = " + m_nontyped);
			::print("m_typed4 is "
				+ ((m_typed4 is DummyClass) ? "" : "not")
				+ "DummyClass");

			::print( "m_typed4 as DummyClass : " + (m_typed4 as DummyClass) );
			::print( "m_typed4 as SqClass : " + (m_typed4 as SqClass) );

			local evil = "eve";
			m_typed3 = evil;
			m_typed = evil;
		}
	};
}

/*
print("creating instance...");
local inst = SqClass();
print("calling function...");
inst.Foo();
print("end calling...");
inst.Foo();
print("end calling...");
inst.Foo();
print("end calling...");
*/