#include "myHeader.h"

int **y, *v, **z, *u, *x, w;

int main(){
    y = &v;
    z = &u;
    x = &w;
    isPointingTo(x,w);
    isPointingTo(y,v);
    while(w>0)
    {
        isLive(u);
        u = x;
        isLive(u);
    }
    return **z;
}
