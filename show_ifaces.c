#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <vppinfra/clib.h>
#include <vlibapi/api.h>
#include <vlibmemory/api.h>
#include <vpp/api/vpe_msg_enum.h>
#include <vpp/api/vpe_all_api_h.h>

static void
vl_api_sw_interface_details_t_handler(vl_api_sw_interface_details_t *mp)
{
    char name[64];
    memcpy(name, mp->interface_name, sizeof(mp->interface_name));
    name[sizeof(mp->interface_name)-1] = 0;

    printf("Interface: %s index=%u\n",
           name,
           ntohl(mp->sw_if_index));
}

int main()
{
    if (vl_client_connect_to_vlib("/run/vpp/api.sock", "c-client", 32) < 0) {
        printf("Failed to connect to VPP\n");
        return -1;
    }

    printf("Connected to VPP\n");

    vl_msg_api_set_handlers(
        VL_API_SW_INTERFACE_DETAILS,
        "sw_interface_details",
        vl_api_sw_interface_details_t_handler,
        NULL, NULL, NULL);

    vl_api_sw_interface_dump_t *mp;
    mp = vl_msg_api_alloc(sizeof(*mp));
    memset(mp, 0, sizeof(*mp));

    mp->_vl_msg_id = htons(VL_API_SW_INTERFACE_DUMP);
    mp->client_index = vl_api_get_client_index();

    vl_msg_api_send_shmem(vl_input_queue, (u8 *)&mp);

    sleep(1);
    vl_client_disconnect_from_vlib();
    return 0;
}
