#ifndef MTK_VENDOR618_COMPAT_H
#define MTK_VENDOR618_COMPAT_H
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/page_frag_cache.h>
/* Preserve the removed strlcpy API's return value (full source length). */
static inline size_t mtk_strlcpy(char *dst, const char *src, size_t size)
{
 size_t len = strlen(src);
 if (size) {
  size_t copied = len < size - 1 ? len : size - 1;
  memcpy(dst, src, copied);
  dst[copied] = '\0';
 }
 return len;
}
#endif
