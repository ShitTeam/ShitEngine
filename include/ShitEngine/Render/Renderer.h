#pragma once

#include <memory>
#include <SDL3/SDL_render.h>

#include "../Core/Core.h"

namespace Shit
{
    class SHIT_API Renderer final {
    public:
        Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer(Renderer&&) = default;
		Renderer& operator=(Renderer&&) = default;

        // --- 静态API ---
        static Renderer& GetInstance();
        static bool Init() { return GetInstance().init(); }
    	static void LimitFPS() { GetInstance().limitFPS(); }
        static SDL_Renderer* GetRenderer() { return GetInstance().m_renderer.get(); }
        static unsigned int GetTargetFPS() { return GetInstance().getTargetFPS(); }
        static void SetTargetFPS(unsigned int fps) { GetInstance().setTargetFPS(fps); }

    private:
        Renderer() = default;
        ~Renderer() = default;

        /**
         * @brief 初始化函数
         * @return 是否初始化成功
         */
        bool init();

        /**
    	 * @brief 限制帧率
    	 */
    	void limitFPS();

        // getter & setter
        unsigned int getTargetFPS() const { return m_targetFPS; }
        void setTargetFPS(unsigned int fps) { m_targetFPS = fps; }

        struct SDLRendererDeleter {
			void operator()(SDL_Renderer* renderer) const {
				if (renderer) {
					SDL_DestroyRenderer(renderer);
				}
			}
		};

        std::unique_ptr<SDL_Renderer, SDLRendererDeleter> m_renderer;
        unsigned int m_targetFPS = 144;
    };
}
