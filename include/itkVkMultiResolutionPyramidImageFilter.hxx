/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef itkVkMultiResolutionPyramidImageFilter_hxx
#define itkVkMultiResolutionPyramidImageFilter_hxx

#include "itkCastImageFilter.h"
#include "itkDiscreteGaussianImageFilter.h"
#include "itkGaussianOperator.h"
#include "itkMacro.h"
#include "itkResampleImageFilter.h"
#include "itkShrinkImageFilter.h"
#include "itkIdentityTransform.h"

#include "itkMath.h"

namespace itk
{
template <typename TInputImage, typename TOutputImage>
VkMultiResolutionPyramidImageFilter<TInputImage, TOutputImage>::VkMultiResolutionPyramidImageFilter()
{
  m_KernelRadiusThreshold.Fill(10);
}

template <typename TInputImage, typename TOutputImage>
void
VkMultiResolutionPyramidImageFilter<TInputImage, TOutputImage>::GenerateData()
{
  // Get the input and output pointers
  InputImageConstPointer inputPtr = this->GetInput();

  // Create caster, smoother and resampleShrinker filters
  using CasterType = CastImageFilter<TInputImage, TOutputImage>;

  using ImageToImageType = ImageToImageFilter<TOutputImage, TOutputImage>;
  using ResampleShrinkerType = ResampleImageFilter<TOutputImage, TOutputImage>;
  using ShrinkerType = ShrinkImageFilter<TOutputImage, TOutputImage>;

  auto                      caster = CasterType::New();
  auto                      spatialSmoother = SpatialSmootherType::New();
  auto                      fftSmoother = FFTSmootherType::New();
  BaseSmootherType::Pointer smoother;

  typename ImageToImageType::Pointer shrinkerFilter;
  //
  // only one of these pointers is going to be valid, depending on the
  // value of UseShrinkImageFilter flag
  typename ResampleShrinkerType::Pointer resampleShrinker;
  typename ShrinkerType::Pointer         shrinker;

  if (this->GetUseShrinkImageFilter())
  {
    shrinker = ShrinkerType::New();
    shrinkerFilter = shrinker.GetPointer();
  }
  else
  {
    resampleShrinker = ResampleShrinkerType::New();
    using LinearInterpolatorType = itk::LinearInterpolateImageFunction<OutputImageType, double>;
    auto interpolator = LinearInterpolatorType::New();
    resampleShrinker->SetInterpolator(interpolator);
    resampleShrinker->SetDefaultPixelValue(0);
    shrinkerFilter = resampleShrinker.GetPointer();
  }
  // Setup the filters
  caster->SetInput(inputPtr);

  spatialSmoother->SetUseImageSpacing(false);
  spatialSmoother->SetInput(caster->GetOutput());
  spatialSmoother->SetMaximumError(m_MaximumError);
  fftSmoother->SetUseImageSpacing(false);
  fftSmoother->SetInput(caster->GetOutput());
  fftSmoother->SetMaximumError(m_MaximumError);

  // shrinkerFilter->SetInput(smoother->GetOutput());

  unsigned int   ilevel, idim;
  unsigned int   factors[ImageDimension];
  VarianceType   variance;
  KernelSizeType radius;
  bool           useFFTSmoothing = false;

  for (ilevel = 0; ilevel < m_NumberOfLevels; ++ilevel)
  {
    this->UpdateProgress(static_cast<float>(ilevel) / static_cast<float>(m_NumberOfLevels));

    // Allocate memory for each output
    OutputImagePointer outputPtr = this->GetOutput(ilevel);
    outputPtr->SetBufferedRegion(outputPtr->GetRequestedRegion());
    outputPtr->Allocate();

    // compute shrink factors
    for (idim = 0; idim < ImageDimension; ++idim)
    {
      factors[idim] = m_Schedule[ilevel][idim];
    }

    // compute kernel radius and select smoother type
    useFFTSmoothing = false;
    unsigned int thresholdExceededCount = 0;
    radius = this->GetKernelRadius(ilevel);
    for (idim = 0; idim < ImageDimension; ++idim)
    {
      if (radius[idim] >= m_KernelRadiusThreshold[idim])
      {
        ++thresholdExceededCount;
      }
    }
    useFFTSmoothing = (thresholdExceededCount >= m_KernelThresholdDimension);

    // Compute

    if (!this->GetUseShrinkImageFilter())
    {
      using IdentityTransformType = itk::IdentityTransform<double, OutputImageType::ImageDimension>;
      auto identityTransform = IdentityTransformType::New();
      resampleShrinker->SetOutputParametersFromImage(outputPtr);
      resampleShrinker->SetTransform(identityTransform);
    }
    else
    {
      shrinker->SetShrinkFactors(factors);
    }

    // use mini-pipeline to compute output
    if (GetUseFFT(this->GetKernelRadius(ilevel)))
    {
      smoother = fftSmoother;
    }
    else
    {
      smoother = spatialSmoother;
    }
    variance = this->GetVariance(ilevel);
    smoother->SetVariance(variance);
    shrinkerFilter->SetInput(smoother->GetOutput());

    shrinkerFilter->GraftOutput(outputPtr);

    // force to always update in case shrink factors are the same
    shrinkerFilter->Modified();
    shrinkerFilter->UpdateLargestPossibleRegion();
    this->GraftNthOutput(ilevel, shrinkerFilter->GetOutput());
  }
}

template <typename TInputImage, typename TOutputImage>
bool
VkMultiResolutionPyramidImageFilter<TInputImage, TOutputImage>::GetUseFFT(const KernelSizeType & kernelRadius) const
{
  // compute kernel radius and select smoother type
  unsigned int thresholdExceededCount = 0;
  for (unsigned int idim = 0; idim < ImageDimension; ++idim)
  {
    if (kernelRadius[idim] >= m_KernelRadiusThreshold[idim])
    {
      ++thresholdExceededCount;
    }
  }
  return thresholdExceededCount >= m_KernelThresholdDimension;
}

template <typename TInputImage, typename TOutputImage>
typename VkMultiResolutionPyramidImageFilter<TInputImage, TOutputImage>::KernelSizeType
VkMultiResolutionPyramidImageFilter<TInputImage, TOutputImage>::GetKernelRadius(unsigned int ilevel) const
{
  using OperatorType = itk::GaussianOperator<OutputPixelType, ImageDimension>;
  auto *         oper = new OperatorType;
  KernelSizeType radius;
  for (unsigned int dim = 0; dim < ImageDimension; ++dim)
  {
    oper->SetDirection(dim);
    oper->SetMaximumError(m_MaximumError);
    // oper->SetMaximumKernelWidth(32);
    oper->SetVariance(this->GetVariance(ilevel)[dim]);
    oper->CreateDirectional();
    radius[dim] = oper->GetRadius()[dim];
  }
  return radius;
}

template <typename TInputImage, typename TOutputImage>
typename VkMultiResolutionPyramidImageFilter<TInputImage, TOutputImage>::VarianceType
VkMultiResolutionPyramidImageFilter<TInputImage, TOutputImage>::GetVariance(unsigned int ilevel) const
{
  VarianceType variance;
  for (unsigned int dim = 0; dim < ImageDimension; ++dim)
  {
    variance[dim] = itk::Math::sqr(0.5 * static_cast<float>(m_Schedule[ilevel][dim]));
  }
  return variance;
}

/**
 * PrintSelf method
 */
template <typename TInputImage, typename TOutputImage>
void
VkMultiResolutionPyramidImageFilter<TInputImage, TOutputImage>::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Kernel radius threshold: " << m_KernelRadiusThreshold << std::endl;
  os << indent << "Kernel threshold dimension: " << m_KernelThresholdDimension << std::endl;
}
} // namespace itk

#endif // itkVkMultiResolutionPyramidImageFilter_hxx
