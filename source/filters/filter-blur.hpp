/*
 * Modern effects for a modern Streamer
 * Copyright (C) 2017-2018 Michael Fabian Dirks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once
#include "common.hpp"
#include <chrono>
#include <functional>
#include <list>
#include <map>
#include "gfx/blur/gfx-blur-base.hpp"
#include "gfx/gfx-source-texture.hpp"
#include "obs/gs/gs-effect.hpp"
#include "obs/gs/gs-helper.hpp"
#include "obs/gs/gs-rendertarget.hpp"
#include "obs/gs/gs-texture.hpp"
#include "obs/obs-source-factory.hpp"

namespace streamfx::filter::blur {
	enum class mask_type : int64_t {
		Region,
		Image,
		Source,
	};

	class blur_instance : public obs::source_instance {
		// Effects
		streamfx::obs::gs::effect _effect_mask;

		// Input
		std::shared_ptr<streamfx::obs::gs::rendertarget> _source_rt;
		std::shared_ptr<streamfx::obs::gs::texture>      _source_texture;
		bool                                             _source_rendered;

		// Rendering
		std::shared_ptr<streamfx::obs::gs::texture>      _output_texture;
		std::shared_ptr<streamfx::obs::gs::rendertarget> _output_rt;
		bool                                             _output_rendered;

		// Blur
		std::shared_ptr<::streamfx::gfx::blur::base> _blur;
		double_t                                     _blur_size;
		double_t                                     _blur_angle;
		std::pair<double_t, double_t>                _blur_center;
		bool                                         _blur_step_scaling;
		std::pair<double_t, double_t>                _blur_step_scale;

		// Masking
		struct {
			bool      enabled;
			mask_type type;
			struct {
				float_t left;
				float_t top;
				float_t right;
				float_t bottom;
				float_t feather;
				float_t feather_shift;
				bool    invert;
			} region;
			struct {
				std::string                                 path;
				std::string                                 path_old;
				std::shared_ptr<streamfx::obs::gs::texture> texture;
			} image;
			struct {
				std::string                                    name_old;
				std::string                                    name;
				bool                                           is_scene;
				std::shared_ptr<streamfx::gfx::source_texture> source_texture;
				std::shared_ptr<streamfx::obs::gs::texture>    texture;
			} source;
			struct {
				float_t r;
				float_t g;
				float_t b;
				float_t a;
			} color;
			float_t multiplier;
		} _mask;

		public:
		blur_instance(obs_data_t* settings, obs_source_t* self);
		~blur_instance();

		public:
		virtual void load(obs_data_t* settings) override;
		virtual void migrate(obs_data_t* settings, uint64_t version) override;
		virtual void update(obs_data_t* settings) override;

		virtual void video_tick(float_t time) override;
		virtual void video_render(gs_effect_t* effect) override;

		private:
		bool apply_mask_parameters(streamfx::obs::gs::effect effect, gs_texture_t* original_texture,
								   gs_texture_t* blurred_texture);
	};

	class blur_factory : public obs::source_factory<filter::blur::blur_factory, filter::blur::blur_instance> {
		std::vector<std::string> _translation_cache;

		public:
		blur_factory();
		virtual ~blur_factory();

		virtual const char* get_name() override;

		virtual void get_defaults2(obs_data_t* settings) override;

		virtual obs_properties_t* get_properties2(filter::blur::blur_instance* data) override;

		std::string translate_string(const char* format, ...);

#ifdef ENABLE_FRONTEND
		static bool on_manual_open(obs_properties_t* props, obs_property_t* property, void* data);
#endif

		public: // Singleton
		static void initialize();

		static void finalize();

		static std::shared_ptr<blur_factory> get();
	};
} // namespace streamfx::filter::blur
