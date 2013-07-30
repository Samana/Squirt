function foo()
{
	local a = 1;
	a = a + 2;
	return a;
}

local tmp = foo();
::print(tmp);