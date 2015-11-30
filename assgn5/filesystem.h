#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>

#define FILESYSTEM_H

/* File Types */
#define FILE_TYPE_MASK 0170000 
#define REGULAR_FILE_MASK 0100000
#define SYMBOLIC_LINK_MASK 0120000
#define DIRECTORY_MASK 0040000 
#define OWNER_RD_PERM 0000400 
#define OWNER_WR_PERM 0000200 
#define OWNER_EX_PERM 0000100 
#define GROUP_RD_PERM 0000040 
#define GROUP_WR_PERM 0000020 
#define GROUP_EX_PERM 0000010 
#define OTHER_RD_PERM 0000004 
#define OTHER_WR_PERM 0000002 
#define OTHER_EX_PERM 0000001 

/* Useful Constants */
#define PARTITION_TABLE_LOC 0x1BE 
#define MINIX_PARTITION_TYPE 0x81 
#define MAGIC_NUMBER 0x4D5A 
#define MAGIC_NUM_REVERSED 0x5A4D 
#define BOOTABLE_MAGIC_NUM 0x80

/* May need to rename these... */
#define BOOT_SECTOR_BYTE_510 510
#define BOOT_SECTOR_BYTE_510_VAL 0x55
#define BOOT_SECTOR_BYTE_511_VAL 0xAA

/* Minix 3 constants */
#define MINIX_VERSION 3
#define ENDIANNESS 0 /* little endian */
#define PTRS_PER_ZONE 1024
#define SECTOR_SIZE 512
#define INODE_SIZE 64
#define DIRECTORY_ENTRY_SIZE 64
#define FILENAME_LENGTH 60
#define DIRECT_ZONES 7
#define NUM_PRIMARY_PARTITIONS 4
#define PERMISSIONS_LENGTH 10
#define INODE_BITMAP_BLOCK 2
#define ZNODE_BITMAP_BLOCK 3

/* Errors */
#define NO_FILE_FOUND -3
#define NOT_REGULAR_FILE -4

struct partitionEntry {
   uint8_t bootind;       /* Boot magic number (0x80 if bootable) */
   uint8_t start_head;    /* Start of partition in CHS            */
   uint8_t start_sec;
   uint8_t start_cyl;
   uint8_t type;          /* Type of partition (0x81 in MINIX)    */
   uint8_t end_head;      /* End of partition in CHS              */
   uint8_t end_sec;
   uint8_t end_cyl;
   uint32_t lFirst;       /* First sector (LBA addressing)        */
   uint32_t size;         /* size of partition (in sectors        */
};

struct superblock { /* Minix Version 3 Superblock
                       * this structure found in fs/super.h
                       * * in minix 3.1.1
                       * */
   /* on disk. These fields and orientation are non–negotiable    */
   uint32_t ninodes;      /* number of inodes in this filesystem  */
   uint16_t pad1;         /* make things line up properly         */
   int16_t i_blocks;      /* # of blocks used by inode bit map    */
   int16_t z_blocks;      /* # of blocks used by zone bit map     */
   uint16_t firstdata;    /* number of first data zone            */
   int16_t log_zone_size; /* log2 of blocks per zone              */
   int16_t pad2;          /* make things line up again            */
   uint32_t max_file;     /* maximum file size                    */
   uint32_t zones;        /* number of zones on disk              */
   int16_t magic;         /* magic number                         */
   int16_t pad3;          /* make things line up again            */
   uint16_t blocksize;    /* block size in bytes                  */
   uint8_t subversion;    /* filesystem sub–version               */
};

struct inode {
   uint16_t mode;               /* mode                                     */
   uint16_t links;              /* number or links                          */
   uint16_t uid;                /* id of user who owns file                 */
   uint16_t gid;                /* owner's group                            */
   uint32_t size;               /* file size, num of bytes in the file      */
   int32_t atime;               /* access time in seconds since Jan 1, 1970 */
   int32_t mtime;               /* modification time                        */
   int32_t ctime;               /* status change time                       */
   uint32_t zone[DIRECT_ZONES]; /* zone numbers for the first seven data 
                                   zones in the file                        */
   uint32_t indirect;           /* used for files larger than 7 zones       */
   uint32_t two_indirect;       /* same as above                            */
   uint32_t unused;             /* could be used for triple indirect zone   */
};

struct directoryEntry {
   uint32_t inode;                          /* inode number    */
   unsigned char filename[FILENAME_LENGTH];  /* filename string */
};

extern void printHelp(void);
extern void printPartitionTable(struct partitionEntry**,
      struct partitionEntry**);
extern void printVerbose(struct partitionEntry**,
      struct partitionEntry**, struct superblock*, struct inode*);
extern void seekPastPartitions(FILE*, struct partitionEntry**, int,
      struct partitionEntry**, int);
extern struct inode* getDirectory(FILE*, struct inode *, struct superblock *,
      struct partitionEntry **, int, struct partitionEntry **, int, char *);
extern struct directoryEntry* getIndirectBlock(FILE*, struct inode *,
      struct superblock *, struct partitionEntry **, int, 
      struct partitionEntry **, int, char *);
extern int navigatePath(FILE*, struct inode **,
      struct superblock *, struct partitionEntry **, int, 
      struct partitionEntry **, int, char *);
