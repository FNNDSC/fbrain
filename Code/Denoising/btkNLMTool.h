/*
Copyright or © or Copr. Université de Strasbourg - Centre National de la Recherche Scientifique

25 january 2011
rousseau@unistra.fr

This software is governed by the CeCILL-B license under French law and
abiding by the rules of distribution of free software.  You can  use, 
modify and/ or redistribute the software under the terms of the CeCILL-B
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info". 

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability. 

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or 
data to be ensured and,  more generally, to use and operate it in the 
same conditions as regards security. 

The fact that you are presently reading this means that you have had
knowledge of the CeCILL-B license and that you accept its terms.
*/

#ifndef btkNLMTool_H
#define btkNLMTool_H


#include "itkImage.h"
#include "itkImageDuplicator.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkCastImageFilter.h"

#include <string>
#include <iomanip>
#include <sstream>
#include <fstream>

template <typename T>
class btkNLMTool
{
 public:
  typedef typename itk::Image< T, 3> itkTImage;
  typedef typename itkTImage::Pointer itkTPointer;
  typedef typename itk::ImageDuplicator< itkTImage > itkTDuplicator;
  typedef typename itk::ImageRegionIterator< itkTImage > itkTIterator;
  typedef typename itk::ImageRegionConstIterator< itkTImage > itkTConstIterator;
  typedef typename itk::ImageRegionIteratorWithIndex< itkTImage > itkTIteratorWithIndex;

  typedef typename itk::Image< float, 3> itkFloatImage;
  typedef typename itkFloatImage::Pointer itkFloatPointer;
  typedef typename itk::ImageRegionIterator< itkFloatImage > itkFloatIterator;
  typedef typename itk::ImageRegionIteratorWithIndex< itkFloatImage > itkFloatIteratorWithIndex;


  itkTPointer m_inputImage;
  itkTPointer m_outputImage;
  itkTPointer m_maskImage;
  itkTPointer m_refImage;
  itkFloatPointer m_meanImage;
  itkFloatPointer m_varianceImage;
  
  //Image information (size, spacing etc.)
  typename itkTImage::SpacingType m_spacing;
  typename itkTImage::SizeType    m_size;
  typename itkTImage::RegionType  m_region;
  
  typename itkTImage::SizeType m_halfPatchSize;          //half of the patch size
  typename itkTImage::SizeType m_fullPatchSize;          //patch size  : 2 * halfPatchSize + 1
  typename itkTImage::SizeType m_halfSpatialBandwidth;   //equivalent to the half size of the volume search area in non-local means
  typename itkTImage::SizeType m_fullSpatialBandwidth;   //spatial bandwidth : 2 * halfSpatialBandwidth + 1

  float m_padding;
  float m_rangeBandwidth;
  int   m_centralPointStrategy;
  int   m_blockwise;
  int   m_optimized;
  float m_lowerMeanThreshold;
  float m_lowerVarianceThreshold;
  bool  m_useTheReferenceImage;

  void SetInput(itkTPointer inputImage);
  void SetReferenceImage(itkTPointer refImage);
  void SetDefaultParameters();
  void SetPatchSize(int h);
  void SetSpatialBandwidth(int s);
  void SetPaddingValue(float padding);
  void SetMaskImage(itkTPointer maskImage);
  void SetCentralPointStrategy(int s);
  void SetBlockwiseStrategy(int b);
  void SetOptimizationStrategy(int o);
  void SetLowerThresholds(float m, float v);
  void SetSmoothing(float beta);                       //set the smoothing parameter and compute the corrected smoothing value (m_rangeBandwidth)
  void SetSmoothingUsingReferenceImage(float beta);    //set the smoothing parameter and compute the corrected smoothing value (m_rangeBandwidth) using the reference image
  void GetOutput(itkTPointer & outputImage);
  void ComputeOutput();
  void PrintInfo();
  
  void CreatePatch(itkTPointer & patch);
  void CreatePatch(itkFloatPointer & patch);
  void GetPatch(typename itkTImage::IndexType p, itkTPointer & patch);
  void GetPatchFromReferenceImage(typename itkTImage::IndexType p, itkTPointer & patch);
  void AddPatchToImage(typename itkTImage::IndexType p, itkFloatPointer & patch, itkFloatPointer & image, itkFloatPointer & weightImage, double weight);
  double PatchDistance(itkTPointer & p,itkTPointer & q);

  double GetDenoisedPatch(typename itkTImage::IndexType p, itkFloatPointer & patch);
  double GetDenoisedPatchUsingTheReferenceImage(typename itkTImage::IndexType p, itkFloatPointer & patch);
  void ComputeSearchRegion(typename itkTImage::IndexType p, typename itkTImage::RegionType & region);
  void ComputePatchRegion(typename itkTImage::IndexType p, typename itkTImage::RegionType & imageRegion, typename itkTImage::RegionType & patchRegion);
  bool CheckSpeed(typename itkTImage::IndexType p, typename itkTImage::IndexType q);
};

template <typename T>
void btkNLMTool<T>::SetInput(itkTPointer inputImage)
{
  m_inputImage = inputImage; 
  //compute characteristics of the input image
  m_region  = m_inputImage->GetLargestPossibleRegion();
  m_size    = m_region.GetSize();
  m_spacing = m_inputImage->GetSpacing();
  
  //duplicate the input image into the output image to keep all header information
  typename itkTDuplicator::Pointer duplicator = itkTDuplicator::New();
  duplicator->SetInputImage( inputImage );
  duplicator->Update();
  m_outputImage = duplicator->GetOutput();
  m_useTheReferenceImage = false;
}

template <typename T>
void btkNLMTool<T>::SetReferenceImage(itkTPointer refImage)
{
  m_refImage = refImage;
  m_useTheReferenceImage = true;
}

template <typename T>
void btkNLMTool<T>::SetDefaultParameters()
{
  SetPaddingValue(0);
  SetPatchSize(1);
  SetSpatialBandwidth(5);
  SetSmoothing(1);
  SetCentralPointStrategy(-1);
  SetBlockwiseStrategy(2);
  SetOptimizationStrategy(1);
  SetLowerThresholds(0.95, 0.5);  
}

template <typename T>
void btkNLMTool<T>::SetPatchSize(int h)
{
  std::cout<<"Computing patch size (taking into account possible image anisotropy)\n";    
  float minVoxSz = m_spacing[0];
  if(m_spacing[1] < minVoxSz) minVoxSz = m_spacing[1];
  if(m_spacing[2] < minVoxSz) minVoxSz = m_spacing[2];

  m_halfPatchSize[0] = (int)(0.5 + h * minVoxSz / m_spacing[0]);
  m_halfPatchSize[1] = (int)(0.5 + h * minVoxSz / m_spacing[1]);
  m_halfPatchSize[2] = (int)(0.5 + h * minVoxSz / m_spacing[2]);
  std::cout<<"patchSize : "<<m_halfPatchSize[0]<<" "<<m_halfPatchSize[1]<<" "<<m_halfPatchSize[2]<<"\n";
  
  m_fullPatchSize[0] = 2 * m_halfPatchSize[0] + 1;
  m_fullPatchSize[1] = 2 * m_halfPatchSize[1] + 1;
  m_fullPatchSize[2] = 2 * m_halfPatchSize[2] + 1;
}

template <typename T>
void btkNLMTool<T>::SetSpatialBandwidth(int s)
{
  std::cout<<"Computing spatial bandwidth (taking into account possible image anisotropy)\n";    
  float minVoxSz = m_spacing[0];
  if(m_spacing[1] < minVoxSz) minVoxSz = m_spacing[1];
  if(m_spacing[2] < minVoxSz) minVoxSz = m_spacing[2];

  m_halfSpatialBandwidth[0] = (int)(0.5 + s * minVoxSz / m_spacing[0]);
  m_halfSpatialBandwidth[1] = (int)(0.5 + s * minVoxSz / m_spacing[1]);
  m_halfSpatialBandwidth[2] = (int)(0.5 + s * minVoxSz / m_spacing[2]);
  std::cout<<"spatialBandwidth : "<<m_halfSpatialBandwidth[0]<<" "<<m_halfSpatialBandwidth[1]<<" "<<m_halfSpatialBandwidth[2]<<"\n";  
  
  m_fullSpatialBandwidth[0] = 2 * m_halfSpatialBandwidth[0] + 1;
  m_fullSpatialBandwidth[1] = 2 * m_halfSpatialBandwidth[1] + 1;
  m_fullSpatialBandwidth[2] = 2 * m_halfSpatialBandwidth[2] + 1;
}

template <typename T>
void btkNLMTool<T>::SetPaddingValue(float padding)
{
  m_padding = padding;
  
  std::cout<<"Creating the mask image using the padding value ("<<padding<<")\n";
  typename itkTDuplicator::Pointer duplicator = itkTDuplicator::New();
  duplicator->SetInputImage( m_inputImage );
  duplicator->Update();
  m_maskImage = duplicator->GetOutput();

  double count = 0;
  itkTIterator iterator( m_maskImage, m_maskImage->GetRequestedRegion() );
  for(iterator.GoToBegin(); !iterator.IsAtEnd(); ++iterator){
    if(iterator.Get() <= padding)
      iterator.Set(0);
    else{
      iterator.Set(1);
      count++;
    }
  }
  std::cout<<"Percentage of points to be processed : "<<(int)(count / m_maskImage->GetLargestPossibleRegion().GetNumberOfPixels() * 100.0) <<"\n";
  
}

template <typename T>
void btkNLMTool<T>::SetMaskImage(itkTPointer maskImage)
{
  m_maskImage = maskImage; 
  
  //check the size and spacing of the mask wrt the input image
  typename itkTImage::SpacingType spacing;
  typename itkTImage::SizeType    size;
  typename itkTImage::RegionType  region;
  
  region  = m_maskImage->GetLargestPossibleRegion();
  size    = m_region.GetSize();
  spacing = m_maskImage->GetSpacing(); 
  
  typename itkTImage::IndexType q;
  for(unsigned int i=0; i!= q.GetIndexDimension(); i++)
    if( (size[i] != m_size[i]) || (spacing[i] != m_spacing[i]) ){
      std::cout<<"*************************************************************************************\n";
      std::cout<<"WARNING : the size or the spacing of the mask image are incorrect wrt the input image\n";
      std::cout<<"*************************************************************************************\n";      
      break;
    }
  
  double count = 0;
  itkTIterator iterator( m_maskImage, m_maskImage->GetRequestedRegion() );
  for(iterator.GoToBegin(); !iterator.IsAtEnd(); ++iterator){
    if(iterator.Get() > 0)
      count++;
  }
  std::cout<<"Percentage of points to be processed : "<<(int)(count / m_maskImage->GetLargestPossibleRegion().GetNumberOfPixels() * 100.0) <<"\n";
  
  
}

template <typename T>
void btkNLMTool<T>::SetCentralPointStrategy(int s)
{
  m_centralPointStrategy = s;
}

template <typename T>
void btkNLMTool<T>::SetBlockwiseStrategy(int b)
{
  m_blockwise = b;
}

template <typename T>
void btkNLMTool<T>::SetOptimizationStrategy(int o)
{
  m_optimized = o;

  if(o==1){
    std::cout<<"Optimized mode. Computing Mean and Variance images\n";
    m_meanImage = itkFloatImage::New();
    m_meanImage->SetRegions(m_inputImage->GetLargestPossibleRegion());
    m_meanImage->SetSpacing( m_inputImage->GetSpacing() );
    m_meanImage->SetOrigin( m_inputImage->GetOrigin() );
    m_meanImage->SetDirection( m_inputImage->GetDirection() );
    m_meanImage->Allocate();
    m_meanImage->FillBuffer(0); 

    m_varianceImage = itkFloatImage::New();
    m_varianceImage->SetRegions(m_inputImage->GetLargestPossibleRegion());
    m_varianceImage->SetSpacing( m_inputImage->GetSpacing() );
    m_varianceImage->SetOrigin( m_inputImage->GetOrigin() );
    m_varianceImage->SetDirection( m_inputImage->GetDirection() );
    m_varianceImage->Allocate();
    m_varianceImage->FillBuffer(0); 

    int x,y,z;
    #pragma omp parallel for private(x,y,z) schedule(dynamic)
    for(z=0; z < (int)m_size[2]; z++)
      for(y=0; y < (int)m_size[1]; y++)
        for(x=0; x < (int)m_size[0]; x++){
	  typename itkTImage::IndexType p;
	  p[0] = x;
	  p[1] = y;
	  p[2] = z;

  	  if( m_maskImage->GetPixel(p) > 0 ){	  
	    itkTPointer patch = itkTImage::New();
            CreatePatch(patch);
            GetPatch(p,patch);
            itkTIterator itp( patch, patch->GetRequestedRegion() );
            double m = 0;
            double m2= 0;
            for(itp.GoToBegin(); !itp.IsAtEnd(); ++itp){
              m += itp.Get();
              m2+= (itp.Get() * itp.Get());	
            }
            int n = patch->GetLargestPossibleRegion().GetNumberOfPixels();
            float mean = m / n;
            float variance = (m2 / n) - (mean * mean) ;
            
            m_meanImage->SetPixel( p, mean );	  	  
            m_varianceImage->SetPixel( p, variance );	  	  
	  }
      }

  }
}

template <typename T>
void btkNLMTool<T>::SetLowerThresholds(float m, float v)
{
  m_lowerMeanThreshold = m;
  m_lowerVarianceThreshold = v;
}


template <typename T>
void btkNLMTool<T>::SetSmoothing(float beta)
{
  //this function should be rewritten using a convolution-based approach
  std::cout<<"Computing the range bandwidth (corresponding to the smoothing parameter for the NLM algorithm).\n";
  uint count = 0;
  double sigma2 = 0;
  int x,y,z;
  typename itkTImage::IndexType pixelIndex;	
  float ei = 0;
  std::vector<float> vecei;
  T value = 0;
  //since we have use to use a neighborhood around the current voxel, we neglect the border to avoid slow tests.
  for(z=1;z<(int)m_size[2]-1;z++)
    for(y=1;y<(int)m_size[1]-1;y++)
      for(x=1;x<(int)m_size[0]-1;x++){
	pixelIndex[0] = x;
	pixelIndex[1] = y;
	pixelIndex[2] = z;
	value = m_inputImage->GetPixel(pixelIndex);
        if( m_maskImage->GetPixel(pixelIndex) > 0){
	  //pourrait se faire avec une convolution !
	  pixelIndex[0] = x+1;	  pixelIndex[1] = y;	  pixelIndex[2] = z;
	  ei = m_inputImage->GetPixel(pixelIndex);
	  pixelIndex[0] = x-1;	  pixelIndex[1] = y;	  pixelIndex[2] = z;
	  ei += m_inputImage->GetPixel(pixelIndex);
	  pixelIndex[0] = x;	          pixelIndex[1] = y+1;	  pixelIndex[2] = z;
	  ei += m_inputImage->GetPixel(pixelIndex);
	  pixelIndex[0] = x;  	  pixelIndex[1] = y-1;	  pixelIndex[2] = z;
	  ei += m_inputImage->GetPixel(pixelIndex);
	  pixelIndex[0] = x;  	  pixelIndex[1] = y;	  pixelIndex[2] = z+1;
	  ei += m_inputImage->GetPixel(pixelIndex);
	  pixelIndex[0] = x;  	  pixelIndex[1] = y;	  pixelIndex[2] = z-1;
	  ei += m_inputImage->GetPixel(pixelIndex);
	  pixelIndex[0] = x;            pixelIndex[1] = y;	  pixelIndex[2] = z;
	  ei = sqrt(6.0/7.0)*(m_inputImage->GetPixel(pixelIndex) -ei/6.0);
	    
	  sigma2 += ei*ei;
	  count ++;
          if(fabs(ei>0))
            vecei.push_back(fabs(ei));
        }
      }
  //Estimation of sigma with MAD
  std::sort(vecei.begin(), vecei.end());
  float med = vecei[(int)(vecei.size()/2)];
  for(uint i=0; i<vecei.size(); i++)
    vecei[i] = fabs(vecei[i] - med);
  std::sort(vecei.begin(), vecei.end());
    
  sigma2 = 1.4826 * vecei[(int)(vecei.size()/2)];
  sigma2 = sigma2 * sigma2;
  std::cout<<"Number of points for estimation : "<<count<<"\n";
  std::cout<<"Estimated Global Sigma : "<<sqrt(sigma2)<<"\n";
  float NLMsmooth = 2 * beta * sigma2 * (2*m_halfPatchSize[0]+1) * (2*m_halfPatchSize[1]+1) * (2*m_halfPatchSize[2]+1);
  std::cout<<"Global smoothing parameter h : "<<sqrt(NLMsmooth)<<"\n";
  m_rangeBandwidth = NLMsmooth;   
}

template <typename T>
void btkNLMTool<T>::SetSmoothingUsingReferenceImage(float beta)
{
  //this function should be rewritten using a convolution-based approach
  std::cout<<"Computing the range bandwidth (corresponding to the smoothing parameter for the NLM algorithm).\n";
  uint count = 0;
  double sigma2 = 0;
  int x,y,z;
  typename itkTImage::IndexType pixelIndex;	
  float ei = 0;
  std::vector<float> vecei;
  T value = 0;
  //since we have use to use a neighborhood around the current voxel, we neglect the border to avoid slow tests.
  for(z=1;z<(int)m_size[2]-1;z++)
    for(y=1;y<(int)m_size[1]-1;y++)
      for(x=1;x<(int)m_size[0]-1;x++){
        pixelIndex[0] = x;
        pixelIndex[1] = y;
        pixelIndex[2] = z;
        value = m_refImage->GetPixel(pixelIndex);
        if( m_maskImage->GetPixel(pixelIndex) > 0){
          //pourrait se faire avec une convolution !
          pixelIndex[0] = x+1;	  pixelIndex[1] = y;	  pixelIndex[2] = z;
          ei = m_refImage->GetPixel(pixelIndex);
          pixelIndex[0] = x-1;	  pixelIndex[1] = y;	  pixelIndex[2] = z;
          ei += m_refImage->GetPixel(pixelIndex);
          pixelIndex[0] = x;	          pixelIndex[1] = y+1;	  pixelIndex[2] = z;
          ei += m_refImage->GetPixel(pixelIndex);
          pixelIndex[0] = x;  	  pixelIndex[1] = y-1;	  pixelIndex[2] = z;
          ei += m_refImage->GetPixel(pixelIndex);
          pixelIndex[0] = x;  	  pixelIndex[1] = y;	  pixelIndex[2] = z+1;
          ei += m_refImage->GetPixel(pixelIndex);
          pixelIndex[0] = x;  	  pixelIndex[1] = y;	  pixelIndex[2] = z-1;
          ei += m_refImage->GetPixel(pixelIndex);
          pixelIndex[0] = x;            pixelIndex[1] = y;	  pixelIndex[2] = z;
          ei = sqrt(6.0/7.0)*(m_refImage->GetPixel(pixelIndex) -ei/6.0);
          
          sigma2 += ei*ei;
          count ++;
          if(fabs(ei>0))
            vecei.push_back(fabs(ei));
        }
      }
  //Estimation of sigma with MAD
  std::sort(vecei.begin(), vecei.end());
  float med = vecei[(int)(vecei.size()/2)];
  for(uint i=0; i<vecei.size(); i++)
    vecei[i] = fabs(vecei[i] - med);
  std::sort(vecei.begin(), vecei.end());
  
  sigma2 = 1.4826 * vecei[(int)(vecei.size()/2)];
  sigma2 = sigma2 * sigma2;
  std::cout<<"Number of points for estimation : "<<count<<"\n";
  std::cout<<"Estimated Global Sigma : "<<sqrt(sigma2)<<"\n";
  float NLMsmooth = 2 * beta * sigma2 * (2*m_halfPatchSize[0]+1) * (2*m_halfPatchSize[1]+1) * (2*m_halfPatchSize[2]+1);
  std::cout<<"Global smoothing parameter h : "<<sqrt(NLMsmooth)<<"\n";
  m_rangeBandwidth = NLMsmooth;   
}

template <typename T>
void btkNLMTool<T>::GetOutput(itkTPointer & outputImage)
{
  outputImage = m_outputImage; 
}

template <typename T>
void btkNLMTool<T>::ComputeOutput()
{
  std::cout<<"Compute the denoised image using NLM algorithm\n";
  if(m_useTheReferenceImage == true)
    std::cout<<"Use of a reference image\n";
  
  itkFloatPointer denoisedImage = itkFloatImage::New();
  denoisedImage->SetRegions(m_inputImage->GetLargestPossibleRegion());
  denoisedImage->SetSpacing( m_inputImage->GetSpacing() );
  denoisedImage->SetOrigin( m_inputImage->GetOrigin() );
  denoisedImage->SetDirection( m_inputImage->GetDirection() );
  denoisedImage->Allocate();
  denoisedImage->FillBuffer(0); 

  
  itkFloatIterator denoisedIt( denoisedImage, denoisedImage->GetLargestPossibleRegion() );
  itkTIterator outputIt( m_outputImage, m_outputImage->GetLargestPossibleRegion());

  int x,y,z;
	  
  if(m_blockwise == 0){
    std::cout<<"pointwise denoising\n";
    typename itkTImage::IndexType q;
    for(unsigned int i=0; i!= q.GetIndexDimension(); i++)
      q[i] = m_halfPatchSize[i];
      #pragma omp parallel for private(x,y,z) schedule(dynamic)
      for(z=0; z < (int)m_size[2]; z++)
        for(y=0; y < (int)m_size[1]; y++)
          for(x=0; x < (int)m_size[0]; x++){
            typename itkTImage::IndexType p;
            p[0] = x;
            p[1] = y;
            p[2] = z;

            if( m_maskImage->GetPixel(p) > 0 ){	  
              itkFloatPointer patch = itkFloatImage::New();
              double sum = 0;
              if(m_useTheReferenceImage==false) sum = GetDenoisedPatch(p, patch);
              else sum = GetDenoisedPatchUsingTheReferenceImage(p, patch);

              denoisedImage->SetPixel( p, patch->GetPixel(q) );	  	  
            }
          }
  }
  if(m_blockwise >= 1){
    itkFloatPointer weightImage = itkFloatImage::New();
    weightImage->SetRegions(m_inputImage->GetLargestPossibleRegion());
    weightImage->SetSpacing( m_inputImage->GetSpacing() );
    weightImage->SetOrigin( m_inputImage->GetOrigin() );
    weightImage->SetDirection( m_inputImage->GetDirection() );
    weightImage->Allocate();
    weightImage->FillBuffer(0); 

    if(m_blockwise == 1){
      std::cout<<"blockwise denoising\n";
      #pragma omp parallel for private(x,y,z) schedule(dynamic)
      for(z=0; z < (int)m_size[2]; z++)
        for(y=0; y < (int)m_size[1]; y++)
          for(x=0; x < (int)m_size[0]; x++){
            typename itkTImage::IndexType p;
            p[0] = x;
            p[1] = y;
            p[2] = z;

            if( m_maskImage->GetPixel(p) > 0 ){	  
              itkFloatPointer patch = itkFloatImage::New();
              double sum = 0;
              if(m_useTheReferenceImage==false) sum = GetDenoisedPatch(p, patch);
              else sum = GetDenoisedPatchUsingTheReferenceImage(p, patch);
              
              double weight = 1.0;
              #pragma omp critical
              AddPatchToImage(p, patch, denoisedImage, weightImage, weight);
            }
          }
    }
    if(m_blockwise == 2){
      std::cout<<"fast blockwise denoising\n";
      #pragma omp parallel for private(x,y,z) schedule(dynamic)
      for(z=0; z < (int)m_size[2]; z++)
        if( z%(m_halfPatchSize[2]+1) == 0){
        for(y=0; y < (int)m_size[1]; y++)
          if( y%(m_halfPatchSize[1]+1) == 0){
          for(x=0; x < (int)m_size[0]; x++)
            if( x%(m_halfPatchSize[0]+1) == 0 ){
              typename itkTImage::IndexType p;
              p[0] = x;
              p[1] = y;
              p[2] = z;

              if( m_maskImage->GetPixel(p) > 0 ){	  
                itkFloatPointer patch = itkFloatImage::New();
                double sum = 0;
                if(m_useTheReferenceImage==false) sum = GetDenoisedPatch(p, patch);
                else sum = GetDenoisedPatchUsingTheReferenceImage(p, patch);
                
                double weight = 1.0;
                #pragma omp critical
                AddPatchToImage(p, patch, denoisedImage, weightImage, weight);
              }
            }
          }
        }
    }

    itkFloatIterator weightIt( weightImage, weightImage->GetLargestPossibleRegion() );
    //weight normalization
    for ( denoisedIt.GoToBegin(), weightIt.GoToBegin(); !denoisedIt.IsAtEnd(); ++denoisedIt, ++weightIt)
      if( weightIt.Get() > 0 )
        denoisedIt.Set( denoisedIt.Get() / weightIt.Get() );

  }

  //Convert float data from denoisedImage to T data for m_outputImage
  for ( denoisedIt.GoToBegin(), outputIt.GoToBegin(); !denoisedIt.IsAtEnd(); ++denoisedIt, ++outputIt)
    //outputIt.Set( (T)rint(denoisedIt.Get()) );
    outputIt.Set( (T)(denoisedIt.Get()) );

  //This can be done also using an ITK filter, but in this case, the output data are truncated (not rounded)
  /*
  typename itk::CastImageFilter< itkFloatImage, itkTImage >::Pointer castFilter = itk::CastImageFilter< itkFloatImage, itkTImage >::New();
  castFilter->SetInput(denoisedImage);
  castFilter->Update();
  m_outputImage = castFilter->GetOutput();
  */
  
}



template <typename T>
void btkNLMTool<T>::PrintInfo()
{
  //Image information (size, spacing etc.)
  std::cout<<"size of the input image: "<<m_size[0]<<" "<<m_size[1]<<" "<<m_size[2]<<"\n";
  std::cout<<"spacing of the input image: "<<m_spacing[0]<<" "<<m_spacing[1]<<" "<<m_spacing[2]<<"\n";
  
  //Image information (size, spacing etc.)
  typename itkTImage::SpacingType spacing;
  typename itkTImage::SizeType    size;
  typename itkTImage::RegionType  region;
  
  region  = m_outputImage->GetLargestPossibleRegion();
  size    = region.GetSize();
  std::cout<<"size of the output image: "<<size[0]<<" "<<size[1]<<" "<<size[2]<<"\n";
  spacing = m_outputImage->GetSpacing();
  std::cout<<"spacing of the output image: "<<spacing[0]<<" "<<spacing[1]<<" "<<spacing[2]<<"\n";
  
}

template <typename T>
void btkNLMTool<T>::CreatePatch(itkTPointer & patch)
{
  //resize and allocate the patch and set the estimate to 0
  typename itkTImage::SizeType size = m_fullPatchSize;
  typename itkTImage::RegionType region;
  region.SetSize(size);
  patch->SetRegions(region);
  patch->Allocate();
  patch->FillBuffer(0);  
}


template <typename T>
void btkNLMTool<T>::CreatePatch(itkFloatPointer & patch)
{
  //resize and allocate the patch and set the estimate to 0
  typename itkFloatImage::SizeType size = m_fullPatchSize;
  typename itkFloatImage::RegionType region;
  region.SetSize(size);
  patch->SetRegions(region);
  patch->Allocate();
  patch->FillBuffer(0.0);  
}

template <typename T>
void btkNLMTool<T>::GetPatch(typename itkTImage::IndexType p, itkTPointer & patch)
{
  //this function is equivalent to a cropping function
  //WARNING : no check about the size of the input patch !
  patch->FillBuffer(0);
  typename itkTImage::RegionType imageRegion;
  typename itkTImage::RegionType patchRegion;
  ComputePatchRegion(p,imageRegion,patchRegion);
  
  itkTConstIterator inputIt( m_inputImage, imageRegion);
  itkTIterator outputIt( patch, patchRegion);

  for ( inputIt.GoToBegin(), outputIt.GoToBegin(); !inputIt.IsAtEnd(); ++inputIt, ++outputIt)
    outputIt.Set( inputIt.Get() );

  
  //Old school style :)
  /*
  typename itkTImage::IndexType pixelIndex;	
  typename itkTImage::IndexType patchIndex;

  for(int pz=-(int)m_halfPatchSize[2];pz<=(int)m_halfPatchSize[2];pz++){
    pixelIndex[2] = p[2] + pz;
    patchIndex[2] = pz + m_halfPatchSize[2];
    if( (pixelIndex[2]>=0) && (pixelIndex[2]<(int)m_size[2]) ){
      for(int py=-(int)m_halfPatchSize[1];py<=(int)m_halfPatchSize[1];py++){
        pixelIndex[1] = p[1] + py;
        patchIndex[1] = py + m_halfPatchSize[1];
        if( (pixelIndex[1]>=0) && (pixelIndex[1]<(int)m_size[1]) ){
          for(int px=-(int)m_halfPatchSize[0];px<=(int)m_halfPatchSize[0];px++){
            pixelIndex[0] = p[0] + px;
            patchIndex[0] = px + m_halfPatchSize[0];
            if( (pixelIndex[0]>=0) && (pixelIndex[0]<(int)m_size[0]) ){
              patch->SetPixel(patchIndex, m_inputImage->GetPixel(pixelIndex));
            }
          }
        }
      }
    }
  }
  */
  
}

template <typename T>
void btkNLMTool<T>::GetPatchFromReferenceImage(typename itkTImage::IndexType p, itkTPointer & patch)
{
  //this function is equivalent to a cropping function
  //WARNING : no check about the size of the input patch !
  patch->FillBuffer(0);
  typename itkTImage::RegionType imageRegion;
  typename itkTImage::RegionType patchRegion;
  ComputePatchRegion(p,imageRegion,patchRegion);
  
  itkTConstIterator refIt( m_refImage, imageRegion);
  itkTIterator outputIt( patch, patchRegion);
  
  for ( refIt.GoToBegin(), outputIt.GoToBegin(); !refIt.IsAtEnd(); ++refIt, ++outputIt)
    outputIt.Set( refIt.Get() );
}

template <typename T>
void btkNLMTool<T>::AddPatchToImage(typename itkTImage::IndexType p, itkFloatPointer & patch, itkFloatPointer & image, itkFloatPointer & weightImage, double weight)
{
  //this function add a patch value to the denoised image
  typename itkTImage::RegionType imageRegion;
  typename itkTImage::RegionType patchRegion;
  ComputePatchRegion(p,imageRegion,patchRegion);
  
  itkFloatIterator imageIt( image, imageRegion);
  itkFloatIterator weightIt( weightImage, imageRegion);
  itkFloatIterator patchIt( patch, patchRegion);

  for ( imageIt.GoToBegin(), patchIt.GoToBegin(), weightIt.GoToBegin(); !imageIt.IsAtEnd(); ++imageIt, ++patchIt, ++weightIt){
    imageIt.Set( imageIt.Get() + patchIt.Get() );
    weightIt.Set( weightIt.Get() + weight );
  }
  
}

template <typename T>
double btkNLMTool<T>::PatchDistance(itkTPointer & p,itkTPointer & q)
{
  double diff=0;
  double dist = 0;
  itkTConstIterator itp( p, p->GetRequestedRegion() );
  itkTConstIterator itq( q, q->GetRequestedRegion() );

  for(itp.GoToBegin(), itq.GoToBegin(); !itp.IsAtEnd(); ++itp, ++itq){
    diff = itp.Get() - itq.Get();
    dist += diff*diff;	
  }
  return dist;
}

template <typename T>
double btkNLMTool<T>::GetDenoisedPatch(typename itkTImage::IndexType p, itkFloatPointer & patch)
{
  double wmax = 0; //maximum weight of patches
  double sum  = 0; //sum of weights (used for normalization purpose)
  
  //create the patch and set the estimate to 0
  CreatePatch(patch);
  itkFloatIterator patchIt(patch, patch->GetRequestedRegion());

  //get the value of the patch around the current pixel
  itkTPointer centralPatch = itkTImage::New();
  CreatePatch(centralPatch);
  GetPatch(p,centralPatch);
  itkTIterator centralPatchIt(centralPatch, centralPatch->GetRequestedRegion());

  //set the search region around the current pixel
  typename itkTImage::RegionType searchRegion;
  ComputeSearchRegion(p,searchRegion);
  
  //create the patch for pixels in the neighbourhood of the current pixel
  itkTPointer neighbourPatch = itkTImage::New();
  CreatePatch(neighbourPatch);
  itkTIterator neighbourPatchIt(neighbourPatch, neighbourPatch->GetRequestedRegion());
  
  //go through the neighbourhood with a region iterator
  itkTIteratorWithIndex it( m_inputImage, searchRegion);
  typename itkTImage::IndexType neighbourPixelIndex;	

  for(it.GoToBegin(); !it.IsAtEnd(); ++it){
    neighbourPixelIndex = it.GetIndex();

    bool goForIt = true;
    if(m_optimized == 1)
      goForIt = CheckSpeed(p, neighbourPixelIndex);

    if(goForIt == true){
    
      GetPatch(neighbourPixelIndex, neighbourPatch);  
      double weight = exp( - PatchDistance(centralPatch, neighbourPatch) / m_rangeBandwidth);

      if(weight>wmax)
        if( (p[0] != neighbourPixelIndex[0]) && (p[1] != neighbourPixelIndex[1]) && (p[2] != neighbourPixelIndex[2]) ) //has to be modify
	  wmax = weight;
      
      sum += weight;
    
      //Add this patch to the current estimate using the computed weight
      for(patchIt.GoToBegin(), neighbourPatchIt.GoToBegin(); !patchIt.IsAtEnd(); ++patchIt, ++neighbourPatchIt){
        patchIt.Set( patchIt.Get() + neighbourPatchIt.Get() * weight );
      }
    }
  }

  
  //consider now the special case of the central patch
  switch(m_centralPointStrategy){
    case 0:                                        //remove the central patch to the estimated patch
      for(patchIt.GoToBegin(), centralPatchIt.GoToBegin(); !patchIt.IsAtEnd(); ++patchIt, ++centralPatchIt)
        patchIt.Set( patchIt.Get() -1.0 * centralPatchIt.Get() );
      sum -= 1.0;
      break;
    case 1: break;                                 //nothing to do
    case -1:
      for(patchIt.GoToBegin(), centralPatchIt.GoToBegin(); !patchIt.IsAtEnd(); ++patchIt, ++centralPatchIt)
        patchIt.Set( patchIt.Get()  + (wmax -1.0) * centralPatchIt.Get() );
      sum += (wmax - 1.0);
      break;
    default:                                       //as in case -1
      for(patchIt.GoToBegin(), centralPatchIt.GoToBegin(); !patchIt.IsAtEnd(); ++patchIt, ++centralPatchIt)
        patchIt.Set( patchIt.Get()  + (wmax -1.0) * centralPatchIt.Get() );
      sum += (wmax - 1.0);
      break;
  }  
    
  if(sum>0.0001)
    //Normalization of the denoised patch 
    for(patchIt.GoToBegin(); !patchIt.IsAtEnd(); ++patchIt)
      patchIt.Set( patchIt.Get() / sum );
  else
    //copy the central patch to the denoised patch
    for(patchIt.GoToBegin(), centralPatchIt.GoToBegin(); !patchIt.IsAtEnd(); ++patchIt, ++centralPatchIt)
      patchIt.Set( centralPatchIt.Get() );  
    
  return sum;
}

template <typename T>
double btkNLMTool<T>::GetDenoisedPatchUsingTheReferenceImage(typename itkTImage::IndexType p, itkFloatPointer & patch)
{
  double wmax = 0; //maximum weight of patches
  double sum  = 0; //sum of weights (used for normalization purpose)
  
  //create the patch and set the estimate to 0
  CreatePatch(patch);
  itkFloatIterator patchIt(patch, patch->GetRequestedRegion());
  
  //get the value of the patch around the current pixel
  itkTPointer centralPatch = itkTImage::New();
  CreatePatch(centralPatch);
  GetPatch(p,centralPatch);
  itkTIterator centralPatchIt(centralPatch, centralPatch->GetRequestedRegion());

  //get the value of the patch around the current pixel in the reference image
  itkTPointer centralPatchReferenceImage = itkTImage::New();
  CreatePatch(centralPatchReferenceImage);
  GetPatchFromReferenceImage(p,centralPatchReferenceImage);
  
  //set the search region around the current pixel
  typename itkTImage::RegionType searchRegion;
  ComputeSearchRegion(p,searchRegion);
  
  //create the patch for pixels in the neighbourhood of the current pixel
  itkTPointer neighbourPatch = itkTImage::New();
  CreatePatch(neighbourPatch);
  itkTIterator neighbourPatchIt(neighbourPatch, neighbourPatch->GetRequestedRegion());

  //create the patch for pixels in the neighbourhood of the current pixel in the reference image
  itkTPointer neighbourPatchReferenceImage = itkTImage::New();
  CreatePatch(neighbourPatchReferenceImage);
  
  //go through the neighbourhood with a region iterator
  itkTIteratorWithIndex it( m_inputImage, searchRegion);
  typename itkTImage::IndexType neighbourPixelIndex;	
  
  for(it.GoToBegin(); !it.IsAtEnd(); ++it){
    neighbourPixelIndex = it.GetIndex();
    
    bool goForIt = true;
    if(m_optimized == 1)
      goForIt = CheckSpeed(p, neighbourPixelIndex);
    
    if(goForIt == true){
      
      GetPatchFromReferenceImage(neighbourPixelIndex, neighbourPatchReferenceImage);
      GetPatch(neighbourPixelIndex, neighbourPatch);
      
      double weight = exp( - PatchDistance(centralPatchReferenceImage, neighbourPatchReferenceImage) / m_rangeBandwidth);
      
      if(weight>wmax)
        if( (p[0] != neighbourPixelIndex[0]) && (p[1] != neighbourPixelIndex[1]) && (p[2] != neighbourPixelIndex[2]) ) //has to be modify
          wmax = weight;
      
      sum += weight;
      
      //Add this patch to the current estimate using the computed weight
      for(patchIt.GoToBegin(), neighbourPatchIt.GoToBegin(); !patchIt.IsAtEnd(); ++patchIt, ++neighbourPatchIt){
        patchIt.Set( patchIt.Get() + neighbourPatchIt.Get() * weight );
      }
    }
  }
  
  
  //consider now the special case of the central patch
  switch(m_centralPointStrategy){
    case 0:                                        //remove the central patch to the estimated patch
      for(patchIt.GoToBegin(), centralPatchIt.GoToBegin(); !patchIt.IsAtEnd(); ++patchIt, ++centralPatchIt)
        patchIt.Set( patchIt.Get() -1.0 * centralPatchIt.Get() );
      sum -= 1.0;
      break;
    case 1: break;                                 //nothing to do
    case -1:
      for(patchIt.GoToBegin(), centralPatchIt.GoToBegin(); !patchIt.IsAtEnd(); ++patchIt, ++centralPatchIt)
        patchIt.Set( patchIt.Get()  + (wmax -1.0) * centralPatchIt.Get() );
      sum += (wmax - 1.0);
      break;
    default:                                       //as in case -1
      for(patchIt.GoToBegin(), centralPatchIt.GoToBegin(); !patchIt.IsAtEnd(); ++patchIt, ++centralPatchIt)
        patchIt.Set( patchIt.Get()  + (wmax -1.0) * centralPatchIt.Get() );
      sum += (wmax - 1.0);
      break;
  }  
  
  if(sum>0.0001)
    //Normalization of the denoised patch 
    for(patchIt.GoToBegin(); !patchIt.IsAtEnd(); ++patchIt)
      patchIt.Set( patchIt.Get() / sum );
  else
    //copy the central patch to the denoised patch
    for(patchIt.GoToBegin(), centralPatchIt.GoToBegin(); !patchIt.IsAtEnd(); ++patchIt, ++centralPatchIt)
      patchIt.Set( centralPatchIt.Get() );  
  
  return sum;
}


template <typename T>
bool btkNLMTool<T>::CheckSpeed(typename itkTImage::IndexType p, typename itkTImage::IndexType q)
{
  bool goForIt = true;

  float mSpeed = 0;
  if(m_meanImage->GetPixel(q) ==0){
    if(m_meanImage->GetPixel(p) == 0)
      mSpeed = 1;
    else
      mSpeed = 0;
  }
  else
    mSpeed = m_meanImage->GetPixel(p) / m_meanImage->GetPixel(q);

  if( (mSpeed < m_lowerMeanThreshold) || (mSpeed > 1/m_lowerMeanThreshold) )
    goForIt = false;

  float vSpeed = 0;
  if(m_varianceImage->GetPixel(q) ==0){
    if(m_varianceImage->GetPixel(p) == 0)
      vSpeed = 1;
    else
      vSpeed = 0;
  }
  else
    vSpeed = m_varianceImage->GetPixel(p) / m_varianceImage->GetPixel(q);

  if( (vSpeed < m_lowerVarianceThreshold) || (vSpeed > 1/m_lowerVarianceThreshold) )
    goForIt = false;

  return goForIt;
}

template <typename T>
void btkNLMTool<T>::ComputeSearchRegion(typename itkTImage::IndexType p, typename itkTImage::RegionType & region)
{
  //create an appropiate search region around the current pixel p
  typename itkTImage::RegionType::IndexType start;
  typename itkTImage::RegionType::SizeType size;
  
  for(unsigned int i=0; i!= p.GetIndexDimension(); i++){
    start[i] = p[i] - m_halfSpatialBandwidth[i];
    size[i] = m_fullSpatialBandwidth[i];
        
    if(start[i] < 0){                  //if the starting index is outside the image (<0)
      size[i] += start[i];
      start[i] = 0;
    }
    else if(start[i] >= (int)m_size[i]){    //if the starting index is outside the image (>image size)
      start[i] = m_size[i]-1;
      size[i] = 0;
    }
    
    int d = (start[i] + size[i]) - m_size[i]; //if the region is not fully inside the image
    if(d>0){
      size[i] = size[i] - d;
      if(size[i] < 0) size[i] = 0;
    }    
  }
  region.SetSize( size );
  region.SetIndex( start );
}

template <typename T>
void btkNLMTool<T>::ComputePatchRegion(typename itkTImage::IndexType p, typename itkTImage::RegionType & imageRegion, typename itkTImage::RegionType & patchRegion)
{
  //create an appropriate patch region around the current pixel p, and consider an offset for pixels close to boundaries.
  //the patch region is defined in the image coordinate system.
  //the offset allows to do the link between the patch coordinate system and the one of the image.
  typename itkTImage::RegionType::IndexType start;
  typename itkTImage::RegionType::IndexType offset;
  typename itkTImage::RegionType::SizeType size;

  for(unsigned int i=0; i!= p.GetIndexDimension(); i++){
    start[i] = p[i] - m_halfPatchSize[i];
    size[i] = m_fullPatchSize[i];
    offset[i] = 0;
    
    if(start[i] < 0){                  //if the starting index is outside the image (<0)
      size[i] += start[i];
      offset[i] = -start[i];
      start[i] = 0;
    }
    else if(start[i] >= (int)m_size[i]){    //if the starting index is outside the image (>image size)
      start[i] = m_size[i]-1;
      size[i] = 0;
    }
    
    int d = (start[i] + size[i]) - m_size[i]; //if the region is not fully inside the image
    if(d>0){
      size[i] = size[i] - d;
      if(size[i] < 0) size[i] = 0;
    }    
  }  
  imageRegion.SetSize( size );
  imageRegion.SetIndex( start );    
  patchRegion.SetSize( size );
  patchRegion.SetIndex( offset );    
}


#endif

