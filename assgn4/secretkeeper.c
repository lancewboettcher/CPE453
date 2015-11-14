#include <minix/drivers.h>
#include <minix/driver.h>
#include <stdio.h>
#include <stdlib.h>
#include <minix/ds.h>
#include "secretkeeper.h"
#include <minix/const.h> 
#include <sys/ucred.h>
#include <minix/endpoint.h>
#include <sys/select.h>
#include <minix/const.h>

#include <unistd.h>

#define O_WRONLY 2
#define O_RDONLY 4
#define O_RDWR 6   

#define MESSAGE_SIZE 8192
#define NO_OWNER -1 

/*
 * Function prototypes for the hello driver.
 */
FORWARD _PROTOTYPE( char * hello_name,   (void) );
FORWARD _PROTOTYPE( int hello_open,      (struct driver *d, message *m) );
FORWARD _PROTOTYPE( int hello_close,     (struct driver *d, message *m) );
FORWARD _PROTOTYPE( struct device * hello_prepare, (int device) );
FORWARD _PROTOTYPE( int hello_transfer,  (endpoint_t endpt, int opcode,
                                          u64_t position, iovec_t *iov,
                                          unsigned nr_req,
                                          endpoint_t user_endpt));
FORWARD _PROTOTYPE( void hello_geometry, (struct partition *entry) );

/* SEF functions and variables. */
FORWARD _PROTOTYPE( void sef_local_startup, (void) );
FORWARD _PROTOTYPE( int sef_cb_init, (int type, sef_init_info_t *info) );
FORWARD _PROTOTYPE( int sef_cb_lu_state_save, (int) );
FORWARD _PROTOTYPE( int lu_state_restore, (void) );

/* Secret Prototypes */ 
void clearSecret();
void initSecret();

/* Entry points to the hello driver. */
PRIVATE struct driver hello_tab =
{
    hello_name,
    hello_open,
    hello_close,
    nop_ioctl,
    hello_prepare,
    hello_transfer,
    nop_cleanup,
    hello_geometry,
    nop_alarm,
    nop_cancel,
    nop_select,
    nop_ioctl,
    do_nop,
};

/** Represents the /dev/hello device. */
PRIVATE struct device hello_device;

/* Secret global variables */
static uid_t secretOwner;
static char secretMessage[MESSAGE_SIZE]; 
static int secretEmpty;
static int secretNumFileDescriptors;

/** State variable to count the number of times the device has been opened. */
PRIVATE int open_counter;

PRIVATE char * hello_name(void)
{
    printf("hello_name()\n");
    return "hello";
}

PRIVATE int hello_open(d, m)
    struct driver *d;
    message *m;
{
 /*  printf("hello_open(). Called %d time(s).\n", ++open_counter);
*/
    struct ucred callerCreds; 
    int openFlags;
    uid_t uid = getuid();

    printf("Open CALLED\n");

    /* Get the caller's credentials */ 
 /*   if (getnucred(m->PROC_NR, &callerCreds)) {
    if (getnucred(-1, &callerCreds)) {
        fprintf(stderr, "Open: getnucred error \n");
        exit(-1);
    }   
*/
    /* Get the flags given to open() */ 
    openFlags = m->COUNT;
    printf("Open Flags: %d\n", openFlags);
    /* Check to make sure open() flags are either read or write, not both */ 
    if (openFlags != O_WRONLY && openFlags != O_RDONLY) {
        fprintf(stderr, "Unknown or unsuppported open() flags. Got '%d'\n",
                openFlags);
        return EACCES;
    }  
    
    if (secretEmpty) {
        secretOwner = uid;
        secretNumFileDescriptors++;
        
        if (openFlags == O_RDONLY) {
            /* Empty secret opened to read. Writing not allowed */  
            printf("Empty secret opened in RD_ONLY\n");  
        
        }
        else {
            /* WR_ONLY. */ 
            printf("OpEning an empty secret for writing \n");
        }    
    }
    else {
        /* Secret Full. No writing. Owner process can read */

        if (openFlags == O_RDONLY) {
            if (secretOwner == uid) {
                /* The owner of the secret is trying to read it - Allowed */ 
                printf("CCCCCCC \n");
                secretNumFileDescriptors++;

                /* The secret has been read. Now empty */ 
        /*        secretEmpty = 1;
                secretOwner = NO_OWNER;*/
            }   
            else {
                fprintf(stderr, "%d is not the secret owner.Permission denied\n",
                       uid);
                return EACCES;
            }   
        } 
        else {
            /* WR_ONLY --> error */
            fprintf(stderr, "Cannot open a full secret for writing\n"); 
            return ENOSPC;
        }     
    }

    printf("%d file descriptors\n", secretNumFileDescriptors); 

    return OK;
}

PRIVATE int hello_close(d, m)
    struct driver *d;
    message *m;
{
    printf("hello_close()\n");

    if (--secretNumFileDescriptors == 0) {
        clearSecret();
    }    
    
    printf("%d file descriptors\n", secretNumFileDescriptors);

    return OK;
}

PRIVATE struct device * hello_prepare(dev)
    int dev;
{
    hello_device.dv_base.lo = 0;
    hello_device.dv_base.hi = 0;
    hello_device.dv_size.lo = strlen(HELLO_MESSAGE);
    hello_device.dv_size.hi = 0;
    return &hello_device;
}

PRIVATE int hello_transfer(endpt, opcode, position, iov, nr_req, user_endpt)
    endpoint_t endpt;  
    int opcode;             /* DEV_GATHER_S or DEV_SCATTER_S            */
    u64_t position;         /* offset on device to read or write        */ 
    iovec_t *iov;           /* pointer to read or write request vector  */
    unsigned nr_req;        /* length of request vector                 */
    endpoint_t user_endpt;
{
    int bytes, ret;

    printf("hello_transfer()\n");

    bytes = MESSAGE_SIZE - position.lo < iov->iov_size ?
            MESSAGE_SIZE - position.lo : iov->iov_size;

    if (bytes <= 0)
    {
        return OK;
    }
    switch (opcode)
    {
        case DEV_GATHER_S:
            /* Write Section */
            /* copy from own address space into a grant */
            ret = sys_safecopyto(endpt, iov->iov_addr, 0,
                                (vir_bytes) (secretMessage + position.lo),
                                 bytes, D);
            iov->iov_size -= bytes;
            break;
        case DEV_SCATTER_S:
              /* Read Section */
              /* Copy from a grant into own address space */
              ret = sys_safecopyfrom(endpt, (cp_grant_id_t) iov->iov_addr, 0,
                                (vir_bytes) (secretMessage + position.lo),
                                bytes, D);
              iov->iov_size +=bytes;
              break;
        default:
              fprintf(stderr, "Invalid opcode entered\n");
            return EINVAL;
    }
    /*iov->iov_size -= bytes;*/
    return ret;
}

PRIVATE void hello_geometry(entry)
    struct partition *entry;
{
    printf("hello_geometry()\n");
    entry->cylinders = 0;
    entry->heads     = 0;
    entry->sectors   = 0;
}

PRIVATE int sef_cb_lu_state_save(int state) {
/* Save the state. */
    ds_publish_u32("open_counter", open_counter, DSF_OVERWRITE);

    return OK;
}

PRIVATE int lu_state_restore() {
/* Restore the state. */
    u32_t value;

    ds_retrieve_u32("open_counter", &value);
    ds_delete_u32("open_counter");
    open_counter = (int) value;

    return OK;
}

PRIVATE void sef_local_startup()
{
    /*
     * Register init callbacks. Use the same function for all event types
     */
    sef_setcb_init_fresh(sef_cb_init);
    sef_setcb_init_lu(sef_cb_init);
    sef_setcb_init_restart(sef_cb_init);

    /*
     * Register live update callbacks.
     */
    /* - Agree to update immediately when LU is requested in a valid state. */
    sef_setcb_lu_prepare(sef_cb_lu_prepare_always_ready);
    /* - Support live update starting from any standard state. */
    sef_setcb_lu_state_isvalid(sef_cb_lu_state_isvalid_standard);
    /* - Register a custom routine to save the state. */
    sef_setcb_lu_state_save(sef_cb_lu_state_save);

    /* Let SEF perform startup. */
    sef_startup();
}

PRIVATE int sef_cb_init(int type, sef_init_info_t *info)
{
/* Initialize the hello driver. */
    int do_announce_driver = TRUE;

    open_counter = 0;
    switch(type) {
        case SEF_INIT_FRESH:
            printf("Hey, Im initing FRESH\n");
        break;

        case SEF_INIT_LU:
            /* Restore the state. */
            lu_state_restore();
            do_announce_driver = FALSE;

            printf("Hey, I'm a new version!\n");
        break;

        case SEF_INIT_RESTART:
            printf("Hey, I've just been restarted!\n");
        break;
    }

    /* Announce we are up when necessary. */
    if (do_announce_driver) {
        driver_announce();
    }

    /* Initialization completed successfully. */
    return OK;
}

void clearSecret() {
    int i;  
    
    secretEmpty = 1;
    secretOwner = NO_OWNER;

    /* Initialize the message to be empty */ 
    for (i = 0; i < MESSAGE_SIZE; i++) {
        secretMessage[i] = '\0';
    }
}

void initSecret() {
    secretNumFileDescriptors = 0;

    clearSecret();
}

PUBLIC int main(int argc, char **argv)
{
    initSecret();

    /*
     * Perform initialization.
     */
    sef_local_startup();

    /*
     * Run the main loop.
     */
    driver_task(&hello_tab, DRIVER_STD);
    return OK;
}

