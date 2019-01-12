#include <stdio.h>
#include <stdlib.h>
#include "ctrmml.h"

void print_usage(char* exename)
{
	printf("ctrmml (pre-alpha version)\n");
	printf("(C) 2019 ian karlsson\n");
	printf("%s <input_file.mml>\n", exename);
}

int main(int argc, char* argv[])
{
	struct song* song;
	if(argc < 2)
	{
		print_usage(argv[0]);
		printf("need to specify input file\n");
		exit(-1);
	}

	song = song_create();
	song_open(song, argv[1]);
	song_finalize(song);
	song_free(song);

	return 0;
}
