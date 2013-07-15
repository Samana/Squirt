using System;
using System.Collections;

namespace TestNamespace
{
	class A
	{
		static function foo(p)
		{
			return p + 1;
		}
		
		static function bar()
		{
			return 1;
		}
	};

	class SqClass
	{
		//test members
		<|EditorVisible = true, ToolTip = "...", Min = 0, Max = 5, Default = 0 |>
		m_nontyped = 0;
		m_nontyped2 = A();
		
		int m_typed = 27;
		float m_typed2 = 1.5;
		string m_typed3 = "##";
		A m_typed4 = A();
		
		function test_locals()
		{
			local nontyped = 0;
			int localint = 0;
			float localfloat = 1.2;
			string localstr = "!!";
			
			localint = nontyped + 1;
			
			A localobj = A();
			A localobj1 = null, localobj2 = A(), localobj3 = localobj;
		}
		
		function test_exprs()
		{
			local a, b, c, d;
			a = 1 + 2 * 3.2 - 5 / ( 2 + 4 ) + 3;
			b = 2 < 3 && 3 * 5 > 10.4 || !(4 > 3);
			c = (1 << 30) | (0xff & a) << 16 | ~5;
			d = 1;
			d = d++;
			d = d--;
			d = ++d;
			d = --d;
			d = d++ * 2;
			d = 2 * d--;
			d += 1;
			d *= 2;
			d -= 3;
			d /= 4;
		}
		
		function test_params(int x, float y, string z, A w)
		{
			return x + y;
		}
		
		function test_return_int(int x) : int
		{
			return x + 1;
		}

		function test_return_object() : A
		{
			return null;
		}

		function test_casts()
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
