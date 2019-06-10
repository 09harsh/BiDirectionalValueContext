#include "myHeader.h"

int **y, *v, **z, *u, *x, w;

void func()
{
    x = &w;
    z = y;
}

int main(){
    y = &v;
    z = &u;
    isLive(z);
    isLive(y);
    if(w>0)
        func();
    isPointingTo(z,v);
    isPointingTo(z,u);
    isLive(x);
    *z = x;
    return 0;
}
