#include "helper.h"

int get_subvol_id(int fd, char *id, u64 *id_ret)
{
	struct btrfs_ioctl_search_args args;
	struct btrfs_ioctl_search_key *sk = &args.key;
	struct btrfs_ioctl_search_header sh;
	struct btrfs_root_ref *ref;
	char *name;
	int namelen;
	int ret;
	int off;
	int i;

	sk->tree_id = 1;
	sk->max_type = sk->min_type = BTRFS_ROOT_BACKREF_KEY;
	sk->min_objectid = BTRFS_FIRST_FREE_OBJECTID;
	sk->max_objectid = BTRFS_LAST_FREE_OBJECTID;
	sk->max_offset = (unsigned long long)-1;
	sk->max_transid = (unsigned long long)-1;
	sk->nr_items = 4096;

	while (1) {
		ret = ioctl(fd, BTRFS_IOC_TREE_SEARCH, &args);
		if (ret < 0)
			return ret;

		if (sk->nr_items == 0)
			break;

		off = 0;
		for (i = 0; i < sk->nr_items; i++) {
			memcpy(&sh, args.buf + off, sizeof(sh));
			off += sizeof(sh);

			/* debug */

			if (sh.type == BTRFS_ROOT_BACKREF_KEY) {
				ref = (struct btrfs_root_ref *)(args.buf + off);
				name = (char *)(ref + 1);
				namelen = btrfs_stack_root_ref_name_len(ref);

		//		fprintf(stdout, "search (%llu %u %llu) len %d id %s\n", (unsigned long long)sh.objectid, sh.type, (unsigned long long)sh.offset, (int)sh.len, id);

				if (!strncmp(name, id, namelen)) {
					if (!id_ret)
						return -EINVAL;
					else
						*id_ret = sh.objectid;
					return 0;
				}
			}

			off += sh.len;
			sk->min_objectid = sh.objectid;
			sk->min_offset = sh.offset;
		}
		sk->nr_items = 4096;
		sk->min_offset++;
		if (!sk->min_offset) /* overflow */
			sk->min_offset++;
		else
			continue;

		if (sk->min_objectid > sk->max_objectid)
			break;
	}

	return 0;
}
