

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <myRing.h>
#include <string.h>	/* for strlen */
#include <netdb.h>      /* for gethostbyname() */
#include <sys/socket.h>
#include <netinet/in.h>


static system_input _g_input_event = SYSTEM_INPUT_MAX;
int32_t fd;

static void system_idle( state_machine_t *sm, system_input input );
static void system_resume( state_machine_t *sm, system_input input );
static void system_suspend( state_machine_t *sm, system_input input );
static void system_busy( state_machine_t *sm, system_input input );
static void system_connected ( state_machine_t *sm, system_input input );
static void system_disconnected( state_machine_t *sm, system_input input );
static void machine_controller( void* machine );
static void notify_current_state( system_state state );
static void notify_invalid_input( system_input input );
static sys_error system_connect_server( state_machine_t *sm );
static sys_error system_disconnect_server( state_machine_t *sm );
static sys_error system_white_led_enable( state_machine_t *sm );
static sys_error system_white_led_disable( state_machine_t *sm );
static sys_error system_red_led_enable( state_machine_t *sm );
static sys_error system_red_led_disable( state_machine_t *sm );

char* get_system_state_str[ SYSTEM_STATE_MAX + 1 ] = {
                                                       "SYSTEM_STATE_IDLE",
                                                       "SYSTEM_STATE_RESUME",
                                                       "SYSTEM_STATE_SUSPEND",
                                                       "SYSTEM_STATE_BUSY",
                                                       "SYSTEM_STATE_CONNECTED",
                                                       "SYSTEM_STATE_DISCONNECTED",
                                                       "SYSTEM_STATE_MAX"
                                                     };

char* get_system_input_str[ SYSTEM_INPUT_MAX + 1 ] = {
                                                        "SYSTEM_INPUT_BAT_LEVEL_LOW",
                                                        "SYSTEM_INPUT_BAT_LEVEL_UP",
                                                        "SYSTEM_INPUT_BUTTON_PRESSED",
                                                        "SYSTEM_INPUT_BUTTON_RELEASED",
                                                        "SYSTEM_INPUT_MAX"

                                                      };

static void notify_invalid_input( system_input input )
{
    DBG_MSG( "Invalid Input for the current state = %s\n", get_system_input_str[ input ] );
}

static void notify_current_state( system_state state )
{
    DBG_MSG( "Current state for the system = %s\n", get_system_state_str[ state ] );
}

static void system_idle( state_machine_t *sm, system_input input )
{
    switch( input )
    {
        case SYSTEM_INPUT_BAT_LEVEL_LOW:
            sm->cur_state = system_suspend;
            notify_current_state(SYSTEM_STATE_SUSPEND);
            break;
        case SYSTEM_INPUT_BAT_LEVEL_UP:
            sm->cur_state = system_resume;
            notify_current_state(SYSTEM_STATE_RESUME);
            break;
        case SYSTEM_INPUT_BUTTON_PRESSED:
            if( sm->cur_state == system_resume )
            {
                sm->cur_state = system_busy;
             notify_current_state(SYSTEM_STATE_BUSY);
            }
            else
            {
                notify_invalid_input( input );
            }
            break;
        case SYSTEM_INPUT_BUTTON_RELEASED:
        default:
            notify_invalid_input( input );
            sm->cur_state = system_idle;
            break;
    }
}

static void system_resume( state_machine_t *sm, system_input input )
{

    switch( input )
    {
        case SYSTEM_INPUT_BAT_LEVEL_LOW:
            sm->cur_state = system_suspend;
            notify_current_state(SYSTEM_STATE_SUSPEND);
            break;
        case SYSTEM_INPUT_BUTTON_PRESSED:
            sm->cur_state = system_busy;
            notify_current_state(SYSTEM_STATE_BUSY);
            break;
        case SYSTEM_INPUT_BAT_LEVEL_UP:
        case SYSTEM_INPUT_BUTTON_RELEASED:
        default:
            notify_invalid_input( input );
            sm->cur_state = system_idle;
            break;
    }
}

static void system_suspend( state_machine_t *sm, system_input input )
{

    switch( input )
    {
        case SYSTEM_INPUT_BAT_LEVEL_UP:
            sm->cur_state = system_resume;
            notify_current_state(SYSTEM_STATE_RESUME);
            break;
        case SYSTEM_INPUT_BAT_LEVEL_LOW:
        case SYSTEM_INPUT_BUTTON_PRESSED:
        case SYSTEM_INPUT_BUTTON_RELEASED:
        default:
            notify_invalid_input( input );
            sm->cur_state = system_idle;
            break;
    }
}

static void system_busy( state_machine_t *sm, system_input input )
{

    switch( input )
    {
        case SYSTEM_INPUT_BAT_LEVEL_LOW:
            sm->cur_state = system_suspend;
            notify_current_state(SYSTEM_STATE_SUSPEND);
            break;
        case SYSTEM_INPUT_BUTTON_PRESSED:
            sm->cur_state = system_connected;
            notify_current_state(SYSTEM_STATE_CONNECTED);
            break;
        case SYSTEM_INPUT_BUTTON_RELEASED:
            sm->cur_state = system_disconnected;
            notify_current_state(SYSTEM_STATE_DISCONNECTED);
        case SYSTEM_INPUT_BAT_LEVEL_UP:
        default:
            notify_invalid_input( input );
            sm->cur_state = system_idle;
            break;
    }
}

static void system_connected ( state_machine_t *sm, system_input input )
{

    switch( input )
    {
        case SYSTEM_INPUT_BAT_LEVEL_LOW:
            sm->cur_state = system_busy;
            notify_current_state(SYSTEM_STATE_BUSY);
            break;
        case SYSTEM_INPUT_BUTTON_RELEASED:
            sm->cur_state = system_busy;
            notify_current_state(SYSTEM_STATE_BUSY);
            break;
        case SYSTEM_INPUT_BAT_LEVEL_UP:
        case SYSTEM_INPUT_BUTTON_PRESSED:
        default:
            notify_invalid_input( input );
            sm->cur_state = system_idle;
            break;
    }
}

static void system_disconnected( state_machine_t *sm, system_input input )
{

    switch( input )
    {
        case SYSTEM_INPUT_BAT_LEVEL_LOW:
            sm->cur_state = system_suspend;
            notify_current_state(SYSTEM_STATE_SUSPEND);
            break;
        case SYSTEM_INPUT_BUTTON_PRESSED:
            sm->cur_state = system_connected;
            notify_current_state(SYSTEM_STATE_CONNECTED);
            break;
        case SYSTEM_INPUT_BUTTON_RELEASED:
            sm->cur_state = system_resume;
            notify_current_state(SYSTEM_STATE_RESUME);
        case SYSTEM_INPUT_BAT_LEVEL_UP:        
        default:
            notify_invalid_input( input );
            sm->cur_state = system_idle;
            break;
    }
}

state_machine_t* device_instance_get( void )
{
    state_machine_t *result;

    /* Allocate storage for the object. */
    result = malloc( sizeof( state_machine_t ) );
    if( NULL == result )
    {
        DBG_MSG( "Cannot create Machine instance\n" );
        return NULL;
    }

    /* Initialize the fields (in particular, current state). */
    result->cur_state = system_idle;

    return result;
}

static sys_error system_connect_server( state_machine_t *sm )
{
   struct hostent *hp;	/* host information */
   uint32_t alen;	/* address length when we get the port number */
   struct sockaddr_in myaddr;	/* our address */
   struct sockaddr_in servaddr;	/* server address */
   uint32_t port = SERVICE_PORT;
   static int32_t counter = 0;
   int32_t counter_rec = 0;
   char buf_send[3] = {'\0', '\0', '\0'};
   char buf_rec[3] = {'\0', '\0', '\0'};
   socklen_t slen = sizeof(servaddr);
   char* host = "test.ring.com";
   DBG_MSG("conn(host=\"%s\", port=\"%d\")\n", host, port);

   /* get a udp socket */
   /* We do this as we did it for the server */
   /* request the Internet address protocol */
   /* and a reliable 2-way byte stream */

   if ( ( fd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 )
   {
       perror("cannot create socket");
       return SYS_FAILURE;
   }

   /* bind to an arbitrary return address */
   /* because this is the client side, we don't care about the */
   /* address since no application will connect here  --- */
   /* INADDR_ANY is the IP address and 0 is the socket */
   /* htonl converts a long integer (e.g. address) to a network */
   /* representation (agreed-upon byte ordering */
   memset( (char *)&myaddr, 0, sizeof(myaddr) );
   myaddr.sin_family = AF_INET;
   myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
   myaddr.sin_port = htons(port);

   if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
       perror("bind failed");
       return SYS_FAILURE;
   }
    /** Connected state */

   /** Change the state to Connected */
   sm->cur_state( sm, SYSTEM_INPUT_BUTTON_PRESSED );
   /** Keep transaction on with server till Button Pressed */

   /* this part is for debugging only - get the port # that the operating */
   /* system allocated for us. */
   alen = sizeof(myaddr);
   if (getsockname(fd, (struct sockaddr *)&myaddr, &alen) < 0)
   {
       perror("getsockname failed");
       return SYS_FAILURE;
   }
   DBG_MSG("local port number = %d\n", ntohs(myaddr.sin_port));

   /* fill in the server's address and data */
   /* htons() converts a short integer to a network representation */

   memset((char*)&servaddr, 0, sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_port = htons(port);

   /* look up the address of the server given its name */
   hp = gethostbyname(host);
   if (!hp) {
       fprintf(stderr, "could not obtain address of %s\n", host);
       return SYS_FAILURE;
   }

   /* put the host's address into the server address structure */
   memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);

   while( _g_input_event == SYSTEM_INPUT_BUTTON_PRESSED )
   {
       DBG_MSG("Sending packet %d\n", counter);

       sprintf( buf_send, "%2d", counter );
       if (sendto(fd, buf_send, 2, 0, (struct sockaddr *)&servaddr, slen)==-1)
       {
           perror("sendto");
           break;
       }
       /* now receive an acknowledgement from the server */
       if ( recvfrom(fd, buf_rec, 2, 0, (struct sockaddr *)&servaddr, &slen) == 2 )
       {
          /* expect a printable string - terminate it */
           DBG_MSG("received message:%s\n", buf_rec);
           sscanf(buf_rec, "%2d", &counter_rec );
           if( counter != counter_rec )
           {
               sleep(500);
               continue;
           }
           else
           {
               sleep(1000);
               counter++;
           }

       }
   }
   counter = 0;
   return SYS_SUCCESS;
}

static sys_error system_disconnect_server( state_machine_t *sm )
{

    close(fd);
    sm->cur_state(sm, SYSTEM_STATE_DISCONNECTED);
    return SYS_SUCCESS;
}

static sys_error system_white_led_enable( state_machine_t *sm )
{
    DBG_MSG( "Enabling White LED---> White LED is Glowing\n" );
    return SYS_SUCCESS;
}

static sys_error system_white_led_disable( state_machine_t *sm )
{
    DBG_MSG( "Disabling White LED---> White LED Disabled\n" );
    return SYS_SUCCESS;
}

static sys_error system_red_led_enable( state_machine_t *sm )
{
    /** Led is configured to blink at 2hz and 25% duty cycle */
    DBG_MSG("Enabling Red LED---> Red LED On \n");
    //Frequency 2hz and duty cycle means Led Would be up for .25 seconds in one second time interval
    sleep(1000);
    DBG_MSG("Disabling Red LED--->Red LED Off");
    return SYS_SUCCESS;
}

static sys_error system_red_led_disable( state_machine_t *sm )
{
    DBG_MSG("Disabling Red LED---> Red LED Disabled\n");
    return SYS_SUCCESS;
}

sys_error machine_state_transit( state_machine_t* sm, system_input input )
{
   if( sm == NULL )
   {
       DBG_MSG( "Invalid Machine state transit\n" );
       return SYS_FAILURE;
   }
   if( input != _g_input_event )
   {
        _g_input_event = input;
   }

   return SYS_SUCCESS;
}

static void machine_controller( void* machine )
{
    state_machine_t* cur_machine = (state_machine_t*) machine;
    system_input current_input_event = SYSTEM_INPUT_MAX;

    while(1)
    {
        if( current_input_event != _g_input_event )
        {
            /** Check if the system is recovering from suspended state stop Red LED */
            if( current_input_event == SYSTEM_INPUT_BAT_LEVEL_LOW
                    && _g_input_event == SYSTEM_INPUT_BAT_LEVEL_UP )
            {
                system_red_led_disable( machine );
            }

            /** update the state */
            current_input_event = _g_input_event;
            /** Transit to new intermediatery state */
            cur_machine->cur_state( cur_machine, current_input_event );

            /** Proceed with input button pressed to connected state*/
            if( current_input_event == SYSTEM_INPUT_BUTTON_PRESSED &&
                    cur_machine->cur_state == system_busy )
            {
                /** Glow The LED */
                system_white_led_enable( machine );
                /** Go for connection with Ring server */
                /** internall change the status of the state to connected */
                system_connect_server( machine );
            }
            if( ( current_input_event == SYSTEM_INPUT_BUTTON_RELEASED ||
                  current_input_event == SYSTEM_INPUT_BAT_LEVEL_LOW )&&
                    cur_machine->cur_state == system_busy )
            {
                /** Stop The White Led */
                system_white_led_disable( machine );
                /** Disconnect the ring server */
                system_disconnect_server( machine );
                if( current_input_event == SYSTEM_INPUT_BAT_LEVEL_LOW )
                {
                    /** Enable Red LED */
                    system_red_led_enable( machine );
                }
                cur_machine->cur_state( cur_machine, current_input_event );
            }

        }
        //Add a delay if system permits
    }
}

sys_error init_machine( state_machine_t* sm )
{
    pthread_t machine_task;

    if( pthread_create( &machine_task, NULL, (void*)&machine_controller, (void*)sm ) )
    {
        DBG_MSG( "Machine controller task create failure\n" );
        return SYS_FAILURE;
    }

    return SYS_SUCCESS;
}
