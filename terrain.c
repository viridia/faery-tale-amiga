/* terrain.c - extract terrain information from landscape files */

char *datanames[] =
{ 	" ",
	"wild",		/* 1 */
	"build",	/* 2 */
	"rock",		/* 3 */
	"mountain1", /* 4 */
	"tower",	/* 5 */
	"castle",	/* 6 */
	"field",	/* 7 */
	"swamp",	/* 8 */
	"palace",	/* 9 */
	"mountain2", /* 10 */
	"doom",		 /* 11 */
	"mountain3", /* 12 */
	"under",	 /* 13 */
	"cave",		 /* 14 */
	"furnish",	 /* 15 */
	"inside",	 /* 16 */
	"astral",	 /* 17 */
};

char order[] = 
{ 1, 9,	/* 1a 5a */
 8,10,  /* 1b 3b */
 1, 2,  /* 2a 3a 4a */
 3, 5,  /* 2b 4b */
 8,12,  /* 5b 7b */
 1, 6,  /* 6a 8a */
 7, 4,  /* 6b 8b */
 1,11,  /* 7a */
13,15,  /* 9a */
16,17,  /* 9b 10b */
13,14,  /* 10a */
};

unsigned char outbuffer[1100]; 	/* holds terrain info */

unsigned char
	maptag[256],		/* image characteristics */
	terrain[256],		/* terrain characteristics (2 nibbles)  */
	tiles[256],			/* terrain feature masks */
	big_colors[256];

#define IPLAN_SZ (5 * 64 * 64)

main()
{	short i,j,k;
	unsigned char *buffer;

	long file;
	file = Open("terra",1006);

	for (i=0; i<22;)
	{	buffer = outbuffer;
		j = order[i];
		load_images(datanames[j]);
		printf("Now processing file %s\n",datanames[j]);
		for (j=0; j<64; j++)
		{	*buffer++ = maptag[j];
			*buffer++ = terrain[j];
			*buffer++ = tiles[j];
			*buffer++ = big_colors[j];
		}
		i++;
		j = order[i];
		load_images(datanames[j]);
		printf("Now processing file %s\n\n",datanames[j]);
		for (j=0; j<64; j++)
		{	*buffer++ = maptag[j];
			*buffer++ = terrain[j];
			*buffer++ = tiles[j];
			*buffer++ = big_colors[j];
		}
		i++;
		Write(file,outbuffer,512);
	}
	Close(file);
}

load_images(name) char *name;
{	long file;
	file = Open(name,1005);
	if (file)
	{	Seek(file,IPLAN_SZ,0);
		Read(file,maptag,64);
		Read(file,terrain,64);
		Read(file,tiles,64);
		Read(file,big_colors,64);
		Close(file);
	}
}
