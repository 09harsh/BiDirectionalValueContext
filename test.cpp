#include "myHeader.h"

int func(int x, int y)
{
	check();
	x = x + y;
	return x;
}

void function()
{
    check();
    int a,b;
    a = 8;
    b = 7;
}

int main(){
	check();
	int a,b,c,d,e,f;
	a = a+b-c*d/e%f;
	a = b+d;
	if(a>b)
	    a=c;
	c = func(a,b);
	b = e-f;
	d = d*f/e;
	while(e>3)
	{
		d = d*c+b;
	}
	function();
	if(4>a)
	{
		e = a*b;
	}
	else
	{
		f = c-f;
	}

	return c;
}
