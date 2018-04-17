#ifndef _HEADER_H_


#define DBG
#ifdef DBG
#define DBG_MSG printf
#endif


#define SERVICE_PORT	13469	/* hard-coded port number */

typedef enum sys_error_e
{
    SYS_SUCCESS = 0,
    SYS_FAILURE = -1,
    SYS_ERROR_MAX
}sys_error;

typedef enum system_state_e
{
    SYSTEM_STATE_IDLE = 0,
    SYSTEM_STATE_RESUME,
    SYSTEM_STATE_SUSPEND,
    SYSTEM_STATE_BUSY,
    SYSTEM_STATE_CONNECTED,
    SYSTEM_STATE_DISCONNECTED,
    SYSTEM_STATE_MAX
}system_state;


typedef enum system_input_e
{
    SYSTEM_INPUT_BAT_LEVEL_LOW = 0,
    SYSTEM_INPUT_BAT_LEVEL_UP,
    SYSTEM_INPUT_BUTTON_PRESSED,
    SYSTEM_INPUT_BUTTON_RELEASED,
    SYSTEM_INPUT_MAX
}system_input;


typedef struct state_machine state_machine_t;
typedef void ( *state_proc )( state_machine_t *sm, system_input input );


struct state_machine
{
    state_proc cur_state;
};

sys_error init_machine( state_machine_t* machine );
state_machine_t* device_instance_get( void );
sys_error machine_state_transit( state_machine_t* machine, system_input input );
sys_error machine_client( state_machine_t* machine );
sys_error machine_server( state_machine_t* machine );
#endif
