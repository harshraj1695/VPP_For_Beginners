#include <stdio.h>
#include <stdlib.h>

#include <vlibmemory/api.h>
#include <vlibmemory/memory_client.h>
#include <vppinfra/types.h>


/*
| Method                | Works today            |
| --------------------- | ---------------------- |
| Standalone gcc client | ❌ crashes              |
| VAT-based client      | ✅ correct              |
| Python API            | ✅ easiest              |
| Go API                | ✅ common in production |

*/
int main()
{
    int rv;

    /* REQUIRED in modern VPP builds */
    vl_set_memory_root_path("/run/vpp");

    /* Map API shared memory */
    if (vl_client_api_map("/run/vpp/api.sock")) {
        printf("ERROR: unable to map API socket\n");
        return -1;
    }

    /* Connect to VPP */
    rv = vl_client_connect_to_vlib(
        "/run/vpp/api.sock",
        "c-client",
        32
    );

    if (rv != 0) {
        printf("ERROR: connection failed\n");
        return -1;
    }

    printf("Connected to VPP!\n");

    /* Clean disconnect */
    vl_client_disconnect_from_vlib();
    printf("Disconnected.\n");

    return 0;
}
