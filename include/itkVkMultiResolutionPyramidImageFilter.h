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
#ifndef itkVkMultiResolutionPyramidImageFilter_h
#define itkVkMultiResolutionPyramidImageFilter_h

#include "itkMultiResolutionPyramidImageFilter.h"

#include "itkDiscreteGaussianImageFilter.h"
#include "itkFFTDiscreteGaussianImageFilter.h"
#include "itkVector.h"
#include "itkMacro.h"
#include "VkFFTBackendExport.h"

#include <string>

namespace itk
{

/** \class VkMultiResolutionPyramidImageFilter
 * \brief TODO
 *
 * TODO
 *
 * \sa MultiResolutionPyramidImageFilter
 * \sa DiscreteGaussianImageFilter
 * \sa ShrinkImageFilter
 *
 * \ingroup PyramidImageFilter MultiThreaded Streamed
 * \ingroup ITKRegistrationCommon
 */
template <typename TInputImage, typename TOutputImage>
class ITK_TEMPLATE_EXPORT VkMultiResolutionPyramidImageFilter
  : public MultiResolutionPyramidImageFilter<TInputImage, TOutputImage>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(VkMultiResolutionPyramidImageFilter);

  /** Standard class type aliases. */
  using Self = VkMultiResolutionPyramidImageFilter;
  using Superclass = MultiResolutionPyramidImageFilter<TInputImage, TOutputImage>;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(VkMultiResolutionPyramidImageFilter, MultiResolutionPyramidImageFilter);

  /** ImageDimension enumeration. */
  static constexpr unsigned int ImageDimension = TInputImage::ImageDimension;

  /** Inherit types from Superclass. */
  using typename Superclass::InputImageType;
  using typename Superclass::OutputImageType;
  using typename Superclass::InputImagePointer;
  using typename Superclass::OutputImagePointer;
  using typename Superclass::InputImageConstPointer;
  using OutputPixelType = typename OutputImageType::PixelType;
  using OutputSizeType = typename OutputImageType::SizeType;
  using typename Superclass::ScheduleType;
  using VarianceType = itk::Vector<double, ImageDimension>;

  /** Types for acceleration.
   *  Assumes and does not verify that FFT backend is accelerated. */
  using BaseSmootherType = DiscreteGaussianImageFilter<OutputImageType, OutputImageType>;
  using SpatialSmootherType = DiscreteGaussianImageFilter<OutputImageType, OutputImageType>;
  using FFTSmootherType = FFTDiscreteGaussianImageFilter<OutputImageType, OutputImageType>;

  /** Set the kernel radius size threshold to decide between
   *  accelerated methods such as CPU-based separable smoothing
   *  versus GPU-based FFT smoothing. Should be adjusted to match
   *  benchmarks for individual hardware setup. Defaults to 10.
   */
  itkSetMacro(KernelRadiusThreshold, OutputSizeType);
  itkGetConstMacro(KernelRadiusThreshold, OutputSizeType);

  /** Set the number of directions for which the kernel radius
   *  threshold must be surpassed to prompt a switch between
   *  acceleration methods.
   *  Default value of 1 indicates that if the kernel radius in
   *  any axis direction is greater than the threshold then
   *  FFT smoothing will be used.
   *  Clipped to [0, ImageDimension].
   */
  void
  SetKernelThresholdDimension(unsigned int dimension)
  {
    if (dimension < 1 || dimension > Self::ImageDimension)
    {
      itkWarningMacro("Kernel threshold dimension must be between 1 and " + std::to_string(ImageDimension));
      if (dimension < 1)
        dimension = 1;
      if (dimension > ImageDimension)
        dimension = ImageDimension;
    }
    m_KernelThresholdDimension = dimension;
  }
  itkGetMacro(KernelThresholdDimension, unsigned int);

  /** Estimate the kernel radius from ilevel settings */
  OutputSizeType
  GetKernelRadius(unsigned int ilevel) const;

  /** Get the kernel variance for the given pyramid level
   *  based on the current schedule */
  VarianceType
  GetVariance(unsigned int ilevel) const;

protected:
  VkMultiResolutionPyramidImageFilter();
  ~VkMultiResolutionPyramidImageFilter() override = default;

  /** Generate the output data. */
  void
  GenerateData() override;

  void
  PrintSelf(std::ostream & os, Indent indent) const override;

protected:
  OutputSizeType m_KernelRadiusThreshold;
  unsigned int   m_KernelThresholdDimension = 1;
};
} // namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkVkMultiResolutionPyramidImageFilter.hxx"
#endif

#endif
