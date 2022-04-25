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

#include <iostream>

#include "itkVkMultiResolutionPyramidImageFilter.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkMath.h"
#include "itkTestingMacros.h"

namespace
{
// The following three classes are used to support callbacks
// on the filter in the pipeline that follows later
class ShowProgressObject
{
public:
  ShowProgressObject(itk::ProcessObject * o) { m_Process = o; }
  void
  ShowProgress()
  {
    std::cout << "Progress " << m_Process->GetProgress() << std::endl;
  }
  itk::ProcessObject::Pointer m_Process;
};
} // namespace

int
itkVkMultiResolutionPyramidImageFilterTest(int argc, char * argv[])
{
  if (argc < 3)
  {
    std::cerr << "Missing Parameters." << std::endl;
    std::cerr << "Usage: " << itkNameOfTestExecutableMacro(argv);
    std::cerr << " inputImage outputImage <threshold0> [threshold1] [kernelThresholdDimension] [useShrinkFilter] "
                 "[numLevels] [expectedFFTLevelCount]"
              << std::endl;
    std::cerr << std::flush;
    return EXIT_FAILURE;
  }

  constexpr unsigned int ImageDimension = 2;
  using InputPixelType = float;
  using ImageType = itk::Image<InputPixelType, ImageDimension>;

  auto inputImage = itk::ReadImage<ImageType>(argv[1]);

  using PyramidType = itk::VkMultiResolutionPyramidImageFilter<ImageType, ImageType>;
  using ScheduleType = typename PyramidType::ScheduleType;
  using KernelSizeType = typename PyramidType::KernelSizeType;

  KernelSizeType kernelRadiusThreshold;
  if (argc == 4)
  {
    kernelRadiusThreshold.Fill(std::atoi(argv[3]));
  }
  else if (argc > 4)
  {
    kernelRadiusThreshold[0] = std::atoi(argv[3]);
    kernelRadiusThreshold[1] = std::atoi(argv[4]);
  }
  else
  {
    kernelRadiusThreshold.Fill(10);
  }

  auto         kernelThresholdDimension = (argc > 5 ? std::atoi(argv[5]) : 1);
  bool         useShrinkFilter = (argc > 6 && std::atoi(argv[6]) == 1);
  unsigned int numLevels = (argc > 7 ? std::atoi(argv[7]) : 3);
  int          expectedFFTCount = (argc > 8 ? std::atoi(argv[8]) : -1); // only test if specified

  // Set up multi-resolution pyramid
  auto pyramidFilter = PyramidType::New();
  pyramidFilter->SetInput(inputImage);

  pyramidFilter->SetUseShrinkImageFilter(useShrinkFilter);
  ITK_TEST_SET_GET_VALUE(pyramidFilter->GetUseShrinkImageFilter(), useShrinkFilter);

  // Verify isotropic radius
  unsigned int   isotropicRadiusSize = 10;
  KernelSizeType radius;
  radius.Fill(isotropicRadiusSize);
  pyramidFilter->SetKernelRadiusThreshold(isotropicRadiusSize);
  ITK_TEST_SET_GET_VALUE(pyramidFilter->GetKernelRadiusThreshold(), radius);

  // Verify anisotropic radius
  pyramidFilter->SetKernelRadiusThreshold(kernelRadiusThreshold);
  ITK_TEST_SET_GET_VALUE(pyramidFilter->GetKernelRadiusThreshold(), kernelRadiusThreshold);

  // Verify threshold condition for switching to FFT smoothing
  pyramidFilter->SetKernelThresholdDimension(kernelThresholdDimension);
  ITK_TEST_SET_GET_VALUE(pyramidFilter->GetKernelThresholdDimension(), kernelThresholdDimension);

  // Use default schedule for testing
  pyramidFilter->SetNumberOfLevels(numLevels);

  // Verify kernel variance and radius match expectations for default schedule
  KernelSizeType prevRadius;
  unsigned int   fftCount = 0;
  for (unsigned int level = 0; level < numLevels; ++level)
  {
    auto schedule = pyramidFilter->GetSchedule();
    auto variance = pyramidFilter->GetVariance(level);
    radius = pyramidFilter->GetKernelRadius(level);
    auto useFFT = pyramidFilter->GetUseFFT(radius);

    std::cout << "FFT will " << (useFFT ? "" : "not ") << "be used for level " << level << " with radius " << radius
              << std::endl;
    if (useFFT)
      ++fftCount;

    for (unsigned int dim = 0; dim < ImageDimension; ++dim)
    {
      // Verify variance output
      ITK_TEST_EXPECT_TRUE(itk::Math::AlmostEquals(variance[dim], itk::Math::sqr(0.5 * schedule[level][dim])));

      // Verify kernel radius output
      // Full calculations for default Gaussian size are outside the scope of this test
      // so just test that radius decreases with level
      if (level > 0)
      {
        ITK_TEST_EXPECT_TRUE(radius[dim] == 1 || prevRadius[dim] == 1 || radius[dim] < prevRadius[dim]);
      }
      else
      {
        prevRadius = radius;
      }
    }
  }

  if (expectedFFTCount != -1)
  {
    // Test number of levels for FFT smoothing matches expectations
    ITK_TEST_EXPECT_EQUAL(fftCount, expectedFFTCount);
  }

  ITK_EXERCISE_BASIC_OBJECT_METHODS(
    pyramidFilter, VkMultiResolutionPyramidImageFilter, MultiResolutionPyramidImageFilter);

  // Run the filter and track progress
  ShowProgressObject                                    progressWatch(pyramidFilter);
  itk::SimpleMemberCommand<ShowProgressObject>::Pointer command;
  command = itk::SimpleMemberCommand<ShowProgressObject>::New();
  command->SetCallbackFunction(&progressWatch, &ShowProgressObject::ShowProgress);
  pyramidFilter->AddObserver(itk::ProgressEvent(), command);
  pyramidFilter->Update();

  for (unsigned int ilevel = 0; ilevel < numLevels; ++ilevel)
  {
    itk::WriteImage(pyramidFilter->GetOutput(ilevel), argv[2] + std::to_string(ilevel) + ".mhd");
  }

  return EXIT_SUCCESS;
}
