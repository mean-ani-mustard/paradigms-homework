#include "hashset.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static const int defaultAlloc = 4;
void HashSetNew(hashset *h, int elemSize, int numBuckets,
		HashSetHashFunction hashfn, HashSetCompareFunction comparefn, HashSetFreeFunction freefn)
{
	assert (elemSize>0 && numBuckets>0);
	assert( hashfn != NULL);
	assert(comparefn != NULL);

	h->elemSize = elemSize;
	h->numOfBuckets = numBuckets;
	h->myHashSize = 0;
	h->buckets = malloc(sizeof(vector) * numBuckets);
	assert(h->buckets != NULL);

	for (int i = 0; i < numBuckets; i++){
		VectorNew(&h->buckets[i], elemSize, freefn, defaultAlloc);
	}
	h->hashfn = hashfn;
	h->comparefn = comparefn;
	

}

void HashSetDispose(hashset *h)
{ 
	for (int i = 0; i < h->numOfBuckets; i++) {
    VectorDispose(&h->buckets[i]);
  }
  free(h->buckets);
}

int HashSetCount(const hashset *h)
{ 
	return h->myHashSize;
 }

void HashSetMap(hashset *h, HashSetMapFunction mapfn, void *auxData)
{
	assert (mapfn!=NULL);
	for (int i = 0; i < h->numOfBuckets; i++){
		VectorMap(&h->buckets[i], mapfn, auxData);
	}
}

void HashSetEnter(hashset *h, const void *elemAddr)
{
  assert(elemAddr != NULL);
  int hashCode = h->hashfn(elemAddr, h->numOfBuckets);
  assert(hashCode >= 0);
  assert (hashCode < h->numOfBuckets);
  vector *bucket = &h->buckets[hashCode];
  int position = VectorSearch(bucket, elemAddr, h->comparefn, 0, false);
  if (position == -1) { 
    VectorAppend(bucket, elemAddr);
    h->myHashSize++;
  } else {
    VectorReplace(bucket, elemAddr, position);
  }
}




void *HashSetLookup(const hashset *h, const void *elemAddr)
{ 
	void * result = NULL;
	assert(elemAddr != NULL);
    int bucketIndex = h->hashfn(elemAddr, h->numOfBuckets); 
    assert(bucketIndex >= 0 && bucketIndex < h->numOfBuckets);

    vector *bucket = &h->buckets[bucketIndex];
    int position = VectorSearch(bucket, elemAddr, h->comparefn, 0, false); 
    if (position != -1) {
        result = VectorNth(bucket, position);
    }
    return result;
	
	
 }
