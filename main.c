#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define IMAGE_SIZE	        (32*32*3)			// raw image
#define IMAGE_RECORD_LEN    (IMAGE_SIZE+1)		// raw image plus label

#define W	32
#define H	32

FILE *fp;

/*
 * Union data for CIFAR10 binary image format:
 * <1..9 label><32*32*3 = 3072 byte>
 * the first byte is the label of the first image, which is a number in the range 0-9.
 * The next 3072 bytes are the values of the pixels of the image. The first 1024 bytes are the
 * red channel values, the next 1024 the green, and the final 1024 the blue. The values are stored
 * in row-major order, so the first 32 bytes are the red channel values of the first row of the image.
 * https://www.cs.toronto.edu/~kriz/cifar.html
*/
typedef union
{
	int8_t image[1+(32*32*3)];      // full array: label + raw image data
	struct
	{
		uint8_t  label;             // label as values from 0 to 9
		int8_t  r[1024];            // RGB raw data
		int8_t  g[1024];
		int8_t  b[1024];
	} color;
} CIFAR10_IMAGE_t;

CIFAR10_IMAGE_t img;

uint32_t lSize;
uint32_t i, byte_read, ret;

const char*labels[10];
uint32_t idx=0;
char filename[128];

/* BMP variable */
unsigned char *imgbmp = NULL;
int filesize = 54 + IMAGE_SIZE;
unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
unsigned char bmppad[3] = {0,0,0};
uint32_t SaveBMP( char*fname);

int main( int argc, char* argv[])
{
    labels[0]="airplane";
    labels[1]="automobile";
    labels[2]="bird";
    labels[3]="cat";
    labels[4]="deer";
    labels[5]="dog";
    labels[6]="frog";
    labels[7]="horse";
    labels[8]="ship";
    labels[9]="truck";

    char *tmptr=strrchr( argv[0], '\\');
    if ( tmptr==NULL)
        tmptr=argv[0];
    else
        tmptr++;

    /* initialize random seed: */
    srand (time(NULL));

    if ( argc == 1)
    {
        fprintf(stdout, "Usage:\n\t%s <binary file> [img idx]\n",tmptr);
        fprintf(stdout, "\t This program take the CIFAR10 binary file as input and extract\n");
        fprintf(stdout, "\t an image in three different format:\n");
        fprintf(stdout, "\t 1. an header file that can be used with the ARM CMSIS Neural Network example\n");
        fprintf(stdout, "\t 2. a raw RGB file\n");
        fprintf(stdout, "\t 3. a BMP file\n");
        fprintf(stdout, "Example usage:\n\t%s test_batch.bin\n\t To extract a random image from the \"test_batch.bin\" binary file\n", tmptr);
        fprintf(stdout, "Example usage:\n\t%s test_batch.bin 456\n\t To extract the image from the \"test_batch.bin\" binary file at the specified position\n", tmptr);
        exit( 1);
    }

    if ( argc == 3)
    {
        /* image index is user specified */
        idx = atoi(argv[2]);
    } else {
        /* image index is random */
        idx = rand() % 10000 + 1;
    }

    /* Open the binary file cifar10... */
    fp=fopen( argv[1], "rb");
    if ( fp==NULL)
	{
        fprintf( stderr, "ERROR: can not open file %s\n", argv[1]);
        exit( 1);
    }

    /* obtain file size: */
    fseek ( fp , 0 , SEEK_END);
    lSize = ftell( fp);
    rewind( fp);

    /* Point to the image inside the binary file */
    fseek ( fp, (IMAGE_RECORD_LEN*idx), SEEK_SET);
	byte_read = fread ( img.image, 1, IMAGE_RECORD_LEN, fp);
    fclose(fp);

	fprintf( stdout, "From file: %s\n Read: %d bytes @pos: %d\n", argv[1], byte_read, idx);

	if ( img.color.label < 10)
    {
        fprintf( stdout, " Label: %s, [%d]\n", labels[img.color.label], img.color.label);
    } else {
        fprintf( stderr, " ERROR: Label not in range.[%d]\n", img.color.label);
        fclose(fp);
        exit(2);
    }

    /* Saving header file for CMSIS NN example **************************** */
    sprintf( filename, "%s_%05d.h", labels[img.color.label], idx);

    fp=fopen( filename, "wb");
    if ( fp==NULL)
	{
        fprintf( stderr, "ERROR: can not write data to include file: %s\n", filename);
        exit( 3);
    }

    fprintf(fp, "/* File: %s, Idx: %d, Label: %s [%d] */\n", argv[1], idx, labels[img.color.label], img.color.label);
    fprintf(fp, "#define IMG_DATA {");
    for ( i=0; i<IMAGE_SIZE; i++)
    {
        fprintf(fp, "%d,", img.image[i+1]);
    }
    fprintf(fp, "}\n");
    fclose(fp);
    fprintf(stdout, " Header file: %s saved!\n", filename);

    /* Saving RGB file ***************************************************** */
    sprintf( filename, "%s_%05d.rgb", labels[img.color.label], idx);

    fp=fopen( filename, "wb");
    if ( fp==NULL)
	{
        fprintf( stderr, "ERROR: can not write data to RGB file: %s\n", filename);
        exit( 4);
    }

    for ( i=0; i<IMAGE_SIZE; i++)
    {
        fputc(img.color.r[i], fp);
        fputc(img.color.g[i], fp);
        fputc(img.color.b[i], fp);
    }
    fclose(fp);
    fprintf(stdout, " RGB file: %s saved!\n", filename);

    /* Saving BMP dile ***************************************************** */
    sprintf( filename, "%s_%05d.bmp", labels[img.color.label], idx);
    ret = SaveBMP(filename);
    if ( !ret)
    {
        fprintf(stdout, " BMP file: %s saved!\n", filename);
    }
    exit(ret);
}

uint32_t SaveBMP( char*fname)
{
    /*
     * https://stackoverflow.com/questions/2654480/writing-bmp-image-in-pure-c-c-without-other-libraries
    */
    int i, j, x, y;

	imgbmp = (unsigned char *)malloc(IMAGE_SIZE);
    if ( imgbmp==NULL)
	{
	    fprintf( stderr, "ERROR: can not allocate memory for BMP file: %s, size: %d\n", filename, IMAGE_SIZE);
	    return 5;
	}

	memset(imgbmp, 0, IMAGE_SIZE);

	for(i=0; i<W; i++)	// w
	{
		for(j=0; j<H; j++)	// h
		{
			x=i; y=(H-1)-j;
            imgbmp[(x+y*W)*3+2] = (unsigned char)(img.color.r[(x+y*W)]);
			imgbmp[(x+y*W)*3+1] = (unsigned char)(img.color.g[(x+y*W)]);
			imgbmp[(x+y*W)*3+0] = (unsigned char)(img.color.b[(x+y*W)]);
		}
	}

	bmpfileheader[ 2] = (unsigned char)(filesize    );
	bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
	bmpfileheader[ 4] = (unsigned char)(filesize>>16);
	bmpfileheader[ 5] = (unsigned char)(filesize>>24);

	bmpinfoheader[ 4] = (unsigned char)(       W    );
	bmpinfoheader[ 5] = (unsigned char)(       W>> 8);
	bmpinfoheader[ 6] = (unsigned char)(       W>>16);
	bmpinfoheader[ 7] = (unsigned char)(       W>>24);
	bmpinfoheader[ 8] = (unsigned char)(       H    );
	bmpinfoheader[ 9] = (unsigned char)(       H>> 8);
	bmpinfoheader[10] = (unsigned char)(       H>>16);
	bmpinfoheader[11] = (unsigned char)(       H>>24);

	fp = fopen(fname,"wb");
    if ( fp==NULL)
	{
        fprintf( stderr, "ERROR: can not write data to BMP file: %s\n", filename);
        free(imgbmp);
        return 6;
    }

	fwrite(bmpfileheader,1,14,fp);
	fwrite(bmpinfoheader,1,40,fp);

	for(i=0; i<H; i++)
	{
		fwrite(imgbmp+(W*(H-i-1)*3), 3, W, fp);
		fwrite(bmppad, 1, (4-(W*3)%4)%4, fp);
	}

	free(imgbmp);
	fclose(fp);

	return 0;
}
