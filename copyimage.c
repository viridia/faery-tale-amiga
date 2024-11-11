
#include <macros.h>

struct IOStdReq	*cdreq;
struct MsgPort	*cdport;
BOOL			cd_open;
char			*devname,
				*filename;
WORD			unit,
				first,
				count;
BPTR			file;
char			*buf;

void closeall(char *msg)
{
	if (msg) printf("%s.\n",msg);
	if (buf) FreeMem(buf,11 * 512);
	if (file) Close(file);
	if (cd_open) CloseDevice((struct IORequest *)cdreq);
	if (cdreq) DeleteStdIO(cdreq);
	if (cdport) DeletePort(cdport);
	exit(0);
}

void main(int argc, char *argv[])
{	WORD	amount;

	if (argc != 6) exit(0);

	devname = argv[1];
	unit = atol(argv[2]);
	first = atol(argv[3]);
	count = atol(argv[4]);
	filename = argv[5];

	if ( !(cdport = CreatePort(NULL,0)) ) closeall("Can't allocate port");		
	if ( !(cdreq = CreateStdIO(cdport)) ) closeall("Can't allocate ioreq");		
	if (OpenDevice(devname,unit,(struct IORequest *)cdreq,0))
		closeall("Can't open device");

	cd_open = 1;

	file = Open(filename,MODE_NEWFILE);
	if (file == NULL) closeall("Can't open file");

	buf = AllocMem(11 * 512, MEMF_CHIP);
	if (buf == NULL) closeall("Can't allocate memory");

	while (count)
	{
		amount = MIN(count,11);
		count -= amount;

		cdreq->io_Command = CMD_READ;
		cdreq->io_Data = (APTR)buf;
		cdreq->io_Offset = 512 * first;
		cdreq->io_Length = 512 * amount;
		cdreq->io_Error = 0;
		DoIO((struct IORequest *)cdreq);
		if (cdreq->io_Error != 0)
		{
err:		cdreq->io_Command = TD_MOTOR;
			cdreq->io_Length = 0;
			cdreq->io_Error = 0;
			DoIO((struct IORequest *)cdreq);
			closeall("Error writing to file");
		}
		if (Write(file,buf,512 * amount) != 512 * amount) goto err;
		first += amount;
	}

	cdreq->io_Command = TD_MOTOR;
	cdreq->io_Length = 0;
	cdreq->io_Error = 0;
	DoIO((struct IORequest *)cdreq);

	closeall(NULL);
}
