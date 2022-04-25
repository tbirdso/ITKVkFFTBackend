# the top-level README is used for describing this module, just
# re-used it for documentation here
get_filename_component(MY_CURRENT_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
file(READ "${MY_CURRENT_DIR}/README.rst" DOCUMENTATION)

# itk_module() defines the module dependencies in VkFFTBackend
# VkFFTBackend depends on ITKCommon
# By convention those modules outside of ITK are not prefixed with
# ITK.

# define the dependencies of the include module and the tests
itk_module(VkFFTBackend
  DEPENDS
    ITKCommon
    ITKStatistics
    ITKFFT
    ITKRegistrationCommon
    ITKConvolution
  COMPILE_DEPENDS
    ITKImageSources
    ITKSmoothing
  TEST_DEPENDS
    ITKTestKernel
    ITKIOImageBase
    ITKImageCompose
    ITKImageIntensity
  DESCRIPTION
    "${DOCUMENTATION}"
  FACTORY_NAMES
    "FFTImageFilterInit::Vk"
  EXCLUDE_FROM_DEFAULT
  ENABLE_SHARED
)
