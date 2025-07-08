
// this program performs run-length encoding of a black & white image
// only run lengths are stored, since the pixel colours alternate between black and white between runs

#include <cstdio>
#include <cstdlib>

int main()
{
    FILE* src=fopen("text.pbm","r");
    FILE* out=fopen("text.h","w");
    
    char line[1024];
    fgets(line,1024,src);
    fgets(line,1024,src);
    fgets(line,1024,src);

    static char pix[2048*2048];
    char* pixp=pix;
    
    int linenum=0;
    while(fgets(line,1024,src))
    {
        for(char* c=line;*c && c[0]!='\n' && c[0]!='\r';++c)
        {
            *(pixp++)=*c;
        }
        ++linenum;
    }
    
    printf("linenum=%d\n",linenum);
    
    fprintf(out,"static const unsigned char imagepixeldata[]={\n");
    char* endpixp=pixp;
    char p='0';
    pixp=pix;
    char* pixp2=pixp;
    int bytecount=0,over127count=0;
    while(pixp<=endpixp)
    {
        if(pixp==endpixp || pixp[0]!=p)
        {
            unsigned int run=pixp-pixp2;
            printf("%d ",run);
            if((run-1)>0xffff)
            {
                printf("\nRUNRUNRUN %d, run=%d\n",bytecount,run);
                fprintf(out,"%d, ",0);
                ++bytecount;
            }
            else if((run-1)>127)
            {
                over127count++;
                fprintf(out,"%d, ",((run-1)&127)|128);
                ++bytecount;
                fprintf(out,"%d, ",(run-1)>>7);
                ++bytecount;
            }
            else
            {
                fprintf(out,"%d, ",run-1);
                ++bytecount;
            }
            if(pixp==endpixp)
                break;
            p=pixp[0];
            pixp2=pixp;
        }
        ++pixp;
    }
    /*
    {
        unsigned int run=pixp-pixp2;
        printf("%d ",run);
        if((run-1)>0xffff)
        {
            printf("\nRUNRUNRUN %d, run=%d\n",bytecount,run);
            fprintf(out,"%d, ",0);
            ++bytecount;
        }
        else if((run-1)>127)
        {
            over127count++;
            fprintf(out,"%d, ",(run-1)&127);
            ++bytecount;
            fprintf(out,"%d, ",(run-1)>>7);
            ++bytecount;
        }
        else
        {
            fprintf(out,"%d, ",run-1);
            ++bytecount;
        }
    }*/
    //printf("%d ",pixp-pixp2);
    //fprintf(out,"%d, ",pixp-pixp2);
    ++bytecount;
    p=pixp[0];
    pixp2=pixp;
    printf("\nbc=%d\n",bytecount);
    fprintf(out,"};\n");
    
    printf("znover255count=%d\n",over127count);
    
    fclose(out);
    fclose(src);

    return 0;
}

