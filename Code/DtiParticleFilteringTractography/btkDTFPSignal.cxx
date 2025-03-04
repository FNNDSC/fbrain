/*
Copyright or © or Copr. Université de Strasbourg - Centre National de la Recherche Scientifique

6 september 2010
< pontabry at unistra dot fr >

This software is governed by the CeCILL-B license under French law and
abiding by the rules of distribution of free software. You can use,
modify and/ or redistribute the software under the terms of the CeCILL-B
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty and the software's author, the holder of the
economic rights, and the successive licensors have only limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading, using, modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean that it is complicated to manipulate, and that also
therefore means that it is reserved for developers and experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and, more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL-B license and that you accept its terms.
*/


#include "btkDTFPSignal.h"


// ITK includes
#include "itkDiffusionTensor3DReconstructionImageFilter.h"
#include "itkVector.h"


namespace btk
{

DTFPSignal::DTFPSignal(Sequence::Pointer signal, std::vector<Real> *sigmas, std::vector<Vector> *directions, unsigned int bval, Image::Pointer baseline, char displayMode)
{
    m_signal = 0;
    m_interp = 0;
    m_sigmas = sigmas;
    m_bval   = bval;

    m_displayMode = displayMode;

    m_directions = directions;


    Display1(m_displayMode, std::cout << "Loading signal..." << std::endl);

    m_baseline = baseline;
    m_baselineInterp = ImageInterpolator::New();
    m_baselineInterp->SetInputImage(m_baseline);

    m_N = 0;

    // Getting images sizes
    SequenceRegion ssignalRegion = signal->GetLargestPossibleRegion();

    unsigned int xMax = ssignalRegion.GetSize(0);
    unsigned int yMax = ssignalRegion.GetSize(1);
    unsigned int zMax = ssignalRegion.GetSize(2);
    unsigned int kMax = ssignalRegion.GetSize(3);

    m_N = kMax;

    if(m_N != m_sigmas->size())
    {
        std::cout << "Error: there are " << m_N << " images and " << m_sigmas->size() << " sigmas !" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    Display2(m_displayMode, std::cout << "\tThere are " << kMax << " images of size ");
    Display2(m_displayMode, std::cout << xMax << "x" << yMax << "x" << zMax << "." << std::endl);


    // Allocate space memory for the array of images
    Display2(m_displayMode, std::cout << "\tAllocating space memory..." << std::flush);
    m_signal = new Image::Pointer[kMax];
    m_interp = new ImageInterpolator::Pointer[kMax];
    Display2(m_displayMode, std::cout << "done." << std::endl);


    Display2(m_displayMode, std::cout << "\tPreparing and interpolating data..." << std::flush);

    // Define images region
    ImageRegion iRegion;

    Image::IndexType iIndex;
    iIndex[0] = 0; iIndex[1] = 0; iIndex[2] = 0;
    iRegion.SetIndex(iIndex);

    Image::SizeType iSize;
    iSize[0] = xMax; iSize[1] = yMax; iSize[2] = zMax;
    iRegion.SetSize(iSize);


    // Define sequence region
    SequenceRegion sRegion;

    Sequence::SizeType sSize;
    sSize[0] = xMax; sSize[1] = yMax;
    sSize[2] = zMax; sSize[3] = 1;
    sRegion.SetSize(sSize);


    // Spacing and origin
    Sequence::SpacingType spacing = signal->GetSpacing();
    Sequence::PointType origin    = signal->GetOrigin();

    Image::SpacingType ispacing;
    ispacing[0] = spacing[0]; ispacing[1] = spacing[1]; ispacing[2] = spacing[2];
    Image::PointType iorigin;
    iorigin[0] = origin[0]; iorigin[1] = origin[1]; iorigin[2] = origin[2];

    // Orientation
    itk::Matrix<Real,3,3> dirMat;
    itk::Matrix<Real,4,4> iniMat = signal->GetDirection();

    dirMat(0,0) = iniMat(0,0);
    dirMat(0,1) = iniMat(0,1);
    dirMat(0,2) = iniMat(0,2);

    dirMat(1,0) = iniMat(1,0);
    dirMat(1,1) = iniMat(1,1);
    dirMat(1,2) = iniMat(1,2);

    dirMat(2,0) = iniMat(2,0);
    dirMat(2,1) = iniMat(2,1);
    dirMat(2,2) = iniMat(2,2);


    // Put sequence in array of images
    for(unsigned int k=0; k<kMax; k++)
    {
        // Allocate space memory
        m_signal[k] = Image::New();
        m_signal[k]->SetRegions(iSize);
        m_signal[k]->SetOrigin(iorigin);
        m_signal[k]->SetSpacing(ispacing);
        m_signal[k]->SetDirection(dirMat);
        m_signal[k]->Allocate();

        // Define correct sequence region
        Sequence::IndexType sIndex;
        sIndex[0] = 0; sIndex[1] = 0;
        sIndex[2] = 0; sIndex[3] = k;
        sRegion.SetIndex(sIndex);

        // Define iterators
        SequenceIterator sIt(signal, sRegion);
        ImageIterator iIt(m_signal[k], iRegion);

        // Copy data
        for(sIt.GoToBegin(), iIt.GoToBegin(); !sIt.IsAtEnd() && !iIt.IsAtEnd(); ++sIt, ++iIt)
            iIt.Set(sIt.Get());

        // Define interpolator
        m_interp[k] = ImageInterpolator::New();
        m_interp[k]->SetInputImage(m_signal[k]);
    } // for k

    Display2(m_displayMode, std::cout << "done." << std::endl);


    Display1(m_displayMode, std::cout << "done." << std::endl);
}

DTFPSignal::~DTFPSignal()
{
    delete[] m_interp;
    delete[] m_signal;
}

Matrix DTFPSignal::signalAt(Point p)
{
    Matrix S(m_N, 1);

    ImageInterpolator::ContinuousIndexType index;
    index[0] = p.x(); index[1] = p.y(); index[2] = p.z();

    for(unsigned int i=0; i<m_N; i++)
        S(i,0) = m_interp[i]->EvaluateAtContinuousIndex(index);

    return S;
}

std::vector<Real> *DTFPSignal::getSigmas()
{
    return m_sigmas;
}

Image::SizeType DTFPSignal::getSize()
{
    return m_signal[0]->GetLargestPossibleRegion().GetSize();
}

Image::PointType DTFPSignal::getOrigin()
{
    return m_signal[0]->GetOrigin();
}

Image::SpacingType DTFPSignal::getSpacing()
{
    return m_signal[0]->GetSpacing();
}

std::vector<Vector> *DTFPSignal::getDirections()
{
    return m_directions;
}

itk::Matrix<Real,3,3> DTFPSignal::GetDirection()
{
    return m_signal[0]->GetDirection();
}

itk::DiffusionTensor3D<Real> DTFPSignal::DiffusionTensorAt(Point p)
{
    Matrix dwi = this->signalAt(p);

    Image::Pointer *dwiImage = new Image::Pointer[dwi.Rows()];

    itk::Vector<Real,3> vector;

    Image::Pointer refImage = Image::New();
    refImage->SetOrigin(m_baseline->GetOrigin());
    refImage->SetSpacing(m_baseline->GetSpacing());
    refImage->SetDirection(m_baseline->GetDirection());
    Image::SizeType size;
    size[0] = 1; size[1] = 1; size[2] = 1;
    refImage->SetRegions(size);
    refImage->Allocate();

    Image::IndexType index;
    index[0] = 0; index[1] = 0; index[2] = 0;

    refImage->SetPixel(index, this->BaselineAt(p));


    itk::DiffusionTensor3DReconstructionImageFilter<Float,Float,Float>::Pointer filter = itk::DiffusionTensor3DReconstructionImageFilter<Float,Float,Float>::New();
    filter->SetReferenceImage(refImage);
    filter->SetBValue(m_bval);

    unsigned int i=0;
    for(std::vector<Vector>::iterator v = m_directions->begin(); v != m_directions->end(); v++, i++)
    {
        dwiImage[i] = Image::New();
        dwiImage[i]->SetOrigin(m_signal[0]->GetOrigin());
        dwiImage[i]->SetSpacing(m_signal[0]->GetSpacing());
        dwiImage[i]->SetDirection(m_signal[0]->GetDirection());
        dwiImage[i]->SetRegions(size);
        dwiImage[i]->Allocate();

        vector[0] = v->x(); vector[1] = v->y(); vector[2] = v->z();
        dwiImage[i]->SetPixel(index, dwi(i,0));

        filter->AddGradientImage(vector.GetVnlVector(), dwiImage[i]);
    }

    filter->Update();
    delete[] dwiImage;

    return filter->GetOutput()->GetPixel(index);
}

Real DTFPSignal::BaselineAt(Point p)
{
    ImageInterpolator::ContinuousIndexType cindex;
    cindex[0] = p.x(); cindex[1] = p.y(); cindex[2] = p.z();
    return m_baselineInterp->EvaluateAtContinuousIndex(cindex);
}

unsigned int DTFPSignal::GetBValue()
{
    return m_bval;
}

Image::PointType DTFPSignal::getImageOrigin()
{
    return m_signal[0]->GetOrigin();
}

Image::SpacingType DTFPSignal::getImageSpacing()
{
    return m_signal[0]->GetSpacing();
}

} // namespace btk

