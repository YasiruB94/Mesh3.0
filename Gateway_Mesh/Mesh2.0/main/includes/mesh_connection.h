#include "utilities.h"
#include "esp_mesh.h"

extern EventGroupHandle_t ethernet_event_group;
extern int ethernet_bit;

extern void MeshConnection_MeshSetup();

typedef struct
{
    size_t last_num;
    uint8_t *last_list;
    size_t change_num;
    uint8_t *change_list;
} node_list_t;
