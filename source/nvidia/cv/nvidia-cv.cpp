// Copyright (c) 2020 Michael Fabian Dirks <info@xaymar.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// NVIDIA CVImage is part of:
// - NVIDIA Video Effects SDK
// - NVIDIA Augmented Reality SDK

#include "nvidia-cv.hpp"
#include <filesystem>
#include <mutex>
#include "nvidia/cuda/nvidia-cuda-obs.hpp"
#include "obs/gs/gs-helper.hpp"
#include "util/util-logging.hpp"
#include "util/util-platform.hpp"

#ifdef _DEBUG
#define ST_PREFIX "<%s> "
#define D_LOG_ERROR(x, ...) P_LOG_ERROR(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_WARNING(x, ...) P_LOG_WARN(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_INFO(x, ...) P_LOG_INFO(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_DEBUG(x, ...) P_LOG_DEBUG(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#else
#define ST_PREFIX "<nvidia::cv::cv> "
#define D_LOG_ERROR(...) P_LOG_ERROR(ST_PREFIX __VA_ARGS__)
#define D_LOG_WARNING(...) P_LOG_WARN(ST_PREFIX __VA_ARGS__)
#define D_LOG_INFO(...) P_LOG_INFO(ST_PREFIX __VA_ARGS__)
#define D_LOG_DEBUG(...) P_LOG_DEBUG(ST_PREFIX __VA_ARGS__)
#endif

#if defined(WIN32)
#include <KnownFolders.h>
#include <ShlObj.h>
#include <Windows.h>

#define LIB_NAME "NVCVImage.dll"
#else
#define LIB_NAME "libNVCVImage.so"
#endif

#define ST_ENV_NVIDIA_AR_SDK_PATH L"NV_AR_SDK_PATH"
#define ST_ENV_NVIDIA_VIDEO_EFFECTS_SDK_PATH L"NV_VIDEO_EFFECTS_PATH"

#define NVCVI_LOAD_SYMBOL(NAME)                                                          \
	{                                                                                    \
		NAME = reinterpret_cast<decltype(NAME)>(_library->load_symbol(#NAME));           \
		if (!NAME)                                                                       \
			throw std::runtime_error("Failed to load '" #NAME "' from '" LIB_NAME "'."); \
	}

streamfx::nvidia::cv::cv::~cv()
{
	D_LOG_DEBUG("Finalizing... (Addr: 0x%" PRIuPTR ")", this);
}

streamfx::nvidia::cv::cv::cv()
{
	std::filesystem::path              vfx_sdk_path;
	std::filesystem::path              ar_sdk_path;
	std::vector<std::filesystem::path> lib_paths;

	auto gctx = ::streamfx::obs::gs::context();
	auto cctx = ::streamfx::nvidia::cuda::obs::get()->get_context()->enter();

	D_LOG_DEBUG("Initializing... (Addr: 0x%" PRIuPTR ")", this);

	// Figure out the location of supported SDKs.
	{
#ifdef WIN32
		DWORD                env_size;
		std::vector<wchar_t> buffer;
		env_size = GetEnvironmentVariableW(ST_ENV_NVIDIA_VIDEO_EFFECTS_SDK_PATH, nullptr, 0);
		if (env_size > 0) {
			buffer.resize(static_cast<size_t>(env_size) + 1);
			env_size     = GetEnvironmentVariableW(ST_ENV_NVIDIA_VIDEO_EFFECTS_SDK_PATH, buffer.data(),
                                               static_cast<DWORD>(buffer.size()));
			vfx_sdk_path = std::wstring(buffer.data(), buffer.size());
		} else {
			PWSTR   str = nullptr;
			HRESULT res = SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, nullptr, &str);
			if (res == S_OK) {
				vfx_sdk_path = std::wstring(str);
				vfx_sdk_path /= "NVIDIA Corporation";
				vfx_sdk_path /= "NVIDIA Video Effects";
				CoTaskMemFree(str);
			}
		}
#else
		throw std::runtime_error("Not yet implemented.");
#endif

		// Check if any of the found paths are valid.
		if (std::filesystem::exists(vfx_sdk_path)) {
			lib_paths.push_back(vfx_sdk_path);
		}
	}
	{
#ifdef WIN32
		DWORD                env_size;
		std::vector<wchar_t> buffer;
		env_size = GetEnvironmentVariableW(ST_ENV_NVIDIA_AR_SDK_PATH, nullptr, 0);
		if (env_size > 0) {
			buffer.resize(static_cast<size_t>(env_size) + 1);
			env_size =
				GetEnvironmentVariableW(ST_ENV_NVIDIA_AR_SDK_PATH, buffer.data(), static_cast<DWORD>(buffer.size()));
			ar_sdk_path = std::wstring(buffer.data(), buffer.size());
		} else {
			PWSTR   str = nullptr;
			HRESULT res = SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, nullptr, &str);
			if (res == S_OK) {
				ar_sdk_path = std::wstring(str);
				ar_sdk_path /= "NVIDIA Corporation";
				ar_sdk_path /= "NVIDIA AR SDK";
				CoTaskMemFree(str);
			}
		}
#else
		throw std::runtime_error("Not yet implemented.");
#endif

		// Check if any of the found paths are valid.
		if (std::filesystem::exists(ar_sdk_path)) {
			lib_paths.push_back(ar_sdk_path);
		}
	}

	// Check if we have any found paths.
	if (lib_paths.size() == 0) {
		D_LOG_ERROR("No supported NVIDIA SDK is installed to provide '%s'.", LIB_NAME);
		throw std::runtime_error("Failed to load '" LIB_NAME "'.");
	}

	// Try and load any available NvCVImage library.
	for (auto path : lib_paths) {
#ifdef WIN32
		// On platforms where it is possible, modify the linker directories.
		SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
		DLL_DIRECTORY_COOKIE ck = AddDllDirectory(vfx_sdk_path.wstring().c_str());
#endif

		try {
			// Try to load it directly first, it may be on the search path already.
			_library = ::streamfx::util::library::load(std::string_view(LIB_NAME));
		} catch (...) {
			auto pathu8 = util::platform::native_to_utf8(path / LIB_NAME);
			try {
				_library = ::streamfx::util::library::load(pathu8);
			} catch (...) {
				D_LOG_WARNING("Failed to load '%s' from '%s'.", LIB_NAME, pathu8.string().c_str());
			}
		}
		if (_library)
			break;

#ifdef WIN32
		RemoveDllDirectory(ck);
#endif
	}

	if (!_library) {
		D_LOG_ERROR("No installed NVIDIA SDK provides '%s'.", LIB_NAME);
		throw std::runtime_error("Failed to load '" LIB_NAME "'.");
	}

	{ // Load Symbols
		NVCVI_LOAD_SYMBOL(NvCVImage_Init);
		NVCVI_LOAD_SYMBOL(NvCVImage_InitView);
		NVCVI_LOAD_SYMBOL(NvCVImage_Alloc);
		NVCVI_LOAD_SYMBOL(NvCVImage_Realloc);
		NVCVI_LOAD_SYMBOL(NvCVImage_Dealloc);
		NVCVI_LOAD_SYMBOL(NvCVImage_Create);
		NVCVI_LOAD_SYMBOL(NvCVImage_Destroy);
		NVCVI_LOAD_SYMBOL(NvCVImage_ComponentOffsets);
		NVCVI_LOAD_SYMBOL(NvCVImage_Transfer);
		NVCVI_LOAD_SYMBOL(NvCVImage_TransferRect);
		NVCVI_LOAD_SYMBOL(NvCVImage_TransferFromYUV);
		NVCVI_LOAD_SYMBOL(NvCVImage_TransferToYUV);
		NVCVI_LOAD_SYMBOL(NvCVImage_MapResource);
		NVCVI_LOAD_SYMBOL(NvCVImage_UnmapResource);
		NVCVI_LOAD_SYMBOL(NvCVImage_Composite);
		NVCVI_LOAD_SYMBOL(NvCVImage_CompositeRect);
		NVCVI_LOAD_SYMBOL(NvCVImage_CompositeOverConstant);
		NVCVI_LOAD_SYMBOL(NvCVImage_FlipY);
		NVCVI_LOAD_SYMBOL(NvCVImage_GetYUVPointers);
		NVCVI_LOAD_SYMBOL(NvCV_GetErrorStringFromCode);
#ifdef WIN32
		NVCVI_LOAD_SYMBOL(NvCVImage_InitFromD3D11Texture);
		NVCVI_LOAD_SYMBOL(NvCVImage_ToD3DFormat);
		NVCVI_LOAD_SYMBOL(NvCVImage_FromD3DFormat);
#ifdef __dxgicommon_h__
		NVCVI_LOAD_SYMBOL(NvCVImage_ToD3DColorSpace);
		NVCVI_LOAD_SYMBOL(NvCVImage_FromD3DColorSpace);
#endif
#endif
	}
}

std::shared_ptr<streamfx::nvidia::cv::cv> streamfx::nvidia::cv::cv::get()
{
	static std::weak_ptr<streamfx::nvidia::cv::cv> instance;
	static std::mutex                              lock;

	std::unique_lock<std::mutex> ul(lock);
	if (instance.expired()) {
		auto hard_instance = std::make_shared<streamfx::nvidia::cv::cv>();
		instance           = hard_instance;
		return hard_instance;
	}
	return instance.lock();
}
