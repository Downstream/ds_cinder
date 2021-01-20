#ifndef DS_UI_SPRITE_SPRITE_H_
#define DS_UI_SPRITE_SPRITE_H_

// Cinder includes
#include "cinder/Cinder.h"
#include "cinder/Color.h"
#include "cinder/Tween.h"
#include "cinder/Timeline.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"

// STL includes
#include <list>
#include <exception>
// DS includes
#include "ds/app/app_defs.h"
#include "ds/data/user_data.h"
#include "ds/gl/uniform.h"
#include "ds/util/bit_mask.h"
#include "ds/ui/sprite/dirty_state.h"
#include "ds/ui/touch/touch_process.h"
#include "ds/ui/touch/multi_touch_constraints.h"
#include "ds/ui/tween/sprite_anim.h"
#include "ds/ui/sprite/shader/sprite_shader.h"
#include "ds/ui/sprite/util/blend.h"
#include "ds/util/idle_timer.h"
#include "ds/debug/debug_defines.h"
#include "ds/app/blob_reader.h"
#include "ds/data/data_buffer.h"
#include "ds/ui/sprite/sprite_engine.h"
#include "ds/cfg/settings.h"

namespace ds {
namespace gl { class ClipPlaneState; }
class BlobRegistry;
class CameraPick;
class DrawParams;
class Engine;
class EngineRoot;
class Event;
class UpdateParams;

namespace ui {
	struct DragDestinationInfo;
	struct TapInfo;
	struct TouchInfo;

	/// Attribute access
	extern const char SPRITE_ID_ATTRIBUTE;

	/**
	 \class Sprite
	 \brief The root unit for almost any on-screen element. 
		- UI elements extend from Sprite to participate in the display hierarchy, drawing transformations, built-in tweens, touch picking, etc.
		- Sprite can have children, which will be positioned in local space relative to the parent sprite.
		- Sprite will release all children when removing. Use release() to remove the sprite from it's parent and delete it.
		- The base Sprite class can be set to display a solid rectangle, assuming it has a size, is non-transparent, and has a color.
		- Sprites can be animated using the SpriteAnimatable functions to gracefully change size, position, opacity, scale, color, and rotation.
		- Sprites can clip their children along their bounds using setClipping(true)	 */
	class Sprite : public SpriteAnimatable
	{
	public:

		/** Generic sprite creation function.
			The variadic args will be passed in the same order to your Sprite's constructor.
			\param engine The SpriteEngine for your app.
			\param parent An optional parent for the new sprite to be added to as a child.
			\param args Parameter arguments that are passed to your custom sprite type, in order, on construction		*/
		template <typename T, typename... Args>
		static T&				make(SpriteEngine& engine, Sprite* parent = nullptr, Args... args);

		/** Sprite creation convenience, throw on failure.
			\param allocFn A function to be called to create the sprite. See makeSprite() for an example.
			\param parent An optional parent for the new sprite to be added to as a child.		*/
		template <typename T>
		static T&				makeAlloc(const std::function<T*(void)>& allocFn, Sprite* parent = nullptr);

		/** Sprite creation convenience for this basic sprite type, throws on failure.
			\param engine The SpriteEngine for your app.
			\param parent An optional parent for the new sprite to be added to as a child.		*/
		static Sprite&			makeSprite(SpriteEngine& engine, Sprite* parent = nullptr){ return makeAlloc<Sprite>([&engine]()->Sprite*{return new Sprite(engine); }, parent); }

		/** Constructor for Sprite.
			\param engine The SpriteEngine for your app.
			\param width Initial horizontal size of the sprite.
			\param height Initial vertical size of the sprite.		*/
		Sprite(SpriteEngine& engine, float width = 0.0f, float height = 0.0f);
		virtual ~Sprite();

		/** Update function for when this app is set to be a client.
			Don't override this function, use onUpdateClient if you need it
			\param updateParams UpdateParams containing some conveniences such as delta time.		*/
		virtual void			updateClient(const ds::UpdateParams& updateParams) final;

		/** Update function for when this app is set to be a client.
			Sprite behaviour can vary whether this is running on the server or client, and you can hook into that here.
			This is called after everything else in the base updateServer, and allows you to update things on a frame-by-frame basis.
			This was added to prevent bugs from not calling the inherited updateServer when overriding that function
		\param updateParams UpdateParams containing some conveniences such as delta time.		*/
		virtual void			onUpdateClient(const ds::UpdateParams& updateParams){};

		/** Update function for when this app is set to be a server.
			Sprite behaviour can vary whether this is running on the server or client, and you can hook into that here.
			Don't override this function (you can't in fact). Override onUpdateServer() for stuff that should change each frame.
			\param updateParams UpdateParams containing some conveniences such as delta time.		*/
		virtual void			updateServer(const ds::UpdateParams& updateParams) final;


		/** Update function for when this app is set to be a server.
			Sprite behaviour can vary whether this is running on the server or client, and you can hook into that here.
			This is called after everything else in the base updateServer, and allows you to update things on a frame-by-frame basis.
			This was added to prevent bugs from not calling the inherited updateServer when overriding that function
		\param updateParams UpdateParams containing some conveniences such as delta time.		*/
		virtual void			onUpdateServer(const ds::UpdateParams& updateParams){}

		/** Draw function for when this app is set to be a client.
			In most cases, you'll want to override drawLocalClient() to do custom drawing, as this function handles drawing for children as well.
			\param transformMatrix The transform matrix of the parent.
			\param drawParams Parameters for drawing, such as the opacity of the parent.		*/
		virtual void			drawClient(const ci::mat4 &transformMatrix, const DrawParams &drawParams);

		/** Draw function for when this app is set to be a server.
			In most cases, you'll want to override drawLocalClient() to do custom drawing, as this function handles drawing for children as well.
			\param transformMatrix The transform matrix of the parent.
			\param drawParams Parameters for drawing, such as the opacity of the parent.		*/
		virtual void			drawServer(const ci::mat4 &transformMatrix, const DrawParams &drawParams);

		/** Returns the unique id for this Sprite. The SpriteEngine will automatically generate an id for the sprite when constructed.
			We recommend you use references or pointers to keep track of sprites, rather than looking up by Id.
			\return Unique sprite id.		*/
		ds::sprite_id_t			getId() const { return mId; }

		/** Gets the SpriteEngine that was passed to this Sprite upon construction.
			\return The app's SpriteEngine.		*/
		ds::ui::SpriteEngine&	getEngine() { return mEngine; }

		/** Get the width, height, and depth of this Sprite. Convenience for getWidth(), getHeight() and getDepth().
			\return Returns a 3-dimensional vector equivalent to ci::vec3(width, height, depth).		*/
		const ci::vec3			getSize() const;

		/** Sets the width and height of the Sprite.
			This does not affect the scale of the Sprite. Many subclasses set the size of the Sprite themselves, such as Image and Text.
			\param size2d The size to set in the form of ci::vec2f(width, height).		*/
		void					setSize(const ci::vec2& size2d);

		/** Sets the width and height of the Sprite.
			This does not affect the scale of the Sprite.
			Many subclasses set the size of the Sprite themselves, such as Image and Text, and in those cases you should not call this function yourself.
			\param width The new width of the Sprite.
			\param height The new height of the Sprite.		*/
		void					setSize(float width, float height);

		/** Sets the width, height, and depth of the Sprite.
			This does not affect the scale of the Sprite.
			Many subclasses set the size of the Sprite themselves, such as Image and Text, and in those cases you should not call this function yourself.
			This is identical to setSize(), except it also sets the depth.
			\param size3d The size to set in the form of ci::vec2f(width, height, depth).		*/
		virtual void			setSizeAll(const ci::vec3& size3d);

		/** Sets the width, height, and depth of the Sprite.
			This does not affect the scale of the Sprite.
			Many subclasses set the size of the Sprite themselves, such as Image and Text, and in those cases you should not call this function yourself.
			This is identical to setSize(), except it also sets the depth.
			\param width The new width of the Sprite.
			\param height The new height of the Sprite.
			\param depth The new depth of the Sprite.		*/
		virtual void			setSizeAll(float width, float height, float depth);

		/** Sets the size based on the boundaries of this Sprite's immediate children, not recursive		*/
		void					sizeToChildBounds();

		/** Answer the preferred size for this object.
			This is intended to be part of a layout pass, so the default preferred size is 0 not width/height.
			Subclasses need to override this to be meaningful.
			\return The 3d vector of the size this sprite should be. Override this function and return a meaningful value.		 */
		virtual ci::vec3		getPreferredSize() const;

		/** The width of this sprite, not including scale.
			For instance, an Image Sprite will always return the width of the image from this function, even if the Sprite has been scaled.
			\return The width in pixels of this Sprite.		*/
		virtual float			getWidth() const;

		/** The height of this sprite, not including scale.
			For instance, an Image Sprite will always return the height of the image from this function, even if the Sprite has been scaled.
			\return The height in pixels of this Sprite.		*/
		virtual float			getHeight() const;

		/** The depth of this sprite, not including scale.
			For instance, an Image Sprite will always return the height of the image from this function, even if the Sprite has been scaled.
			\return The height in pixels of this Sprite.		*/
		float					getDepth() const;

		/** The local display width of this sprite.
			Assuming all parents are scale=1.0, this will return the number of pixels this Sprite displays.
			\return The display width of this Sprite, width * scale.x		*/
		float					getScaleWidth() const;

		/** The local display height of this sprite.
			Assuming all parents are scale=1.0, this will return the number of pixels this Sprite displays.
			\return The display height of this Sprite, height * scale.y		*/
		float					getScaleHeight() const;

		/** The local display depth of this sprite.
			Assuming all parents are scale=1.0, this will return the number of pixels this Sprite displays.
			\return The display depth of this Sprite, depth * scale.z		*/
		float					getScaleDepth() const;

		/** Set the position of the Sprite in local space (the parent's relative co-ordinates).
			For perspective Sprites, the y position is inverted, so greater y values will move upwards.
			For ortho Sprites, positive y position is downwards.
			Z position forwards or backwards depends on your camera setup.
			\param pos The 3d vector of the new position, in pixels. */
		void					setPosition(const ci::vec3& pos);

		/** Set the position of the Sprite in local space (the parent's relative co-ordinates).
			For perspective Sprites, the y position is inverted, so greater y values will move upwards.
			For ortho Sprites, positive y position is downwards.
			Z position will use the current z-position of the sprite.
			\param pos The x and y position. */
		void					setPosition(const ci::vec2& pos);

		/** Set the position of the Sprite in local space (the parent's relative co-ordinates).
			For perspective Sprites, the y position is inverted, so greater y values will move upwards.
			For ortho Sprites, positive y position is downwards.
			Z position forwards or backwards depends on your camera setup.
			\param x The x (horizontal) position, in pixels.
			\param y The y (vertical) position, in pixels.
			\param z The z (depth) position, in pixels. */
		void					setPosition(float x, float y, float z = 0.0f);

		/** Get the position of the Sprite in local space (the parent's relative co-ordinates).
			For perspective Sprites, the y position is inverted, so greater y values will move upwards.
			For ortho Sprites, positive y position is downwards.
			Z position forwards or backwards depends on your camera setup.
			\return The 3d vector of the new position, in pixels. */
		const ci::vec3&			getPosition() const;

		/** Get the position of the Sprite in global space. 
			Returns ci::vec3::zero() if this sprite has no parent.
			\return The 3d vector of the world-space position, in pixels. */
		const ci::vec3			getGlobalPosition() const;

		/** Get the center position of the Sprite in global space. This is a convenience that's getGlobalPostion() + getLocalCenterPosition();
		Returns ci::vec3::zero() + getLocalCenterPosition() if this sprite has no parent.
		\return The 3d vector of the center of this sprite world-space position, in pixels. */
		const ci::vec3			getGlobalCenterPosition() const;

		/** Change the position of the Sprite relative to it's current position.
			\param delta 3d vector of the amount to move the Sprite in pixels		*/
		void					move(const ci::vec3 &delta);

		/** Change the position of the Sprite relative to it's current position.
			\param deltaX Horizontal amount to move the Sprite in pixels
			\param deltaY Vertical amount to move the Sprite in pixels. Inverted for perspective Sprites.
			\param deltaZ Depth amount to move the Sprite in pixels	*/
		void					move(float deltaX, float deltaY, float deltaZ = 0.0f);

		/** Change the scale of the Sprite.
			\param scale 3d vector of the new scale of the Sprite		*/
		void					setScale(const ci::vec3 &scale);

		/** Change the scale of the Sprite.
			\param x New horizontal scale of the Sprite
			\param y New vertical scale of the Sprite
			\param z New depth scale of the Sprite		*/
		void					setScale(float x, float y, float z = 1.0f);

		/** Change the scale of the Sprite. Scales proportional in all 3 directions
			\param scale New scale (x, y, and z will be the same) of the Sprite */
		void					setScale(float scale);

		/** The scale of the Sprite.
			\return scale 3d vector of the current scale of the Sprite		*/
		const ci::vec3&			getScale() const;

		/** Center point of the Sprite for transform calculations as a percentage (0.0 - 1.0).
			Scale, rotate and position will happen around this center point.
			Default is 0.0, 0.0, 0.0, which is the top left corner of the Sprite.
			\param center 3d vector of the center percentage. */
		virtual void			setCenter(const ci::vec3 &center);

		/** Center point of the Sprite for transform calculations as a percentage (0.0 - 1.0).
			Scale, rotate and position will happen around this center point.
			Default is 0.0, 0.0, 0.0, which is the top left corner of the Sprite.
			\param x Horizontal center percentage.
			\param y Vertical center percentage.
			\param z Depth center percentage.  */
		virtual void			setCenter(float x, float y, float z = 0.0f);

		/** Center point of the Sprite for transform calculations as a percentage (0.0 - 1.0).
			Scale, rotate and position happen around this center point.
			Default is 0.0, 0.0, 0.0, which is the top left corner of the Sprite.
			\return center 3d vector of the center percentage. */
		const ci::vec3&			getCenter() const;

		/** The "midde" of the Sprite, NOT related to setCenter and getCenter (anchor), in the parent's co-ordinate space.
			Effectively mPosition + getLocalCenterPosition();
			\return 3d Vector of the pixel position of the center of this Sprite. */
		ci::vec3				getCenterPosition() const;

		/** The "middle" of the Sprite, NOT related to setCenter and getCenter (anchor), in this Sprite's co-ordinate space.
			Effectively \code ci::vec3(floorf(mWidth/2.0f), floorf(mHeight/2.0f), mPosition.z) \endcode
			\return 3d Vector of the pixel position of the center of this Sprite, in local space. */
		ci::vec3				getLocalCenterPosition() const;

		/** Set the rotation around the z-axis, in degrees.
			Leaves the x and y axis rotations alone.
			\param zRot Sets the rotation around the z-axis, in degrees. 	*/
		void					setRotation(float zRot);

		/** Set the rotation around all 3 axis'es, in degrees.
			\param xRot Sets the rotation around the x-axis, in degrees.
			\param yRot Sets the rotation around the y-axis, in degrees.
			\param zRot Sets the rotation around the z-axis, in degrees. 	*/
		void					setRotation(const float xRot, const float yRot, const float zRot);

		/** Set the rotation around all 3 axis'es, in degrees.
			\param rot 3d vector of the new rotation, in degrees.*/
		void					setRotation(const ci::vec3 &rot);

		/** Set the rotation around all 3 given axis'es with the given degree
			\param axis    axis to rotate around
			\param angle   Amount of rotation in degrees */
		void					setRotation(const ci::vec3 &axis, const float angle);
		bool					mDoSpecialRotation;
		float					mDegree;

		/** Get the rotation around all 3 axis'es, in degrees.
			\return 3d vector of the current rotation, in degrees.*/
		ci::vec3				getRotation() const;

		/** Get the rectangle that contains this sprite, in Parent's local space.
			Includes all transformation, including scale and rotation.
			\return Rectangle that contains this sprite, in Parent's local space.		*/
		ci::Rectf				getBoundingBox() const;

		/** Get the rectangle that contains all children, in this Sprite's space.
			Includes all transformation, including scale and rotation.
			\return Rectangle that contains this sprite, in Parent's local space.		*/
		ci::Rectf				getChildBoundingBox() const;

		/** Whether to draw be by Sprite order or z position.
			Only works on a per Sprite basis, not recursive.
			See sendToFront(), sendToBack() for sprite order drawing control.
			\param drawSorted True for drawing by z-position of children, false to draw by sprite order. */
		void					setDrawSorted(bool drawSorted);

		/** Whether to draw be by Sprite order or z position.
			Only works on a per Sprite basis, not recursive.
			See sendToFront(), sendToBack() for sprite order drawing control.
			\return True for drawing by z-position of children, false to draw by sprite order. */
		bool					getDrawSorted() const;

		const ci::mat4&			getTransform() const;
		const ci::mat4&			getInverseTransform() const;
		const ci::mat4&			getGlobalTransform() const;
		const ci::mat4&			getInverseGlobalTransform() const;

		/** Arbitrary data (float's and int's) on this sprite. See UserData for storage usage. */
		ds::UserData&			getUserData();
		/** Arbitrary data (float's and int's) on this sprite. See UserData for storage usage. */
		const ds::UserData&		getUserData() const;

		/** Adds a sprite as a child of this Sprite.
			The child will be placed at it's position in this Sprite's coordinate space,
			and take on this Sprite's rotation, scale, position, and opacity.
			Transform changes to this Sprite will also affect the new child.
			New children are added at the bottom of the Sprite and will display on top of other children and this Sprite.
			If you have a pointer to a Sprite, you can use addChildPtr().
			\param newChild The Sprite to be added as a child*/
		void					addChild(Sprite& newChild);

		/** Adds a sprite as a child of this Sprite.
			The child will be placed at it's position in this Sprite's coordinate space,
			and take on this Sprite's rotation, scale, position, and opacity.
			Transform changes to this Sprite will also affect the new child.
			New children are added at the bottom of the Sprite and will display on top of other children and this Sprite.
			If you have a reference to a Sprite, you can use addChild().
			\param newChild The Sprite to be added as a child*/
		template<class T>
		T*						addChildPtr(T* newChild){
			if(!newChild) return nullptr;
			addChild(*newChild);
			return newChild;
		}

		/** Removes child from this Sprite, but does not delete it.
			It's only recommend to use this function if you want to reuse the child sprite later.
			\param child The Sprite to be removed from this Sprite. Must be a child of this Sprite. */
		void					removeChild(Sprite &child);

		/** Remove this Sprite from it's parent, does not delete.
			It's only recommend to use this function if you want to reuse this sprite later. */
		void					removeParent();

		/** Removes this Sprite from parent; removes and deletes all it's children. Does not delete this Sprite.
			It's only recommend to use this function if you want to reuse this sprite later. */
		void					remove();


		/** The recommended "removal" API: removes this Sprite from parent; removes and deletes all it's children, deletes this Sprite.
			In most cases, you will want to use this function to get rid of sprites.
			This removes this sprite and it's children from the display list and clears all of that memory.
			Remember to clear any pointers related to this Sprite or any references in any vectors you have saved yourself. */
		void					release();

		/** Removes and deletes all children.
			Equivalent to calling release() on every child. */
		void					clearChildren();

		std::vector<Sprite*>	getChildren() { return mChildren; }

		/** Check to see if this Sprite contains child.
			\param child The child to check if it is contained on this Sprite.
			\return True if the child is a child of this Sprite. False if the child is a damn stranger. */
		bool					containsChild(Sprite *child) const;

		/** Run a function for every child of this Sprite.
			\param funkyTown The lambda function to be called for each Sprite.
			\param recurse Call this function for all children of all children of all my children. */
		void					forEachChild(const std::function<void(Sprite&)>& funkyTown, const bool recurse = false);

		/** Traverse children recursively and find a sprite the with the given name.
			\param name The name given from layout or from setSpriteName(). */
		Sprite*					getFirstDescendantWithName(const std::wstring& name);

		/** Sends sprite to front of parents child list. */
		void					sendToFront();
		/** Sends sprite to back of parents child list.	*/
		void					sendToBack();

		/** Set the display color of this Sprite. Implementation can vary by Sprite type. */
		virtual void			setColor(const ci::Color&);
		/** Set the display color of this Sprite. Implementation can vary by Sprite type.
			\param r Red component of the color from 0.0 to 1.0
			\param g Green component of the color from 0.0 to 1.0
			\param b Blue component of the color from 0.0 to 1.0 */
		virtual void			setColor(float r, float g, float b);

		/** Get the display color of this Sprite. Implementation can vary by Sprite type. */
		ci::Color				getColor() const;

		/** A convenience to set the color and the opacity.
			The alpha component of the color will set the opacity.
			Setting the opacity after this call will overwrite this value. */
		virtual void			setColorA(const ci::ColorA&);

		/** A convenience to get the color and the opacity.
			The alpha component of the color is the opacity.*/
		ci::ColorA				getColorA() const;

		/** Set the opacity of this Sprite (transparency).
			0.0 is invisible, 1.0 is fully opaque.
			\param opacity The new opacity value of this Sprite. */
		void					setOpacity(float opacity);

		/** Get the opacity of this Sprite (transparency).
			0.0 is invisible, 1.0 is fully opaque.
			\return The current opacity value of this Sprite. */
		float					getOpacity() const;
		/** Returns the final opacity of the Sprite decided based on all its parents.
			Useful for custom drawing routines where getOpacity() does not work.
			The difference between this and getOpacity() is that, getOpacity() is the opacity
			that user decides what it needs to be via setOpacity() but getDrawOpacity() is
			the opacity calculated in the scene-graph by sprite's parent.*/
		float					getDrawOpacity() const;

		/** Whether or not to render this Sprite in the draw cycle; does not affect children.
			For basic Sprites, to draw a rectangle, this needs to be set to false.
			\param transparent True means this sprite will not draw (but it's children could). False will render this Sprite. */
		void					setTransparent(bool transparent);
		/** Whether or not to render this Sprite in the draw cycle; does not affect children.
			\return True means this sprite will not draw (but it's children could). False will render this Sprite. */
		bool					getTransparent() const;

		/** The opposite of hide(), affects this Sprite and it's children.
			Does not set the value of children recursively, just shortcuts the draw loop. */
		virtual void			show();

		/** The opposite of show(), affects this Sprite and it's children.
			A hidden Sprite doesn't participate in drawing or touch picking, but does get updated and can be animated. */
		virtual void			hide();

		/** If this sprite is show() or hide().
			\return True == this Sprite is visible (show()), or false == hide()*/
		bool					visible() const;

		/** Subclasses can handle the event, a convenience for handling events without setting up event clients.
			\param event The Event to be handled. */
		virtual void			eventReceived(const ds::Event& event){};

		/** A convenience for notifying parents of events. Passes the event up through the parent hierarchy to the root Sprite.
			Calls eventReceived() for each parent.
			\param event The Event to be passed up the chain. */
		void					parentEventReceived(const ds::Event& event);

		/** Returns the parent for this Sprite, if any. Can return nullptr if there is no parent, so check before using. */
		Sprite*					getParent() const;

		/** Convert coordinate space from global (world) space to the local coordinate space of this Sprite. May not work for perspective Sprites.
			For example, if you have a global touch point, you can find it's local location like this:
			\code	if(getParent()){
			ci::vec3 localPoint = getParent()->globalToLocal(touchInfo.mCurrentGlobalPoint);
			}
			\endcode
			\param globalPoint The 3d vector in global coordinate space to be converted.
			\return A local 3d vector in the coordinate space of this Sprite. */
		ci::vec3				globalToLocal(const ci::vec3 &globalPoint);

		/** Convert coordinate space from local coordinate space of this Sprite to global (world) coordinate space. May not work for perspective Sprites.
			For example, you could figure out where to launch a media viewer from a button on a panel like this:
			\code //In the button click handler
			ci::vec3 globalPosition = localToGlobal(mTheLaunchButton.getPosition());
			mEngine.getNotifier().notify(CustomMediaViewerLaunchEvent(globalPosition));
			\endcode
			\param localPoint The 3d vector in local coordinate space to be converted.
			\return A local 3d vector in the coordinate space of this Sprite. */
		ci::vec3				localToGlobal(const ci::vec3 &localPoint);

		/** Check if a global point is inside the Sprite's bounding box, does not take perspective into account.
			\param point The global point to check if it's inside this Sprite's bounding box.
			\param pad Extra pixel size outside this Sprite's bounding box. Useful for ad-hoc touch padding.
			\return True if the point is inside the bounding box, false for outside. */
		virtual bool			contains(const ci::vec3& point, const float pad = 0.0f) const;

		/** Recursively checks the Sprite hierarchy list for an enabled, visible sprite with a scale > 0.0 and any size for touch picking.
			This is for Ortho Sprites. Perspective Sprites use getPerspectiveHit()
			\param point The global point to check.
			\return The Sprite that is the best candidate for touch picking. Can return nullptr if there was no valid pick.*/
		virtual Sprite*			getHit(const ci::vec3 &point);

		/** Recursively checks the Sprite hierarchy list for an enabled, visible sprite with a scale > 0.0 and any size for touch picking.
			This is for Perspective Sprites. Ortho Sprites use getHit()
			\param pick Some parameters for perspective picking.
			\return The Sprite that is the best candidate for touch picking. Can return nullptr if there was no valid pick.*/
		Sprite*					getPerspectiveHit(CameraPick& pick);

		/** Touch handling enabling and disabling. Does not affect children. Sprite needs to be visible to handle touches.
			\param flag True to enable touch handling, false disables this Sprite. */
		void					enable(bool flag);

		/** Is this Sprite enabled to handle touch events? */
		bool					isEnabled() const;

		/**	Defines the type of touch handling for this Sprite. Requires enable(true); to have been called at some point.
			Constraints defined in multi_touch_constraints.h. For example: ds::ui::MULTITOUCH_INFO_ONLY.
			\param constraints A bitmask of how touch is handled. For example: ds::ui::MULTITOUCH_CAN_POSITION | ds::ui::MULTITOUCH_CAN_ROTATE */
		void					enableMultiTouch(const BitMask & constraints);

		/** Disables specific MultiTouch handling, but leaves the enable() setting alone.
			After calling this, the Sprite will still grab touch events, but not do any specific multitouch handling (like position, rotate, etc). Will send out tap and touchinfo callbacks. */
		void					disableMultiTouch();

		/** The BitMask of*/
		const BitMask&			getMultiTouchConstraints(){ return mMultiTouchConstraints; }

		/** Has enableMultiTouch() been called?  */
		bool					multiTouchEnabled() const;

		/** Does this Sprite have a specific Multitouch bitmask set? see ds::ui::MULTITOUCH_INFO_ONLY */
		bool					hasMultiTouchConstraint(const BitMask &constraint = MULTITOUCH_NO_CONSTRAINTS) const;

		/** Set a lambda callback for each time this sprite gets interaction. Requires the sprite to be enabled. See TouchInfo for what data comes in. */
		void					setProcessTouchCallback(const std::function<void(Sprite *, const TouchInfo &)> &func);

		/** Stateful tap is the new-style and should be preferred over others.  It lets you
			know the state of the tap in addition to the count.  It also lets clients cancel
			 a tap if, for example, they only want to handle single taps.  Answer true as
			long as you want to keep checking for a tap, false otherwise. */
		void					setTapInfoCallback(const std::function<bool(Sprite *, const TapInfo &)> &func);

		/** Get notified if a swipe occurred. This is called after the touch completes. Set the swipe threshold in engine.xml for how sensitive this is */
		void					setSwipeCallback(const std::function<void(Sprite *, const ci::vec3 &)> &func);

		/** Get notified when this sprite has been tapped. A tap is a single finger down and up within the tap threshold. More than one finger at a time cancels taps. 
			If doubleTapCallback has been set, waits for the double tap time to expire before being notified. */
		void					setTapCallback(const std::function<void(Sprite *, const ci::vec3 &)> &func);

		/** Get notified if a double tap has occurred within the double tap threshold. */
		void					setDoubleTapCallback(const std::function<void(Sprite *, const ci::vec3 &)> &func);

		/** Get notified if this sprite gets dragged over any desintation sprites. Set destination sprites with mEngine.addToDragDestinationList()*/
		void					setDragDestinationCallback(const std::function<void(Sprite *, const DragDestinationInfo &)> &func);

		/** Used internally by touch process to set the destination sprite when this sprite is being checked for drag destinations */
		void					setDragDestination(Sprite *dragDestination);
		/** Returns the sprite destination this sprite has been dragged over. Requires setDragDestinationCallback() to have been set. */
		Sprite*					getDragDestination() const;

		/** true will size the sprite using setSize() on a touch scale gesture
			false (the default) will scale the sprite using setScale9) on a touch scale gesture.
			MULTITOUCH_CAN_SCALE has to be a touch flag as well as the sprite being enabled to take effect */
		void					setTouchScaleMode(bool doSizeScale);

		void					setInnerHitFunction(std::function<const bool(const ci::vec3&)> func);

		/** Calls a function after the delay in seconds. Only one function is active at a time, so if you set this twice, the first delayed call will be ignored.
			Also see src/ds/time_callback or mEngine.timedCallback() or mEngine.repeatedCallback() for a generic timed callback*/
		void					callAfterDelay(const std::function<void(void)>&, const float delay_in_seconds);

		/** Cancels any pending callAfterDelay() funcitons */
		void					cancelDelayedCall();

		/** This sprite is within the boundaries of the src_rect for this instance */
		bool					inBounds() const;

		/** If this sprite should check if it's in bounds or not */
		void					setCheckBounds(bool checkBounds);
		bool					getCheckBounds() const;

		/** If the content of this sprite has been loaded. Base Sprite always returns true. Override for specific sprite types. */
		virtual bool			isLoaded() const;

		/** If this sprite got some dirt on it. hahaha just kidding.
			This is if any properties have been modified since the last frame. */
		bool					isDirty() const;
		void					writeTo(ds::DataBuffer&);
		void					readFrom(ds::BlobReader&);
		/// Only used when running in client mode
		void					writeClientTo(ds::DataBuffer&);
		/// IMPORTANT: readClientFrom must not be virtual. If any of the clients try to communicate
		/// back with server, the communication must happen through "readClientAttributeFrom".
		void					readClientFrom(ds::DataBuffer&);

		void					setBlendMode(const BlendMode &blendMode);
		BlendMode				getBlendMode() const;

		///	Determines if the final render will be to the display or a texture.
		void					setFinalRenderToTexture(bool render_to_texture, ci::gl::Fbo::Format format = ci::gl::Fbo::Format());
		bool					isFinalRenderToTexture();
		//Retrieve the rendered output texture
		ci::gl::TextureRef		getFinalOutTexture();
		void					setupFinalRenderBuffer();

		/// WARNING: ONLY shader loading is network safe. Uniforms are not synchronized.
		void					setBaseShader(const std::string &location, const std::string &shadername, bool applyToChildren = false);
		void					setBaseShader(const std::string &vertShaderString, const std::string& fragShaderString, const std::string &shadername, bool applyToChildren = false);

		/// Set the resource content for a sprite. This is a base function that should be overridden by anything that can take a Resource (Image, Video, PDF, etc)
		virtual void			setResource(const ds::Resource&);

		SpriteShader&			getBaseShader();
		std::string				getBaseShaderName() const;
		ds::gl::Uniform&		getUniform();
		void					setShaderExtraData(const ci::vec4& data);
		bool					getUseShaderTexture() const;

		void					setClipping(bool flag);
		bool					getClipping() const;

		/// Call this to extend the idle timeout all the way up the chain to the root
		virtual void			userInputReceived();

		/// Set the number of seconds since the last userInputReceived() before this sprite reports as idle
		void					setSecondBeforeIdle(const double);
		double					secondsToIdle() const;
		bool					isIdling() const;
		void					startIdling();
		void					resetIdleTimer();
		void					clearIdleTimer();

		/// Prevent this sprite (and all children) from replicating. NOTE: Should
		/// only be done once on construction, if you change it, weird things could happen.
		void					setNoReplicationOptimization(const bool = false);
		/// Special function to mark every sprite from me down as dirty.
		void					markTreeAsDirty();

		/// When true, the touch input is automatically rotated to account for my rotation.
		void					setRotateTouches(const bool = false);
		bool					isRotateTouches() const;

		bool					getPerspective() const;
		/// Total hack resulting from my unfamiliarity with 3D systems. This can sometimes be necessary for
		/// views that are inside of perspective cameras, but are expressed in screen coordinates.
		void					setIsInScreenCoordsHack(const bool);

		void					setUseDepthBuffer(bool useDepth);
		bool					getUseDepthBuffer() const;

		/// If this sprite renders locally and the radius is > 0, will draw a rounded rect
		void					setCornerRadius(const float newRadius);
		float					getCornerRadius() const;

		/// Answer true if this sprite currently has any touches.
		bool					hasTouches() const;
		/*
		 * \brief must be passed inside handle touch Moved or else will result in an infinite loop.
		 */
		void					passTouchToSprite(Sprite *destinationSprite, const TouchInfo &touchInfo);

		/// A hack needed by the engine, which constructs root types before the blobs are assigned. 
		void					postAppSetup();

		/// Mark this sprite to be a debug sprite layer.
		///	The primary use case is server-only or client-only setups, so the stats view can draw when not enabled and not be colored weird.
		///	Client apps don't generally need to set this flag, as it happens automagically.	
		void					setDrawDebug(const bool doDebug);

		/// If this sprite has been flagged to draw as a debug layer. Will draw in the server draw loop even if disabled. 		
		bool					getDrawDebug();

		/// Set the name of this sprite. No guarantee of uniqueness
		void					setSpriteName(const std::wstring& name);

		// attempts to get the settings used for the layout that this
		// sprite is part of. The sprite need not have been created in
		// the layout for this to work, but on of its ancestor sprite needs to have been.
		// if this sprite or no ancestor was created via layout, then this will return nullptr
		ds::cfg::Settings* getLayoutSettings();
		void setLayoutSettings(ds::cfg::Settings& settings);

		/// Return the sprite's name, no guarantee of uniqueness. Returns the Id if there's no name set and useDefault is true.
		const std::wstring		getSpriteName(const bool useDefault = true) const;

		/// For use by any layout classes you may want to implement. Default = 0.0f or 0 for all of these
		float					mLayoutTPad;
		float					mLayoutBPad;
		float					mLayoutLPad;
		float					mLayoutRPad;
		ci::vec2				mLayoutSize;
		ci::vec3				mLayoutFudge;
		int						mLayoutHAlign;
		int						mLayoutVAlign;
		int						mLayoutUserType;
		bool					mLayoutFixedAspect;

		bool					mExportWithXml;

	protected:
		friend class        TouchManager;
		friend class        TouchProcess;
		friend class		ds::gl::ClipPlaneState;
		friend class		SpriteAnimatable;

		void				swipe(const ci::vec3 &swipeVector);
		bool				tapInfo(const TapInfo&);
		void				tap(const ci::vec3 &tapPos);
		void				doubleTap(const ci::vec3 &tapPos);
		void				dragDestination(Sprite *sprite, const DragDestinationInfo &dragInfo);
		void				processTouchInfo(const TouchInfo &touchInfo);
		void				processTouchInfoCallback(const TouchInfo &touchInfo);

		void				buildTransform() const;
		void				buildGlobalTransform() const;
		virtual void		drawLocalClient();
		virtual void		drawLocalServer();
		bool				hasDoubleTap() const;
		bool				hasTap() const;
		bool				hasTapInfo() const;
		void				updateCheckBounds() const;
		bool				checkBounds() const;

		/// Once the sprite has passed the getHit() sprite bounds, this is a second
		/// stage that allows the sprite itself to determine if the point is interior,
		/// in the case that the sprite has transparency or other special rules.
		virtual bool		getInnerHit(const ci::vec3&) const;

		virtual void		doSetPosition(const ci::vec3&);
		virtual void		doSetScale(const ci::vec3&);
		virtual void		doSetRotation(const ci::vec3&);
		void				doPropagateVisibilityChange(bool before, bool after);

		virtual void		onCenterChanged(){}
		virtual void		onPositionChanged(){}
		virtual void		onScaleChanged(){}
		virtual void		onSizeChanged(){}
		virtual void		onRotationChanged(){}
		virtual void		onChildAdded(Sprite& child){}
		virtual void		onChildRemoved(Sprite& child){}
		virtual void		onParentSet(){}

		/// If this or any parent's visibility has changed
		virtual void		onAppearanceChanged(bool visible){}

		/// Override this to build mRenderBatch when you need to.
		/// Recommend to look at this implementation before implementing your own
		virtual void		buildRenderBatch();
		/// This is called when it's time to actually build the render batch
		/// Not called if the sprite is not visible or the sprite is in multi-shader pass mode
		/// Recommend re-using mRenderBatch if possible by calling replaceVboMesh on mRenderBatch
		virtual void		onBuildRenderBatch();	

		/// Always access the bounds via this, which will build them if necessary
		const ci::Rectf&	getClippingBounds(ds::ui::Sprite* clippingParent = nullptr);
		void				computeClippingBounds(ds::ui::Sprite* clippingParent = nullptr);

		void				setSpriteId(const ds::sprite_id_t&);
		/// Helper utility to set a flag
		void				setFlag(const int newBit, const bool on, const DirtyState&, int& oldFlags);
		bool				getFlag(const int bit, const int flags) const;

		virtual void		markAsDirty(const DirtyState&);
		/// Special function that marks all children as dirty, without sending anything up the hierarchy.
		virtual void		markChildrenAsDirty(const DirtyState&);
		virtual void		writeAttributesTo(ds::DataBuffer&);
		/// Used during client mode, to let clients get info back to the server. Use the
		/// engine_io.defs::ScopedClientAtts at the top of the function to do all the boilerplate.
		virtual void		writeClientAttributesTo(ds::DataBuffer&){};
		virtual void		readClientAttributeFrom(const char attributeId, ds::DataBuffer&){}
		/// Read a single attribute
		virtual void		readAttributeFrom(const char attributeId, ds::DataBuffer&){}

		void				setUseShaderTexture(bool flag);
		

		void				sendSpriteToFront(Sprite &sprite);
		void				sendSpriteToBack(Sprite &sprite);

		void				setPerspective(const bool);

		mutable bool			mBoundsNeedChecking;
		mutable bool			mInBounds;


		SpriteEngine&			mEngine;
		/// The ID must always be assigned through setSpriteId(), which has some
		/// behaviour associated with the ID changing.
		ds::sprite_id_t			mId;
		ci::Color8u				mUniqueColor;

		float					mWidth,
								mHeight,
								mDepth;

		mutable ci::mat4		mTransformation;
		mutable ci::mat4		mInverseTransform;
		mutable bool			mUpdateTransform;

		int						mSpriteFlags;
		ci::vec3				mPosition,
								mCenter,
								mScale,
								mRotation;
		bool					mRotationOrderZYX;
		float					mOpacity;
		ci::Color				mColor;
		ci::Rectf				mClippingBounds;
		bool					mClippingBoundsDirty;

		bool					mNeedsBatchUpdate;
		ci::gl::BatchRef		mRenderBatch;
		SpriteShader			mSpriteShader;

		ci::gl::TextureRef		mShaderTexture;
		ci::gl::FboRef			mOutputFbo;
		ci::gl::Fbo::Format		mFboFormat;
		bool					mIsRenderFinalToTexture;

		ci::vec4				mShaderExtraData;

		mutable ci::mat4		mGlobalTransform;
		mutable ci::mat4		mInverseGlobalTransform;

		ds::UserData			mUserData;

		Sprite*					mParent;
		std::vector<Sprite *>	mChildren;
		/// A cache for when I need to sort my children. This could be
		/// a lot more efficient, only running the sort when Z changes.
		std::vector<Sprite*>	mSortedTmp;

		/// Class-unique key for this type.  Subclasses can replace.
		char					mBlobType;
		DirtyState				mDirty;

		std::function<void(Sprite *, const TouchInfo &)> mProcessTouchInfoCallback;
		std::function<void(Sprite *, const ci::vec3 &)> mSwipeCallback;
		std::function<bool(Sprite *, const TapInfo &)> mTapInfoCallback;
		std::function<void(Sprite *, const ci::vec3 &)> mTapCallback;
		std::function<void(Sprite *, const ci::vec3 &)> mDoubleTapCallback;
		std::function<void(Sprite *, const DragDestinationInfo &)> mDragDestinationCallback;
		std::function<bool(const ci::vec3&)> mInnerHitFunction;

		bool				mMultiTouchEnabled;
		BitMask				mMultiTouchConstraints;
		bool				mTouchScaleSizeMode;

		/// All touch processing happens in the process touch class
		TouchProcess		mTouchProcess;

		bool				mCheckBounds;
		Sprite*				mDragDestination;
		IdleTimer			mIdleTimer;
		bool				mUseDepthBuffer;
		float				mCornerRadius;
		/// For clients that do their own drawing -- this is the current parent * me opacity.
		/// Essentially anyone who sets alpha in drawLocalClient should probably use this value.
		/// \see Sprite::getDrawOpacity()
		float				mDrawOpacity;

		/// Transport uniform data to the shader
		ds::gl::Uniform		mUniform;

	private:
		/// Utility to reorder the sprites
		void				setSpriteOrder(const std::vector<sprite_id_t>&);

		friend class ds::Engine;
		friend class ds::EngineRoot;
		/// Disable copy constructor; sprites are managed by their parent and
		/// must be allocated
		Sprite(const Sprite&);
		/// Internal constructor just for the Engine, used to create the root sprite,
		/// which always exists and is identical across all architectures.
		Sprite(SpriteEngine&, const ds::sprite_id_t id, const bool perspective = false);

		void				init(const ds::sprite_id_t);
		void				readAttributesFrom(ds::DataBuffer&);

		void				dimensionalStateChanged();
		/// Applies to all children, too.
		void				markClippingDirty();
		/// Store all children in mSortedTmp by z order.
		/// XXX Need to optimize this so only built when needed.
		void				makeSortedChildren();
		/// calls removeParent then addChild to parent.
		/// setParent was previously public, but calling it by itself can cause an infinite loop
		/// Use addChild() from outside sprite.cpp
		void				setParent(Sprite *parent);

		ci::gl::TextureRef	mRenderTarget;
		BlendMode			mBlendMode;

		//set flag for determining whether to use orthoganol or perspective.
		/// this flag is only set on the root perspective sprite.
		bool				mPerspective;

		//set by sprite constructors. doesn't need to be passed through.
		bool				mUseShaderTexture;

		ci::ColorA			mServerColor;
		/// This to make onSizeChanged() more efficient -- it can get
		/// triggered as a result of position changes, which shouldn't affect it.
		float				mLastWidth, mLastHeight, mLastDepth;

		/// Total hack needed in certain cases where you're using a perspective camera.
		/// This is used by the picking to let the touch system know that the sprite
		/// (position/dimensions) are in the screen coordinate space.
		bool				mIsInScreenCoordsHack;

		/// Store a CueRef from the cinder timeline to clear the callAfterDelay() function
		/// Cleared automatically on destruction
		ci::CueRef			mDelayedCallCueRef;

		/// For debugging, and in a super-duper pinch, in production. 
		std::wstring		mSpriteName;

		///if this sprite was an interface root (or <xml> root) then this will hold the settings; null otherwise;
		ds::cfg::Settings*	mSettings = nullptr;

	public:
#ifdef _DEBUG
		/// Debugging aids to write out my state. write() calls writeState
		/// on me and all my children.
		void				write(std::ostream&, const size_t tab) const;
		virtual void		writeState(std::ostream&, const size_t tab) const;
#endif

		static void			installAsServer(ds::BlobRegistry&);
		static void			installAsClient(ds::BlobRegistry&);

		template <typename T>
		static void			handleBlobFromServer(ds::BlobReader&);
		static void			handleBlobFromClient(ds::BlobReader&);
	};

	template <typename T, typename... Args>
	T& Sprite::make(SpriteEngine& e, Sprite* parent, Args... args)
	{
		T*                    s = new T(e, args...);
		if(!s) throw std::runtime_error("Can't create sprite");
		if(parent) parent->addChild(*s);
		return *s;
	}

	template <typename T>
	T& Sprite::makeAlloc(const std::function<T*(void)>& allocFn, Sprite* parent)
	{
		T*                    s = allocFn();
		if(!s) throw std::runtime_error("Can't create sprite");
		if(parent) parent->addChild(*s);
		return *s;
	}


	/** Handle basic communication from the server. This creates sprites that don't exist, or updates ones that already exist.
		Note: Cannot use Logs here, as including the logger doesn't compile.
		Also Note: Due to the way VS handles templatization, this cannot be moved to the cpp file. 
		Also also Note: It would be great if this weren't in the Sprite header! */
	template <typename T>
	void Sprite::handleBlobFromServer(ds::BlobReader& r)	{
		ds::DataBuffer&       buf(r.mDataBuffer);
		char attributey = buf.read<char>();
		if(attributey != SPRITE_ID_ATTRIBUTE){
			std::cout << "ERROR: Handle blob from server, attribute " << (int)attributey << " is not the sprite attribute! This likely means you haven't installed your sprite type correctly." << attributey << std::endl;
			return;
		}
		ds::sprite_id_t       id = buf.read<ds::sprite_id_t>();
		Sprite*               s = r.mSpriteEngine.findSprite(id);
		if(s) {
			s->readFrom(r);
		} else {
			s = new T(r.mSpriteEngine);
			if(!s){
				std::cout << "ERROR: Failed to create sprite with id " << id << " Ensure your sprite has a constructor that only takes a SpriteEngine as a parameter." << std::endl;
				return;
			}
			s->setSpriteId(id);
			s->readFrom(r);
			/// If it didn't get assigned to a parent, something is wrong,
			/// and it would disappear forever from memory management if I didn't
			/// clean up here.
			if(!s->mParent) {
				std::cout << "ERROR: No parent created for sprite id: " << id << " Be sure you add your sprite to a parent directly after construction." << std::endl;
				//assert(false);
				delete s;
			}
		}
	}

} // namespace ui
} // namespace ds

#endif // DS_UI_SPRITE_SPRITE_H_
