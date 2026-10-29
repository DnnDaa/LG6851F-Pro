#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef u8 UCHAR; typedef u8 UINT8;
#define TRUE 1
#define FALSE 0
#define DOT11_BE_D12 1
#define MAC_ADDR_LEN 6
#define BSS_MNGR_MAX_BAND_NUM 3
#define BSS_MNGR_MAX_PER_STA_PROFILE_LEN 576
#define MAX_LEN_OF_IE_DATA 255
#define IE_WLAN_EXTENSION 255
#define EID_EXT_EHT_MULTI_LINK 107
#define SUB_IE_MLD_PER_STA_PROFILE 0
#define SUB_IE_MLD_FRAGMENT 254
#define ML_CTRL_TYPE_BASIC 0
#define ML_CTRL_TYPE_PROBE_REQ 1
#define ML_CTRL_TYPE_RECONFIG 2
#define CAP_BITS_MASK(x) (((1u << x##_BITS)-1) << x##_SHIFT)
#define GET_CAP_BITS(x, v) (((v) & CAP_BITS_MASK(x)) >> x##_SHIFT)
#define BIT(x) (1u << (x))
#define cpu_to_le16(x) (x)
#define le16_to_cpu(x) (x)
#define NdisMoveMemory memcpy
#define NdisZeroMemory(p,n) memset(p,0,n)
#define COPY_MAC_ADDR(d,s) memcpy(d,s,6)
#define MTWF_DBG(...) do {} while(0)
#define hex_dump_with_cat_and_lvl(...) do {} while(0)
#define os_alloc_mem(a,p,n) (*(p)=malloc(n))
#define os_free_mem free
struct _EID_STRUCT {u8 Eid,Len,Octet[1];};
typedef struct _EID_STRUCT *PEID_STRUCT;
struct frag_ie_info {bool is_frag;u16 ie_len_all,ie_len_defrag;u8 *frm_end;};
#define SET_DOT11BE_ML_CTRL_MLD_MAC_PRESENT(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_CTRL_MLD_MAC_PRESENT, _capinfo, _value)
#define GET_DOT11BE_ML_CTRL_MLD_MAC_PRESENT(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_CTRL_MLD_MAC_PRESENT, _capinfo)
#define DOT11BE_ML_CTRL_MLD_MAC_PRESENT_SHIFT \
	DOT11BE_ML_CTRL_TYPE_BITS
#define DOT11BE_ML_CTRL_MLD_MAC_PRESENT_BTIS 1
#define DOT11BE_ML_CTRL_MLD_MAC_PRESENT_MASK \
	BIT(DOT11BE_ML_CTRL_MLD_MAC_PRESENT_SHIFT)
#define DOT11BE_ML_BASIC_PRESENCE_LINK_ID           BIT(0)
#define DOT11BE_ML_BASIC_PRESENCE_BSS_CHG_CNT       BIT(1)
#define DOT11BE_ML_BASIC_PRESENCE_MD_SYNC_DELAY     BIT(2)
#define DOT11BE_ML_BASIC_PRESENCE_EML_CAPS          BIT(3)
#define DOT11BE_ML_BASIC_PRESENCE_MLD_CAPS_N_OP     BIT(4)
#define DOT11BE_ML_BASIC_PRESENCE_MLD_ID            BIT(5)
#define DOT11BE_ML_BASIC_PRESENCE_EXT_MLD_CAPS_N_OP BIT(6) /* D3.0 */
#define DOT11BE_ML_PROBE_REQ_PRESENCE_MLD_ID    BIT(0)
#define DOT11BE_ML_RECONFIG_PRESENCE_MLD_MAC_ADDR    BIT(0)
#define DOT11BE_ML_CTRL_TYPE_SHIFT 0
#define DOT11BE_ML_CTRL_TYPE_BITS 3
#define DOT11BE_ML_CTRL_TYPE_MASK \
	CAP_BITS_MASK(DOT11BE_ML_CTRL_TYPE)
#define DOT11BE_ML_CTRL_PRESENCE_BMAP_SHIFT 4
#define DOT11BE_ML_CTRL_PRESENCE_BMAP_BITS 12
#define DOT11BE_ML_CTRL_PRESENCE_BMAP_MASK \
	CAP_BITS_MASK(DOT11BE_ML_CTRL_PRESENCE_BMAP)
#define SET_DOT11BE_ML_CTRL_TYPE(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_CTRL_TYPE, _capinfo, _value)
#define GET_DOT11BE_ML_CTRL_TYPE(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_CTRL_TYPE, _capinfo)
#define SET_DOT11BE_ML_CTRL_PRESENCE_BMAP(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_CTRL_PRESENCE_BMAP, _capinfo, _value)
#define GET_DOT11BE_ML_CTRL_PRESENCE_BMAP(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_CTRL_PRESENCE_BMAP, _capinfo)
#define DOT11BE_ML_BASIC_CMM_LINK_ID_SHIFT 0
#define DOT11BE_ML_BASIC_CMM_LINK_ID_BITS 4
#define DOT11BE_ML_BASIC_CMM_LINK_ID_MASK \
	CAP_BITS_MASK(DOT11BE_ML_BASIC_CMM_LINK_ID)
#define DOT11BE_ML_BASIC_CMM_MD_SYNC_DELAY_DUR_SHIFT 0
#define DOT11BE_ML_BASIC_CMM_MD_SYNC_DELAY_DUR_BITS 8
#define DOT11BE_ML_BASIC_CMM_MD_SYNC_DELAY_DUR_MASK \
	CAP_BITS_MASK(DOT11BE_ML_BASIC_CMM_MD_SYNC_DELAY_DUR)
#define DOT11BE_ML_BASIC_CMM_MD_SYNC_OFDM_ED_THR_SHIFT 8
#define DOT11BE_ML_BASIC_CMM_MD_SYNC_OFDM_ED_THR_BITS 4
#define DOT11BE_ML_BASIC_CMM_MD_SYNC_OFDM_ED_THR_MASK \
	CAP_BITS_MASK(DOT11BE_ML_BASIC_CMM_MD_SYNC_OFDM_ED_THR)
#define DOT11BE_ML_BASIC_CMM_MD_SYNC_MAX_TXOP_SHIFT 12
#define DOT11BE_ML_BASIC_CMM_MD_SYNC_MAX_TXOP_BITS 4
#define DOT11BE_ML_BASIC_CMM_MD_SYNC_MAX_TXOP_MASK \
	CAP_BITS_MASK(DOT11BE_ML_BASIC_CMM_MD_SYNC_MAX_TXOP)
#define DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_SUP_SHIFT 0
#define DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_SUP_BITS 1
#define DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_SUP_MASK \
	CAP_BITS_MASK(DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_SUP)
#define DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_PADDING_DELAY_SHIFT 1
#define DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_PADDING_DELAY_BITS 3
#define DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_PADDING_DELAY_MASK \
	CAP_BITS_MASK(DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_PADDING_DELAY)
#define DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_TRANS_DELAY_SHIFT 4
#define DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_TRANS_DELAY_BITS 3
#define DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_TRANS_DELAY_MASK \
	CAP_BITS_MASK(DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_TRANS_DELAY)
#define DOT11BE_ML_BASIC_CMM_EML_CAP_EMLMR_SUP_SHIFT 7
#define DOT11BE_ML_BASIC_CMM_EML_CAP_EMLMR_SUP_BITS 1
#define DOT11BE_ML_BASIC_CMM_EML_CAP_EMLMR_SUP_MASK \
	CAP_BITS_MASK(DOT11BE_ML_BASIC_CMM_EML_CAP_EMLMR_SUP)
#define DOT11BE_ML_BASIC_CMM_EML_CAP_EMLMR_DELAY_SHIFT 8
#define DOT11BE_ML_BASIC_CMM_EML_CAP_EMLMR_DELAY_BITS 3
#define DOT11BE_ML_BASIC_CMM_EML_CAP_EMLMR_DELAY_MASK \
	CAP_BITS_MASK(DOT11BE_ML_BASIC_CMM_EML_CAP_EMLMR_DELAY)
#define DOT11BE_ML_BASIC_CMM_EML_CAP_TRANS_TO_SHIFT 11
#define DOT11BE_ML_BASIC_CMM_EML_CAP_TRANS_TO_BITS 4
#define DOT11BE_ML_BASIC_CMM_EML_CAP_TRANS_TO_MASK \
	CAP_BITS_MASK(DOT11BE_ML_BASIC_CMM_EML_CAP_TRANS_TO)
#define DOT11BE_ML_BASIC_CMM_EML_CAP_RESERVED_SHIFT 15
#define DOT11BE_ML_BASIC_CMM_EML_CAP_RESERVED_BITS 1
#define DOT11BE_ML_BASIC_CMM_EML_CAP_RESERVED_MASK \
	CAP_BITS_MASK(DOT11BE_ML_BASIC_CMM_EML_CAP_RESERVED)
#define DOT11BE_ML_BASIC_CMM_MLD_CAP_MAX_LINKS_SHIFT 0
#define DOT11BE_ML_BASIC_CMM_MLD_CAP_MAX_LINKS_BITS 4
#define DOT11BE_ML_BASIC_CMM_MLD_CAP_MAX_LINKS_MASK \
	CAP_BITS_MASK(DOT11BE_ML_BASIC_CMM_MLD_CAP_MAX_LINKS)
#define DOT11BE_ML_BASIC_CMM_MLD_CAP_SRS_SUP_SHIFT 4
#define DOT11BE_ML_BASIC_CMM_MLD_CAP_SRS_SUP_BITS 1
#define DOT11BE_ML_BASIC_CMM_MLD_CAP_SRS_SUP_MASK \
	CAP_BITS_MASK(DOT11BE_ML_BASIC_CMM_MLD_CAP_SRS_SUP)
#define DOT11BE_ML_BASIC_CMM_MLD_CAP_TID2LNK_NEGO_SUP_SHIFT 5
#define DOT11BE_ML_BASIC_CMM_MLD_CAP_TID2LNK_NEGO_SUP_BITS 2
#define DOT11BE_ML_BASIC_CMM_MLD_CAP_TID2LNK_NEGO_SUP_MASK \
	CAP_BITS_MASK(DOT11BE_ML_BASIC_CMM_MLD_CAP_TID2LNK_NEGO_SUP)
#define DOT11BE_ML_BASIC_CMM_MLD_CAP_FREQ_SEP_STR_SHIFT 7
#define DOT11BE_ML_BASIC_CMM_MLD_CAP_FREQ_SEP_STR_BITS 5
#define DOT11BE_ML_BASIC_CMM_MLD_CAP_FREQ_SEP_STR_MASK \
	CAP_BITS_MASK(DOT11BE_ML_BASIC_CMM_MLD_CAP_FREQ_SEP_STR)
#define SET_DOT11BE_ML_BASIC_CMM_LINK_ID(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_BASIC_CMM_LINK_ID, _capinfo, _value)
#define GET_DOT11BE_ML_BASIC_CMM_LINK_ID(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_BASIC_CMM_LINK_ID, _capinfo)
#define SET_DOT11BE_ML_BASIC_CMM_MD_SYNC_DELAY_DUR(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_BASIC_CMM_MD_SYNC_DELAY_DUR, _capinfo, _value)
#define GET_DOT11BE_ML_BASIC_CMM_MD_SYNC_DELAY_DUR(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_BASIC_CMM_MD_SYNC_DELAY_DUR, _capinfo)
#define SET_DOT11BE_ML_BASIC_CMM_MD_SYNC_OFDM_ED_THR(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_BASIC_CMM_MD_SYNC_OFDM_ED_THR, _capinfo, _value)
#define GET_DOT11BE_ML_BASIC_CMM_MD_SYNC_OFDM_ED_THR(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_BASIC_CMM_MD_SYNC_OFDM_ED_THR, _capinfo)
#define SET_DOT11BE_ML_BASIC_CMM_MD_SYNC_MAX_TXOP(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_BASIC_CMM_MD_SYNC_MAX_TXOP, _capinfo, _value)
#define GET_DOT11BE_ML_BASIC_CMM_MD_SYNC_MAX_TXOP(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_BASIC_CMM_MD_SYNC_MAX_TXOP, _capinfo)
#define SET_DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_SUP(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_SUP, _capinfo, _value)
#define GET_DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_SUP(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_SUP, _capinfo)
#define SET_DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_PADDING_DELAY(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_PADDING_DELAY, _capinfo, _value)
#define GET_DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_PADDING_DELAY(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_PADDING_DELAY, _capinfo)
#define SET_DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_TRANS_DELAY(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_TRANS_DELAY, _capinfo, _value)
#define GET_DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_TRANSITION_DELAY(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_TRANS_DELAY, _capinfo)
#define SET_DOT11BE_ML_BASIC_CMM_EML_CAP_EMLMR_SUP(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_BASIC_CMM_EML_CAP_EMLMR_SUP, _capinfo, _value)
#define GET_DOT11BE_ML_BASIC_CMM_EML_CAP_EMLMR_SUP(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_BASIC_CMM_EML_CAP_EMLMR_SUP, _capinfo)
#define SET_DOT11BE_ML_BASIC_CMM_EML_CAP_EMLMR_DELAY(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_BASIC_CMM_EML_CAP_EMLMR_DELAY, _capinfo, _value)
#define GET_DOT11BE_ML_BASIC_CMM_EML_CAP_EMLMR_DELAY(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_BASIC_CMM_EML_CAP_EMLMR_DELAY, _capinfo)
#define SET_DOT11BE_ML_BASIC_CMM_EML_CAP_TRANS_TO(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_BASIC_CMM_EML_CAP_TRANS_TO, _capinfo, _value)
#define GET_DOT11BE_ML_BASIC_CMM_EML_CAP_TRANS_TO(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_BASIC_CMM_EML_CAP_TRANS_TO, _capinfo)
#define SET_DOT11BE_ML_BASIC_CMM_MLD_CAP_MAX_LINKS(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_BASIC_CMM_MLD_CAP_MAX_LINKS, _capinfo, _value)
#define GET_DOT11BE_ML_BASIC_CMM_MLD_CAP_MAX_LINKS(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_BASIC_CMM_MLD_CAP_MAX_LINKS, _capinfo)
#define SET_DOT11BE_ML_BASIC_CMM_MLD_CAP_SRS_SUP(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_BASIC_CMM_MLD_CAP_SRS_SUP, _capinfo, _value)
#define GET_DOT11BE_ML_BASIC_CMM_MLD_CAP_SRS_SUP(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_BASIC_CMM_MLD_CAP_SRS_SUP, _capinfo)
#define SET_DOT11BE_ML_BASIC_CMM_MLD_CAP_TID2LNK_NEGO_SUP(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_BASIC_CMM_MLD_CAP_TID2LNK_NEGO_SUP, _capinfo, _value)
#define GET_DOT11BE_ML_BASIC_CMM_MLD_CAP_TID2LNK_NEGO_SUP(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_BASIC_CMM_MLD_CAP_TID2LNK_NEGO_SUP, _capinfo)
#define SET_DOT11BE_ML_BASIC_CMM_MLD_CAP_FREQ_SEP_STR(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_BASIC_CMM_MLD_CAP_FREQ_SEP_STR, _capinfo, _value)
#define GET_DOT11BE_ML_BASIC_CMM_MLD_CAP_FREQ_SEP_STR(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_BASIC_CMM_MLD_CAP_FREQ_SEP_STR, _capinfo)
#define DOT11BE_ML_PER_STA_CTRL_LINK_ID_SHIFT 0
#define DOT11BE_ML_PER_STA_CTRL_LINK_ID_BITS 4
#define DOT11BE_ML_PER_STA_CTRL_LINK_ID_MASK \
	CAP_BITS_MASK(DOT11BE_ML_PER_STA_CTRL_LINK_ID)
#define DOT11BE_ML_PER_STA_CTRL_COMP_PROF_SHIFT 4
#define DOT11BE_ML_PER_STA_CTRL_COMP_PROF_BITS 1
#define DOT11BE_ML_PER_STA_CTRL_COMP_PROF_MASK \
	CAP_BITS_MASK(DOT11BE_ML_PER_STA_CTRL_COMP_PROF)
#define DOT11BE_ML_PER_STA_CTRL_MAC_ADDR_PRES_SHIFT 5
#define DOT11BE_ML_PER_STA_CTRL_MAC_ADDR_PRES_BITS 1
#define DOT11BE_ML_PER_STA_CTRL_MAC_ADDR_PRES_MASK \
	CAP_BITS_MASK(DOT11BE_ML_PER_STA_CTRL_MAC_ADDR_PRES)
#define DOT11BE_ML_PER_STA_CTRL_BCN_INT_PRES_SHIFT 6
#define DOT11BE_ML_PER_STA_CTRL_BCN_INT_PRES_BITS 1
#define DOT11BE_ML_PER_STA_CTRL_BCN_INT_PRES_MASK \
	CAP_BITS_MASK(DOT11BE_ML_PER_STA_CTRL_BCN_INT_PRES)
#define DOT11BE_ML_PER_STA_CTRL_TSF_OFFSET_PRES_SHIFT 7
#define DOT11BE_ML_PER_STA_CTRL_TSF_OFFSET_PRES_BITS 1
#define DOT11BE_ML_PER_STA_CTRL_TSF_OFFSET_PRES_MASK \
	CAP_BITS_MASK(DOT11BE_ML_PER_STA_CTRL_TSF_OFFSET_PRES)
#define DOT11BE_ML_PER_STA_CTRL_DTIM_PRES_SHIFT 8
#define DOT11BE_ML_PER_STA_CTRL_DTIM_PRES_BITS 1
#define DOT11BE_ML_PER_STA_CTRL_DTIM_PRES_MASK \
	CAP_BITS_MASK(DOT11BE_ML_PER_STA_CTRL_DTIM_PRES)
#define DOT11BE_ML_PER_STA_CTRL_NSTR_LPR_PRES_SHIFT 9
#define DOT11BE_ML_PER_STA_CTRL_NSTR_LPR_PRES_BITS 1
#define DOT11BE_ML_PER_STA_CTRL_NSTR_LPR_PRES_MASK \
	CAP_BITS_MASK(DOT11BE_ML_PER_STA_CTRL_NSTR_LPR_PRES)
#define DOT11BE_ML_PER_STA_CTRL_NSTR_BMAP_SIZE_SHIFT 10
#define DOT11BE_ML_PER_STA_CTRL_NSTR_BMAP_SIZE_BITS 1
#define DOT11BE_ML_PER_STA_CTRL_NSTR_BMAP_SIZE_MASK \
	CAP_BITS_MASK(DOT11BE_ML_PER_STA_CTRL_NSTR_BMAP_SIZE)
#define DOT11BE_ML_PER_STA_CTRL_BSS_CHG_CNT_PRES_SHIFT 11
#define DOT11BE_ML_PER_STA_CTRL_BSS_CHG_CNT_PRES_BITS 1
#define DOT11BE_ML_PER_STA_CTRL_BSS_CHG_CNT_PRES_MASK \
	CAP_BITS_MASK(DOT11BE_ML_PER_STA_CTRL_BSS_CHG_CNT_PRES)
#define SET_DOT11BE_ML_PER_STA_CTRL_LINK_ID(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_PER_STA_CTRL_LINK_ID, _capinfo, _value)
#define GET_DOT11BE_ML_PER_STA_CTRL_LINK_ID(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_PER_STA_CTRL_LINK_ID, _capinfo)
#define SET_DOT11BE_ML_PER_STA_CTRL_COMP_PROF(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_PER_STA_CTRL_COMP_PROF, _capinfo, _value)
#define GET_DOT11BE_ML_PER_STA_CTRL_COMP_PROF(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_PER_STA_CTRL_COMP_PROF, _capinfo)
#define SET_DOT11BE_ML_PER_STA_CTRL_MAC_ADDR_PRES(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_PER_STA_CTRL_MAC_ADDR_PRES, _capinfo, _value)
#define GET_DOT11BE_ML_PER_STA_CTRL_MAC_ADDR_PRES(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_PER_STA_CTRL_MAC_ADDR_PRES, _capinfo)
#define SET_DOT11BE_ML_PER_STA_CTRL_BCN_INT_PRES(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_PER_STA_CTRL_BCN_INT_PRES, _capinfo, _value)
#define GET_DOT11BE_ML_PER_STA_CTRL_BCN_INT_PRES(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_PER_STA_CTRL_BCN_INT_PRES, _capinfo)
#define SET_DOT11BE_ML_PER_STA_CTRL_TSF_OFFSET_PRES(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_PER_STA_CTRL_TSF_OFFSET_PRES, _capinfo, _value)
#define GET_DOT11BE_ML_PER_STA_CTRL_TSF_OFFSET_PRES(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_PER_STA_CTRL_TSF_OFFSET_PRES, _capinfo)
#define SET_DOT11BE_ML_PER_STA_CTRL_DTIM_PRES(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_PER_STA_CTRL_DTIM_PRES, _capinfo, _value)
#define GET_DOT11BE_ML_PER_STA_CTRL_DTIM_PRES(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_PER_STA_CTRL_DTIM_PRES, _capinfo)
#define SET_DOT11BE_ML_PER_STA_CTRL_NSTR_LPR_PRES(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_PER_STA_CTRL_NSTR_LPR_PRES, _capinfo, _value)
#define GET_DOT11BE_ML_PER_STA_CTRL_NSTR_LPR_PRES(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_PER_STA_CTRL_NSTR_LPR_PRES, _capinfo)
#define SET_DOT11BE_ML_PER_STA_CTRL_NSTR_BMAP_SIZE(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_PER_STA_CTRL_NSTR_BMAP_SIZE, _capinfo, _value)
#define GET_DOT11BE_ML_PER_STA_CTRL_NSTR_BMAP_SIZE(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_PER_STA_CTRL_NSTR_BMAP_SIZE, _capinfo)
#define SET_DOT11BE_ML_PER_STA_CTRL_BSS_CHG_CNT_PRES(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_PER_STA_CTRL_BSS_CHG_CNT_PRES, _capinfo, _value)
#define GET_DOT11BE_ML_PER_STA_CTRL_BSS_CHG_CNT_PRES(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_PER_STA_CTRL_BSS_CHG_CNT_PRES, _capinfo)
#define DOT11BE_ML_PROBE_REQ_CMM_MLD_ID_SHIFT 0
#define DOT11BE_ML_PROBE_REQ_CMM_MLD_ID_BITS 1
#define DOT11BE_ML_PROBE_REQ_CMM_MLD_ID_MASK \
	CAP_BITS_MASK(DOT11BE_ML_PROBE_REQ_CMM_MLD_ID)
#define DOT11BE_ML_PROBE_REQ_PER_STA_CTRL_LINK_ID_SHIFT 0
#define DOT11BE_ML_PROBE_REQ_PER_STA_CTRL_LINK_ID_BITS 4
#define DOT11BE_ML_PROBE_REQ_PER_STA_CTRL_LINK_ID_MASK \
	CAP_BITS_MASK(DOT11BE_ML_PROBE_REQ_PER_STA_CTRL_LINK_ID)
#define DOT11BE_ML_PROBE_REQ_PER_STA_CTRL_COMP_PROF_SHIFT 4
#define DOT11BE_ML_PROBE_REQ_PER_STA_CTRL_COMP_PROF_BITS 1
#define DOT11BE_ML_PROBE_REQ_PER_STA_CTRL_COMP_PROF_MASK \
	CAP_BITS_MASK(DOT11BE_ML_PROBE_REQ_PER_STA_CTRL_COMP_PROF)
#define DOT11BE_ML_RECONFIG_PER_STA_CTRL_LINK_ID_SHIFT 0
#define DOT11BE_ML_RECONFIG_PER_STA_CTRL_LINK_ID_BITS 4
#define DOT11BE_ML_RECONFIG_PER_STA_CTRL_LINK_ID_MASK \
	CAP_BITS_MASK(DOT11BE_ML_RECONFIG_PER_STA_CTRL_LINK_ID)
#define DOT11BE_ML_RECONFIG_PER_STA_CTRL_COMP_PROF_SHIFT 4
#define DOT11BE_ML_RECONFIG_PER_STA_CTRL_COMP_PROF_BITS 1
#define DOT11BE_ML_RECONFIG_PER_STA_CTRL_COMP_PROF_MASK \
	CAP_BITS_MASK(DOT11BE_ML_RECONFIG_PER_STA_CTRL_COMP_PROF)
#define DOT11BE_ML_RECONFIG_PER_STA_CTRL_MAC_ADDR_PRES_SHIFT 5
#define DOT11BE_ML_RECONFIG_PER_STA_CTRL_MAC_ADDR_PRES_BITS 1
#define DOT11BE_ML_RECONFIG_PER_STA_CTRL_MAC_ADDR_PRES_MASK \
	CAP_BITS_MASK(DOT11BE_ML_RECONFIG_PER_STA_CTRL_MAC_ADDR_PRES)
#define DOT11BE_ML_RECONFIG_PER_STA_CTRL_AP_RM_TMR_PRES_SHIFT 6
#define DOT11BE_ML_RECONFIG_PER_STA_CTRL_AP_RM_TMR_PRES_BITS 1
#define DOT11BE_ML_RECONFIG_PER_STA_CTRL_AP_RM_TMR_PRES_MASK \
	CAP_BITS_MASK(DOT11BE_ML_RECONFIG_PER_STA_CTRL_AP_RM_TMR_PRES)
#define DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_UPD_TYPE_SHIFT 7
#define DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_UPD_TYPE_BITS 4
#define DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_UPD_TYPE_MASK \
	CAP_BITS_MASK(DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_UPD_TYPE)
#define DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_PARAM_PRES_SHIFT 11
#define DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_PARAM_PRES_BITS 1
#define DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_PARAM_PRES_MASK \
	CAP_BITS_MASK(DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_PARAM_PRES)
#define SET_DOT11BE_ML_RECONFIG_PER_STA_CTRL_LINK_ID(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_RECONFIG_PER_STA_CTRL_LINK_ID, _capinfo, _value)
#define GET_DOT11BE_ML_RECONFIG_PER_STA_CTRL_LINK_ID(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_RECONFIG_PER_STA_CTRL_LINK_ID, _capinfo)
#define SET_DOT11BE_ML_RECONFIG_PER_STA_CTRL_COMP_PROF(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_RECONFIG_PER_STA_CTRL_COMP_PROF, _capinfo, _value)
#define GET_DOT11BE_ML_RECONFIG_PER_STA_CTRL_COMP_PROF(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_RECONFIG_PER_STA_CTRL_COMP_PROF, _capinfo)
#define SET_DOT11BE_ML_RECONFIG_PER_STA_CTRL_MAC_ADDR_PRES(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_RECONFIG_PER_STA_CTRL_MAC_ADDR_PRES, _capinfo, _value)
#define GET_DOT11BE_ML_RECONFIG_PER_STA_CTRL_MAC_ADDR_PRES(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_RECONFIG_PER_STA_CTRL_MAC_ADDR_PRES, _capinfo)
#define SET_DOT11BE_ML_RECONFIG_PER_STA_CTRL_AP_RM_TMR_PRES(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_RECONFIG_PER_STA_CTRL_AP_RM_TMR_PRES, _capinfo, _value)
#define GET_DOT11BE_ML_RECONFIG_PER_STA_CTRL_AP_RM_TMR_PRES(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_RECONFIG_PER_STA_CTRL_AP_RM_TMR_PRES, _capinfo)
#define SET_DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_UPD_TYPE(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_UPD_TYPE, _capinfo, _value)
#define GET_DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_UPD_TYPE(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_UPD_TYPE, _capinfo)
#define SET_DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_PARAM_PRES(_capinfo, _value) \
	SET_CAP_BITS(DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_PARAM_PRES, _capinfo, _value)
#define GET_DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_PARAM_PRES(_capinfo) \
	GET_CAP_BITS(DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_PARAM_PRES, _capinfo)
struct ml_ie_link {
	u8		link_id;
	u8		ch_band;
	u8		channel;

	/* generic */
	bool	active;
	u16		sta_ctrl;
	u8		sta_info_len;
	u8		link_addr[MAC_ADDR_LEN];

	/* basic ml ie */
	u16		bcn_interval;
	u8		dtim_count;
	u8		dtim_period;
	u16		nstr_bmap;
	u8		sta_profile[BSS_MNGR_MAX_PER_STA_PROFILE_LEN];
	u16		sta_profile_len; /* for both sub-IE defragment and store STA Profile */

	/* reconfiguration ml ie */
	u8		reconfig_type;
};
struct ml_ie_info {
	u8		type;
	u16		pres_bmap;
	u8		mld_addr[MAC_ADDR_LEN];
	u8		link_id;
	u8		bss_chg_cnt;
	u16		md_sync_delay;
	u16		eml_caps;
	u8		emlsr_sup;
	u16		mld_caps;
	u16		ext_mld_caps;
	u8		cmm_info_len;
	u8		mld_id; /* 11be AP MLD ID */
	u8		rsvd[3];
	struct ml_ie_link link[BSS_MNGR_MAX_BAND_NUM];
};
u16 defrag_multi_link_per_sta_profile(u8 *input, u8 *end, u8 *output, u16 *out_len)
{
	u32 wire = 0, used = 0;
	u8 len;
	if (!input || !end || !output || !out_len || end < input)
		return 0;
	*out_len = 0;
	if (end - input < 2 || input[0] != SUB_IE_MLD_PER_STA_PROFILE)
		return 0;
	len = input[1];
	if (end - input < 2 + len || 2 + len > BSS_MNGR_MAX_PER_STA_PROFILE_LEN)
		return 0;
	wire = used = 2 + len;
	NdisMoveMemory(output, input, used);
	while (len == MAX_LEN_OF_IE_DATA && wire < end - input) {
		if (end - input - wire < 2)
			return 0;
		if (input[wire] != SUB_IE_MLD_FRAGMENT)
			break;
		len = input[wire + 1];
		if (!len || end - input - wire < 2 + len ||
		    len > BSS_MNGR_MAX_PER_STA_PROFILE_LEN - used || wire + 2 + len > 65535)
			return 0;
		NdisMoveMemory(output + used, input + wire + 2, len);
		used += len;
		wire += 2 + len;
	}
	*out_len = used;
	return wire - 2;
}
int parse_multi_link_per_sta_profile(u8 *input, u16 len, struct ml_ie_info *info)
{
	u8 *pos, *end, *info_end;
	u16 ctrl;
	u8 id, info_len;
	struct ml_ie_link *link;
	if (!input || !info || len < 2)
		return -1;
	pos = input + 2;
	end = pos + len;
	NdisMoveMemory(&ctrl, pos, 2);
	ctrl = le16_to_cpu(ctrl);
	pos += 2;
	id = GET_DOT11BE_ML_PER_STA_CTRL_LINK_ID(ctrl);
	if (id >= BSS_MNGR_MAX_BAND_NUM)
		return 0; /* Unsupported link is not an array index. */
	link = &info->link[id];
	link->active = FALSE;
	link->sta_profile_len = 0;
	link->sta_ctrl = ctrl;
	if (info->type == ML_CTRL_TYPE_BASIC || info->type == ML_CTRL_TYPE_RECONFIG) {
		if (pos == end)
			return -1;
		info_len = *pos;
		/* Length includes its own octet, not the two-octet STA Control. */
		if (!info_len || info_len > end - pos)
			return -1;
		info_end = pos + info_len;
		link->sta_info_len = info_len;
		pos++;
		if (info->type == ML_CTRL_TYPE_RECONFIG) {
			link->link_id = id;
			link->reconfig_type = GET_DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_UPD_TYPE(ctrl);
			link->active = TRUE;
			return 0;
		}
#define LG_STA_TAKE(dst, count) do { \
	if ((count) > info_end - pos) return -1; \
	NdisMoveMemory((dst), pos, (count)); pos += (count); \
} while (0)
		if (GET_DOT11BE_ML_PER_STA_CTRL_MAC_ADDR_PRES(ctrl))
			LG_STA_TAKE(link->link_addr, MAC_ADDR_LEN);
		if (GET_DOT11BE_ML_PER_STA_CTRL_BCN_INT_PRES(ctrl)) {
			LG_STA_TAKE(&link->bcn_interval, 2);
			link->bcn_interval = le16_to_cpu(link->bcn_interval);
		}
		if (GET_DOT11BE_ML_PER_STA_CTRL_TSF_OFFSET_PRES(ctrl)) {
			if (info_end - pos < 8) return -1;
			pos += 8;
		}
		if (GET_DOT11BE_ML_PER_STA_CTRL_DTIM_PRES(ctrl)) {
			LG_STA_TAKE(&link->dtim_count, 1);
			LG_STA_TAKE(&link->dtim_period, 1);
		}
		if (GET_DOT11BE_ML_PER_STA_CTRL_COMP_PROF(ctrl) &&
		    GET_DOT11BE_ML_PER_STA_CTRL_NSTR_LPR_PRES(ctrl)) {
			link->nstr_bmap = 0;
			LG_STA_TAKE(&link->nstr_bmap,
				GET_DOT11BE_ML_PER_STA_CTRL_NSTR_BMAP_SIZE(ctrl) ? 2 : 1);
			link->nstr_bmap = le16_to_cpu(link->nstr_bmap);
		}
		if (GET_DOT11BE_ML_PER_STA_CTRL_BSS_CHG_CNT_PRES(ctrl)) {
			if (pos == info_end) return -1;
			pos++;
		}
#undef LG_STA_TAKE
		/* Unknown future STA Info fields are skipped using the declared length. */
		pos = info_end;
	}
	if (end - pos > sizeof(link->sta_profile))
		return -1;
	link->sta_profile_len = end - pos;
	NdisMoveMemory(link->sta_profile, pos, link->sta_profile_len);
	link->active = TRUE;
	return 0;
}
int parse_multi_link_ie(u8 *input, struct frag_ie_info *frag, struct ml_ie_info *info)
{
	u8 *pos, *end, *common_end, *buf = NULL;
	u16 len, ctrl, presence, plen, wire;
	if (!input || !info)
		return -1;
	NdisZeroMemory(info, sizeof(*info));
	if (input[0] != IE_WLAN_EXTENSION)
		return -1;
	if (frag && frag->is_frag) {
		if (frag->ie_len_defrag < 2) return -1;
		len = frag->ie_len_defrag - 2;
	} else
		len = input[1];
	if (len < 3 || input[2] != EID_EXT_EHT_MULTI_LINK)
		return -1;
	pos = input + 3;
	end = input + 2 + len;
	NdisMoveMemory(&ctrl, pos, 2);
	ctrl = le16_to_cpu(ctrl);
	pos += 2;
	info->type = GET_DOT11BE_ML_CTRL_TYPE(ctrl);
	info->pres_bmap = presence = GET_DOT11BE_ML_CTRL_PRESENCE_BMAP(ctrl);
	common_end = end;
#ifdef DOT11_BE_D12
	if (pos == end || !*pos || *pos > end - pos)
		goto bad;
	info->cmm_info_len = *pos;
	common_end = pos + *pos;
	pos++;
#endif
#define LG_ML_TAKE(dst, count) do { \
	if ((count) > common_end - pos) goto bad; \
	NdisMoveMemory((dst), pos, (count)); pos += (count); \
} while (0)
#define LG_ML_LE16(dst) do { LG_ML_TAKE(&(dst), 2); (dst) = le16_to_cpu(dst); } while (0)
	switch (info->type) {
	case ML_CTRL_TYPE_BASIC:
		LG_ML_TAKE(info->mld_addr, MAC_ADDR_LEN);
		if (presence & DOT11BE_ML_BASIC_PRESENCE_LINK_ID)
			LG_ML_TAKE(&info->link_id, 1);
		if (presence & DOT11BE_ML_BASIC_PRESENCE_BSS_CHG_CNT)
			LG_ML_TAKE(&info->bss_chg_cnt, 1);
		if (presence & DOT11BE_ML_BASIC_PRESENCE_MD_SYNC_DELAY)
			LG_ML_LE16(info->md_sync_delay);
		if (presence & DOT11BE_ML_BASIC_PRESENCE_EML_CAPS) {
			LG_ML_LE16(info->eml_caps);
			info->emlsr_sup = GET_DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_SUP(info->eml_caps);
		}
		if (presence & DOT11BE_ML_BASIC_PRESENCE_MLD_CAPS_N_OP)
			LG_ML_LE16(info->mld_caps);
		if (presence & DOT11BE_ML_BASIC_PRESENCE_MLD_ID)
			LG_ML_TAKE(&info->mld_id, 1);
		if (presence & DOT11BE_ML_BASIC_PRESENCE_EXT_MLD_CAPS_N_OP)
			LG_ML_LE16(info->ext_mld_caps);
		break;
	case ML_CTRL_TYPE_PROBE_REQ:
		if (presence & DOT11BE_ML_PROBE_REQ_PRESENCE_MLD_ID)
			LG_ML_TAKE(&info->mld_id, 1);
		break;
	case ML_CTRL_TYPE_RECONFIG:
		if (presence & DOT11BE_ML_RECONFIG_PRESENCE_MLD_MAC_ADDR)
			LG_ML_TAKE(info->mld_addr, MAC_ADDR_LEN);
		break;
	default:
		goto bad;
	}
#undef LG_ML_TAKE
#undef LG_ML_LE16
#ifdef DOT11_BE_D12
	pos = common_end;
#endif
	while (pos < end) {
		if (end - pos < 2 || pos[1] > end - pos - 2)
			goto bad;
		wire = pos[1];
		if (pos[0] == SUB_IE_MLD_PER_STA_PROFILE) {
			if (pos[1] == MAX_LEN_OF_IE_DATA) {
				if (!buf)
					os_alloc_mem(NULL, (UCHAR **)&buf, BSS_MNGR_MAX_PER_STA_PROFILE_LEN);
				if (!buf) goto bad;
				wire = defrag_multi_link_per_sta_profile(pos, end, buf, &plen);
				if (!wire || plen < 2 || parse_multi_link_per_sta_profile(buf, plen - 2, info))
					goto bad;
			} else if (parse_multi_link_per_sta_profile(pos, pos[1], info))
				goto bad;
		}
		pos += 2 + wire;
	}
	if (buf) os_free_mem(buf);
	return 0;
bad:
	if (buf) os_free_mem(buf);
	NdisZeroMemory(info, sizeof(*info));
	return -1;
}
static unsigned rng=0x6990;
static unsigned next(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
int main(int argc, char **argv){
 struct ml_ie_info *o=calloc(1,sizeof(*o));
 if(argc>1 && !strcmp(argv[1],"underflow")){
  u8 *p=calloc(1,5);p[1]=3;p[4]=4;o->type=ML_CTRL_TYPE_BASIC;
  int r=parse_multi_link_per_sta_profile(p,3,o);printf("underflow: %d\n",r);
  free(p);free(o);return r==-1?0:1;
 }
 if(argc>1 && !strcmp(argv[1],"fragment")){
  u8 *p=calloc(1,771), *b=calloc(1,576);u16 n=0;
  p[1]=255;p[257]=254;p[258]=255;p[514]=254;p[515]=255;
  int r=defrag_multi_link_per_sta_profile(p,p+771,b,&n);
  printf("fragment: %d/%u\n",r,n);free(p);free(b);free(o);return r==0?0:1;
 }
 /* Valid basic, all optional common fields incl. EML and extended MLD. */
 u8 valid[]={255,24,107,0xf0,0x07,20,1,2,3,4,5,6,1,9,0x34,0x12,0x81,0,0x67,0x45,0,0xab,0x89,0,0,0};
 valid[1]=21; /* extension+control+20-byte common */
 /* Build dynamically so the exact wire length is checked. */
 u8 frame[128]={255,0,107,0xf0,0x07};u8 *q=frame+5;
 *q++=18;for(int i=0;i<6;i++)*q++=i+1;
 *q++=1;*q++=9;*q++=0x34;*q++=0x12;*q++=0x81;*q++=0;
 *q++=0x67;*q++=0x45;*q++=0;*q++=0xab;*q++=0x89;
 *q++=0;*q++=6;*q++=1;*q++=0;*q++=1;*q++=0x11;*q++=0x22;*q++=0x33;
 frame[1]=q-frame-2;
 assert(parse_multi_link_ie(frame,NULL,o)==0);
 assert(o->eml_caps==0x81 && o->emlsr_sup==1 && o->ext_mld_caps==0x89ab);
 assert(o->link[1].active && o->link[1].sta_profile_len==3 && o->link[1].sta_profile[0]==0x11);
 /* Forward-compatible STA Info padding must not become profile bytes. */
 u8 extra[]={0,7,0,0,3,0xaa,0xbb,0x77,0x88};
 o->type=ML_CTRL_TYPE_BASIC;assert(!parse_multi_link_per_sta_profile(extra,7,o));
 assert(o->link[0].sta_profile_len==2 && o->link[0].sta_profile[0]==0x77);
 /* Valid fragmentation at 255 + 10 payload bytes. */
 u8 frag[269]={0},dst[576];u16 n;
 frag[1]=255;frag[257]=254;frag[258]=10;
 assert(defrag_multi_link_per_sta_profile(frag,frag+269,dst,&n)==267 && n==267);
 /* Truncated IEs and declared common-info overrun. */
 for(unsigned l=0;l<frame[1];l++){
  u8 *p=malloc(l+2);memcpy(p,frame,l+2);p[1]=l;
  parse_multi_link_ie(p,NULL,o);free(p);
 }
 for(unsigned i=0;i<150000;i++){
  unsigned n=next()%1200;u8 *p=malloc(n+2);for(unsigned j=0;j<n+2;j++)p[j]=next();
  memset(o,0,sizeof(*o));o->type=next()%3;
  parse_multi_link_per_sta_profile(p,n,o);
  u8 b[576];u16 bl;defrag_multi_link_per_sta_profile(p,p+n+2,b,&bl);
  p[0]=255;p[1]=n>255?255:n;if(n) p[2]=107;
  parse_multi_link_ie(p,NULL,o);
  free(p);
 }
 free(o);puts("PASS valid EML/MLD, per-STA, fragments, truncations, 150000 randomized inputs");return 0;
}
