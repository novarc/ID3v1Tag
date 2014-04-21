// Compile with `gcc -std=c99 -pedantic`.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

const char * const id3v1_header = "TAG";

struct id3v1_1
{
	char header[3]; // "TAG"
	char title[30]; // -t
	char artist[30]; // -a
	char album[30]; // -A
	char year[4]; // -y
	char comment[28]; // -c
	unsigned char guard; // '\0'
	unsigned char track; // -n
	unsigned char genre; // -g
};

static char check_header(char *header)
{
	return (header[0]=='T' && header[1]=='A' && header[2]=='G');
}

void print_non_null_terminated_str(const char *s, int len)
{
	for(int i = 0; *s && i < len; i++)
		printf("%c", *s++);
}

static void display_tag(const struct id3v1_1 *tag)
{
	puts("----------------------------------------");

	printf("Title: ");
	print_non_null_terminated_str(tag->title, 30);
	puts("");

	printf("Artist: ");
	print_non_null_terminated_str(tag->artist, 30);
	puts("");

	printf("Album: ");
	print_non_null_terminated_str(tag->album, 30);
	puts("");

	printf("Year: ");
	print_non_null_terminated_str(tag->year, 4);
	puts("");

	printf("Comment: ");
	print_non_null_terminated_str(tag->comment, 28);
	puts("");

	printf("Genre: %d\n", tag->genre);
	printf("Track: %d\n", tag->track);

	puts("----------------------------------------");
}

void copy_str_field(char * const src, char *dst, const int len)
{
	char *sp = src;
	int i = 0;

	if(sp != NULL) {
		while(*sp != '\0' && i < len)
			dst[i++] = *sp++;

		if(*sp != '\0' && i == len)
			fprintf(stderr, "The following string has been truncated to %d characters: %s\n", len, src);
	}

	while(i < len)
		dst[i++] = '\0'; // zero out the rest/all of *dst
}

static char parse_cmdl_args(int argc, char *argv[], struct id3v1_1 *tag, char upd_msgs)
{
	char modify = 0;
	for(int c; (c = getopt (argc, argv, "t:a:A:y:c:n:g:")) != -1; modify=1)
	{
		switch (c) {
		case 't':
			copy_str_field(optarg, tag->title, 30);
			break;
		case 'a':
			copy_str_field(optarg, tag->artist, 30);
			break;
		case 'A':
			copy_str_field(optarg, tag->album, 30);
			break;
		case 'y':
			copy_str_field(optarg, tag->year, 4);
			break;
		case 'c':
			copy_str_field(optarg, tag->comment, 28);
			break;
		case 'n':
			tag->track = (unsigned char) atoi( optarg );
			break;
		case 'g':
			tag->genre = (unsigned char) atoi( optarg );
			break;
		case '?':
			return -1;
		default:
			return -1;
		}
	}

	if(optind < argc) {
		fprintf(stderr, "The following unnecessary non-option arguments were ignored:");
		for(int i = optind; i < argc; i++)
			fprintf(stderr, " %s", argv[i]);
		fprintf(stderr, "\n");
	}

	return modify;
}

int main(int argc, char *argv[])
{
	if(argc < 2) {
		fprintf(stderr, "Please specify a file name (as the last argument).\n");
		return EXIT_FAILURE;
	}

	char *filename = argv[--argc];

	struct id3v1_1 file_tag = { .header = "\0\0\0", .guard = '\0', .genre = 12 };

	FILE *f = fopen(filename, "rb+");
	if(f == NULL) {
		fprintf(stderr, "Error opening file: %s\n", filename);
		return EXIT_FAILURE;
	}

	fseek(f, - sizeof(struct id3v1_1), SEEK_END);
	fread(&file_tag, sizeof(struct id3v1_1), 1, f);

	if(check_header(file_tag.header)) {
		puts("This file contains the following ID3v1 tag:");

		display_tag(&file_tag);

		char result = parse_cmdl_args(argc, argv, &file_tag, 1);

		if(result == -1) {
			fprintf(stderr, "Exiting due to malformed arguments...\n");
			return EXIT_FAILURE;
		}

		if(result == 1) {
			puts("Updating ID3v1 tag to the following:");
			display_tag(&file_tag);
			fseek(f, - sizeof(struct id3v1_1), SEEK_END);
			fwrite(&file_tag, sizeof(struct id3v1_1), 1, f);
		}
	}
	else {
		puts("No ID3v1 tag found in this file.");

		struct id3v1_1 new_tag = { .header = {'T','A','G'}, .guard = '\0', .genre = 12 };

		char result = parse_cmdl_args(argc, argv, &new_tag, 0);

		if(result == -1) {
			fprintf(stderr, "Exiting due to malformed arguments...\n");
			return EXIT_FAILURE;
		}

		if(result == 1) {
			puts("Inserting the following tag:");
			display_tag(&new_tag);
			fseek(f, 0, SEEK_END);
			fwrite(&new_tag, sizeof(struct id3v1_1), 1, f);
		}
	}

	fclose(f);
	return EXIT_SUCCESS;
}