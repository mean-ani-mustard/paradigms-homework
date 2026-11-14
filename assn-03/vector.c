#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <search.h>
static const int defaultAlloc = 4;
void VectorNew(vector *v, int elemSize, VectorFreeFunction freeFn, int initialAllocation)
{
   assert (elemSize>0 && initialAllocation>=0);
    v->myElemSize = elemSize;
    if(initialAllocation==0){
        v->allocSpace = defaultAlloc;
    }else{
        v->allocSpace = initialAllocation;
    }
    v->logicSpace = 0;
    v->func = freeFn;
    v->elems = malloc(v->allocSpace*elemSize);
    assert(v->elems!=NULL);
   
}

void VectorDispose(vector *v)
{
    if (v->func!=NULL){
        for (int i = 0; i < v->logicSpace; i++){
            v->func ((char*)v->elems + i*v->myElemSize);
        }
    }
    free(v->elems);
}

static void vectorGrow (vector *v){
    v->allocSpace *= 2;
    v->elems = realloc (v->elems, v->allocSpace * v->myElemSize);
    assert(v->elems!=NULL);
}

int VectorLength(const vector *v)
{ 
    int length = v->logicSpace; 
    return length;
}

void *VectorNth(const vector *v, int position)
{   
    assert(position>=0 && position < v->logicSpace);
    return (void *)((char *) v->elems + position *v->myElemSize);
}

void VectorReplace(vector *v, const void *elemAddr, int position)
{
    assert(position>=0 && position < v->logicSpace);
    void * toReplace = (char *)v->elems + position * v->myElemSize;
    if (v->func!=NULL){
        v->func (toReplace);
    }
    memcpy (toReplace, elemAddr, v->myElemSize);
}

void VectorInsert(vector *v, const void *elemAddr, int position)
{
     assert(position>=0 && position <= v->logicSpace);
     assert(elemAddr!=NULL);
    if(v->allocSpace == v->logicSpace){
        vectorGrow(v);
    }
    void * target1 = (char *)v->elems + position * v->myElemSize;
    void * target2 = (char *)v->elems + (position + 1) * v->myElemSize;
    memmove (target2, target1, v->logicSpace*v->myElemSize-position*v->myElemSize);
    memcpy(target1, elemAddr, v->myElemSize);
    v->logicSpace++;
}

void VectorAppend(vector *v, const void *elemAddr)
{
    assert(elemAddr!=NULL);
    if(v->allocSpace == v->logicSpace){
        vectorGrow(v);
    }
    void *target = (char *)v->elems + v->logicSpace * v->myElemSize;
    memcpy(target, elemAddr, v->myElemSize);
    v->logicSpace++;

}

void VectorDelete(vector *v, int position)
{
    assert(position>=0 && position < v->logicSpace);
    void * target1 = (char *)v->elems + position * v->myElemSize;
    void * target2 = (char *)v->elems + (position + 1) * v->myElemSize;
    if (v->func!=NULL){
        v->func (target1);
    }
    memmove (target1, target2, (v->logicSpace-position-1)*v->myElemSize);
    v->logicSpace--;

}

void VectorSort(vector *v, VectorCompareFunction compare)
{
    assert(compare!=NULL);
    qsort(v->elems, v->logicSpace, v->myElemSize, compare);
}

void VectorMap(vector *v, VectorMapFunction mapFn, void *auxData)
{
    assert(mapFn!=NULL);
    for (int i = 0; i < v->logicSpace; i++){
        mapFn(VectorNth(v, i), auxData);
    }
}

static const int kNotFound = -1;
int VectorSearch(const vector *v, const void *key, VectorCompareFunction searchFn, int startIndex, bool isSorted)
{ 
    assert (startIndex>=0);
    assert( startIndex <= v->logicSpace);
    void *target;
    size_t size = v->logicSpace - startIndex;
    if(isSorted){
        target = bsearch(key, (char *) v->elems + v->myElemSize * startIndex, v->logicSpace - startIndex, v->myElemSize, searchFn);
    }else{
        target = lfind (key, (char *) v->elems + v->myElemSize * startIndex, &size, v->myElemSize, searchFn);
    }
    if(target==NULL) return kNotFound;
    return ((char*)target-(char*)v->elems)/v->myElemSize;
        
} 
