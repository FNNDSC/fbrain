/*==========================================================================

  © Université de Strasbourg - Centre National de la Recherche Scientifique

  Date: 18/11/2010
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

#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

#include "itkEuler3DTransform.h"

#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include "itkImageMaskSpatialObject.h"

#include "itkTransformFileReader.h"

#include "btkResampleLabelsByInjectionFilter.h"

#include <tclap/CmdLine.h>

int main( int argc, char *argv[] )
{

  try {

  std::vector< std::string > input;
  std::vector< std::string > flabel;
  std::vector< std::string > mask;
  std::vector< std::string > transform;
  std::vector< int >         labels;

  const char *outImage = NULL;
  const char *refImage = NULL;

  std::vector< int > x1, y1, z1, x2, y2, z2;

  // Parse arguments

  TCLAP::CmdLine cmd("Resample a set of images using the injection method", ' ', "Unversioned");

  TCLAP::MultiArg<std::string> inputArg("i","input","image file",true,"string",cmd);
  TCLAP::MultiArg<std::string> fLabelArg("f","fl","fuzzy label file",true,"string",cmd);
  TCLAP::MultiArg<int> labelArg("l","label","label number",true,"int",cmd);
  TCLAP::MultiArg<std::string> maskArg("m","mask","mask file",false,"string",cmd);
  TCLAP::ValueArg<std::string> refArg  ("r","reference","Reference image",true,"none","string",cmd);
  TCLAP::ValueArg<std::string> outArg  ("o","output","High resolution image",true,"none","string",cmd);

  TCLAP::MultiArg<std::string> transArg("t","transform","transform file",true,"string",cmd);

  // Parse the argv array.
  cmd.parse( argc, argv );

  input = inputArg.getValue();
  flabel = fLabelArg.getValue();
  mask = maskArg.getValue();
  refImage = refArg.getValue().c_str();
  outImage = outArg.getValue().c_str();
  transform = transArg.getValue();
  labels = labelArg.getValue();

  // typedefs
  const   unsigned int    Dimension = 3;
  typedef itk::Euler3DTransform< double > TransformType;
  typedef TransformType::Pointer                    TransformPointer;
  typedef std::vector<TransformPointer>             TransformPointerArray;

  typedef float  PixelType;

  typedef itk::Image< PixelType, Dimension >  ImageType;
  typedef ImageType::Pointer                  ImagePointer;
  typedef std::vector<ImagePointer>           ImagePointerArray;

  typedef itk::Image< unsigned char, Dimension >  ImageMaskType;
  typedef itk::ImageFileReader< ImageMaskType > MaskReaderType;
  typedef itk::ImageMaskSpatialObject< Dimension > MaskType;

  typedef ImageType::RegionType               RegionType;
  typedef std::vector< RegionType >           RegionArrayType;

  typedef itk::ImageFileReader< ImageType >   ImageReaderType;
  typedef itk::ImageFileWriter< ImageType >   WriterType;

  typedef itk::TransformFileReader     TransformReaderType;
  typedef TransformReaderType::TransformListType * TransformListType;

  typedef btk::ResampleLabelsByInjectionFilter< ImageType, ImageType >  ResamplerType;
  ResamplerType::Pointer resampler = ResamplerType::New();

  unsigned int numberOfLabels = 6;
  unsigned int numberOfImages = input.size();

  // Filter setup

  ImageType::IndexType  roiIndex;
  ImageType::SizeType   roiSize;

  for (unsigned int i=0; i<numberOfImages; i++)
  {
    // add image
    ImageReaderType::Pointer imageReader = ImageReaderType::New();
    imageReader -> SetFileName( input[i].c_str() );
    imageReader -> Update();
    resampler -> AddInput( imageReader -> GetOutput(), labels[i] );

    // add region
    if ( mask.size() > 0 )
    {
      MaskReaderType::Pointer maskReader = MaskReaderType::New();
      maskReader -> SetFileName( mask[i].c_str() );
      maskReader -> Update();

      MaskType::Pointer mask = MaskType::New();
      mask -> SetImage( maskReader -> GetOutput() );

      RegionType roi = mask -> GetAxisAlignedBoundingBoxRegion();
      roiIndex = roi.GetIndex();
      roiSize  = roi.GetSize();

    } else
        {
          roiSize  = imageReader -> GetOutput() -> GetLargestPossibleRegion().GetSize();
          roiIndex = imageReader -> GetOutput() -> GetLargestPossibleRegion().GetIndex();
        }

    RegionType imageRegion;
    imageRegion.SetIndex(roiIndex);
    imageRegion.SetSize (roiSize);
    resampler -> AddRegion( imageRegion );

    // set transformation
    TransformReaderType::Pointer transformReader = TransformReaderType::New();
    transformReader -> SetFileName( transform[i] );
    transformReader -> Update();

    TransformListType transforms = transformReader->GetTransformList();
    TransformReaderType::TransformListType::const_iterator titr = transforms->begin();

    for(unsigned int j=0; j<transforms -> size(); j++,titr++)
    {
      resampler -> SetTransform(i, j, dynamic_cast< TransformType * >( titr->GetPointer() ) ) ;
    }

  }

  // Set reference image

  ImageReaderType::Pointer refReader = ImageReaderType::New();
  refReader -> SetFileName( refImage );
  refReader -> Update();

  resampler -> UseReferenceImageOn();
  resampler -> SetReferenceImage( refReader -> GetOutput() );

  resampler -> Update();

  // Write fuzzy labels

  for (unsigned int i=0; i<numberOfLabels; i++)
  {
    resampler -> WriteFuzzyLabel( i, flabel[i].c_str() );
  }

  // Write image

  WriterType::Pointer writer =  WriterType::New();
  writer -> SetFileName( outImage );
  writer -> SetInput( resampler -> GetOutput() );

  if ( strcmp(outImage,"none") != 0)
  {
    writer->Update();
  }


  } catch (TCLAP::ArgException &e)  // catch any exceptions
  { std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; }

  return EXIT_SUCCESS;
}

