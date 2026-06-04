#include "ns_common.h"

#include <stdio.h>

#include "sfs_client.h"

int32_t sfs_example_parse_ns_name(int argc,
                                  char *argv[],
                                  const char *usage,
                                  const char **ns_name_out)
{
    if (argc < 2 || ns_name_out == NULL) {
        fprintf(stderr, "Usage: %s\n", usage);
        return InvalidArgument;
    }

    *ns_name_out = argv[1];
    printf("Namespace '%s'\n", *ns_name_out);
    return 0;
}
