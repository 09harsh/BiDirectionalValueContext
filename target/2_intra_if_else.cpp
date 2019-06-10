#include "myHeader.h"

int **y, *v, **z, *u, *x, w;

int main(){
    y = &v;
    z = &u;
    x = &w;
    isLive(y);
    if(w)
    {
        isPointingTo(y,v);
        isPointingTo(z,u);
        *z = x;
    }
    else
    {
        z = y;
    }
    return *u;
}
