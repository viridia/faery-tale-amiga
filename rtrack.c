/* mtrack - make tracks on a disk */

#include "exec/types.h"
#include "devices/trackdisk.h"

char *buffer, *AllocMem();

#define MEMSIZE	50000

#define MAP_ENTRIES	21

struct MsgPort *diskport, *CreatePort();
struct IOExtTD *diskreq, *CreateExtIO();

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
	{ "terra",		882, 11 }	/* T1 - T11 */
};	

main()
{	short i, error;
	buffer = AllocMem(MEMSIZE,NULL);

	diskport = CreatePort(0,0);
	if (diskport == 0) goto exit_mem;
	printf("Created the port\n");

	diskreq = CreateExtIO(diskport,sizeof (struct IOExtTD));
	if (diskreq == 0) goto exit_port;
	printf("Created the IOReq\n");

	error = OpenDevice(TD_NAME,1,diskreq,0);
	if (error) goto exit_ior;
	printf("Opened Disk Device\n");

	for (i=0; i<MAP_ENTRIES; i++)
	{	transfer_file (
			diskmap[i].filename,
			diskmap[i].block_start,
			diskmap[i].block_count);
	}

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

save_track_range(first_block,block_count,buffer)
	short first_block, block_count; char *buffer;
{	short error;
	unsigned long first, count;
	first = first_block * 512;
	count = block_count * 512;
	diskreq->iotd_Req.io_Length = count;
	diskreq->iotd_Req.io_Data = (APTR)buffer;
	diskreq->iotd_Req.io_Command = CMD_READ;
	diskreq->iotd_Req.io_Offset = first;
	DoIO(diskreq);
	error = diskreq->iotd_Req.io_Error;
	if (error)
		printf("Error writing block. Error = %ld\n",error);
	else printf("Block Written.\n");
}




#ifdef blarg
save_track_range(first_block,block_count,buffer)
	short first_block, block_count; char *buffer;
{	short last_block, trackend_block, read_length;

	last_block = first_block + block_count;

	while (first_block < last_block)
	{	trackend_block = ((first_block/10)+1)*10;
		if (trackend_block >= last_block)
			read_length = last_block - first_block;
		else read_length = trackend_block - first_block;
		/* write (first_block,buffer,read_length) */
		buffer += (512 * read_length);
		first_block += read_length;
	}
}
#endif
