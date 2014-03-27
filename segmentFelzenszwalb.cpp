#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mex.h>

#include "./image.h"
#include "./misc.h"
#include "./pnmfile.h"
#include "./image.h"
#include "./segment-image.h"
#include "./segment-image-labelOutput.h"

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y)) /* min and max value macros */
#endif
#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

void mexFunction(int nOutput, mxArray *pOutput[], /* Output variables */
  int nInput, const mxArray *pInput[]) /* Input variables */
{ 
   
   /* -----------------------------------------------------------------
   *                         Check the arguments
   * -------------------------------------------------------------- */

  if (nInput < 1) {
    mexErrMsgTxt ( "To few input arguments.");
    return;
  }
  
  if (nOutput > 2) {
    mexErrMsgTxt ( "Too many output arguments.");
    return;
  }
  
  if ( (nOutput == 0) && (nInput < 5) ) {
    mexErrMsgTxt ( "No destination for segmentation result specified - chose either return value or string for destination!");
    return;
  }  
  
  
  /* -----------------------------------------------------------------
  *                      Get the arguments
  * -------------------------------------------------------------- */  
  
  bool verbose ( false );
  if ( nInput >= 7)
  {
    if ( mxIsLogicalScalar( pInput[6] ) && mxIsLogicalScalarTrue( pInput[6] ) )
    {
      verbose = true;
    }
    else
      verbose = false; 
  }
   
  /* Get string of input image*/  
  image<rgb> * imgInput;
  
  /* was the input image given as a string?*/ 
  if ( mxGetM( pInput[0] ) == 1 )
  {
    char *input;     
    input = (char *) mxCalloc( mxGetN(pInput[0] )+1, sizeof(char) );
    mxGetString(pInput[0], input, mxGetN( pInput[0] )+1);
    if ( verbose ) 
      mexPrintf("The input string is:  %s\n", input);    
    
    imgInput = loadPPM( input );
    mxFree ( input );
  }
  /* the image was directly passed as matlab matrix*/ 
  else
  {    
    /*double * imgInputMatlab = mxGetPr( pInput[0] );*/
    signed char* imgInputMatlab = (signed char *)mxGetData( pInput[0] );
    
    size_t K = mxGetNumberOfDimensions(  pInput[0] );
    const mwSize *N = mxGetDimensions(  pInput[0] );
    
    uint height  ( N[0] ); // matlab gives first height
    uint width ( N[1] ); // matlab gives then width
  
    
    imgInput = new image<rgb>( width, height, 0 );
    
    /* start with RED channel*/     
    /* keep in mind that matlab stores images col by width, whereas C does it the other way round*/
    for ( uint y = 0; y < height; y++)
    {
      for ( uint x = 0; x < width; x++)
      {
        imRef(imgInput, x, y).r = imgInputMatlab[y + height*(x + width*0) ];
        imRef(imgInput, x, y).g = imgInputMatlab[y + height*(x + width*1) ];
        imRef(imgInput, x, y).b = imgInputMatlab[y + height*(x + width*2) ];
      }
    }    
  }     
  
  char buf [1024] ;
  
  /* Get sigma */  
  double sigma;
  if ( nInput < 2) 
  {
    sigma = 0.5; 
  }
  else
    sigma = mxGetScalar( pInput[1] ); 
  if ( verbose ) 
    mexPrintf("The sigma value is:  %f\n", sigma);
    
  /* Get k */  
  int k;
  if ( nInput < 3) 
  {
    k = 500;
  }
  else
    k = mxGetScalar( pInput[2] );
  
  if ( verbose ) 
    mexPrintf("The k is:  %i\n", k);
   
  /* Get minSize*/  
  int minSize;  
  if ( nInput < 4)
  {
    minSize = 50;
  }
  else
    minSize = mxGetScalar(  pInput[3] );
  
  if ( verbose ) 
    mexPrintf("The minSize is:  %i\n", minSize);
   
  /* Get bool whether to compute the label img (int) or colored img (rgb)*/  
  bool computeColorOutput;
  if (nInput >=  5)
  {
    if ( mxIsLogicalScalar( pInput[4] ) && mxIsLogicalScalarTrue( pInput[4] ) )
    {
      computeColorOutput = true;
    }
    else
      computeColorOutput = false;   
  }
  else
  {
    computeColorOutput = false;
  }
  if ( verbose ) 
    mexPrintf("To we compute RGB colored segmentation result?  :  %i\n", computeColorOutput );
   
  /* Get string of output image if given*/  
  char * output;
  if (nInput >=  6)
  {
    output = (char *) mxCalloc(mxGetN(pInput[5])+1, sizeof(char));
    mxGetString(pInput[5], output, mxGetN(pInput[5])+1);
    
    if ( verbose ) 
      mexPrintf("The output string is:  %s\n", output);
  }
    
  if ( verbose )
    mexPrintf("image loaded, now start segmentation\n");  
  
   
  /* -----------------------------------------------------------------
  *                       Do the main stuff 
  *                        >segmentation<
  * -------------------------------------------------------------- */   
   
   
  int num_ccs; 
  image<rgb> * imgResultRGB = NULL;
  image<int> * imgResult  = NULL;
   
  if ( computeColorOutput )
  {
    imgResultRGB = segment_image(imgInput, sigma, k, minSize, &num_ccs); 
  }
  else
  {
    imgResult = segment_image_labelOutput(imgInput, sigma, k, minSize, &num_ccs); 
  }
       
  if ( verbose )
    mexPrintf("segmentation done\n");   
   
   
  /* -----------------------------------------------------------------
  *                      Conversion to Matlab structures
  * -------------------------------------------------------------- */      
   
  if ( nOutput == 0 )
  {
    //was a filename given to store the image in?
    if (nInput >=  6) 
    {
      if ( computeColorOutput )
        savePPM( imgResultRGB, output );
      else
        save_image( imgResult, output );
      mexPrintf("save results\n"); 
    }
    else
    {
      mexPrintf("ATTENTION -- Neither output variables nor filename to store the result to given!");
    }
  }
  else
  {
    if ( verbose )
      mexPrintf("convert to matlab structure and hand result back to main program\n"); 
     
    /* convert segmentation result to matlab matrix*/
        
    if ( computeColorOutput )
    {
      int width ( imgResultRGB->width() );
      int height (imgResultRGB->height() );       
      /* keep in mind that matlab stores images  height × width × color, whereas C does it the other way round*/
      int dims[] = {height, width,3};
      pOutput[0] = mxCreateNumericArray (3, dims, mxINT32_CLASS, mxREAL);
      //unsigned char *out1; /* pointer to output 1 */
      //out1 = (unsigned char *)mxGetPr( pOutput[0] ); /* pointer to output 1 */
      //unsigned char *out1; /* pointer to output 1 */
      //out1 = (unsigned char *)mxGetPr( pOutput[0] ); /* pointer to output 1 */
      int *out1; /* pointer to output 1 */
      out1 = (int *)mxGetData( pOutput[0] ); /* pointer to output 1 *	

              
      /* start with RED channel*/     
      /* keep in mind that matlab stores images col by width, whereas C does it the other way round*/
      for ( uint x = 0; x < width; x++)
      {
        uint rowOffset ( x*height );
        for ( uint y = 0; y < height; y++)
        {
          out1[rowOffset + y ] = (int) (imRef(imgResultRGB, x, y)).r;
        }
      }
        
      /* GREEN channel*/
      uint channelOffsetG ( width * height );
      for ( uint x = 0; x < width; x++)
      {
        uint rowOffset ( x*height );
          for ( uint y = 0; y < height; y++)
          {
            out1[channelOffsetG + rowOffset + y ] = (int) (imRef(imgResultRGB, x, y)).g;
          }
      }
        
      /* BLUE channel*/
      uint channelOffsetB ( 2 * width * height );
      for ( uint x = 0; x < width; x++)
      {
        uint rowOffset ( x*height );
          for ( uint y = 0; y < height; y++)
          {
            out1[channelOffsetB + rowOffset + y ] = (int) (imRef(imgResultRGB, x, y)).b;
          }
      }
    }
    else /* do not compute colored rgb segmentation image, but only an int img*/
    {
      int width ( imgResult->width() );
      int height (imgResult->height() ); 
      int dims[] = {height, width};
      
      if (num_ccs < 255)
      {
        pOutput[0] = mxCreateNumericArray (2, dims, mxUINT8_CLASS, mxREAL);
        unsigned char *out1; /* pointer to output 1 */
        out1 = (unsigned char *)mxGetPr( pOutput[0] ); /* pointer to output 1 */
        
        /* keep in mind that matlab stores images col by width, whereas C does it the other way round*/
        for ( uint x = 0; x < width; x++)
        {
          uint rowOffset ( x*height );
          for ( uint y = 0; y < height; y++)
          {
        out1[rowOffset + y ] = (unsigned char ) (imRef(imgResult, x, y));
          }
        }
      }
      else
      {
        pOutput[0] = mxCreateNumericArray (2, dims, mxINT32_CLASS, mxREAL);
        int *out1; /* pointer to output 1 */
        out1 = (int *)mxGetData( pOutput[0] ); /* pointer to output 1 *
        
        /* keep in mind that matlab stores images col by width, whereas C does it the other way round*/
        for ( uint x = 0; x < width; x++)
        {
          uint rowOffset ( x*height );
          for ( uint y = 0; y < height; y++)
          {
            out1[rowOffset + y ] = (int) (imRef(imgResult, x, y));
          }
        }
      }
    }
     
    /* return number of segments*/
    if ( nOutput >= 2 )
    {
      int dims[] = {1};
      if (num_ccs < 255)
      {
        pOutput[1] = mxCreateNumericArray(1,dims,mxUINT8_CLASS,mxREAL);
        unsigned char *out2; /* pointer to output 2 */
        out2 = (unsigned char *)mxGetPr( pOutput[1] ); /* pointer to output 2 */
          
        out2[0] = num_ccs;
      }
      else
      {
        pOutput[1] = mxCreateNumericArray(1,dims,mxINT32_CLASS,mxREAL);
        //unsigned char *out2; /* pointer to output 2 */
        //out2 = (unsigned char *)mxGetPr( pOutput[1] ); /* pointer to output 2 */
        int *out2; /* pointer to output 2 */
        out2 = (int *) mxGetData( pOutput[1] ); /* pointer to output 2 */
          
        out2[0] = num_ccs;

      }
       
      if ( verbose ) 
        mexPrintf("number of components: %i", num_ccs);
    }

    /* done */
  }
   
  /* don't waste memory*/
  if ( imgInput != NULL )
    delete imgInput;
  if ( imgResultRGB != NULL )
    delete imgResultRGB;
  if ( imgResult != NULL )
    delete imgResult;
  
  return;
}