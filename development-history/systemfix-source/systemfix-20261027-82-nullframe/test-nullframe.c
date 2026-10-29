#include <assert.h>
#define FC_TYPE_DATA 2
#define SUBTYPE_DATA_NULL 4
#define SUBTYPE_QOS_NULL 12
#define TRUE 1
#define NDIS_STATUS_SUCCESS 0
#define RELEASE_NDIS_PACKET(a,p,s) (freed++)
struct FC {int Type,SubType;}; struct RX {int DataSize;void *pRxPacket;};
static int freed;
static int attempt0(int type,int subtype,int length,int hdr_len) { struct FC f={type,subtype};struct RX r={length,0};struct FC *pFmeCtrl=&f;struct RX *pRxBlk=&r;void *pAd=0;(void)pAd;(void)hdr_len; if (pFmeCtrl->Type == FC_TYPE_DATA &&
		    pFmeCtrl->SubType == SUBTYPE_DATA_NULL &&
		    pRxBlk->DataSize == hdr_len) {
			RELEASE_NDIS_PACKET(pAd, pRxBlk->pRxPacket, NDIS_STATUS_SUCCESS);
			return TRUE;
		} return 0; }
static int attempt1(int type,int subtype,int length,int hdr_len) { struct FC f={type,subtype};struct RX r={length,0};struct FC *pFmeCtrl=&f;struct RX *pRxBlk=&r;void *pAd=0;(void)pAd;(void)hdr_len; if (pFmeCtrl->Type == FC_TYPE_DATA &&
			    pFmeCtrl->SubType == SUBTYPE_QOS_NULL &&
			    pRxBlk->DataSize == 2) {
				RELEASE_NDIS_PACKET(pAd, pRxBlk->pRxPacket, NDIS_STATUS_SUCCESS);
				return TRUE;
			} return 0; }
int main(void) {int n=0;
#define CHECK(fn,t,s,z,h,want) do {freed=0;assert(fn(t,s,z,h)==want);assert(freed==want);n++;}while(0)
CHECK(attempt0,2,4,24,24,1); CHECK(attempt0,2,4,23,24,0);CHECK(attempt0,2,4,25,24,0);CHECK(attempt0,2,12,24,24,0);CHECK(attempt0,0,4,24,24,0);
CHECK(attempt1,2,12,2,24,1);CHECK(attempt1,2,12,1,24,0);CHECK(attempt1,2,12,3,24,0);CHECK(attempt1,2,4,2,24,0);CHECK(attempt1,0,12,2,24,0);
return n==10 ? 0 : 1;}
