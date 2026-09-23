/////////////////////////////////
// Fontsystem.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include <string>
#include <unordered_map>
#include <vector>
/////////////////////////////////


// --------------------------------
// FONT SYSTEM - Supports BMFont for now, plans to expand to SDF later
// Notes: 
// --------------------------------


/////////////////////////////////
//	|	FontType - Enum representing different font types supported by the font system.
//	|___________________________________________________________________________________
enum class FontType {
	BMFont, // Bitmap font
	SDF		// Signed Distance Field font (planned for future support)
};
/////////////////////////////////




/////////////////////////////////
//	|	Glyph - Represents a single glyph (character) in a font, including its texture coordinates, size, offset, and advance for rendering text.
//	|___________________________________________________________________________________
struct Glyph {
	int id;					// Character ID (Unicode code point)
	
	// Texture coordinates in the font atlas (normalized)
	float x, y;				// Texture coordinates in the font atlas
	float w, h;				// Size of the glyph in pixels
	
	// Bearing
	float xoffset, yoffset; // Offset for rendering the glyph relative to the baseline
	
	// Cursor advance
	float xadvance;			// How much to advance the cursor after rendering this glyph

	// UV coordinates for rendering the glyph
	float u0, v0; // Top-left UV coordinates
	float u1, v1; // Bottom-right UV coordinates
};
/////////////////////////////////



/////////////////////////////////
//	|	KearningPair - Represents a kerning pair for adjusting the spacing between specific pairs of characters in a font.
//	|___________________________________________________________________________________
struct KerningPair {
	int first;	  // First character ID in the kerning pair
	int second;	  // Second character ID in the kerning pair
	float amount; // Amount to adjust the spacing between the two characters
};
/////////////////////////////////



/////////////////////////////////
//	|	FontAsset - Represents a font asset, including its type, size, line height, and a mapping of glyphs for rendering text.
//	|___________________________________________________________________________________
struct FontAsset {
	// Font type and name
	FontType type;							// Type of the font (BMFont, SDF, etc.)
	std::string name;						// Name of the font

	// Atlas texture ID (for OpenGL texture binding)
	unsigned int textureID = 0;

	// Atlas dimensions
	float scaleW = 0;						// Width of the font atlas in pixels
	float scaleH = 0;						// Height of the font atlas in pixels

	// Line metrics
	float lineHeight = 0;					// Height of a line of text in pixels
	float base = 0;							// Base offset for the font (distance from top of line to baseline)

	// Glyph table
	std::unordered_map<int, Glyph> glyphs;	// Mapping of character ID to Glyph data

	// Kerning pairs for adjusting spacing between specific character pairs
	std::vector<KerningPair> kerning;

	// Check if a glyph exists for a given character ID
	bool HasGlyph(int charID) const { return glyphs.find(charID) != glyphs.end(); }
	int fallbackGlyph = '?'; // Fallback glyph ID for missing characters
};
/////////////////////////////////



/////////////////////////////////
//	|	Fontsystem - Manages font assets, loading, and rendering of text using different font types (BMFont, SDF, etc.). This class is responsible for loading font data, managing glyphs, and providing access to font metrics for rendering text in the game engine.
//	|___________________________________________________________________________________
class Fontsystem {
public:
	// LoadBMFont - Loads a font asset
	bool LoadBMFont(const std::string& fontName, const std::string& filePath, const std::string& pngPath);


	// GetFont - Retrieves a font asset by its name. Returns a pointer to the FontAsset if found, nullptr otherwise.
	const FontAsset* GetFont(const std::string& fontName) const;


	// GetKerning - Retrieves the kerning amount for a specific pair of characters in a given font. Returns the kerning amount if found, 0.0f otherwise.
	float GetKerning(const FontAsset& font, int firstCharID, int secondCharID) const;


	const Glyph* GetGlyph(const FontAsset& font, int charID) const;


	// GetLoadedFonts - Retrieves a list of names of all loaded font assets.
	std::vector<std::string> GetLoadedFonts() const;


	// Private data members for the Fontsystem class
private:
	std::unordered_map<std::string, FontAsset> m_fonts;

	bool ParseBMFontText(const std::string& filePath, FontAsset& fontAsset);
	bool LoadBMFontTexture(const std::string& pngPath, FontAsset& fontAsset);	
};
////////////////////////////////