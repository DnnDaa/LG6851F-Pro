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
u16 defrag_multi_link_per_sta_profile(u8 *per_sta_pf, u8 *ml_ie_end, u8 *pf_buf, u16 *pf_len)
{
	u8 *pos;
	u8 *output_buf = pf_buf;
	u8 tag, length;
	u16 done_len = 0, result_len = 0;
	u16 ie_len_all = 2 + MAX_LEN_OF_IE_DATA;
	struct _EID_STRUCT *sub_eid = (struct _EID_STRUCT *)per_sta_pf;

	while ((per_sta_pf + ie_len_all) < ml_ie_end) {
		sub_eid = (struct _EID_STRUCT *)(per_sta_pf + ie_len_all);
		if (((UCHAR *)sub_eid + 2) > ml_ie_end)
			break;
		if (sub_eid->Eid == SUB_IE_MLD_FRAGMENT && sub_eid->Len > 0)
			ie_len_all += (2 + sub_eid->Len);
		else
			break;
	}

	sub_eid = (struct _EID_STRUCT *)per_sta_pf;

	/* leading subelement */
	result_len = done_len = 2 + sub_eid->Len;
	NdisMoveMemory(output_buf, per_sta_pf, result_len);

	MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_NOTICE,
		"[leading] buf_len=%d, eid->Len=%d, done_len=%d, result_len=%d\n",
		ie_len_all, sub_eid->Len, done_len, result_len);

	/* concatenate data field of Fragment subelements to leading subelement */
	while (done_len < ie_len_all) {
		MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
			"[LOG frag begin] buf_len=%d, done_len=%d, result_len=%d\n",
			ie_len_all, done_len, result_len);

		pos = (UINT8 *)per_sta_pf + done_len;
		sub_eid = (struct _EID_STRUCT *)pos;
		if (((UCHAR *)sub_eid + 2) > ml_ie_end)
			break;
		tag = sub_eid->Eid;
		length = sub_eid->Len;

		if (tag == SUB_IE_MLD_FRAGMENT && length > 0) {
			if (((UCHAR *)sub_eid + 2 + length) > ml_ie_end)
				break;
			/* shift 2 bytes for removal of tag and length fields of Fragment subIE */
			NdisMoveMemory(output_buf + result_len,	pos + 2, length);
			/* add length of fragmented portion (Data field) */
			result_len += length;

			/* goto next Fragment subelement */
			done_len += (2 + length);
		} else {
			MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
				"Error: the next element is not Fragment subelement or len(%d) <= 0\n",
				length);
			break;
		}
		MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
			"[LOG frag end] buf_len=%d, eid->Len=%d, done_len=%d, result_len=%d, tag=%d, length=%d\n",
			ie_len_all, sub_eid->Len, done_len, result_len, tag, length);
	}

	MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_NOTICE,
		"[end] buf_len=%d, done_len=%d, result_len=%d\n",
		ie_len_all, done_len, result_len);
	/* update new length of buffer (only reconstructed leading IE) */
	*pf_len = result_len;
	done_len = done_len - 2;
	return done_len;

}
int parse_multi_link_per_sta_profile(u8 *per_sta_pf, u16 pf_len, struct ml_ie_info *ml_info)
{
	struct _EID_STRUCT *eid = (struct _EID_STRUCT *)per_sta_pf;
	u8 link_id;
	struct ml_ie_link *ml_link;
	u8 *pos;
	u16 sta_ctrl;
	u16 total_len = 0;

	pos = per_sta_pf + 2;	/* Subelement ID and Length */

	hex_dump_with_cat_and_lvl("Per-STA:", (u8 *)eid, pf_len + 2,
		DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_DEBUG);

	if (pf_len >= sizeof(sta_ctrl)) {
		/* STA Control */
		total_len += sizeof(sta_ctrl);
		NdisMoveMemory((u8 *)&sta_ctrl, pos, sizeof(sta_ctrl));
		sta_ctrl = cpu_to_le16(sta_ctrl);
		pos += sizeof(sta_ctrl);

		link_id = GET_DOT11BE_ML_PER_STA_CTRL_LINK_ID(sta_ctrl);
		MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
				 "\t sta_ctrl(0x%x), link_id(%d)\n", sta_ctrl, link_id);

		if (link_id < BSS_MNGR_MAX_BAND_NUM) {
			ml_link = &ml_info->link[link_id];

			ml_link->active = TRUE;
			ml_link->sta_ctrl = sta_ctrl;

			if (ml_info->type == ML_CTRL_TYPE_BASIC) {
				/* STA Info Length */
				total_len += sizeof(ml_link->sta_info_len);
				if (total_len > pf_len)
					return -1;
				NdisMoveMemory(&ml_link->sta_info_len, pos, sizeof(ml_link->sta_info_len));
				pos += sizeof(ml_link->sta_info_len);
				MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
						 "\t sta_info_len(%d)\n", ml_link->sta_info_len);

				/* STA Info - MAC Address Present */
				if (GET_DOT11BE_ML_PER_STA_CTRL_MAC_ADDR_PRES(sta_ctrl)) {
					total_len += MAC_ADDR_LEN;
					if (total_len > pf_len)
						return -1;
					COPY_MAC_ADDR(ml_link->link_addr, pos);
					pos += MAC_ADDR_LEN;

					MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
							 "\t link_addr(%pM)\n", ml_link->link_addr);
				}

				/* STA Info - Beacon Interval Present */
				if (GET_DOT11BE_ML_PER_STA_CTRL_BCN_INT_PRES(sta_ctrl)) {
					u16 bcn_interval = 0;

					total_len += sizeof(bcn_interval);
					if (total_len > pf_len)
						return -1;
					NdisMoveMemory((u8 *)&bcn_interval, pos, sizeof(bcn_interval));
					ml_link->bcn_interval = cpu_to_le16(bcn_interval);
					pos += sizeof(bcn_interval);

					MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
							 "\t bcn_interval(%d)\n", bcn_interval);
				}


				/* STA Info - TSF Offset */
				if (GET_DOT11BE_ML_PER_STA_CTRL_TSF_OFFSET_PRES(sta_ctrl)) {
					u32 tsf_offset[2] = {0};

					total_len += sizeof(tsf_offset);
					if (total_len > pf_len)
						return -1;
					NdisMoveMemory((u8 *)&tsf_offset, pos, sizeof(tsf_offset));
					pos += sizeof(tsf_offset);

					MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
							 "\t tsf_offset(%08x %08x)\n", tsf_offset[0], tsf_offset[1]);
				}

				/* STA Info - DTIM Info Present */
				if (GET_DOT11BE_ML_PER_STA_CTRL_DTIM_PRES(sta_ctrl)) {
					total_len += (sizeof(ml_link->dtim_count) + sizeof(ml_link->dtim_period));
					if (total_len > pf_len)
						return -1;
					NdisMoveMemory(&ml_link->dtim_count, pos, sizeof(ml_link->dtim_count));
					pos += sizeof(ml_link->dtim_count);
					NdisMoveMemory(&ml_link->dtim_period, pos, sizeof(ml_link->dtim_period));
					pos += sizeof(ml_link->dtim_period);
					MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
							 "\t dtim_count(%d) period(%d)\n",
							 ml_link->dtim_count, ml_link->dtim_period);
				}

				/* STA Info - NSTR Link Pair Present */
				if (GET_DOT11BE_ML_PER_STA_CTRL_COMP_PROF(sta_ctrl) &&
					GET_DOT11BE_ML_PER_STA_CTRL_NSTR_LPR_PRES(sta_ctrl)) {
					if (GET_DOT11BE_ML_PER_STA_CTRL_NSTR_BMAP_SIZE(sta_ctrl) == 0) {
						u8 nstr_bmap0 = 0;

						total_len += sizeof(nstr_bmap0);
						if (total_len > pf_len)
							return -1;
						NdisMoveMemory(&nstr_bmap0, pos, sizeof(nstr_bmap0));
						ml_link->nstr_bmap = nstr_bmap0;
						pos += sizeof(nstr_bmap0);

						MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
								 "\t NSTR en, bmap0(0x%x)\n", ml_link->nstr_bmap);
					} else {
						u16 nstr_bmap1 = 0;

						total_len += sizeof(nstr_bmap1);
						if (total_len > pf_len)
							return -1;
						NdisMoveMemory(&nstr_bmap1, pos, sizeof(nstr_bmap1));
						ml_link->nstr_bmap = cpu_to_le16(nstr_bmap1);
						pos += sizeof(nstr_bmap1);

						MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
								 "\t NSTR en, bmap1(0x%x)\n", ml_link->nstr_bmap);
					}
				}

				/* STA Info - BSS Parameters Change Count */
				if (GET_DOT11BE_ML_PER_STA_CTRL_BSS_CHG_CNT_PRES(sta_ctrl)) {
					u8 bss_chg_cnt = 0;

					total_len += sizeof(bss_chg_cnt);
					if (total_len > pf_len)
						return -1;
					NdisMoveMemory(&bss_chg_cnt, pos, sizeof(bss_chg_cnt));
					pos++;

					MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
							 "\t bss_chg_cnt(%d)\n", bss_chg_cnt);
				}
			} else if (ml_info->type == ML_CTRL_TYPE_RECONFIG) {
				/* STA Info Length */
				total_len += sizeof(ml_link->sta_info_len);
				if (total_len > pf_len)
					return -1;
				NdisMoveMemory(&ml_link->sta_info_len, pos, sizeof(ml_link->sta_info_len));
				pos += sizeof(ml_link->sta_info_len);
				ml_link->link_id = link_id;
				ml_link->reconfig_type = GET_DOT11BE_ML_RECONFIG_PER_STA_CTRL_OP_UPD_TYPE(sta_ctrl);

				/*D3.0 reconfig type have no sta profile*/
				return 0;
			}

			/* STA Profile */
			ml_link->sta_profile_len = pf_len - sizeof(ml_link->sta_ctrl) - ml_link->sta_info_len;
			total_len += ml_link->sta_profile_len;
			if (total_len > pf_len)
				return -1;
			NdisMoveMemory(ml_link->sta_profile, pos, ml_link->sta_profile_len);
			// ml_link->sta_profile = pos;
		} else {
			MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_WARN, "\t Link(%d) not supported\n", link_id);
		}
	}

	return 0;
}
int parse_multi_link_ie(u8 *ml_ie, struct frag_ie_info *frag_info, struct ml_ie_info *ml_info)
{
	struct _EID_STRUCT *eid = (struct _EID_STRUCT *)ml_ie;
	u8 *pos = ml_ie;
	u8 *ml_ie_end = NULL, *pf_pos = NULL, *sub_ie_buf = NULL;
	u16 ml_ie_len = 0, sub_ie_len = 0;
	u16 sub_ie_total_len = 0;
	u16 mld_ctrl = 0;
	struct _EID_STRUCT *sub_eid = NULL;
	u16 present_bmap = 0;
	int total_len = 0;
	struct _EID_STRUCT *tmp_sub_eid = NULL;

	if (eid->Eid == IE_WLAN_EXTENSION) {
		ml_ie_len = (frag_info && frag_info->is_frag) ? (frag_info->ie_len_defrag - 2) : eid->Len;
		ml_ie_end = ml_ie + 2 + ml_ie_len;

		if (ml_ie_len && (eid->Octet[0] == EID_EXT_EHT_MULTI_LINK)) {
			if (pos + sizeof(struct _EID_STRUCT) > ml_ie_end) {
				MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
								"ML IE is abnormal\n");
				return -1;
			}
			pos += sizeof(struct _EID_STRUCT);

			/* Multi-Link Control */
			total_len += sizeof(mld_ctrl);
			if (total_len > ml_ie_len) {
				MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
								"ML IE is abnormal total_len= %d, ml_ie_len =%d\n", total_len, ml_ie_len);
				return -1;
			}
			NdisMoveMemory(&mld_ctrl, (u8 *)pos, sizeof(mld_ctrl));
			mld_ctrl = cpu_to_le16(mld_ctrl);
			pos += sizeof(mld_ctrl);

			ml_info->type = GET_DOT11BE_ML_CTRL_TYPE(mld_ctrl);
			ml_info->pres_bmap = present_bmap = GET_DOT11BE_ML_CTRL_PRESENCE_BMAP(mld_ctrl);

			MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
					"%s: type(%d), pres_bmap(0x%x), ml_ie_len(%d)\n",
					__func__, ml_info->type, ml_info->pres_bmap, ml_ie_len);

#ifdef DOT11_BE_D12
			/* Common Info - Common Info Length */
			total_len += sizeof(ml_info->cmm_info_len);
			if (total_len > ml_ie_len) {
				MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
								"ML IE is abnormal total_len= %d, ml_ie_len =%d\n", total_len, ml_ie_len);
				NdisZeroMemory(ml_info, sizeof(struct ml_ie_info));
				return -1;
			}
			NdisMoveMemory(&ml_info->cmm_info_len, pos, sizeof(ml_info->cmm_info_len));
			pos += sizeof(ml_info->cmm_info_len);

			MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
					 "\tcmm_info_len(%d)\n", ml_info->cmm_info_len);
#endif
			switch (ml_info->type) {
			case ML_CTRL_TYPE_BASIC:
				/* Common Info - MLD MAC Address */
				total_len += MAC_ADDR_LEN;
				if (total_len > ml_ie_len) {
					MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
								"ML IE is abnormal total_len= %d, ml_ie_len =%d\n", total_len, ml_ie_len);
					NdisZeroMemory(ml_info, sizeof(struct ml_ie_info));
					return -1;
				}
				COPY_MAC_ADDR(ml_info->mld_addr, pos);
				pos += MAC_ADDR_LEN;

				MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
						 "\tmld_addr(%pM)\n", ml_info->mld_addr);

				/* Common Info - Link ID Info */
				if (present_bmap & DOT11BE_ML_BASIC_PRESENCE_LINK_ID) {
					total_len += sizeof(ml_info->link_id);
					if (total_len > ml_ie_len) {
						MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
								"ML IE is abnormal total_len= %d, ml_ie_len =%d\n", total_len, ml_ie_len);
						NdisZeroMemory(ml_info, sizeof(struct ml_ie_info));
						return -1;
					}
					NdisMoveMemory(&ml_info->link_id, pos, sizeof(ml_info->link_id));
					pos += sizeof(ml_info->link_id);

					MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
							 "\tlink_id(%d)\n", ml_info->link_id);
				}

				/* Common Info - BSS Parameters Change Count */
				if (present_bmap & DOT11BE_ML_BASIC_PRESENCE_BSS_CHG_CNT) {
					total_len += sizeof(ml_info->bss_chg_cnt);
					if (total_len > ml_ie_len) {
						MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
								"ML IE is abnormal total_len= %d, ml_ie_len =%d\n", total_len, ml_ie_len);
						NdisZeroMemory(ml_info, sizeof(struct ml_ie_info));
						return -1;
					}
					NdisMoveMemory(&ml_info->bss_chg_cnt, pos, sizeof(ml_info->bss_chg_cnt));
					pos += sizeof(ml_info->bss_chg_cnt);

					MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
							 "\tbss_chg_cnt(%d)\n", ml_info->bss_chg_cnt);
				}

				/* Common Info - Medium Synchronization Delay Info */
				if (present_bmap & DOT11BE_ML_BASIC_PRESENCE_MD_SYNC_DELAY) {
					u16 md_sync_delay = 0;

					total_len += sizeof(md_sync_delay);
					if (total_len > ml_ie_len) {
						MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
								"ML IE is abnormal total_len= %d, ml_ie_len =%d\n", total_len, ml_ie_len);
						NdisZeroMemory(ml_info, sizeof(struct ml_ie_info));
						return -1;
					}
					NdisMoveMemory(&md_sync_delay, pos, sizeof(md_sync_delay));
					ml_info->md_sync_delay = cpu_to_le16(md_sync_delay);
					pos += sizeof(ml_info->md_sync_delay);

					MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
							 "\tmd_sync_delay(0x%x)\n", ml_info->md_sync_delay);
				}

				/* Common Info - EML Capabilities */
				if (present_bmap & DOT11BE_ML_BASIC_PRESENCE_EML_CAPS) {
					u16 eml_caps = 0;

					total_len += sizeof(eml_caps);
					if (total_len > ml_ie_len) {
						MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
								"ML IE is abnormal total_len= %d, ml_ie_len =%d\n", total_len, ml_ie_len);
						NdisZeroMemory(ml_info, sizeof(struct ml_ie_info));
						return -1;
					}
					NdisMoveMemory(&eml_caps, pos, sizeof(eml_caps));
					ml_info->eml_caps = cpu_to_le16(eml_caps);
					ml_info->emlsr_sup = GET_DOT11BE_ML_BASIC_CMM_EML_CAP_EMLSR_SUP(ml_info->eml_caps);
					pos += sizeof(eml_caps);

					MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
							 "\teml_caps(0x%x) \n", ml_info->eml_caps);
				}

				/* Common Info - MLD Capabilities and Operations */
				if (present_bmap & DOT11BE_ML_BASIC_PRESENCE_MLD_CAPS_N_OP) {
					u16 mld_caps = 0;

					total_len += sizeof(mld_caps);
					if (total_len > ml_ie_len) {
						MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
								"ML IE is abnormal total_len= %d, ml_ie_len =%d\n", total_len, ml_ie_len);
						NdisZeroMemory(ml_info, sizeof(struct ml_ie_info));
						return -1;
					}
					NdisMoveMemory(&mld_caps, pos, sizeof(mld_caps));
					ml_info->mld_caps = cpu_to_le16(mld_caps);
					pos += sizeof(mld_caps);

					MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
							 "\tmld_caps(0x%x)\n", ml_info->mld_caps);
				}

				/* Common Info - MLD ID */
				if (present_bmap & DOT11BE_ML_BASIC_PRESENCE_MLD_ID) {
					total_len += sizeof(ml_info->mld_id);
					if (total_len > ml_ie_len) {
						MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
								"ML IE is abnormal total_len= %d, ml_ie_len =%d\n", total_len, ml_ie_len);
						NdisZeroMemory(ml_info, sizeof(struct ml_ie_info));
						return -1;
					}
					NdisMoveMemory(&ml_info->mld_id, pos, sizeof(ml_info->mld_id));
					pos += sizeof(ml_info->mld_id);

					MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
							 "\tmld_id(%d)\n", ml_info->mld_id);
				}

				/* Common Info - Extended MLD Capabilities and Operations */
				if (present_bmap & DOT11BE_ML_BASIC_PRESENCE_EXT_MLD_CAPS_N_OP) {
					u16 ext_mld_caps = 0;

					NdisMoveMemory(&ext_mld_caps, pos, sizeof(ext_mld_caps));
					ml_info->ext_mld_caps = cpu_to_le16(ext_mld_caps);
					pos += sizeof(ext_mld_caps);

					MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
							 "\text_mld_caps(0x%x)\n", ml_info->ext_mld_caps);
				}
				break;

			case ML_CTRL_TYPE_PROBE_REQ:
				/* Common Info - MLD ID */
				if (present_bmap & DOT11BE_ML_PROBE_REQ_PRESENCE_MLD_ID) {
					total_len += sizeof(ml_info->mld_id);
					if (total_len > ml_ie_len) {
						MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
								"ML IE is abnormal total_len= %d, ml_ie_len =%d\n", total_len, ml_ie_len);
						NdisZeroMemory(ml_info, sizeof(struct ml_ie_info));
						return -1;
					}
					NdisMoveMemory(&ml_info->mld_id, pos, sizeof(ml_info->mld_id));
					pos += sizeof(ml_info->mld_id);

					MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
							 "\tmld_id(%d)\n", ml_info->mld_id);
				}
				break;

			case ML_CTRL_TYPE_RECONFIG:
				if (present_bmap & DOT11BE_ML_RECONFIG_PRESENCE_MLD_MAC_ADDR) {
					total_len += MAC_ADDR_LEN;
					if (total_len > ml_ie_len) {
						MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
									"ML IE is abnormal total_len= %d, ml_ie_len =%d\n", total_len, ml_ie_len);
						NdisZeroMemory(ml_info, sizeof(struct ml_ie_info));
						return -1;
					}
					COPY_MAC_ADDR(ml_info->mld_addr, pos);
					pos += MAC_ADDR_LEN;
				}
				break;

			default:
				MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
					"Error: unknown ML IE type (%u)\n", ml_info->type);
			}

			hex_dump_with_cat_and_lvl("ML IE:", (u8 *)ml_ie, (pos - ml_ie),
				DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_DEBUG);

			/* Link Info */
			sub_eid = (struct _EID_STRUCT *)pos;

			os_alloc_mem(NULL, (UCHAR **)&sub_ie_buf, BSS_MNGR_MAX_PER_STA_PROFILE_LEN);
			if (!sub_ie_buf) {
				MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
					"failed to allocate memory for parsing ML IE\n");
				NdisZeroMemory(ml_info, sizeof(struct ml_ie_info));
				return -1;
			}
			while (((u8 *)sub_eid) < ml_ie_end) {
				MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_INFO,
						"  Per-STA, eid(%d) len=(%d)\n", sub_eid->Eid, sub_eid->Len);
				sub_ie_len = sub_eid->Len;
				sub_ie_total_len = sub_ie_len;

				switch (sub_eid->Eid) {
				case SUB_IE_MLD_PER_STA_PROFILE:
					if (sub_eid->Len == MAX_LEN_OF_IE_DATA) {
						/* Per-STA Profile defragmentation */
						NdisZeroMemory(sub_ie_buf, BSS_MNGR_MAX_PER_STA_PROFILE_LEN);
						total_len = 0;
						tmp_sub_eid = sub_eid;

						while (((u8 *)tmp_sub_eid + total_len) < ml_ie_end) {
							tmp_sub_eid = sub_eid + total_len;
							if (tmp_sub_eid->Eid == SUB_IE_MLD_FRAGMENT && tmp_sub_eid->Len > 0)
								total_len += (2 + tmp_sub_eid->Len);
							else
								break;
						}
						if (total_len > BSS_MNGR_MAX_PER_STA_PROFILE_LEN) {
							MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
								"  Per-STA-PROFILE exceed the maxmum length:%d\n", total_len);
							if (sub_ie_buf)
								os_free_mem(sub_ie_buf);
							NdisZeroMemory(ml_info, sizeof(struct ml_ie_info));
							return -1;
						}

						sub_ie_total_len = defrag_multi_link_per_sta_profile(
							(u8 *)sub_eid, ml_ie_end, sub_ie_buf, &sub_ie_len);

						pf_pos = sub_ie_buf;
						sub_ie_len -= 2;
					} else {
						pf_pos = (u8 *)sub_eid;
						sub_ie_len = sub_eid->Len;
						sub_ie_total_len = sub_ie_len;
					}

					if (parse_multi_link_per_sta_profile(pf_pos, sub_ie_len, ml_info) < 0) {
						MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
								"prase multi link per sta profile fail\n");
						if (sub_ie_buf)
							os_free_mem(sub_ie_buf);
						NdisZeroMemory(ml_info, sizeof(struct ml_ie_info));
						return -1;
					}
					break;
//				case SUB_IE_MLD_VENDOR_SPECIFIC:
//					break;
				default:
					MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_WARN,
							"  unsupport sub_eid(%d)\n", sub_eid->Eid);
					break;
				}
				sub_eid = (PEID_STRUCT)((u8 *)sub_eid + 2 + sub_ie_total_len);
			}

			if (sub_ie_buf)
				os_free_mem(sub_ie_buf);

		} else {
			MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
					 "%s: incorrect eid_ext(%d)\n", __func__, eid->Octet[0]);
			return -1;
		}
	} else {
		MTWF_DBG(NULL, DBG_CAT_MLO, CATMLO_BMGR, DBG_LVL_ERROR,
				 "%s: incorrect eid(%d)\n", __func__, eid->Eid);
		return -1;
	}

	return 0; //pos;
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
