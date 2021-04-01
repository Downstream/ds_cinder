#pragma once
#ifndef DS_UI_LAYOUT_SMART_LAYOUT_SPRITE
#define DS_UI_LAYOUT_SMART_LAYOUT_SPRITE


#include <ds/ui/layout/layout_sprite.h>
#include <ds/ui/sprite/sprite.h>
#include <ds/ui/sprite/sprite_engine.h>

namespace ds {
namespace ui {

/**
 * \class SmartLayout
 *        SmartLayout combines a layout sprite with XML importing.
 */
class SmartLayout : public ds::ui::LayoutSprite {
  public:

	SmartLayout(ds::ui::SpriteEngine& engine);

	/// Automatically loads the xmlLayout file (at %APP%/data/layouts/xmlLayoutFile) and runs the layout upon creation
	SmartLayout(ds::ui::SpriteEngine& engine, const std::string& xmlLayoutFile,
				const std::string xmlFileLocation = "%APP%/data/layouts/", const bool loadImmediately = true);

	/// Updates the layoutfile for this sprite & reloads (by default)
	void setLayoutFile(const std::string& xmlLayoutFile, const std::string xmlFileLocation = "%APP%/data/layouts/",
					   const bool loadImmediately = true);

	/// Clears children and spritemap, then reloads from layout file
	void initialize();

	/// If this smart layout has a child with this name
	bool hasSprite(const std::string& spriteName);

	/// Gets the sprite with the name specified. Returns nullptr if it doesn't exist.
	ds::ui::Sprite* getSprite(const std::string& spriteName);

	/// Gets the sprite with the name specified typecast to the template type. Returns nullptr if it doesn't exist or is
	/// a different type.
	template <typename T>
	T* getSprite(const std::string& spriteName) {
		static_assert(std::is_base_of<ds::ui::Sprite, T>::value,
					  "getSprite requires template type to be derived from ds::ui::Sprite");
		return dynamic_cast<T*>(getSprite(spriteName));
	}

	/// If spriteName exisits, calls spriteGenerator function to return a sprite. Otherwise logs an
	/// error
	void tryAddChild(const std::string spriteName, std::function<ds::ui::Sprite*(void)> spriteGenerator);

	/// Adds newChild to the sprite with spriteName
	void addSpriteChild(const std::string spriteName, ds::ui::Sprite* newChild);

	/// Calls the lambda callback for the event type from Template, casting event automatically
	/// \note These templates need to be in the header to work
	template <class EVENT>
	void listenToEvents(std::function<void(const EVENT&)> callback) {
		mEventClient.listenToEvents<EVENT>(callback);
	}

	/// Disables / removes callback (if it exists) for the event from the template
	template <class EVENT>
	void stopListeningToEvents() {
		mEventClient.stopListeningToEvents<EVENT>();
	}

	/// Sets the wide text for a Text sprite with a name of spriteName
	void setSpriteText(const std::string& spriteName, const std::wstring& theText);
	/// Sets the text for a Text sprite with a name of spriteName
	void setSpriteText(const std::string& spriteName, const std::string& theText);
	/// Configurations the Text sprite with a name of spriteName
	void setSpriteFont(const std::string& spriteName, const std::string& textCfgName);

	/// Set the image file for an Image sprite with name of spriteName. Image path is automatically expanded
	void setSpriteImage(const std::string& spriteName, const std::string& imagePath, bool cache = false, bool skipMetaData = false); 
	void setSpriteImage(const std::string& spriteName, const std::string& imagePath, int flags);
	/// Set the image resource for an Image sprite with name of spriteName.
	void setSpriteImage(const std::string& spriteName, ds::Resource imageResource, bool cache = false, bool skipMetaData = false);
	void setSpriteImage(const std::string& spriteName, ds::Resource imageResource, int flags);

	/// Set the tap function on a sprite named spriteName
	void setSpriteTapFn(const std::string&											 spriteName,
						const std::function<void(ds::ui::Sprite*, const ci::vec3&)>& tapCallback);

	/// Set the click function on a sprite named spriteName, only for ImageButton, SpriteButton, and LayoutButton
	void setSpriteClickFn(const std::string&		 spriteName,
						const std::function<void()>& clickCallback);

	/// Set the content model for this SmartLayout.
	///  - this will apply the content to each named sprite in the map, if they had a "model" property applied
	///  - model="resource:this->image_resource" for instance will map the "image_resource" property of the set content model to the sprite's resource property
	///  - You can use any sprite property in the first field
	///  - The content model set here can be retrieved with getContentModel()
	///  - After all sprites have been set, the callback for setContentUpdatedCallback() is called
	virtual void setContentModel(ds::model::ContentModelRef& theData);

	/// Returns the last-set ContentModelRef
	ds::model::ContentModelRef getContentModel() { return mContentModel; }

	void setContentUpdatedCallback(std::function<void()> func) { mContentUpdatedCallback = func; }

  protected:
	using sMap = std::map<std::string, ds::ui::Sprite*>;

	virtual void onUpdateServer(const ds::UpdateParams& p) override;

	bool					   mInitialized;
	std::string				   mLayoutFile;
	bool					   mNeedsLayout;
	ds::EventClient			   mEventClient;
	sMap					   mSpriteMap;
	ds::model::ContentModelRef mContentModel;
	std::function<void()>		mContentUpdatedCallback;

  private:
	/// Apply a single model to a given sprite child
	void applyModelToSprite(ds::ui::Sprite* child, const std::string& childName, const std::string& model);

	/// Creates child sprites for each child of a ContentModelRef
	void applyEachModelToSprite(ds::ui::Sprite* child, const std::string& eachModel);
};

}  // namespace ui
}  // namespace ds

#endif
