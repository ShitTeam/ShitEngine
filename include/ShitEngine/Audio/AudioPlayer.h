// #include "../Core/Core.h"

// namespace Shit {
//     /**
//      * @brief 音频播放器
//      * 
//      * 负责播放音频和管理音轨
//      */
//     class SHIT_API AudioPlayer final {
//     public:
//         AudioPlayer(const AudioPlayer&) = delete;
// 		AudioPlayer& operator=(const AudioPlayer&) = delete;
// 		AudioPlayer(AudioPlayer&&) = default;
// 		AudioPlayer& operator=(AudioPlayer&&) = default;

//         // --- 静态API ---
//         static AudioPlayer& GetInstance();
//         static bool Init() { return GetInstance().init(); }

//     private:
//         AudioPlayer();
//         ~AudioPlayer();

//         bool init();



//         MIX_Mixer* m_mixer = nullptr;
//         std::unordered_map<std::string, std::vector<MIX_Track*>> m_trackGro; // 创建的音轨
//     };
// }