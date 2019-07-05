#include "myHeader.h"

int **y, *v, **z, *u, *x, w, b;

void func()
{
    x = u;
    isLive(v);
}

int main(){
    y = &v;
    z = &u;
    x = &w;
    v = &b;
    isLive(u);
    isLive(x);

    while(w>0)
    {
        func();
        u = *y;
    }
    isPointingTo(x,b);
    return *x;
}
