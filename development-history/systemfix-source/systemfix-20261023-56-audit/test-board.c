#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <libubus.h>
#include <libubox/blobmsg_json.h>
static struct blob_buf blob;
static const char *fixture_path;
#define ubus_send_reply(ctx, req, head) ((void)0)
static int
rpc_luci_public_board_attr(struct blob_attr *attr, bool in_wlan, unsigned depth)
{
 const char *name = blobmsg_name(attr);
 struct blob_attr *child;
 void *cookie;
 int rem, type = blobmsg_type(attr), rc = 0;
 if ((!depth && !strcmp(name, "credentials")) ||
     !strcmp(name, "password") || !strcmp(name, "root_password_plain") ||
     !strcmp(name, "sae_password") || !strcmp(name, "token") ||
     (in_wlan && !strcmp(name, "key"))) return 0;
 if (depth >= 16) return -1;
 in_wlan = in_wlan || (!depth && !strcmp(name, "wlan"));
 if (type != BLOBMSG_TYPE_TABLE && type != BLOBMSG_TYPE_ARRAY)
  return blobmsg_add_blob(&blob, attr);
 cookie = type == BLOBMSG_TYPE_TABLE ? blobmsg_open_table(&blob, name) : blobmsg_open_array(&blob, name);
 if (!cookie) return -1;
 blobmsg_for_each_attr(child, attr, rem) {
  if (rpc_luci_public_board_attr(child, in_wlan, depth + 1)) {rc = -1; break;}
 }
 if (type == BLOBMSG_TYPE_TABLE) blobmsg_close_table(&blob, cookie);
 else blobmsg_close_array(&blob, cookie);
 return rc;
}

static int
rpc_luci_get_board_json(struct ubus_context *ctx, struct ubus_object *obj,
                        struct ubus_request_data *req, const char *method,
                        struct blob_attr *msg)
{
	struct blob_buf source = {0};
	struct blob_attr *attr;
	int rem, rc = UBUS_STATUS_OK;
	blob_buf_init(&blob, 0);
	blob_buf_init(&source, 0);
	if (!blobmsg_add_json_from_file(&source, fixture_path)) {
		blob_buf_free(&source);
		return UBUS_STATUS_UNKNOWN_ERROR;
	}
	blobmsg_for_each_attr(attr, source.head, rem) {
		if (rpc_luci_public_board_attr(attr, false, 0)) {rc = UBUS_STATUS_UNKNOWN_ERROR; break;}
	}
	blob_buf_free(&source);
	if (rc) return rc;

	ubus_send_reply(ctx, req, blob.head);
	return UBUS_STATUS_OK;
}

static int fds(void){DIR*d=opendir("/proc/self/fd");struct dirent*e;int n=0;if(!d)return -1;while((e=readdir(d)))n++;closedir(d);return n;}
int main(int argc,char**argv){if(argc!=2)return 2;fixture_path=argv[1];int before=fds();
 for(int i=0;i<100;i++){int rc=rpc_luci_get_board_json(NULL,NULL,NULL,NULL,NULL);if(rc){blob_buf_free(&blob);return 3;}
  if(!i){char*s=blobmsg_format_json(blob.head,true);if(!s)return 4;puts(s);free(s);}}
 int after=fds();blob_buf_free(&blob);return before==after?0:5;}
