#pragma once

#include "common.hpp"

namespace Framework
{
	namespace App
	{
		bool run();
		
		bool is_running();

		void exit();

		glm::ivec2 get_size();

		void* get_window_ptr();
	}

	namespace Internal
	{
		void app_step();
	}
}