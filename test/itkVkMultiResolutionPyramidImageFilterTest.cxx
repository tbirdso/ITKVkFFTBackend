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
    std::cerr << " inputImage outputImage [kernelCondition] [useShrinkFilter] [numLevels]" << std::endl;
    std::cerr << std::flush;
    return EXIT_FAILURE;
  }

  constexpr unsigned int ImageDimension = 2;
  using InputPixelType = float;
  using ImageType = itk::Image<InputPixelType, ImageDimension>;

  auto inputImage = itk::ReadImage<ImageType>(argv[1]);

  auto kernelCondition =
    (argc > 3 ? std::atoi(argv[3]) : ImageDimension);
  bool useShrinkFilter = (argc > 4 && std::atoi(argv[4]) == 1);
  unsigned int numLevels = (argc > 5 ? std::atoi(argv[5]) : 3);

  // Set up multi-resolution pyramid
  using PyramidType = itk::VkMultiResolutionPyramidImageFilter<ImageType, ImageType>;
  using ScheduleType = PyramidType::ScheduleType;

  auto pyramidFilter = PyramidType::New();
  pyramidFilter->SetInput(inputImage);
  pyramidFilter->SetUseShrinkImageFilter(useShrinkFilter);

  // Use default schedule
  pyramidFilter->SetNumberOfLevels(numLevels);

  // TODO set schedule by specifying the starting shrink factors
  /*numLevels = 4;
  factors[0] = 8;
  factors[1] = 4;
  factors[2] = 2;
  pyramid->SetNumberOfLevels(numLevels);
  pyramid->SetStartingShrinkFactors(factors.Begin());*/

  // TODO test get variance
  using RadiusSizeType = typename PyramidType::OutputSizeType;
  RadiusSizeType radius, prevRadius;
  for (unsigned int level = 0; level < numLevels; ++level)
  {
    auto schedule = pyramidFilter->GetSchedule();
    auto variance = pyramidFilter->GetVariance(level);
    radius = pyramidFilter->GetKernelRadius(level);

    for (unsigned int dim = 0; dim < ImageDimension; ++dim)
    {
      ITK_TEST_EXPECT_TRUE(itk::Math::AlmostEquals(variance[dim], itk::Math::sqr(0.5 * schedule[level][dim])));
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

  // TODO test get kernel radius

  // TODO test using recursive filter for additional speedup
  ITK_EXERCISE_BASIC_OBJECT_METHODS(
    pyramidFilter, VkMultiResolutionPyramidImageFilter, MultiResolutionPyramidImageFilter);

  ShowProgressObject                                    progressWatch(pyramidFilter);
  itk::SimpleMemberCommand<ShowProgressObject>::Pointer command;
  command = itk::SimpleMemberCommand<ShowProgressObject>::New();
  command->SetCallbackFunction(&progressWatch, &ShowProgressObject::ShowProgress);
  pyramidFilter->AddObserver(itk::ProgressEvent(), command);

  pyramidFilter->Update();

  //  update pyramid at a particular level
  /*for (unsigned int testLevel = 0; testLevel < numLevels; ++testLevel)
  {
    pyramid->GetOutput(testLevel)->Update();
  }*/
  for (unsigned int ilevel = 0; ilevel < numLevels; ++ilevel)
  {
    itk::WriteImage(pyramidFilter->GetOutput(ilevel), argv[2] + std::to_string(ilevel) + ".mhd");
  }

  return EXIT_SUCCESS;
}
