#include "myHeader.h"

int *a,b, *c, *d;


void func(int b)
{
    isLive(a);
    c = d;
}

int main(){
    a = d;
    func(b);
    isLive(c);
    d = a;
    func(*c);
	return *c;
}
