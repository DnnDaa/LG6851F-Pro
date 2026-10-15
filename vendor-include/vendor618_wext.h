/* SPDX-License-Identifier: GPL-2.0 */
/* WEXT serialization helpers from Linux 6.18 net/wireless/wext-core.c. */
#include <net/iw_handler.h>
static inline char *mtk_iwe_stream_add_event(struct iw_request_info *info, char *stream,
			   char *ends, struct iw_event *iwe, int event_len)
{
	int lcp_len = iwe_stream_lcp_len(info);

	event_len = iwe_stream_event_len_adjust(info, event_len);

	/* Check if it's possible */
	if (likely((stream + event_len) < ends)) {
		iwe->len = event_len;
		/* Beware of alignment issues on 64 bits */
		memcpy(stream, (char *) iwe, IW_EV_LCP_PK_LEN);
		memcpy(stream + lcp_len, &iwe->u,
		       event_len - lcp_len);
		stream += event_len;
	}

	return stream;
}
static inline char *mtk_iwe_stream_add_point(struct iw_request_info *info, char *stream,
			   char *ends, struct iw_event *iwe, char *extra)
{
	int event_len = iwe_stream_point_len(info) + iwe->u.data.length;
	int point_len = iwe_stream_point_len(info);
	int lcp_len   = iwe_stream_lcp_len(info);

	/* Check if it's possible */
	if (likely((stream + event_len) < ends)) {
		iwe->len = event_len;
		memcpy(stream, (char *) iwe, IW_EV_LCP_PK_LEN);
		memcpy(stream + lcp_len,
		       ((char *) &iwe->u) + IW_EV_POINT_OFF,
		       IW_EV_POINT_PK_LEN - IW_EV_LCP_PK_LEN);
		if (iwe->u.data.length && extra)
			memcpy(stream + point_len, extra, iwe->u.data.length);
		stream += event_len;
	}

	return stream;
}
#define iwe_stream_add_event mtk_iwe_stream_add_event
#define iwe_stream_add_point mtk_iwe_stream_add_point
