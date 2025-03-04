/*==========================================================================

  © Université de Strasbourg - Centre National de la Recherche Scientifique

  Date: 14/09/2010
  Author(s): Estanislao Oubel (oubel@unistra.fr)

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

==========================================================================*/

#ifndef _btkImageIntersectionCalculator_cxx
#define _btkImageIntersectionCalculator_cxx

#include "btkImageIntersectionCalculator.h"

namespace btk
{

/*
 * Constructor
 */
template < typename ImageType >
ImageIntersectionCalculator<ImageType>
::ImageIntersectionCalculator()
{

  m_NumberOfImages = 0;

}

template < typename ImageType >
void ImageIntersectionCalculator<ImageType>
::AddImage( ImageType * image)
{
  unsigned int n = m_NumberOfImages;

  m_ImageArray.resize(n+1);
  m_ImageArray[n] = image;

  m_ImageMaskArray.resize(n+1);
  m_ImageMaskArray[n] = ImageMaskType::New();
  m_ImageMaskArray[n] -> SetRegions( m_ImageArray[n] -> GetLargestPossibleRegion() );
  m_ImageMaskArray[n] -> Allocate();

  m_ImageMaskArray[n] -> SetOrigin( m_ImageArray[n] -> GetOrigin() );
  m_ImageMaskArray[n] -> SetSpacing( m_ImageArray[n] -> GetSpacing() );
  m_ImageMaskArray[n] -> SetDirection( m_ImageArray[n] -> GetDirection() );
  m_ImageMaskArray[n] -> FillBuffer( 0 );

  m_MaskArray.resize(n+1);
  m_MaskArray[n] = MaskType::New();

  m_RegionArray.resize(n+1);

  m_InterpolatorArray.resize(n+1);
  m_InterpolatorArray[n] = InterpolatorType::New();
  m_InterpolatorArray[n] -> SetInputImage( image );

  m_NumberOfImages++;

}

/*
 * Writes a specific transform to file
 */
template < typename ImageType >
void
ImageIntersectionCalculator<ImageType>
::Compute()
{
  IndexType index;
  PointType point;

  for (unsigned int i=0; i < m_NumberOfImages; i++)
  {
    IteratorType imageMaskIt( m_ImageMaskArray[i],
                         m_ImageMaskArray[i] -> GetLargestPossibleRegion() );

    // creates image mask
    for(imageMaskIt.GoToBegin(); !imageMaskIt.IsAtEnd(); ++imageMaskIt )
    {
      imageMaskIt.Set(1);
      index = imageMaskIt.GetIndex();
      m_ImageMaskArray[i] -> TransformIndexToPhysicalPoint(index, point);
      for (unsigned int j=0; j < m_NumberOfImages; j++)
      {
        if ( !(m_InterpolatorArray[j] -> IsInsideBuffer(point)) )
        {
          imageMaskIt.Set(0);
          break;
        }
      }
    }

    m_MaskArray[i] -> SetImage( m_ImageMaskArray[i] );
    m_RegionArray[i] = m_MaskArray[i] -> GetAxisAlignedBoundingBoxRegion();

  }
}

/*
 * Writes a specific mask to disk
 */
template < typename ImageType >
void
ImageIntersectionCalculator<ImageType>
::WriteMask( unsigned int i, const char *filename )
{

  typename ImageWriterType::Pointer imageWriter = ImageWriterType::New();

  imageWriter -> SetInput( m_ImageMaskArray[i] );
  imageWriter -> SetFileName ( filename );

  try
  {
    imageWriter -> Update();
  }
  catch ( itk::ExceptionObject & excp )
  {
    std::cerr << "Error while saving the resampled image" << std::endl;
    std::cerr << excp << std::endl;
    std::cout << "[FAILED]" << std::endl;
    throw excp;
  }

}


/*
 * PrintSelf
 */
template < typename ImageType >
void
ImageIntersectionCalculator<ImageType>
::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf( os, indent );
}


} // end namespace itk


#endif
