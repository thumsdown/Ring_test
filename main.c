#include <stdio.h>
#include <unistd.h>
#include <myRing.h>


static void show_user_options( void )
{
    /** get user input */
    DBG_MSG( "Enter a  for Charge Complete event\n" );
    DBG_MSG( "Enter b  for System power low event\n" );
    DBG_MSG( "Enter c  for Button Press Action\n" );
    DBG_MSG( "Enter d  for Button release Action\n" );
    DBG_MSG( "Enter e for exiting the machine\n");
}

static system_input system_event_to_machine_input( char event )
{
    system_input input;

    if( event == 'a' )
    {
        input = SYSTEM_INPUT_BAT_LEVEL_UP;
    }
    else if ( event == 'b' )
    {
        input = SYSTEM_INPUT_BAT_LEVEL_LOW;
    }
    else if ( event == 'c' )
    {
        input = SYSTEM_INPUT_BUTTON_PRESSED;
    }
    else if ( event == 'd' )
    {
        input = SYSTEM_INPUT_BUTTON_RELEASED;
    }
    else
    {
        input = SYSTEM_INPUT_MAX;
    }

    return input;
}

int main( int argc, char* argv[] )
{

    char event;
    /** initialize myDevice task */
    state_machine_t *machine = NULL;
    system_input input;

    /** Create a task for machine */
    machine = device_instance_get();
    if( machine == NULL )
    {
        DBG_MSG( "Device memory issue\n" );
        return SYS_FAILURE;
    }

    init_machine( machine );

    /** Assume the battery level is enough and put the state
        to Resume state so that the machine is functional*/
    machine_state_transit( machine, SYSTEM_INPUT_BAT_LEVEL_UP );

    while( 1 )
    {
        show_user_options();
        while (( event = getchar()) != '\n' && event != EOF );
        DBG_MSG("Enter choice:");
        event = getchar();
        if ( event == 'e' )
        {
            DBG_MSG("GOOD BYE! HAVE A NICE DAY!!\n");
            break;
        }
        /** Translate Event into a valid input */
        input = system_event_to_machine_input( event );
        if( input == SYSTEM_INPUT_MAX )
        {
            DBG_MSG("Continue\n");
            continue;
        }
        machine_state_transit( machine, input );
    }

    return SYS_SUCCESS;
}
