// FFMPEG Video Encoder Integration for OBS Studio
// Copyright (c) 2019 Michael Fabian Dirks <info@xaymar.com>
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

#include "nvenc_shared.hpp"
#include "encoders/encoder-ffmpeg.hpp"
#include "ffmpeg/tools.hpp"

extern "C" {
#pragma warning(push)
#pragma warning(disable : 4244)
#include <libavutil/opt.h>
#pragma warning(pop)
}

#define ST_I18N_PRESET "FFmpegEncoder.NVENC.Preset"
#define ST_I18N_PRESET_(x) ST_I18N_PRESET "." D_VSTR(x)
#define ST_I18N_RATECONTROL "FFmpegEncoder.NVENC.RateControl"
#define ST_I18N_RATECONTROL_MODE ST_I18N_RATECONTROL ".Mode"
#define ST_I18N_RATECONTROL_MODE_(x) ST_I18N_RATECONTROL_MODE "." D_VSTR(x)
#define ST_I18N_RATECONTROL_TWOPASS ST_I18N_RATECONTROL ".TwoPass"
#define ST_I18N_RATECONTROL_LOOKAHEAD ST_I18N_RATECONTROL ".LookAhead"
#define ST_I18N_RATECONTROL_ADAPTIVEI ST_I18N_RATECONTROL ".AdaptiveI"
#define ST_I18N_RATECONTROL_ADAPTIVEB ST_I18N_RATECONTROL ".AdaptiveB"
#define ST_I18N_RATECONTROL_LIMITS ST_I18N_RATECONTROL ".Limits"
#define ST_I18N_RATECONTROL_LIMITS_BUFFERSIZE ST_I18N_RATECONTROL_LIMITS ".BufferSize"
#define ST_I18N_RATECONTROL_LIMITS_QUALITY ST_I18N_RATECONTROL_LIMITS ".Quality"
#define ST_I18N_RATECONTROL_LIMITS_BITRATE ST_I18N_RATECONTROL_LIMITS ".Bitrate"
#define ST_I18N_RATECONTROL_LIMITS_BITRATE_TARGET ST_I18N_RATECONTROL_LIMITS_BITRATE ".Target"
#define ST_I18N_RATECONTROL_LIMITS_BITRATE_MAXIMUM ST_I18N_RATECONTROL_LIMITS_BITRATE ".Maximum"
#define ST_I18N_RATECONTROL_QP ST_I18N_RATECONTROL ".QP"
#define ST_I18N_RATECONTROL_QP_MINIMUM ST_I18N_RATECONTROL_QP ".Minimum"
#define ST_I18N_RATECONTROL_QP_MAXIMUM ST_I18N_RATECONTROL_QP ".Maximum"
#define ST_I18N_RATECONTROL_QP_I ST_I18N_RATECONTROL_QP ".I"
#define ST_I18N_RATECONTROL_QP_P ST_I18N_RATECONTROL_QP ".P"
#define ST_I18N_RATECONTROL_QP_B ST_I18N_RATECONTROL_QP ".B"
#define ST_I18N_AQ "FFmpegEncoder.NVENC.AQ"
#define ST_I18N_AQ_SPATIAL ST_I18N_AQ ".Spatial"
#define ST_I18N_AQ_TEMPORAL ST_I18N_AQ ".Temporal"
#define ST_I18N_AQ_STRENGTH ST_I18N_AQ ".Strength"
#define ST_I18N_OTHER "FFmpegEncoder.NVENC.Other"
#define ST_I18N_OTHER_BFRAMES ST_I18N_OTHER ".BFrames"
#define ST_I18N_OTHER_BFRAMEREFERENCEMODE ST_I18N_OTHER ".BFrameReferenceMode"
#define ST_I18N_OTHER_ZEROLATENCY ST_I18N_OTHER ".ZeroLatency"
#define ST_I18N_OTHER_WEIGHTEDPREDICTION ST_I18N_OTHER ".WeightedPrediction"
#define ST_I18N_OTHER_NONREFERENCEPFRAMES ST_I18N_OTHER ".NonReferencePFrames"
#define ST_I18N_OTHER_ACCESSUNITDELIMITER ST_I18N_OTHER ".AccessUnitDelimiter"
#define ST_I18N_OTHER_DECODEDPICTUREBUFFERSIZE ST_I18N_OTHER ".DecodedPictureBufferSize"

#define ST_KEY_PRESET "Preset"
#define ST_KEY_RATECONTROL_MODE "RateControl.Mode"
#define ST_KEY_RATECONTROL_TWOPASS "RateControl.TwoPass"
#define ST_KEY_RATECONTROL_LOOKAHEAD "RateControl.LookAhead"
#define ST_KEY_RATECONTROL_ADAPTIVEI "RateControl.AdaptiveI"
#define ST_KEY_RATECONTROL_ADAPTIVEB "RateControl.AdaptiveB"
#define ST_KEY_RATECONTROL_LIMITS_BUFFERSIZE "RateControl.Limits.BufferSize"
#define ST_KEY_RATECONTROL_LIMITS_QUALITY "RateControl.Limits.Quality"
#define ST_KEY_RATECONTROL_LIMITS_BITRATE_TARGET "RateControl.Limits.Bitrate.Target"
#define ST_KEY_RATECONTROL_LIMITS_BITRATE_MAXIMUM "RateControl.Limits.Bitrate.Maximum"
#define ST_KEY_RATECONTROL_QP_MINIMUM "RateControl.Quality.Minimum"
#define ST_KEY_RATECONTROL_QP_MAXIMUM "RateControl.Quality.Maximum"
#define ST_KEY_RATECONTROL_QP_I "RateControl.QP.I"
#define ST_KEY_RATECONTROL_QP_P "RateControl.QP.P"
#define ST_KEY_RATECONTROL_QP_B "RateControl.QP.B"
#define ST_KEY_AQ_SPATIAL "AQ.Spatial"
#define ST_KEY_AQ_TEMPORAL "AQ.Temporal"
#define ST_KEY_AQ_STRENGTH "AQ.Strength"
#define ST_KEY_OTHER_BFRAMES "Other.BFrames"
#define ST_KEY_OTHER_BFRAMEREFERENCEMODE "Other.BFrameReferenceMode"
#define ST_KEY_OTHER_ZEROLATENCY "Other.ZeroLatency"
#define ST_KEY_OTHER_WEIGHTEDPREDICTION "Other.WeightedPrediction"
#define ST_KEY_OTHER_NONREFERENCEPFRAMES "Other.NonReferencePFrames"
#define ST_KEY_OTHER_ACCESSUNITDELIMITER "Other.AccessUnitDelimiter"
#define ST_KEY_OTHER_DECODEDPICTUREBUFFERSIZE "Other.DecodedPictureBufferSize"

using namespace streamfx::encoder::ffmpeg::handler;

std::map<nvenc::preset, std::string> nvenc::presets{
	{nvenc::preset::DEFAULT, ST_I18N_PRESET_(Default)},
	{nvenc::preset::SLOW, ST_I18N_PRESET_(Slow)},
	{nvenc::preset::MEDIUM, ST_I18N_PRESET_(Medium)},
	{nvenc::preset::FAST, ST_I18N_PRESET_(Fast)},
	{nvenc::preset::HIGH_PERFORMANCE, ST_I18N_PRESET_(HighPerformance)},
	{nvenc::preset::HIGH_QUALITY, ST_I18N_PRESET_(HighQuality)},
	{nvenc::preset::BLURAYDISC, ST_I18N_PRESET_(BluRayDisc)},
	{nvenc::preset::LOW_LATENCY, ST_I18N_PRESET_(LowLatency)},
	{nvenc::preset::LOW_LATENCY_HIGH_PERFORMANCE, ST_I18N_PRESET_(LowLatencyHighPerformance)},
	{nvenc::preset::LOW_LATENCY_HIGH_QUALITY, ST_I18N_PRESET_(LowLatencyHighQuality)},
	{nvenc::preset::LOSSLESS, ST_I18N_PRESET_(Lossless)},
	{nvenc::preset::LOSSLESS_HIGH_PERFORMANCE, ST_I18N_PRESET_(LosslessHighPerformance)},
};

std::map<nvenc::preset, std::string> nvenc::preset_to_opt{
	{nvenc::preset::DEFAULT, "default"},
	{nvenc::preset::SLOW, "slow"},
	{nvenc::preset::MEDIUM, "medium"},
	{nvenc::preset::FAST, "fast"},
	{nvenc::preset::HIGH_PERFORMANCE, "hp"},
	{nvenc::preset::HIGH_QUALITY, "hq"},
	{nvenc::preset::BLURAYDISC, "bd"},
	{nvenc::preset::LOW_LATENCY, "ll"},
	{nvenc::preset::LOW_LATENCY_HIGH_PERFORMANCE, "llhp"},
	{nvenc::preset::LOW_LATENCY_HIGH_QUALITY, "llhq"},
	{nvenc::preset::LOSSLESS, "lossless"},
	{nvenc::preset::LOSSLESS_HIGH_PERFORMANCE, "losslesshp"},
};

std::map<nvenc::ratecontrolmode, std::string> nvenc::ratecontrolmodes{
	{nvenc::ratecontrolmode::CQP, ST_I18N_RATECONTROL_MODE_(CQP)},
	{nvenc::ratecontrolmode::VBR, ST_I18N_RATECONTROL_MODE_(VBR)},
	{nvenc::ratecontrolmode::VBR_HQ, ST_I18N_RATECONTROL_MODE_(VBR_HQ)},
	{nvenc::ratecontrolmode::CBR, ST_I18N_RATECONTROL_MODE_(CBR)},
	{nvenc::ratecontrolmode::CBR_HQ, ST_I18N_RATECONTROL_MODE_(CBR_HQ)},
	{nvenc::ratecontrolmode::CBR_LD_HQ, ST_I18N_RATECONTROL_MODE_(CBR_LD_HQ)},
};

std::map<nvenc::ratecontrolmode, std::string> nvenc::ratecontrolmode_to_opt{
	{nvenc::ratecontrolmode::CQP, "constqp"},   {nvenc::ratecontrolmode::VBR, "vbr"},
	{nvenc::ratecontrolmode::VBR_HQ, "vbr_hq"}, {nvenc::ratecontrolmode::CBR, "cbr"},
	{nvenc::ratecontrolmode::CBR_HQ, "cbr_hq"}, {nvenc::ratecontrolmode::CBR_LD_HQ, "cbr_ld_hq"},
};

std::map<nvenc::b_ref_mode, std::string> nvenc::b_ref_modes{
	{nvenc::b_ref_mode::INVALID, S_STATE_DEFAULT},
	{nvenc::b_ref_mode::DISABLED, S_STATE_DISABLED},
	{nvenc::b_ref_mode::EACH, ST_I18N_OTHER_BFRAMEREFERENCEMODE ".Each"},
	{nvenc::b_ref_mode::MIDDLE, ST_I18N_OTHER_BFRAMEREFERENCEMODE ".Middle"},
};

std::map<nvenc::b_ref_mode, std::string> nvenc::b_ref_mode_to_opt{
	{nvenc::b_ref_mode::DISABLED, "disabled"},
	{nvenc::b_ref_mode::EACH, "each"},
	{nvenc::b_ref_mode::MIDDLE, "middle"},
};

bool streamfx::encoder::ffmpeg::handler::nvenc::is_available()
{
#if defined(D_PLATFORM_WINDOWS)
#if defined(D_PLATFORM_64BIT)
	std::filesystem::path lib_name = "nvEncodeAPI64.dll";
#else
	std::filesystem::path lib_name = "nvEncodeAPI.dll";
#endif
#else
	std::filesystem::path lib_name = "libnvidia-encode.so.1";
#endif
	try {
		streamfx::util::library::load(lib_name);
		return true;
	} catch (...) {
		return false;
	}
}

void nvenc::override_update(ffmpeg_instance* instance, obs_data_t*)
{
	AVCodecContext* context = const_cast<AVCodecContext*>(instance->get_avcodeccontext());

	int64_t rclookahead = 0;
	int64_t surfaces    = 0;
	int64_t async_depth = 0;

	av_opt_get_int(context, "rc-lookahead", AV_OPT_SEARCH_CHILDREN, &rclookahead);
	av_opt_get_int(context, "surfaces", AV_OPT_SEARCH_CHILDREN, &surfaces);
	av_opt_get_int(context, "async_depth", AV_OPT_SEARCH_CHILDREN, &async_depth);

	// Calculate and set the number of surfaces to allocate (if not user overridden).
	if (surfaces == 0) {
		surfaces = std::max<int64_t>(4ll, (context->max_b_frames + 1ll) * 4ll);
		if (rclookahead > 0) {
			surfaces = std::max<int64_t>(1ll, std::max<int64_t>(surfaces, rclookahead + (context->max_b_frames + 5ll)));
		} else if (context->max_b_frames > 0) {
			surfaces = std::max<int64_t>(4ll, (context->max_b_frames + 1ll) * 4ll);
		} else {
			surfaces = 4;
		}

		av_opt_set_int(context, "surfaces", surfaces, AV_OPT_SEARCH_CHILDREN);
	}

	// Set delay
	context->delay = std::min<int>(std::max<int>(static_cast<int>(async_depth), 3), static_cast<int>(surfaces - 1));
}

void nvenc::get_defaults(obs_data_t* settings, const AVCodec*, AVCodecContext*)
{
	obs_data_set_default_int(settings, ST_KEY_PRESET, static_cast<int64_t>(nvenc::preset::DEFAULT));

	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_MODE, static_cast<int64_t>(ratecontrolmode::CBR_HQ));
	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_TWOPASS, -1);
	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_LOOKAHEAD, -1);
	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_ADAPTIVEI, -1);
	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_ADAPTIVEB, -1);

	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_LIMITS_BITRATE_TARGET, 6000);
	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_LIMITS_BITRATE_MAXIMUM, 0);
	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_LIMITS_BUFFERSIZE, 12000);
	obs_data_set_default_double(settings, ST_KEY_RATECONTROL_LIMITS_QUALITY, 0);

	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_QP_MINIMUM, -1);
	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_QP_MAXIMUM, -1);
	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_QP_I, -1);
	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_QP_P, -1);
	obs_data_set_default_int(settings, ST_KEY_RATECONTROL_QP_B, -1);

	obs_data_set_default_int(settings, ST_KEY_AQ_SPATIAL, -1);
	obs_data_set_default_int(settings, ST_KEY_AQ_STRENGTH, -1);
	obs_data_set_default_int(settings, ST_KEY_AQ_TEMPORAL, -1);

	obs_data_set_default_int(settings, ST_KEY_OTHER_BFRAMES, -1);
	obs_data_set_default_int(settings, ST_KEY_OTHER_BFRAMEREFERENCEMODE, static_cast<int64_t>(b_ref_mode::INVALID));
	obs_data_set_default_int(settings, ST_KEY_OTHER_ZEROLATENCY, -1);
	obs_data_set_default_int(settings, ST_KEY_OTHER_WEIGHTEDPREDICTION, -1);
	obs_data_set_default_int(settings, ST_KEY_OTHER_NONREFERENCEPFRAMES, -1);
	obs_data_set_default_int(settings, ST_KEY_OTHER_ACCESSUNITDELIMITER, -1);
	obs_data_set_default_int(settings, ST_KEY_OTHER_DECODEDPICTUREBUFFERSIZE, -1);

	// Replay Buffer
	obs_data_set_default_int(settings, "bitrate", 0);
}

static bool modified_ratecontrol(obs_properties_t* props, obs_property_t*, obs_data_t* settings) noexcept
{
	bool have_bitrate       = false;
	bool have_bitrate_range = false;
	bool have_quality       = false;
	bool have_qp            = false;

	nvenc::ratecontrolmode rc =
		static_cast<nvenc::ratecontrolmode>(obs_data_get_int(settings, ST_KEY_RATECONTROL_MODE));
	switch (rc) {
	case nvenc::ratecontrolmode::CQP:
		have_qp = true;
		break;
	case nvenc::ratecontrolmode::INVALID:
	case nvenc::ratecontrolmode::CBR:
	case nvenc::ratecontrolmode::CBR_HQ:
	case nvenc::ratecontrolmode::CBR_LD_HQ:
		have_bitrate = true;
		break;
	case nvenc::ratecontrolmode::VBR:
	case nvenc::ratecontrolmode::VBR_HQ:
		have_bitrate       = true;
		have_bitrate_range = true;
		have_quality       = true;
		have_qp            = true;
		break;
	}

	obs_property_set_visible(obs_properties_get(props, ST_I18N_RATECONTROL_LIMITS), have_bitrate || have_quality);
	obs_property_set_visible(obs_properties_get(props, ST_KEY_RATECONTROL_LIMITS_BUFFERSIZE), have_bitrate);
	obs_property_set_visible(obs_properties_get(props, ST_KEY_RATECONTROL_LIMITS_QUALITY), have_quality);
	obs_property_set_visible(obs_properties_get(props, ST_KEY_RATECONTROL_LIMITS_BITRATE_TARGET), have_bitrate);
	obs_property_set_visible(obs_properties_get(props, ST_KEY_RATECONTROL_LIMITS_BITRATE_MAXIMUM), have_bitrate_range);

	obs_property_set_visible(obs_properties_get(props, ST_I18N_RATECONTROL_QP), have_qp || have_bitrate_range);
	obs_property_set_visible(obs_properties_get(props, ST_KEY_RATECONTROL_QP_MINIMUM), have_bitrate_range);
	obs_property_set_visible(obs_properties_get(props, ST_KEY_RATECONTROL_QP_MAXIMUM), have_bitrate_range);
	obs_property_set_visible(obs_properties_get(props, ST_KEY_RATECONTROL_QP_I), have_qp);
	obs_property_set_visible(obs_properties_get(props, ST_KEY_RATECONTROL_QP_P), have_qp);
	obs_property_set_visible(obs_properties_get(props, ST_KEY_RATECONTROL_QP_B), have_qp);

	return true;
}

static bool modified_aq(obs_properties_t* props, obs_property_t*, obs_data_t* settings) noexcept
{
	bool spatial_aq = streamfx::util::is_tristate_enabled(obs_data_get_int(settings, ST_KEY_AQ_SPATIAL));
	obs_property_set_visible(obs_properties_get(props, ST_KEY_AQ_STRENGTH), spatial_aq);
	return true;
}

void nvenc::get_properties_pre(obs_properties_t* props, const AVCodec*)
{
	auto p = obs_properties_add_list(props, ST_KEY_PRESET, D_TRANSLATE(ST_I18N_PRESET), OBS_COMBO_TYPE_LIST,
									 OBS_COMBO_FORMAT_INT);
	for (auto kv : presets) {
		obs_property_list_add_int(p, D_TRANSLATE(kv.second.c_str()), static_cast<int64_t>(kv.first));
	}
}

void nvenc::get_properties_post(obs_properties_t* props, const AVCodec* codec)
{
	{ // Rate Control
		obs_properties_t* grp = props;
		if (!streamfx::util::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(props, ST_I18N_RATECONTROL, D_TRANSLATE(ST_I18N_RATECONTROL), OBS_GROUP_NORMAL,
									 grp);
		}

		{
			auto p = obs_properties_add_list(grp, ST_KEY_RATECONTROL_MODE, D_TRANSLATE(ST_I18N_RATECONTROL_MODE),
											 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_set_modified_callback(p, modified_ratecontrol);
			for (auto kv : ratecontrolmodes) {
				obs_property_list_add_int(p, D_TRANSLATE(kv.second.c_str()), static_cast<int64_t>(kv.first));
			}
		}

		{
			auto p = streamfx::util::obs_properties_add_tristate(grp, ST_KEY_RATECONTROL_TWOPASS,
																 D_TRANSLATE(ST_I18N_RATECONTROL_TWOPASS));
		}

		{
			auto p = obs_properties_add_int_slider(grp, ST_KEY_RATECONTROL_LOOKAHEAD,
												   D_TRANSLATE(ST_I18N_RATECONTROL_LOOKAHEAD), -1, 32, 1);
			obs_property_int_set_suffix(p, " frames");
			//obs_property_set_modified_callback(p, modified_lookahead);
		}

		{
			auto p = streamfx::util::obs_properties_add_tristate(grp, ST_KEY_RATECONTROL_ADAPTIVEI,
																 D_TRANSLATE(ST_I18N_RATECONTROL_ADAPTIVEI));
		}

		if (strcmp(codec->name, "h264_nvenc") == 0) {
			auto p = streamfx::util::obs_properties_add_tristate(grp, ST_KEY_RATECONTROL_ADAPTIVEB,
																 D_TRANSLATE(ST_I18N_RATECONTROL_ADAPTIVEB));
		}
	}

	{
		obs_properties_t* grp = props;
		if (!streamfx::util::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(props, ST_I18N_RATECONTROL_LIMITS, D_TRANSLATE(ST_I18N_RATECONTROL_LIMITS),
									 OBS_GROUP_NORMAL, grp);
		}

		{
			auto p = obs_properties_add_float_slider(grp, ST_KEY_RATECONTROL_LIMITS_QUALITY,
													 D_TRANSLATE(ST_I18N_RATECONTROL_LIMITS_QUALITY), 0, 100, 0.01);
		}

		{
			auto p = obs_properties_add_int(grp, ST_KEY_RATECONTROL_LIMITS_BITRATE_TARGET,
											D_TRANSLATE(ST_I18N_RATECONTROL_LIMITS_BITRATE_TARGET), -1,
											std::numeric_limits<int32_t>::max(), 1);
			obs_property_int_set_suffix(p, " kbit/s");
		}

		{
			auto p = obs_properties_add_int(grp, ST_KEY_RATECONTROL_LIMITS_BITRATE_MAXIMUM,
											D_TRANSLATE(ST_I18N_RATECONTROL_LIMITS_BITRATE_MAXIMUM), -1,
											std::numeric_limits<int32_t>::max(), 1);
			obs_property_int_set_suffix(p, " kbit/s");
		}

		{
			auto p = obs_properties_add_int(grp, ST_KEY_RATECONTROL_LIMITS_BUFFERSIZE,
											D_TRANSLATE(ST_I18N_RATECONTROL_LIMITS_BUFFERSIZE), 0,
											std::numeric_limits<int32_t>::max(), 1);
			obs_property_int_set_suffix(p, " kbit");
		}
	}

	{
		obs_properties_t* grp = props;
		if (!streamfx::util::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(props, ST_I18N_RATECONTROL_QP, D_TRANSLATE(ST_I18N_RATECONTROL_QP),
									 OBS_GROUP_NORMAL, grp);
		}

		{
			auto p = obs_properties_add_int_slider(grp, ST_KEY_RATECONTROL_QP_MINIMUM,
												   D_TRANSLATE(ST_I18N_RATECONTROL_QP_MINIMUM), -1, 51, 1);
		}
		{
			auto p = obs_properties_add_int_slider(grp, ST_KEY_RATECONTROL_QP_MAXIMUM,
												   D_TRANSLATE(ST_I18N_RATECONTROL_QP_MAXIMUM), -1, 51, 1);
		}

		{
			auto p = obs_properties_add_int_slider(grp, ST_KEY_RATECONTROL_QP_I, D_TRANSLATE(ST_I18N_RATECONTROL_QP_I),
												   -1, 51, 1);
		}
		{
			auto p = obs_properties_add_int_slider(grp, ST_KEY_RATECONTROL_QP_P, D_TRANSLATE(ST_I18N_RATECONTROL_QP_P),
												   -1, 51, 1);
		}
		{
			auto p = obs_properties_add_int_slider(grp, ST_KEY_RATECONTROL_QP_B, D_TRANSLATE(ST_I18N_RATECONTROL_QP_B),
												   -1, 51, 1);
		}
	}

	{
		obs_properties_t* grp = props;
		if (!streamfx::util::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(props, ST_I18N_AQ, D_TRANSLATE(ST_I18N_AQ), OBS_GROUP_NORMAL, grp);
		}

		{
			auto p =
				streamfx::util::obs_properties_add_tristate(grp, ST_KEY_AQ_SPATIAL, D_TRANSLATE(ST_I18N_AQ_SPATIAL));
			obs_property_set_modified_callback(p, modified_aq);
		}
		{
			auto p =
				obs_properties_add_int_slider(grp, ST_KEY_AQ_STRENGTH, D_TRANSLATE(ST_I18N_AQ_STRENGTH), -1, 15, 1);
		}
		{
			auto p =
				streamfx::util::obs_properties_add_tristate(grp, ST_KEY_AQ_TEMPORAL, D_TRANSLATE(ST_I18N_AQ_TEMPORAL));
		}
	}

	{
		obs_properties_t* grp = props;
		if (!streamfx::util::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(props, ST_I18N_OTHER, D_TRANSLATE(ST_I18N_OTHER), OBS_GROUP_NORMAL, grp);
		}

		{
			auto p =
				obs_properties_add_int_slider(grp, ST_KEY_OTHER_BFRAMES, D_TRANSLATE(ST_I18N_OTHER_BFRAMES), -1, 4, 1);
			obs_property_int_set_suffix(p, " frames");
		}

		{
			auto p = obs_properties_add_list(grp, ST_KEY_OTHER_BFRAMEREFERENCEMODE,
											 D_TRANSLATE(ST_I18N_OTHER_BFRAMEREFERENCEMODE), OBS_COMBO_TYPE_LIST,
											 OBS_COMBO_FORMAT_INT);
			for (auto kv : b_ref_modes) {
				if (kv.first == nvenc::b_ref_mode::EACH && (std::string_view("h264_nvenc") == codec->name)) {
					// H.264 does not support using all B-Frames as reference.
					continue;
				}
				obs_property_list_add_int(p, D_TRANSLATE(kv.second.c_str()), static_cast<int64_t>(kv.first));
			}
		}

		{
			auto p = streamfx::util::obs_properties_add_tristate(grp, ST_KEY_OTHER_ZEROLATENCY,
																 D_TRANSLATE(ST_I18N_OTHER_ZEROLATENCY));
		}

		{
			auto p = streamfx::util::obs_properties_add_tristate(grp, ST_KEY_OTHER_WEIGHTEDPREDICTION,
																 D_TRANSLATE(ST_I18N_OTHER_WEIGHTEDPREDICTION));
		}

		{
			auto p = streamfx::util::obs_properties_add_tristate(grp, ST_KEY_OTHER_NONREFERENCEPFRAMES,
																 D_TRANSLATE(ST_I18N_OTHER_NONREFERENCEPFRAMES));
		}

		{
			auto p = streamfx::util::obs_properties_add_tristate(grp, ST_KEY_OTHER_ACCESSUNITDELIMITER,
																 D_TRANSLATE(ST_I18N_OTHER_ACCESSUNITDELIMITER));
		}

		{
			auto p = obs_properties_add_int_slider(grp, ST_KEY_OTHER_DECODEDPICTUREBUFFERSIZE,
												   D_TRANSLATE(ST_I18N_OTHER_DECODEDPICTUREBUFFERSIZE), -1, 16, 1);
			obs_property_int_set_suffix(p, " frames");
		}
	}
}

void nvenc::get_runtime_properties(obs_properties_t* props, const AVCodec*, AVCodecContext*)
{
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_PRESET), false);
	obs_property_set_enabled(obs_properties_get(props, ST_I18N_RATECONTROL), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_RATECONTROL_MODE), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_RATECONTROL_TWOPASS), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_RATECONTROL_LOOKAHEAD), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_RATECONTROL_ADAPTIVEI), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_RATECONTROL_ADAPTIVEB), false);
	obs_property_set_enabled(obs_properties_get(props, ST_I18N_RATECONTROL_LIMITS), true);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_RATECONTROL_LIMITS_BUFFERSIZE), true);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_RATECONTROL_LIMITS_BITRATE_TARGET), true);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_RATECONTROL_LIMITS_BITRATE_MAXIMUM), true);
	obs_property_set_enabled(obs_properties_get(props, ST_I18N_RATECONTROL_LIMITS_QUALITY), false);
	obs_property_set_enabled(obs_properties_get(props, ST_I18N_RATECONTROL_QP), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_RATECONTROL_QP_MINIMUM), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_RATECONTROL_QP_MAXIMUM), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_RATECONTROL_QP_I), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_RATECONTROL_QP_P), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_RATECONTROL_QP_B), false);
	obs_property_set_enabled(obs_properties_get(props, ST_I18N_AQ), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_AQ_SPATIAL), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_AQ_STRENGTH), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_AQ_TEMPORAL), false);
	obs_property_set_enabled(obs_properties_get(props, ST_I18N_OTHER), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_OTHER_BFRAMES), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_OTHER_BFRAMEREFERENCEMODE), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_OTHER_ZEROLATENCY), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_OTHER_WEIGHTEDPREDICTION), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_OTHER_NONREFERENCEPFRAMES), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_OTHER_ACCESSUNITDELIMITER), false);
	obs_property_set_enabled(obs_properties_get(props, ST_KEY_OTHER_DECODEDPICTUREBUFFERSIZE), false);
}

void nvenc::update(obs_data_t* settings, const AVCodec* codec, AVCodecContext* context)
{
	{
		preset c_preset = static_cast<preset>(obs_data_get_int(settings, ST_KEY_PRESET));
		auto   found    = preset_to_opt.find(c_preset);
		if (found != preset_to_opt.end()) {
			av_opt_set(context->priv_data, "preset", found->second.c_str(), 0);
		} else {
			av_opt_set(context->priv_data, "preset", nullptr, 0);
		}
	}

	{ // Rate Control
		bool have_bitrate       = false;
		bool have_bitrate_range = false;
		bool have_quality       = false;
		bool have_qp            = false;

		ratecontrolmode rc    = static_cast<ratecontrolmode>(obs_data_get_int(settings, ST_KEY_RATECONTROL_MODE));
		auto            rcopt = ratecontrolmode_to_opt.find(rc);
		if (rcopt != ratecontrolmode_to_opt.end()) {
			av_opt_set(context->priv_data, "rc", rcopt->second.c_str(), AV_OPT_SEARCH_CHILDREN);
		} else {
			have_bitrate       = true;
			have_bitrate_range = true;
			have_quality       = true;
			have_qp            = true;
		}

		av_opt_set_int(context->priv_data, "cbr", 0, AV_OPT_SEARCH_CHILDREN);
		switch (rc) {
		case ratecontrolmode::CQP:
			have_qp = true;
			break;
		case ratecontrolmode::INVALID:
		case ratecontrolmode::CBR:
		case ratecontrolmode::CBR_HQ:
		case ratecontrolmode::CBR_LD_HQ:
			have_bitrate = true;
			av_opt_set_int(context->priv_data, "cbr", 1, AV_OPT_SEARCH_CHILDREN);
			break;
		case ratecontrolmode::VBR:
		case ratecontrolmode::VBR_HQ:
			have_bitrate_range = true;
			have_bitrate       = true;
			have_quality       = true;
			have_qp            = true;
			break;
		}

		// Two Pass
		if (int tp = static_cast<int>(obs_data_get_int(settings, ST_KEY_RATECONTROL_TWOPASS)); tp > -1) {
			av_opt_set_int(context->priv_data, "2pass", tp ? 1 : 0, AV_OPT_SEARCH_CHILDREN);
		}

		// Look Ahead # of Frames
		int la = static_cast<int>(obs_data_get_int(settings, ST_KEY_RATECONTROL_LOOKAHEAD));
		if (!streamfx::util::is_tristate_default(la)) {
			av_opt_set_int(context->priv_data, "rc-lookahead", la, AV_OPT_SEARCH_CHILDREN);
		}

		// Adaptive I-Frames
		if (int64_t adapt_i = obs_data_get_int(settings, ST_KEY_RATECONTROL_ADAPTIVEI);
			!streamfx::util::is_tristate_default(adapt_i) && (la != 0)) {
			// no-scenecut is inverted compared to our UI.
			av_opt_set_int(context->priv_data, "no-scenecut", 1 - adapt_i, AV_OPT_SEARCH_CHILDREN);
		}

		// Adaptive B-Frames
		constexpr std::string_view h264_encoder_name = "h264_nvenc";
		if (h264_encoder_name == codec->name) {
			if (int64_t adapt_b = obs_data_get_int(settings, ST_KEY_RATECONTROL_ADAPTIVEB);
				!streamfx::util::is_tristate_default(adapt_b) && (la != 0)) {
				av_opt_set_int(context->priv_data, "b_adapt", adapt_b, AV_OPT_SEARCH_CHILDREN);
			}
		}

		if (have_bitrate) {
			int64_t v = obs_data_get_int(settings, ST_KEY_RATECONTROL_LIMITS_BITRATE_TARGET);
			if (v > -1)
				context->bit_rate = static_cast<int>(v * 1000);

			// Support for Replay Buffer
			obs_data_set_int(settings, "bitrate", v);
		} else {
			context->bit_rate = 0;
		}
		if (have_bitrate_range) {
			if (int64_t max = obs_data_get_int(settings, ST_KEY_RATECONTROL_LIMITS_BITRATE_MAXIMUM); max > -1)
				context->rc_max_rate = static_cast<int>(max * 1000);
		} else {
			//context->rc_min_rate = 0;
			context->rc_max_rate = 0;
		}

		// Buffer Size
		if (have_bitrate || have_bitrate_range) {
			if (int64_t v = obs_data_get_int(settings, ST_KEY_RATECONTROL_LIMITS_BUFFERSIZE); v > -1)
				context->rc_buffer_size = static_cast<int>(v * 1000);
		} else {
			context->rc_buffer_size = 0;
		}

		// Quality Limits
		if (have_quality) {
			if (int qmin = static_cast<int>(obs_data_get_int(settings, ST_KEY_RATECONTROL_QP_MINIMUM)); qmin > -1)
				context->qmin = qmin;
			if (int qmax = static_cast<int>(obs_data_get_int(settings, ST_KEY_RATECONTROL_QP_MAXIMUM)); qmax > -1)
				context->qmax = qmax;
		} else {
			context->qmin = -1;
			context->qmax = -1;
		}

		// Quality Target
		if (double_t v = obs_data_get_double(settings, ST_KEY_RATECONTROL_LIMITS_QUALITY) / 100.0 * 51.0; v > 0) {
			av_opt_set_double(context->priv_data, "cq", v, AV_OPT_SEARCH_CHILDREN);
		}

		// QP Settings
		if (have_qp) {
			if (int64_t qp = obs_data_get_int(settings, ST_KEY_RATECONTROL_QP_I); qp > -1)
				av_opt_set_int(context->priv_data, "init_qpI", static_cast<int>(qp), AV_OPT_SEARCH_CHILDREN);
			if (int64_t qp = obs_data_get_int(settings, ST_KEY_RATECONTROL_QP_P); qp > -1)
				av_opt_set_int(context->priv_data, "init_qpP", static_cast<int>(qp), AV_OPT_SEARCH_CHILDREN);
			if (int64_t qp = obs_data_get_int(settings, ST_KEY_RATECONTROL_QP_B); qp > -1)
				av_opt_set_int(context->priv_data, "init_qpB", static_cast<int>(qp), AV_OPT_SEARCH_CHILDREN);
		}
	}

	{ // AQ
		int64_t saq = obs_data_get_int(settings, ST_KEY_AQ_SPATIAL);
		int64_t taq = obs_data_get_int(settings, ST_KEY_AQ_TEMPORAL);

		if (strcmp(codec->name, "h264_nvenc") == 0) {
			if (!streamfx::util::is_tristate_default(saq))
				av_opt_set_int(context->priv_data, "spatial-aq", saq, AV_OPT_SEARCH_CHILDREN);
			if (!streamfx::util::is_tristate_default(taq))
				av_opt_set_int(context->priv_data, "temporal-aq", taq, AV_OPT_SEARCH_CHILDREN);
		} else {
			if (!streamfx::util::is_tristate_default(saq))
				av_opt_set_int(context->priv_data, "spatial_aq", saq, AV_OPT_SEARCH_CHILDREN);
			if (!streamfx::util::is_tristate_default(taq))
				av_opt_set_int(context->priv_data, "temporal_aq", taq, AV_OPT_SEARCH_CHILDREN);
		}
		if (streamfx::util::is_tristate_enabled(saq))
			if (int64_t aqs = obs_data_get_int(settings, ST_KEY_AQ_STRENGTH); aqs > -1)
				av_opt_set_int(context->priv_data, "aq-strength", static_cast<int>(aqs), AV_OPT_SEARCH_CHILDREN);
	}

	{ // Other
		if (int64_t bf = obs_data_get_int(settings, ST_KEY_OTHER_BFRAMES); bf > -1)
			context->max_b_frames = static_cast<int>(bf);

		if (int64_t zl = obs_data_get_int(settings, ST_KEY_OTHER_ZEROLATENCY); !streamfx::util::is_tristate_default(zl))
			av_opt_set_int(context->priv_data, "zerolatency", zl, AV_OPT_SEARCH_CHILDREN);
		if (int64_t nrp = obs_data_get_int(settings, ST_KEY_OTHER_NONREFERENCEPFRAMES);
			!streamfx::util::is_tristate_default(nrp))
			av_opt_set_int(context->priv_data, "nonref_p", nrp, AV_OPT_SEARCH_CHILDREN);
		if (int64_t v = obs_data_get_int(settings, ST_KEY_OTHER_ACCESSUNITDELIMITER);
			!streamfx::util::is_tristate_default(v))
			av_opt_set_int(context->priv_data, "aud", v, AV_OPT_SEARCH_CHILDREN);
		if (int64_t v = obs_data_get_int(settings, ST_KEY_OTHER_DECODEDPICTUREBUFFERSIZE); v > -1)
			av_opt_set_int(context->priv_data, "dpb_size", v, AV_OPT_SEARCH_CHILDREN);

		int64_t wp = obs_data_get_int(settings, ST_KEY_OTHER_WEIGHTEDPREDICTION);
		if ((context->max_b_frames > 0) && streamfx::util::is_tristate_enabled(wp)) {
			DLOG_WARNING("[%s] Weighted Prediction disabled because of B-Frames being used.", codec->name);
			av_opt_set_int(context->priv_data, "weighted_pred", 0, AV_OPT_SEARCH_CHILDREN);
		} else if (!streamfx::util::is_tristate_default(wp)) {
			av_opt_set_int(context->priv_data, "weighted_pred", wp, AV_OPT_SEARCH_CHILDREN);
		}

		{
			auto found = b_ref_mode_to_opt.find(
				static_cast<b_ref_mode>(obs_data_get_int(settings, ST_KEY_OTHER_BFRAMEREFERENCEMODE)));
			if (found != b_ref_mode_to_opt.end()) {
				av_opt_set(context->priv_data, "b_ref_mode", found->second.c_str(), AV_OPT_SEARCH_CHILDREN);
			}
		}
	}
}

void nvenc::log_options(obs_data_t*, const AVCodec* codec, AVCodecContext* context)
{
	using namespace ::streamfx::ffmpeg;

	DLOG_INFO("[%s]   NVIDIA NVENC:", codec->name);
	tools::print_av_option_string2(context, "preset", "    Preset",
								   [](int64_t v, std::string_view o) { return std::string(o); });
	tools::print_av_option_string2(context, "rc", "    Rate Control",
								   [](int64_t v, std::string_view o) { return std::string(o); });
	tools::print_av_option_bool(context, "2pass", "      Two Pass");
	tools::print_av_option_int(context, "rc-lookahead", "      Look-Ahead", "Frames");
	tools::print_av_option_bool(context, "no-scenecut", "      Adaptive I-Frames", true);
	if (strcmp(codec->name, "h264_nvenc") == 0)
		tools::print_av_option_bool(context, "b_adapt", "      Adaptive B-Frames");

	DLOG_INFO("[%s]       Bitrate:", codec->name);
	tools::print_av_option_int(context, "b", "        Target", "bits/sec");
	tools::print_av_option_int(context, "minrate", "        Minimum", "bits/sec");
	tools::print_av_option_int(context, "maxrate", "        Maximum", "bits/sec");
	tools::print_av_option_int(context, "bufsize", "        Buffer", "bits");
	DLOG_INFO("[%s]       Quality:", codec->name);
	tools::print_av_option_int(context, "cq", "        Target", "");
	tools::print_av_option_int(context, "qmin", "        Minimum", "");
	tools::print_av_option_int(context, "qmax", "        Maximum", "");
	DLOG_INFO("[%s]       Quantization Parameters:", codec->name);
	tools::print_av_option_int(context, "init_qpI", "        I-Frame", "");
	tools::print_av_option_int(context, "init_qpP", "        P-Frame", "");
	tools::print_av_option_int(context, "init_qpB", "        B-Frame", "");

	tools::print_av_option_int(context, "bf", "    B-Frames", "Frames");
	tools::print_av_option_string2(context, "b_ref_mode", "      Reference Mode",
								   [](int64_t v, std::string_view o) { return std::string(o); });

	DLOG_INFO("[%s]     Adaptive Quantization:", codec->name);
	if (strcmp(codec->name, "h264_nvenc") == 0) {
		tools::print_av_option_bool(context, "spatial-aq", "      Spatial AQ");
		tools::print_av_option_int(context, "aq-strength", "        Strength", "");
		tools::print_av_option_bool(context, "temporal-aq", "      Temporal AQ");
	} else {
		tools::print_av_option_bool(context, "spatial_aq", "      Spatial AQ");
		tools::print_av_option_int(context, "aq-strength", "        Strength", "");
		tools::print_av_option_bool(context, "temporal_aq", "      Temporal AQ");
	}

	DLOG_INFO("[%s]     Other:", codec->name);
	tools::print_av_option_bool(context, "zerolatency", "      Zero Latency");
	tools::print_av_option_bool(context, "weighted_pred", "      Weighted Prediction");
	tools::print_av_option_bool(context, "nonref_p", "      Non-reference P-Frames");
	tools::print_av_option_bool(context, "strict_gop", "      Strict GOP");
	tools::print_av_option_bool(context, "aud", "      Access Unit Delimiters");
	tools::print_av_option_bool(context, "bluray-compat", "      Bluray Compatibility");
	if (strcmp(codec->name, "h264_nvenc") == 0)
		tools::print_av_option_bool(context, "a53cc", "      A53 Closed Captions");
	tools::print_av_option_int(context, "dpb_size", "      DPB Size", "Frames");
}

void streamfx::encoder::ffmpeg::handler::nvenc::migrate(obs_data_t* settings, uint64_t version, const AVCodec* codec,
														AVCodecContext* context)
{
	switch (static_cast<uint64_t>(obs_data_get_int(settings, S_VERSION)) & STREAMFX_MASK_UPDATE) {
	default:
	case STREAMFX_MAKE_VERSION(0, 8, 0, 0):

		obs_data_set_int(settings, ST_KEY_RATECONTROL_LIMITS_BITRATE_TARGET,
						 obs_data_get_int(settings, "RateControl.Bitrate.Target"));
		obs_data_set_int(settings, ST_KEY_RATECONTROL_LIMITS_BITRATE_MAXIMUM,
						 obs_data_get_int(settings, "RateControl.Bitrate.Maximum"));
		obs_data_set_int(settings, ST_KEY_RATECONTROL_LIMITS_BUFFERSIZE,
						 obs_data_get_int(settings, "RateControl.BufferSize"));
		obs_data_set_double(settings, ST_KEY_RATECONTROL_LIMITS_QUALITY,
							obs_data_get_double(settings, "RateControl.Quality.Target"));
		obs_data_set_int(settings, ST_KEY_RATECONTROL_QP_MINIMUM,
						 obs_data_get_int(settings, "RateControl.Quality.Minimum"));
		obs_data_set_int(settings, ST_KEY_RATECONTROL_QP_MAXIMUM,
						 obs_data_get_int(settings, "RateControl.Quality.Maximum"));

		break;
	}
}
