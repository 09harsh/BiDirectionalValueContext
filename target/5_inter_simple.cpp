#include "myHeader.h"

int **y, *v, **z, *u, *x, w;

void func()
{
    x = &w;
    isLive(x);
    isLive(z);
}

int main(){
    y = &v;
    z = &u;
    func();
    *z = x;
    return *u;
}
