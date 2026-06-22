// #pragma once

// #include <string>
// #include <unordered_map>
// #include <memory>

// namespace Shit {
//     class FontManager final {
//         friend class ResourceManager;
//     public:
// 		FontManager(const FontManager&) = delete;
// 		FontManager& operator=(const FontManager&) = delete;
// 		FontManager(FontManager&&) = default;
// 		FontManager& operator=(FontManager&&) = default;
    
//     private:
// 		FontManager() = default;
// 		~FontManager() = default;
        
// 		sf::Font* loadFont(const std::string& filePath);
// 		sf::Font* getFont(const std::string& filePath);
// 		void unloadFont(const std::string& filePath);
// 		void clearFont();

//         // Fonts缓存
//         std::unordered_map<std::string, std::unique_ptr<sf::Font>> m_fonts;
//     };
// }