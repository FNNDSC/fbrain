/*==========================================================================

  © Université de Strasbourg - Centre National de la Recherche Scientifique

  Date: 03/12/2010
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


#include <iostream>
#include <algorithm>
#include <string>

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImage.h"
#include "itkExtractImageFilter.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "btkNiftiFilenameRadix.h"
#include "btkAffineRegistration.h"
#include "itkResampleImageFilter.h"
#include "itkImageMaskSpatialObject.h"
#include "itkImageDuplicator.h"
#include "itkImageRegionIterator.h"
#include "itkJoinSeriesImageFilter.h"
#include "btkDiffusionGradientTable.h"

#include <tclap/CmdLine.h>

int main( int argc, const char * argv[] )
{
  // Parse arguments
  try{

  TCLAP::CmdLine cmd("Writes a dwi sequence as a single image B0 + the diffusion "
      "images. The new B0 is the mean of all B0 images in the original sequence, "
      "or a user-provided B0.", ' ', "Unversioned");

  TCLAP::ValueArg<std::string> inputArg("i","input","Original DWI sequence.",
      true,"","string",cmd);
  TCLAP::ValueArg<std::string> outputArg("o","output","Normalized DWI Sequence. ",
      true,"","string",cmd);
  TCLAP::ValueArg<std::string> maskArg("m","mask","Image mask for registration. ",
      false,"","string",cmd);

  cmd.parse( argc, argv );

  std::string inputFileStr;
  inputFileStr = inputArg.getValue();
  const char* inputFile = inputFileStr.c_str();

  std::string bvalFileStr;
  bvalFileStr = btk::GetRadixOf(inputFileStr);
  bvalFileStr = bvalFileStr + ".bval";
  const char* bvalFile = bvalFileStr.c_str();

  std::string bvecFileStr;
  bvecFileStr = btk::GetRadixOf(inputFileStr);
  bvecFileStr = bvecFileStr + ".bvec";
  const char* bvecFile = bvecFileStr.c_str();

  std::string outputFileStr;
  outputFileStr = outputArg.getValue();
  const char* outputFile = outputFileStr.c_str();

  // Both the bval file and bvec file must be modified to remove some zero entries

  std::string bvalOutputFileStr;
  bvalOutputFileStr = btk::GetRadixOf(outputFileStr);
  bvalOutputFileStr = bvalOutputFileStr + ".bval";
  const char* bvalOutputFile = bvalOutputFileStr.c_str();

  std::string bvecOutputFileStr;
  bvecOutputFileStr = btk::GetRadixOf(outputFileStr);
  bvecOutputFileStr = bvecOutputFileStr + ".bvec";
  const char* bvecOutputFile = bvecOutputFileStr.c_str();

  std::string maskFileStr;
  maskFileStr = maskArg.getValue();
  const char* maskFile = maskFileStr.c_str();

  // typedefs

  typedef short         PixelType;
  const   unsigned int  Dimension = 4;

  typedef itk::Image< PixelType, Dimension >   SequenceType;
  typedef itk::ImageFileReader< SequenceType > ReaderType;

  // Read sequence

  ReaderType::Pointer reader = ReaderType::New();

  reader -> SetFileName( inputFile );
  reader -> Update();

  SequenceType::Pointer    sequence  = reader -> GetOutput();
  SequenceType::RegionType region    = sequence -> GetLargestPossibleRegion();
  SequenceType::SizeType 	 size      = region.GetSize();
  SequenceType::IndexType	 index     = region.GetIndex();

  unsigned int length = size[3];

  // Read b-values and identify B0 images

  std::vector< int > b0_idx;
  std::vector< int > b_values(length,0);

  FILE* f;

  f = fopen( bvalFile, "r" );
  int bvalue;
  int idx = 0;
  while ( !feof(f) )
  {
    fscanf( f, "%d ", &bvalue);
    b_values[idx] = bvalue;
    if (bvalue == 0)
      b0_idx.push_back(idx);
    idx++;
  }
  fclose (f);

  // Read mask

  typedef itk::Image< unsigned char, 3 >    				ImageMaskType;
  typedef ImageMaskType::Pointer            				ImageMaskPointer;

  typedef itk::ImageFileReader< ImageMaskType >     MaskReaderType;

  typedef itk::ImageMaskSpatialObject< 3 >  MaskType;
  typedef MaskType::Pointer  MaskPointer;

  MaskPointer mask = MaskType::New();

  if (strcmp(maskFile,"") != 0)
  {
    MaskReaderType::Pointer maskReader = MaskReaderType::New();
    maskReader -> SetFileName( maskFile );
    maskReader -> Update();

    mask -> SetImage( maskReader -> GetOutput() );
  }


  // Extraction of B0s
  typedef itk::Image< PixelType, 3 >  ImageType;
  std::vector< ImageType::Pointer > b0;

  size[3]  = 0;

  typedef itk::ExtractImageFilter<SequenceType,ImageType> ExtractorType;

  for(unsigned int i=0; i< b0_idx.size(); i++ )
  {
    index[3] = b0_idx[i];

    SequenceType::RegionType imageRegion;
    imageRegion.SetIndex( index );
    imageRegion.SetSize( size );

    ExtractorType::Pointer extractor = ExtractorType::New();
    extractor -> SetInput( sequence );
    extractor -> SetExtractionRegion( imageRegion );
    extractor -> SetDirectionCollapseToSubmatrix( );
    extractor -> Update();

    b0.push_back( extractor ->  GetOutput() );

    std::cout << "Extracted image " << b0_idx[i] << std::endl;
  }

  // Registration of B0s

  std::vector< ImageType::Pointer > b0_resampled;
  b0_resampled.push_back(b0[0]);

  typedef btk::AffineRegistration<ImageType> RegistrationType;
  typedef itk::ResampleImageFilter<ImageType,ImageType,double> ResamplerType;

  for(unsigned int i=1; i<b0_idx.size(); i++ )
  {
    std::cout << "Registering image " << b0_idx[i] << " to " << b0_idx[0]
              << " ... ";

    RegistrationType::Pointer registration = RegistrationType::New();
    registration -> SetFixedImage(  b0[0] );
    registration -> SetMovingImage( b0[i] );

    if (strcmp(maskFile,"") != 0)
    {
      registration -> SetFixedImageMask( mask );
      registration -> SetFixedImageRegion( mask -> GetAxisAlignedBoundingBoxRegion() );
    } else
      {
        registration -> SetFixedImageRegion( b0[0] -> GetLargestPossibleRegion() );
      }

    registration -> StartRegistration();

    ResamplerType::Pointer resampler = ResamplerType::New();
    resampler -> UseReferenceImageOn();
    resampler -> SetReferenceImage(b0[0]);
    resampler -> SetInput(b0[i]);
    resampler -> SetTransform( registration -> GetTransform() );

    resampler -> Update();
    b0_resampled.push_back( resampler -> GetOutput() );

    std::cout << "done." << std::endl;

  }

  // Calculates the mean image

  typedef itk::ImageDuplicator< ImageType > DuplicatorType;
  DuplicatorType::Pointer duplicator = DuplicatorType::New();
  duplicator -> SetInputImage (b0[0]);
  duplicator -> Update();
  ImageType::Pointer b0_mean = duplicator -> GetOutput();
  b0_mean -> FillBuffer(0);

  typedef itk::ImageRegionIterator<ImageType>  IteratorType;

  for(unsigned int i=0; i<b0_resampled.size(); i++ )
  {

    IteratorType b0_resampled_it( b0_resampled[i], b0_resampled[i]->GetLargestPossibleRegion() );
    IteratorType b0_mean_it( b0_mean, b0_mean->GetLargestPossibleRegion() );

    for(b0_resampled_it.GoToBegin(), b0_mean_it.GoToBegin();
        !b0_resampled_it.IsAtEnd();
        ++b0_resampled_it, ++b0_mean_it)
    {
      b0_mean_it.Set( b0_mean_it.Get() + b0_resampled_it.Get() );
    }

  }

  IteratorType b0_mean_it( b0_mean, b0_mean->GetLargestPossibleRegion() );

  for( b0_mean_it.GoToBegin(); !b0_mean_it.IsAtEnd(); ++b0_mean_it)
  {
    b0_mean_it.Set( b0_mean_it.Get() / b0_resampled.size() );
  }

  // Join images

  std::cout << "Joining images ... ";

  SequenceType::SpacingType  spacing  = sequence -> GetSpacing();

  typedef itk::JoinSeriesImageFilter< ImageType, SequenceType > JoinerType;
  JoinerType::Pointer joiner = JoinerType::New();
  joiner -> SetOrigin( 0.0 );
  joiner -> SetSpacing( spacing[3] );
  joiner -> SetInput( 0, b0_mean );

  unsigned j = 1;

  for (unsigned int i = 0; i < length; i++)
  {
    if ( b_values[i] != 0 )
    {
      index[3] = i;

      SequenceType::RegionType imageRegion;
      imageRegion.SetIndex( index );
      imageRegion.SetSize( size );

      ExtractorType::Pointer extractor = ExtractorType::New();
      extractor -> SetInput( sequence );
      extractor -> SetExtractionRegion( imageRegion );
      extractor -> SetDirectionCollapseToSubmatrix( );
      extractor -> Update();

      joiner -> SetInput( j, extractor -> GetOutput() );
      j++;
    }
  }

  std::cout << "done." << std::endl;

  joiner -> Update();

  // Write modified sequence

  std::cout << "Writing sequence ... ";

  typedef itk::ImageFileWriter< SequenceType >  WriterType;

  WriterType::Pointer writer =  WriterType::New();

  writer->SetFileName( outputFile );
  writer->SetInput( joiner -> GetOutput() );
  writer->Update();

  std::cout << "done." << std::endl;

  // Write new b-values with removed zero entries

  std::cout << "Writing b-values ... ";

  f = fopen( bvalOutputFile, "w" );
  fprintf( f, "%d ", 0);

  for(unsigned int i=0; i<length; i++)
  {
    if ( b_values[i] != 0)
      fprintf( f, "%d ", b_values[i]);
  }
  fclose (f);

  std::cout << "done." << std::endl;

  std::cout << "Writing gradient table ... ";

  typedef btk::DiffusionGradientTable< SequenceType > GradientTableType;
  GradientTableType::Pointer gradientTable = GradientTableType::New();

  gradientTable -> SetNumberOfGradients(length);
  gradientTable -> LoadFromFile( bvecFile);
  gradientTable -> RemoveRepeatedZeroEntries();
  gradientTable -> SaveToFile( bvecOutputFile );

  std::cout << "done." << std::endl;


  } catch (TCLAP::ArgException &e)  // catch any exceptions
  { std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; }

  return EXIT_SUCCESS;
}
