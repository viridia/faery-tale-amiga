/* mtrack - make tracks on a disk */

#include "exec/types.h"
#include "exec/memory.h"
#include "devices/trackdisk.h"

char *buffer, *AllocMem();

#define MEMSIZE	50000

#define MAP_ENTRIES	25

struct MsgPort *diskport, *CreatePort();
struct IOExtTD *diskreq, *CreateExtIO();

#define	BK_SIZE	128		/* in longwords */

struct Block {
	ULONG	offset[128];
} rootblock, bmapblock;

struct dmap {
	char	*filename;
	short	block_start,
			block_count;
} diskmap[MAP_ENTRIES] = {
	{ "f6a",		 32, 64 },
	{ "f9a",		 96, 64 },
	{ "map1",		160, 40 },
	{ "wild",		320, 40 },
	{ "build",		360, 40 },
	{ "rock",		400, 40 },
	{ "mountain1",	200, 40 },
	{ "tower",		440, 40 },
	{ "castle",		280, 40 },
	{ "field",		240, 40 },
	{ "swamp",		520, 40 },
	{ "palace",		480, 40 },
	{ "mountain2",	560, 40 },
	{ "doom",		640, 40 },
	{ "mountain3",	600, 40 },
	{ "under",		680, 40 },
	{ "cave",		760, 40 },
	{ "furnish",	720, 40 },
	{ "inside",		800, 40 },
	{ "astral",		840, 40 },
	{ "terra",		149, 11 },	/* T1 - T11 - inside sector2 */
	{ "mask",		896, 8  },
	{ "mask2",		904, 8  },
	{ "mask3",		912, 8  },
	{ "dh0:z/samples",920, 11  }
};	

#define CHAR_ENTRIES	17

struct cmap {
	char	*filename;
	short	block_start,
			block_count;
} char_map[CHAR_ENTRIES] = {
	{ "dh0:z/julian",	1376, 42 },
	{ "dh0:z/phillip",	1418, 42 },
	{ "dh0:z/kevin",	1460, 42 },
	{ "dh0:z/objects",	1312, 36 },
	{ "dh0:z/raft",		1348,  3 },

	{ "dh0:z/turtle",	1351, 20 },
	{ "dh0:z/bird",		1120, 40 },
	{ "dh0:z/dragon",	1160, 12 },
	{ "dh0:z/ogre",		 960, 40 },
	{ "dh0:z/ghost",	1080, 40 },

	{ "dh0:z/dKnight",	1000, 40 },
	{ "dh0:z/spirit",	1040, 40 },
	{ "dh0:z/royal",	 931,  5 },
	{ "dh0:z/wizard",	 936,  5 },
	{ "dh0:z/bartender", 941,  5 },

	{ "dh0:z/witch",	 946,  5 },
	{ "dh0:z/beggar",	 951,  5 }
};

main()
{	short i, error;
	long bmap;
	buffer = AllocMem(MEMSIZE,MEMF_CHIP);

	diskport = CreatePort(0,0);
	if (diskport == 0) goto exit_mem;
	printf("Created the port\n");

	diskreq = CreateExtIO(diskport,sizeof (struct IOExtTD));
	if (diskreq == 0) goto exit_port;
	printf("Created the IOReq\n");

	error = OpenDevice(TD_NAME,0,diskreq,0);
	if (error) goto exit_ior;
	printf("Opened Disk Device\n");

	printf("Reading root block\n");
	read_block(880,&rootblock);			/* assume root block is 880 */
	bmap = rootblock.offset[BK_SIZE-49];
	printf("Allocation map is block %ld\n",bmap);
	read_block(bmap,&bmapblock);

	for (i=0; i<MAP_ENTRIES; i++)
	{	transfer_file (
			diskmap[i].filename,
			diskmap[i].block_start,
			diskmap[i].block_count);
	}

	for (i=0; i<CHAR_ENTRIES; i++)
	{	transfer_char_file (
			char_map[i].filename,
			char_map[i].block_start,
			char_map[i].block_count);
	}

	bmapblock.offset[0] = 0;
	bmapblock.offset[0] = checksum(&bmapblock);
	write_block(bmap,&bmapblock);

	diskreq->iotd_Req.io_Length = 0;
	diskreq->iotd_Req.io_Command = TD_MOTOR;
	DoIO(diskreq);

exit_disk: CloseDevice(diskreq);
exit_ior:  DeleteExtIO(diskreq,sizeof (struct IOExtTD));
exit_port: DeletePort(diskport);
exit_mem:  FreeMem(buffer,MEMSIZE);
}

/* we can insert a validation code in the empty spaces */

transfer_file(name,start,length) char *name; short start; long length;
{	long file;
	short blocks;
	file = Open(name,1005);
	if (file)
	{	printf("Reading file %s   ",name);
		Read(file,buffer,length*512);
		printf("start = %ld, length = %ld\n",start,length);
		save_track_range(start,length,buffer); 
		Close(file);
	}
	else printf("Error opening file %s\n",name);
}

transfer_char_file(name,start,length) char *name; short start; long length;
{	long file;
	short blocks;
	file = Open(name,1005);
	if (file)
	{	printf("Reading file %s   ",name);
		Seek(file,6,0);				/* skip over header info */
		Read(file,buffer,length*512);
		printf("start = %ld, length = %ld\n",start,length);
		save_track_range(start,length,buffer);
		Close(file);
	}
	else printf("Error opening file %s\n",name);
}

save_track_range(first_block,block_count,buffer)
	short first_block, block_count; char *buffer;
{	short error, i; long bit, off, mask;
	diskreq->iotd_Req.io_Length = (block_count * 512);
	diskreq->iotd_Req.io_Data = (APTR)buffer;
	diskreq->iotd_Req.io_Command = CMD_WRITE;
	diskreq->iotd_Req.io_Offset = (first_block * 512);
	DoIO(diskreq);
	error = diskreq->iotd_Req.io_Error;
	if (error) printf("Error writing block. Error = %ld\n",error);
	else
	{	printf("Block Written.\n");
		for (i=first_block; block_count--; i++)
		{	bit = i - 2;			/* don't count first 2 blocks */
			off = ((bit/32) + 1);	/* starting from word 1 */
			bit = bit & 31;			
			mask = 1<<bit;
			bmapblock.offset[off] &= (~mask);
		}
	}
}

read_block(block,buffer) short block; APTR buffer;
{	short error;
	diskreq->iotd_Req.io_Length = 512;
	diskreq->iotd_Req.io_Data = buffer;
	diskreq->iotd_Req.io_Command = CMD_READ;
	diskreq->iotd_Req.io_Offset = (block * 512);
	DoIO(diskreq);
	error = diskreq->iotd_Req.io_Error;
	if (error)
		printf("Error reading block. Error = %ld\n",error);
	else printf("Block Read.\n");
}

write_block(block,buffer) short block; APTR buffer;
{	short error;
	diskreq->iotd_Req.io_Length = 512;
	diskreq->iotd_Req.io_Data = buffer;
	diskreq->iotd_Req.io_Command = CMD_WRITE;
	diskreq->iotd_Req.io_Offset = (block * 512);
	DoIO(diskreq);
	error = diskreq->iotd_Req.io_Error;
	if (error)
		printf("Error reading block. Error = %ld\n",error);
	else printf("Block Written.\n");
}

checksum(block) struct Block *block;
{	short i;
	ULONG sum = 0;
	for (i=0; i<128; i++) sum -= block->offset[i];
	return sum;
}
