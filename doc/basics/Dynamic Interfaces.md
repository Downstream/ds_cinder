Dynamic Interfaces
======================

```XML
<interface>
	<sprite
		name="sprite_name"
		size="400, 200"
		position="100, 200, 300"
		color="#ff0000"
		enable="true"
		multitouch="all" >
		<text name="child_text" font="sample:config" resize_limit="300" />
		<image name="child_image" filename="RELATIVE PATH TO IMG" />
	</sprite>
</interface>
```

Creating a Sprite and loading XML
-------------------------------
The tag value is the sprite type. For instance, <sprite/> creates a blank ds::ui::Sprite. Sprites are created as parents/children based on the XML hierarchy. Positions work relative to the parent (like normal).

Loading an interface XML to your class:

```cpp
    std::map<std::string, ds::ui::Sprite*>    spriteMap;
    ds::ui::XmlImporter::loadXMLto(this, ds::Environment::expand("%APP%/data/layout/layout_view.xml"), spriteMap);
```

Adds the hierarchy in the interface as a child of "this".
Use the spriteMap to look up specific sprites by string name. This lets you apply a data model or deal with touch callbacks, etc. Avoid setting the layout in c++ land, as that can confuse where the layout is managed.

You can also preload the xml by keeping a ds::ui::XmlImporter::XmlPreloadData generated from ds::ui::XmlImporter::preloadXml() and pass that as the second parameter of loadXMLto(). This avoids a run-time hit to the disk, though all the parameter mapping still happens in real time.

Parameter Types
------------------------------
* **Vector**: Always 3d, but the second 2 parameters are optional. Vectors are comma delimited, with a space after the comma. Example: position="x, y, z", so position="100, 200, 300". Also valid: position="100" (which map to ci::Vec3f(100.0f, 0.0f, 0.0f)) or position="100, 200" (which is ci::Vec3f(100.0f, 200.0f, 0.0f))
* **Color**: Format: #AARRGGBB or #RRGGBB, or a named color specified by a call to mEngine.getColors().install().
* **Boolean**: These values will map to true: true, TRUE, yes, YES, on, ON. Everything else is false.
* **Blend**: Valid values: normal, multiply, screen, add, subtract, lighten, darken. Default = normal.
* **Float**: Specify the number with the decimal. The trailing "f" is unneccasary. For instance, opacity="0.5" not opacity="0.5f"

Sprite Types
--------------------------
Xml Element Name                                                      | DsCinder Class Name
---                                                                   | ---
[sprite](#Sprite-Parameters)                                          | ds::ui::Sprite
[image](#Image-Parameters)                                            | ds::ui::Image
[image_with_thumbnail](#Image-Parameters)                             | ds::ui::ImageWithThumbnail
[image_button](#Image-Button-Parameters)                              | ds::ui::ImageButton
[sprite_button](#Button-Parameters)                                   | ds::ui::SpriteButton
[layout_button](#Button-Parameters)                                   | ds::ui::LayoutButton
[text](#Text-Parameters)                                              | ds::ui::Text
[gradient](#Gradient-Parameters)                                      | ds::ui::GradientSprite
[layout](#Layout-Parameters)                                          | ds::ui::LayoutSprite
[persp_layout](#Perspective-Layout-Parameters)                        | ds::ui::PerspectiveLayout
[circle](#Circle-Parameters)                                          | ds::ui::Circle
[border](#Border-Parameters)                                          | ds::ui::Border
[dashed_line](#Dashed-Line-Parameters)                                | ds::ui::DashedLine
[donut_arc](#Donut-Arc-Parameters)                                    | ds::ui::DonutArc
[scroll_list{_(vertical\|horizontal)](#Scroll-List-Parameters)        | ds::ui::ScrollList
[scroll_area](#Scroll-Area-Parameters)                                | ds::ui::ScrollArea
[smart_scroll_list](#Smart-Scroll-List-Parameters)                    | ds::ui::SmartScrollList
[centered_scroll_area](#Centered_scroll_area-Parameters)              | ds::ui::CenteredScrollArea
[control_check_box](#Control-Check-Box-Parameters)                    | ds::ui::ControlCheckBox
[control_slider{_(horizontal\|vertical)}](#Control-Slider-Parameters) | ds::ui::ControlSlider
[scroll_bar](#Scroll-Bar-Parameters)                                  | ds::ui::ScrollBar
[entry_field](#EntryField-and-SoftKeyboard-Parameters)                | ds::ui::EntryField
[soft_keyboard](#EntryField-and-SoftKeyboard-Parameters)              | ds::ui::SoftKeyboard
[web](#Web-Parameters)                                                | ds::ui::Web (if the cef web project is included)
[pdf](#Pdf-Parameters)                                                | ds::ui::Pdf (if the pdf project is included)
[video](#Video-Parameters)                                            | ds::ui::GstVideo (if the gstreamer-1.0 project is included)
[media_player](#Media-Player-Parameters)                              | ds::ui::MediaPlayer (if the viewers project is included)
[media_slideshow](#Media-Slideshow-Parameters)                        | ds::ui::MediaSlideshow (if the viewers project is included)
[xml](#XML-Parameters)                                                | Load another xml interface. See details below.
`[custom]`                                                            | Calls a custom callback function with a string for the type. Requires you instantiate the sprite type yourself.

Variables
----------------------

Add dynamic variables to any sprite parameter. Variables are pulled from app_settings.xml automagically or from a `<settings>` block in the xml. In addition, there are a few default variables for common engine properties. Specify a variable in an xml layout by pre-pending it with "$_".

```xml
<interface>
	<layout name="example"
		size="$_world_size"
		pad_all="$_padding"
		animate_on="$_default:anim"
		animate_off="$_default:anim"
		/>
</interface>
```

Variables are replaced first before expressions (see below), and parsed recursively. For example, in app_settings.xml you can specify a variable that references another variable:

```xml
	<setting name="default:delay" value="0.1" />
	<setting name="default:anim" value="fade; slide:$_world_width, 0; ease:outQuint; delay:$_default:delay" />
```

* Add variables in c++ using ds::ui::XmlImporter::addVariable()
* Variables are all treated as strings, so you can replace any sprite property, such as font name, animation script, alignment, etc
* If a variable replacement isn't found, the original value is used and a warning is logged

### Default Variables

* **world_width**: The equivalent of mEngine.getWorldWidth()
* **world_height**: The equivalent of mEngine.getWorldHeight()
* **world_size**: The equivalent of "mEngine.getWorldWidth(), mEngine.getWorldHeight()"
* **anim_dur**: The equivalent of mEngine.getAnimDur()

### Layout `<settings>` block

```xml
<interface>
	<settings>
		<setting name="main_color" value="#ffaeaeff"/>
		<setting name="secondary_color" value="#ffffffff"/>
	</settings>
	<layout name="example"
		size="$_world_size"
		pad_all="$_padding"
		animate_on="$_default:anim"
		animate_off="$_default:anim"
		>
		<sprite size="100, 100" color="$_main_color"/>
		<sprite size="100, 100" color="$_secondary_color"/>
		<sprite size="100, 100" color="$_secondary_color"/>
</interface>
```

A layout file can have a settings block. the settings in that block exist only in that file and do not propagate back to the main app settings. If a layout setting and an app setting have the same name, the layout settings takes priority.

`<xml>` sprite type can also have a child <settings> block. See [xml](#XML-Parameters) sprite for more details.

Expressions
--------------------

Layouts support math expressions that are parsed when the layout is loaded. Expressions are parsed after variables, and before properties are assigned. There are two ways to indicate that a value is an expression: By starting a line with #expr or by wrapping an expression in #expr{}.

```xml
<interface>
	<layout name="example"
		pad_all="#expr 123.45 + 67.89 - sin(pi)"
		size="#expr{50.0 * 2.0}, #expr{$_world_height / 2.0}"
		/>
</interface>
```

Syntax Rules:

* Expression parsing syntax: http://warp.povusers.org/FunctionParser/fparser.html#usage
* Note that we don't support expression variables (such as sqrt(x * x + y * y)), the expression must evaluate to a single value
* The starting syntax must be "#expr{" with no spaces
* If you start a line with #expr (with no brackets), the entire rest of the line is evaluated as an expression
* pi is automatically defined

`target` parameter && `<override>` elements
--------------------------
**target** adding a `target="`*`value`*`"` parameter to an element will cause the importer to attempt to match the value to the value in `xml_importer:target` engine setting. 
If the values match, then the tag is processed as normal. **If the tag does not match then the element will be ignored. as if it was not in the file. This includes any children**
  
The `<override>` element can be added as a child to another element and it will add/replace the properties on that element. 
Note that it cannot remove a property. This is only really useful with the `target` property set on the `<override>` element, allowing for Layouts to morph based on different hardware (or other conditions).

Example:
```xml
engine.xml
<?xml version="1.0" encoding="utf-8"?>
<settings>
	...
	<setting name="xml_importer:target" value="landscape, 3x3" type="string" default="true"/>
	...
</settings>

myLayout.xml
<interface>
	<layout name="example_shown" layout_type="vert" target="landscape">
		<override layout_type="vert_wrap" target="3x3"/>
		<override layout_size_mode="flex" target="5x4"/>
		... 
	</layout>
	<layout name="example_not_shown" layout_type="horiz" target="portrait">
		<override layout_type="vert" target="3x3"/>
		... 
	</layout>
</interface>
```

in this example the "example_shown" layout is included and it's layout_type is overridden, but its layout_size_mode is not. the "example_not_shown" is skipped entirely.

Events
--------------------------------------------------------

Using the **on_tap_event** and **on_click_event** sprite parameters, you can trigger events for other parts of the app directly from a layout xml. You'll need to do a couple things to get this to work.

* **Register events by name:** Create a ds::RegisteredEvent like normal by extending the RegisteredEvent class. You'll need to provide a creation function to the event registry so the event can be created dynamically when called. To do this, add a line like the below on app instantiation (in your root app class) for each event.

        ds::event::Registry::get().addEventCreator(RequestCloseAllEvent::NAME(), [this]()->ds::Event*{return new RequestCloseAllEvent(); });

* **Built-in event parameters:** The event will automatically have the sprite that triggered the event applied to the ds::Event::mSpriteOriginator property, and mEventOrigin paramter will be the global position of the tap or click of the interaction.
* **Custom event parameters:** You can apply certain parameters to each event dispatched from a layout xml: Data, Id, and UserSize. Pass these properties to the event like so:

        RequestCustomEvent; data:myCustomStringData; id:1234; user_size:400, 300, 1;
	data can also be set from the sprite's userData cache by using the user_data parameter and a key to a string in the userData collection:

         RequestCustomEvent; user_data:keyInUserData; id:1234; user_size:400, 300, 1;
	This can be combined with the model property's ability to set model data in the userData cache. See the ContentModel and SmartLayout section for details.

* **Multiple events:** Send multiple events from the same button press by wrapping each event in brackets and separating them by commas. Do not use spaces between events. Example:

        on_tap_event="{RequestCloseAllEvent},{RequestMediaOpenEvent; data:%APP%/data/temp/test.pdf; user_size:900},{RequestLayoutEvent; user_data:data_key}"

* **Handling events:** The app will need to handle the events like normal using an event client and handling the app event. The parameters described above are automatically applied to the Event by the xml importer and you can access them through the event:

```cpp
void ViewerController::onAppEvent(const ds::Event& in_e){
	if(in_e.mWhat == RequestMediaOpenEvent::WHAT()){
		std::string fileName = in_e.mUserStringData;
		//Do something with the filename and open media
	}
}
```
	

Sprite Parameters
------------------------------------------------------------

* **width**: Specify a float for the width of the sprite. width="100"
* **height**: Specify a float for the height of the sprite. height="100"
* **depth**: Specify a float for the depth of the sprite. depth="100"
* **size**: Specify a 2d vector for the width and height (depth not supported). size="400, 100"
* **color**: Sets the color of the sprite and makes it non-transparent (so it renders). Format: #AARRGGBB or #RRGGBB, AARRGGBB, or a named color specified by a call to mEngine.getColors().install(). For instance: color="#FF0033bb" or color="black".
* **opacity**: The opacity as a float from 0.0 to 1.0. Overrides any alpha specified in color. opacity="0.5"
* **position**: Position in pixels. position="100, 200, 300" or position="0, 50"
* **rotation**: 3d vector of the degrees (not radians) of rotation in x, y, z. rotation="100, 200, 300" or rotation="0, 0, 90"
* **scale**: 3d vector of the scale in x, y, z. Scale is from 0.0 (nothing) to 1.0 (100%), not bounded. scale="1, 1, 1" or scale="0.5, 0.5, 1"
* **center**: Center is where the anchor of the sprite is calculated from (for scaling and rotation) and is a percentage from 0.0 (top/left) to 1.0 (bottom/right), not bounded. center="0, 0.5, 1"
* **clipping**: Boolean of whether to clip it's children.
* **blend_mode**: Valid values: normal, multiply, screen, add, subtract, lighten, darken, premultiply, fbo_in, fbo_out, transparent_black. Default = normal.
* **enable**: Boolean of whether to handle touch input or not.
* **multitouch**: String of multitouch mode. Possible values:
    * info = MULTITOUCH_INFO_ONLY
    * all = MULTITOUCH_NO_CONSTRAINTS
    * pos = MULTITOUCH_CAN_POSITION
    * scale = MULTITOUCH_CAN_SCALE
    * pos_x = MULTITOUCH_CAN_POSITION_X
    * pos_y = MULTITOUCH_CAN_POSITION_Y
    * pos_scale = MULTITOUCH_CAN_POSITION | MULTITOUCH_CAN_SCALE
    * pos_rotate = MULTITOUCH_CAN_POSITION | MULTITOUCH_CAN_ROTATE
    * rotate = MULTITOUCH_CAN_ROTATE
* **transparent**: A boolean of wheather the sprite should draw or not. This works only locally. For base sprite, this will draw a rectangle (if the sprite has a size)
* **visible**: A boolean of the visibilty flag. Doesn't affect the draw status, but does turn off this sprite and any children if hidden. True = show(), false = hide()
* **animate_on**: Supply a script to run when tweenAnimateOn() is called on this sprite. See the animation section for details.
* **animate_off**: Supply a script to run when tweenAnimateOff() is called on this sprite. See the animation section for details.
* **corner_radius**: A float the changes the corner radius. Only applies to some sprite types like Sprite and Border. Many types ignore this setting. Default=0.0.
* **on_tap_event** and **on_click_event**: Dispatches one or more events from tap or button click. on_tap_event uses the built-in tap callback for any sprite, on_click_event only applies to ImageButton and SpriteButton. See the events section for more details.
* **shader**: A path to a shader frag and vert. For instance, supply %APP%/data/shaders/alpha_video if you have alpha_video.frag and alpha_video.vert in that location
* **model**: Apply ContentModelRef properties to sprite properties. See the Content Model section below.
* **each_model**: Apply ContentModelRef properties to sprite properties. See the Content Model section below.
* **each_model_limit**: Only create child sprites for the first N ContentModel children.
* Parameters for sprites within layouts:
	* **t_pad**: Padding on the top part of this sprite in the layout
	* **b_pad**: Padding on the bottom part of this sprite in the layout
	* **l_pad**: Padding on the left part of this sprite in the layout
	* **r_pad**: Padding on the right part of this sprite in the layout
	* **padding**: Set padding for all sides of sprite (order: L, T, R, B) ex: padding="20, 30, 20, 30"
	* **pad_all**: Set padding for all sides of sprite together. ex: pad_all="60"
	* **layout_fudge**: An offset in pixels for the sprite during the layout. Doesn't get added to the overall layout positioning, but allows you to fudge the position a bit if something isn't rendering just right
	* **layout_size**: Desired size for the "fixed" layout size mode. This is separate from the sprite size, since some sprites (like Image) will be able to calculate size only after the thing is loaded, and this will try to scale the image (or other thing) up to fit in this size, letterboxed.
	* **layout_size_mode**: The method to calculate the size of this sprite during layout
		1. fixed: (Default) The size of this sprite is not modified during the layout
		2. flex: Sprite will be made wide enough (for V layouts) or tall enough (for H layouts) during layout. Images will be sized proportionally to fit the size. Text fields in Vertical Layouts will be resized to the width of the layout (minus padding), and the height calculated (plus padding). Text fields in Horizontal layouts will be resized to their existing resize width limit and constrained to the height of the layout (including padding).
		3. stretch: Sprite and all stretch siblings will be sized to share the remaining space available in the layout, separated by spacing, and respecting padding. Stretch sprites will be given no space at all if the layout shrinks to its children along its layout axis.
		4. fill: Sprite will be moved to the parent layout origin and set to the size of the layout, respecting padding.
	* **layout_v_align**: For sprites added as a child of a horizontal layout, the sprite will be aligned appropriately.
		1. top: (Default) Aligns the sprite to the top of the layout.
		2. middle: Vertically centers the sprite in the layout (only works on Fixed size sprites)
		3. bottom: Aligns the sprite to the bottom of the layout (only works on Fixed size sprites)
	* **layout_h_align**: For sprites added as a child of a vertical layout, the sprite will be aligned appropriately.
		1. left: (Default) Aligns the sprite to the left of the layout.
		2. center: Horizontally centers the sprite in the layout (only works on Fixed size sprites)
		3. right: Aligns the sprite to the right of the layout (only works on Fixed size sprites)
	* **layout_fixed_aspect**: Tells the sprite's parent layout if this sprite should be resized proportionally or not. Some sprites are fixed aspect ratio by default: Image, PDF, Video, ImageButton, Circle. This parameter is used for layout_size_mode of Flex, Stretch and Fill. If layout_fixed_aspect is true, the sprite will be fit inside the destination area, with letterboxing (unless it's a stretch size mode in a SizeType layout, then it won't letterbox). For layout_fixed_aspect to work, the sprite needs to have w & h != 0.0.

Layout Parameters
------------------------------------------------------------

* **layout_type**: For a LayoutSprite only (has no effect on children).
    1. vert: (Default) lays out all children in a vertical (top to bottom) flow.
    2. horiz: Lays out children horizontally (left to right)
    3. vert_wrap: Lays out children horizontally (left to right), and wrapping when sprites overflow
	   its height.
    4. horiz_wrap: Lays out children horizontally (left to right), and wrapping when sprites overflow
	   its width.
    5. size: Only adjusts the size of children, but does not modify their position. Useful to constrain some elements to a size, but keep them at a particular position
    4. <any other value>: None, does no layout of children.
* **layout_spacing**: For LayoutSprite only, a float that sets the spacing between all elements. layout_spacing="10.0"
* **shrink_to_children**: For LayoutSprite's only, determines how the sprite adjusts to its children.
    1. none, false, or no value: None, will not alter the size of this layout sprite after laying out the children
    2. width: Adjusts the width of this sprite to its children (for vertical, the widest child, for horiz, the total width of the children, plus spacing)
    3. height: Adjusts the height of this sprite to its children (for vertical, the total height of the children, for horiz, the tallest child)
    4. both or true: Both width and height
* **skip_hidden_children**: For LayoutSprite, when true, will ignore hidden children when calculating layouts, including padding. Default=false. Note that this is hide() on the child sprite and not setTransparent(false)
* **overall_alignment**: For LayoutSprite, allows you to align the contents to Top, Left, Center, Right, Middle or Bottom inside the layout

Perspective Layout Parameters
------------------------------------------------------------
* **persp_fov**: Float field of view in degrees. Default is 30
* **persp_auto_clip**: Bool enable or disable automatically setting the near / far clip distance
* **persp_auto_clip_range**: Float for how far around the eye distance to calculate the clip distance if auto clip is on
* **persp_near_clip**: Float for the near clipping distance, only if auto clip is disabled
* **persp_far_clip**: Float for the far clipping distance, only if auto clip is disabled
* **persp_enabled**: Bool to enable or disable rendering in 3d at all

Text Parameters
------------------------------------------------------------
* **text**: Set the content show on the screen. text="Hello World"
* **text_update**: Set the text content show on the screen, but only if there is content. See ContentModel section below. text="" // does nothing, text="Hey" // set's the text
* **text_uppercase**: Set the content show on the screen. text_uppercase="Hello World" shows up as HELLO WORLD
* **text_lowercase**: Set the content show on the screen. text_uppercase="Hello World" shows up as hello world
* Date/Time Handlers (Note: parse & format options must come before text_utc in the layout)
	* **text_utc_parse**: Date format to parse (default: "%H:%M:%S %d-%m-%Y")
	* **text_utc_format**: Output date format to convert to (default: "%H:%M:%S %d-%m-%Y")
	* **text_utc**: The time/date string OR "now" (example: "11:42:00 14-02-2018")
* **markdown**: Parses the string into markdown then applies it as text. markdown="Hello World, but including **markdown**"
* **text_style**: The text style name or settings string. Set in settings/styles.xml. The text style sets the font name, size, leading and color. text_style="sample:config". Or you can use the full style syntax of text_style="font:Arial; size:20; leading:1.2; letter_spacing:5.0; align:center; fit_sizes:12, 24, 36, 40"
* **text_allow_markup**: Sets if the text entered should parse pango markup (e.g. <span weight='bold'>bold text</span>). Default: true
* **font**: The text style name. Set in settings/styles.xml. The text config sets the font name, size, leading and color. font="sample:config"
* **font_name**: The name of the font registered in the app. **Note:** It's recommended you use the font setting above (a whole config) OR font_name and font_size, and not mix the two.
* **font_size**: Replace the original font size of Text sprites. font_size="20"
* **font_leading**: The multiplier of font_size to use for line height (only when text consists of multiple lines)
* **font_letter_spacing**: Set the letter spacing in pts (0.0 = normal, 1.0 = 1pt extra letter spacing, etc.)
* **text_ellipses**: Sets the ellipses mode of the text. Possible values: none, start, middle, end. Default=none. Use this with resize_limit to change what happens if the text gets too long. 
* **resize_limit**: If you set only the width value, the layout will continue for all the text in the sprite, and the height will be calculated from that. resize_limit="400" or resize_limit="400, 500" Note: the text_ellipses value must be something other than "none" for the height limit to take effect. Set the y value to a negative number to show that number of lines. For instance, -2 will show two lines of text per paragraph before adding an ellipses.
* **text_align**: Set the alignment of the Text Sprite
    1. "left": The default, normal text
    2. "center": Center-aligns the layout.
    3. "right": Align rows of text to the right side of the resize_limit
* **shrink_to_bounds**: If text has 'resize_limit' set, this will set the sprite size to the size of the text texture, rather than the full resize_limit.
* **text_model_format**: Allows combining of plain text, data from a content model, and text
	post-processing functions. Requires being in a smart layout, and having setContentModel called.
	See [ContentModel & SmartLayout](#ContentModel-&-SmartLayout) below for more details.
* **preserve_span_colors**: If the text contains a <span> tag with any color info, you **must** turn this on to see it. Otherwise the text will only render as the color from this sprite. This is due to pre-multiplication of the alpha channel, which we ignore for better font rendering, particularly for thinner fonts.
* **fit_to_limit**: The font size is calculated so that all of of the text fits in the resize_limit rectangle with the largest possible font. (true or false)
* **fit_font_size_range**: a 2 element vector of font sizes. If fit_to_limit is true, the calculated font will not be smaller than the first element and will not be larger than the second element. Example: fit_font_size_range="12, 72"
* **fit_max_font_size**: If fit_to_limit is true, the calculated font will not be larger than this value. the same as the second element of **fit_font_size_range**
* **fit_min_font_size**: If fit_to_limit is true, the calculated font will not be smaller than this value. the same as the first element of **fit_font_size_range**
* **fit_font_sizes**: a list of font sizes. If fit_to_limit is true, the calculated font will come from this list. Fit font maxmin is respected so size above max or below min will not be used.
* **text_wrap**: Controls how the text will break when a line is to long.
    1. "off" or "false" turns of text wrapping.
	1. "word" break on word boundries.
	1. "char" break on any character.
	1. "wordchar" try to break on word boundries and then on a character is that doesn't work.

Image Parameters
-------------------------
* **filename** OR **src**: File path RELATIVE to XML. For instance: src="../data/images/refresh_btn.png". Image loading flags can be added to the property name seperated by underscores (_). For example filename_cache with cause the ds::ui::Image::IMG_CACHE_F flag to be passed to the loade, while filename_mipmap_cache will cause both ds::ui::Image::IMG_CACHE_F and ds::ui::Image::IMG_MIPMAP to be passed.
	* **_cache** OR **_c**: include the ds::ui::Image::IMG_CACHE_F flag, which permanently caches the image
	* **_mipmap** OR **_m**: include the ds::ui::Image::IMG_MIPMAP flag, which generates mipmaps for the given image.
	* **_preload** OR **_p**: include the ds::ui::Image::IMG_PRELOAD flag, which preloads the image.
	* **_skipmeta** OR **_s**: include the ds::ui::Image::IMG_SKIP_METADATA flag, which skips metadata loading.

These flags can also be applied the the resource property when working with a model:  
```xml
		model="resource_mipmap:this->img_resource"
```
	
* **circle_crop**: Boolean. If true, will crop image content outside of an ellipse centered within the bounding box.
* **auto_circle_crop**: Boolean. If true, centers the crop in the middle of the image and always makes it a circle, persists through image file changes.

Image Button Parameters
-------------------------
* **filename**: Will apply the same image to down and up state. Will override down_image and up_image, and vice-versa.
* **down_image**: The image to display when touch is happening on this button. See filename for explanation. down_image="../data/images/refresh_btn.png"
* **up_image**: The image to display when touch is happening on this button. See filename for explanation. up_image="../data/images/refresh_btn.png"
* **btn_touch_padding**: How much space to add to the sprite on every side of the image. btn_touch_padding="40"
* **down_image_color**: Applies a color to the down image, to make creating responsive buttons easy from a single image.
* **up_image_color**: Applies a color to the up (normal) image, to make creating responsive buttons easy from a single image.


Button Parameters
----------------------------
**attach_state**: Add this to a sprite that's a child of a sprite button or a layout button. Valid values: "normal" for the unpressed state and "high" for the pressed state. For example:

```xml
<interface>
	<sprite_button
		name="sample_button"
		size="400, 200" >
		<text  name="child_text" font="sample:config" text="The Button" />
		<image name="child_image" filename="%APP%/data/images/icons/up_icon.png" attach_state="normal" />
		<image name="child_image" filename="%APP%/data/images/icons/down_icon.png" attach_state="high" />
	</sprite>
</interface>
```

Gradient Parameters
---------------------------
* **color_top**: A color value to set the TL and TR colors of the gradient to. colorTop="#ffffff"
* **color_bot**: A color value to set the BL and BR colors of the gradient to. colorBot="#ffffff"
* **color_left**: A color value to set the TL and BL colors of the gradient to. colorLeft="#ffffff"
* **color_right**: A color value to set the TR and BR colors of the gradient to. colorRight="#ffffff"
* **gradient_colors**: Set all four colors for the gradient, specified clockwise from TL "[colorTL], [colorTR], [colorBR], [colorBL]". Example: gradientColors="#ff0000, #000000, #00ff00, #0000ff"

Circle Parameters
---------------------------
* **filled**: Boolean, whether to draw just the outline or fill in the circle
* **radius**: Float, the radius of the circle to draw
* **line_width**: Float, the width for non-filled circle border

Border Parameters
---------------------------
* **border_width**: Float, the width of the border, which has its outer edge at the extent of the sprite

Donut Arc Parameters
---------------------------
* **donut_width**: Float, the distance in pixels from the outside of the sprite to the inside of the donut
* **donut_percent**: Float, the percentage that the donut is filled in

Dashed Line Parameters
---------------------------
* **dash_length**: The length of each dash in a dashed line
* **dash_space_inc**: The distance between each dash in a dashed line

Control Check Box Parameters
---------------------------
* **font**: Sets the font text config of the label (which also adds a true/false label at all, optional)
* **check_box_true_label**: Sets the text string that shows up when the box is checked
* **check_box_false_label**: Sets the text string that shows up when the box is unchecked

Scroll List Parameters
-------------------------------
* **Note:** You'll need to supply the usual callbacks for this to work (for creating items in the list, setting data, etc) OR use smart_scroll_list
* **scroll_list_layout**: Sets the parameters for layout from the format "x, y, z", which translates to setLayoutParams(xStart, yStart, incrementAmount, true);
* **scroll_list_animate**: Sets the animation parameters, from the format "x, y", where x==startDelay and y==deltaDelay on ScrollList::setAnimateOnParams(startDelay, deltaDelay);
* **scroll_fade_colors**: **Also applicable to ScrollArea**. Set the colors of the scroll area, in the format "[colorFull], [colorTransparent]". Example: scroll_fade_colors="ff000000, 00000000" or scroll_fade_colors="44000000, 000000"
* **scroll_fade_size**: Set the size of the fade as a float.
* **scroll_shader_fade**: **Also applicable to ScrollArea**. Uses a shader for fading out the sides instead of putting gradients on top. NOTE: Any Children cannot use blend modes; this scroll area cannot be inside of a clipping sprite; any children with clipping cannot be rotated

Scroll Area Parameters
-------------------------------
* **scroll_area_vert**: Sets the direction parameters, where true==vertical and false==horizontal on ScrollArea::setVertical(bool)
* **scroll_fade_colors**: **Also applicable to ScrollList**. Set the colors of the scroll area, in the format "[colorFull], [colorTransparent]". Example: scroll_fade_colors="ff000000, 00000000" or scroll_fade_colors="44000000, 000000"
* **scroll_fade_size**: **Also applicable to ScrollList**. Set the size of the fade as a float.
* **scroll_shader_fade**: **Also applicable to ScrollList**. Uses a shader for fading out the sides instead of putting gradients on top. NOTE: Any Children cannot use blend modes; this scroll area cannot be inside of a clipping sprite; any children with clipping cannot be rotated

Smart Scroll List Parameters
--------------------------------------
* **Note:** Defaults to a vertical scroll list. Use `smart_scroll_list_horizontal` for horizontal layout.
* **smart_scroll_item_layout**: Sets the layout file for each list item, relative to %APP%/data/layouts/

Scroll Bar Parameters
--------------------------------------
* **sprite_link** Add the name of a ScrollList or ScrollArea to control one of those areas
* **scroll_bar_nub_color**: A color for the current-position indicator
* **scroll_bar_background_color**: A color for the area in the back behind the nub
* **scroll_bar_corner_radius**: Sets the corner radius of both the nub and the background

EntryField and SoftKeyboard Parameters
--------------------------------------
EntryFields and SoftKeyboards need some parameters set for instantiation, so they are set as in the body of the node rather than as attributes. Example:

```xml
<entry_field
	name="search_field"
	sprite_link="primary_keyboard"
	>text_config:keyboard:key:up; cursor_offset:4, -10; cursor_size:1, 40; field_size:500, 40; cursor_color:orange</entry_field>

<soft_keyboard name="primary_keyboard">
type:lowercase; key_scale:1; key_up_color:bright_grey; key_down_color:orange; key_text_offset:-2, -2; key_touch_padding:4</soft_keyboard>
```

**ENTRY FIELD PARAMETERS**
* **sprite_link**: Allows you to link the text entry field with a soft keyboard. Set the value of sprite_link to the name of the soft_keyboard. The keyboard needs to be in the same sprite map as entry field to be linked. Once linked, the keyboard will type it's text into the entry field.
* **text_config**: The text config for the text of the entry field. Default: entry_field:text
* **cursor_size**: How big the blinking cursor is in pixels. Default: 2, 36
* **field_size**: The size of the entry field, which also sets the resize limit of the text in the field. You can also set the size of this sprite to change the field size. Default: 500, 100
* **cursor_offset**: X/Y Pixels to offset the cursor from the end of the text sprite. For fudging the position of the cursor. Default: 2, -5
* **cursor_color**: The color of the blinking cursor. Engine colors are allowed. Default: white
* **blink_rate**: How many seconds to wait between blinks. Total blink time is animate_rate + animate_rate + blink_rate. Default: 0.5
* **animate_rate**: How many seconds to fade the cursor on and off. Default: 0.3.
* **text_offset**: How many pixels to offset the text sprite. Default: 0.0, 0.0
* **search_mode**: If true, will not add returns when the enter button is hit. Default: false
* **password_mode**: If true, will show bullets instead of text. Default: false
* **auto_resize**: If true, sizes the resize limit of the text to the size of the EntryField sprite, otherwise the field_size is assumed to be static. Default: false

**SOFT KEYBOARD PARAMETERS**
* **type**: Determines which kind of keyboard this is. Valid types: standard, lowercase, extended, simplified, pinpad and pincode. Standard has shift abilities and some extended keys. Lowercase is simplified and only has lowercase keys. Simplified only has letters, space bar, and delete keys. Pinpad is like an ATM pin pad with an enter button. Pincode is a number entry keyboard with a back/delete button. Default: standard
* **key_up_text_config**: The text config of the text in the keys when not pressed. Default: keyboard:key:up
* **key_dn_text_config**: The text config of the text in the keys when pressed. Default: keyboard:key:down
* **key_up_color**: The color of the keys when not pressed. Engine colors allowed. Default: white
* **key_down_color**: The color of the keys when pressed. Engine colors allowed. Default: medium grey
* **key_text_offset**: Vector, amount to fudge the offset of the text inside each key. Default: -5.0, -5.0
* **key_touch_padding**: Float amount between each key in pixels. Default: 4.0
* **key_initial_position**: Vector x/y of the start position of the first key when creating the keys. Default: 0, 0
* **key_scale**: The amount the keyboard is scaled as a single float. Default: 1.0
* **graphic_keys**: True uses sprite graphics instead of images for keys. Default: false
* **graphic_type**: Values: border (the default), solid, circular_border, circular_solid
* **graphic_key_size**: Float value for the size of graphic keys
* **graphic_corner_radius**: For non-circular key types, the corner radius
* **graphic_border_width**: For border types, the width of the line on the border
* See soft_keyboard_settings.h for the default text configs and key images.

Web Parameters
-------------------------------
If you have the web project included, you can create web sprites.
* **web_url**: The full url of a site to load.

PDF Parameters
-------------------------------
If you have the pdf project included, you can create pdf sprites.
* **pdf_src**: Relative or absolute path to the pdf. For example: pdf_src="%APP%/data/test/test.pdf" or pdf_src="c:/test.pdf"

Video Parameters
-------------------------------
If you have the video project included, you can create video sprites.
* **video_src**: Relative or absolute path to the video. For example: video_src="%APP%/data/test/test.mp4" or video_src="c:/test.mp4"
* **stream_src**: A pipeline for a live stream source or the URI of a stream. Example: stream_src="rtsp://192.168.1.37:5015/Stream1"
* **video_gl_mode**: Accepts only "true". Will enable openGL elements in GStreamer. This is required to be on when using nvdecode
* **video_nvdecode**: Boolean. if true, will use the nvdec NVidia CUDA video decoder for video playback. See the readme in the video project for details.

Media Player Parameters
-------------------------------
If you have the viewers project included, you can create media players. Media players are a simple way to view Images, PDFs, Videos, and Web sites. Media Players created this way automatically have an embedded interface (to flip through pages or control videos). Media types are deduced by file extension. MediaPlayer sprites need to be enabled or accept some user input for the interface to re-appear.

*Please Note*: The order of attributes matters. The attribute *media_player_src* must come last for the layout to respect the other options.

* **media_player_src**: Relative or absolute path to the media. For example:

        media_player_src="%APP%/data/test/test.mp4" or media_player_src="c:/test.pdf"
* **media_player_auto_start**: Boolean, if true, videos play automatically. If false, they'll play the first frame then stop. Also applies to YouTube links
* **media_player_show_interface**: Boolean, true shows interfaces for pdf, web and video immediately
* **media_player_web_size**: Vector, sets the w/h in pixels of web views
* **media_player_web_start_interactive**: Boolean, enables touching the web view immediately upon creation
* **media_player_video_volume**: Float, sets the volume of videos when they start
* **media_player_video_loop**: Boolean, true, the default, loops the video, false will play the video once and stop
* **media_player_video_reset_on_complete**: Boolean, true, the default, resets the video to 0.0 when the video finishes in non-loop mode
* **media_player_letterbox**: Boolean, true, the default, will letterbox the media inside the size of the media player, false fills (with no cropping by default). Web always fills
* **media_player_standard_click**: Boolean, default is false, true will allow tapping to start/stop videos, advance pdf's, and click into web pages. 
* **media_player_cache_images**: Boolean, default is false, true enables image caching
* **media_player_video_gl_mode**: Accepts only "true". Will enable openGL elements in GStreamer. This is required to be on when using nvdecode
* **media_player_video_nvdecode**: Boolean. if true, will use the nvdec NVidia CUDA video decoder for video playback. See the readme in the video project for details.

XML Parameters
-------------------------------
You can load another xml interface from within an xml interface. This is super handy for menus and such that have a bunch of identical buttons or to make a consistent close button for your whole app.
Limitations:
* You cannot load the same interface recursively, it'd be infinite
* You can't add children to the loaded interface from the parent interface, since the child interface might have many children

Properties:
* You can set the properties of any loaded child from the parent with a "property" tag.
* Children are given a dot naming scheme, and calling properties uses names relative to the child's interface.

Settings:
* you can set the settings of the loaded layout with a `<settings>` block. These take precedence over the same setting in the loaded xml or in app settings.
* If the loading layout has a settings block it is also applied to the incomming layout but it has a lower precedence than the incoming layout. the settings order with first used at top:
	* `<xml>` sprite settings
	* loaded layout file settings
	* loading layout settings
	* app settings

Example:

**menu_view.xml:**

```xml
<interface>
<layout name="layout" >
	<xml name="home" src="%APP%/data/layouts/menu_button.xml" >
		<!-- note that the "name" here refers to the name of a sprite inside menu_button.xml.
			Any other property of the child can be modified here as well -->
		<property name="normal_icon" src="%APP%/data/images/icons/home_up.png" />
		<property name="high_icon" src="%APP%/data/images/icons/home_down.png" />
	</xml>
   <xml name="map" src="%APP%/data/layouts/menu_button.xml" >
		<settings>
			<setting name="gradient1" value="green" />
			<setting name="gradient2" value="blue_green"/>
		</settings>
		<!-- you can't have any normal children here,
			 only set properties of the children of the parent xml interface -->
		<property name="normal_icon" src="%APP%/data/images/icons/map_up.png" />
		<property name="high_icon" src="%APP%/data/images/icons/map_down.png" />
		<property name="down_gradient" opacity="0.5" animate_on="fade" />
	</xml>
</layout>
</interface>
```

**menu_button.xml:**

```xml
<interface>
<settings>
	<setting name="gradient1" value="red_orange" />
	<setting name="gradient2" value="orange"/>
</settings>
<sprite_button name="the_button" size="80, 80">
	<gradient name="down_gradient" attach_state="high" size="80, 80"
			  gradientColors="$_gradient1, $_gradient1, $_gradient1, $_gradient2"/>
	<image attach_state="normal" name="normal_icon"/>
	<image attach_state="high" name="high_icon" />
</sprite_button>
</interface>
```

**In c++:**

```cpp
std::map<std::string, ds::ui::Sprite*>    spriteMap;
ds::ui::XmlImporter::loadXMLto(this, ds::Environment::expand("%APP%/data/layouts/menu_view.xml"), spriteMap);
// Note that the home button is given the name of the xml "home" plus it's local name.
// This allows you to have multiple instances of the same loaded xml and address all of them
mHomeButton = dynamic_cast<ds::ui::SpriteButton*>(spriteMap["home.the_button"]);
if(mHomeButton){
	mHomeButton->setClickFn([this]{ /* do something to load the home screen */ });
}
mMapButton = dynamic_cast<ds::ui::SpriteButton*>(spriteMap["map.the_button"]);
if(mMapButton){
	mMapButton->setClickFn([this]{ /* do something to load the map screen */ });
}
```

Animation
==============================
You can supply an animation script for the sprite to run when tweenAnimateOn() is called. This allows you to create animations for the interface right from the XML, and tweak them while the app is running. There are some limitations to this method, but should work for most circumstances. You'll supply a simple script that calls the basic tween functions from SpriteAnimatable.

Syntax
------------------------------------------
Animation scripts follow this syntax for each tween (valueY and valueZ are optional):

    <type>:<valueX, valueY, valueZ>;

So for example, this would tween the sprite to ci::Vec3f(100.0f, 200.0f, 300.0f):

    "position:100, 200, 300;"

Tween types can be chained together:

    "position:100, 200, 300; opacity:1.0; scale:1.0, 1.0, 1.0"

    "position:0.0, 100; opacity:0.5; scale:1.0, 1.0; ease:inOutBack; duration:0.5; delay:1.1"

**Basic tween types:**
* **scale**
* **position**
* **opacity** (only the x-value is used for the opacity, y and z are ignored)
* **color** (x,y,z map to r,g,b, respectively, and use 0.0-1.0 values (not 0-255))
* **size** (does not work for some sprite types, like Image)
* **rotation**

**Advanced tween types:**
These tweens move to the current value offset by the supplied value. The first time these are called, the destination positions are cached, so these can be called multiple times. If the destination should be moved, set the sprite to the destination and call setAnimateOnTargets().
* **fade:** Tweens to the current value, and starts at an offset supplied. For instance, fade:-1.0 would tween the opacity of a sprite from 0.0 to 1.0 (assuming it started at 1.0). fade:-0.5 would tween the sprite's opacity from 0.5 to 1.0.
* **slide:** Tweens to the current position, and starts offset by the amount supplied. "slide:-100, 0, 0" would offset the sprite 100 pixels to the left, then tween to the current position.
* **grow:** Tweens to the current scale, offset by the amount supplied.

**Easing:**
Supply a string for the easing type desired. Default is inOutCubic.

    easing:inOutBack;

Valid types:
* **none** = ci::easeNone;
* **inQuad** = ci::easeInQuad;
* **outQuad** = ci::easeOutQuad;
* **inOutQuad** = ci::easeInOutQuad;
* **inCubic** = ci::easeInCubic;
* **outCubic** = ci::easeOutCubic;
* **inOutCubic** = ci::easeInOutCubic;
* **inQuart** = ci::easeInQuart;
* **outQuart** = ci::easeOutQuart;
* **inOutQuart** = ci::easeInOutQuart;
* **inQuint** = ci::easeInQuint;
* **outQuint** = ci::easeOutQuint;
* **inOutQuint** = ci::easeInOutQuint;
* **inSine** = ci::easeInSine;
* **outSine** = ci::easeOutSine;
* **inOutSine** = ci::easeInOutSine;
* **inExpo** = ci::easeInExpo;
* **outExpo** = ci::easeOutExpo;
* **inOutExpo** = ci::easeInOutExpo;
* **inCirc** = ci::easeInCirc;
* **outCirc** = ci::easeOutCirc;
* **inOutCirc** = ci::easeInOutCirc;
* **inBounce** = ci::EaseInBounce();
* **outBounce** = ci::EaseOutBounce();
* **inOutBounce** = ci::EaseInOutBounce();
* **inBack** = ci::EaseInBack();
* **outBack** = ci::EaseOutBack();
* **inOutBack** = ci::EaseInOutBack();
* **inAtan** = ci::EaseInAtan();
* **outAtan** = ci::EaseOutAtan();
* **inOutAtan** = ci::EaseInOutAtan();

**Duration:**
Supply a value for the duration of this tween in seconds. Default is 0.35 seconds.

    duration:1.0;

**Delay:**
Supply a delay in seconds for the start of the tween. Default is 0.0 seconds.

    delay:0.4;

**Cascading delays:**
When calling tweenAnimateOn()/tweenAnimateOff(), you can optionally supply a delay and a delta delay. The delta delay is added to the delay for each child sprite. This enables a more staggered animation.


ContentModel & SmartLayout
=========================

If you're using ds::model::ContentModelRef for your data model and queries (see Content Model doc) and SmartLayouts for your layouts, you can apply the content models in the XML itself. You'll do this with the **model** property. **Note:** the sprite **must** be named for this value to be read.

`model` Property
----------------

* **model**: String, colon-separated sprite parameters, semi-colon and space separated for multiple settings.

**Syntax**:  
{sprite property}:{content model reference}->{content model property}.  
or
{_userData key}:{content model reference}->{content model property}. 

**Example**: 

```XML
<text name="must_name_your_sprite"
	font="sample:font"
	model="text:this->title"
	/>
```

When setting a ContentModelRef, you first specify the **sprite property**. Nearly any sprite property in the above works. Setting the model uses the same code path as the initial parsing. You can set the position, color, font, animation, tap events, text, image source, etc. The advantage of this solution is the ability to put more ui control in the database, allowing for easier re-skinning and tweaking.  

You can also set a key in the userData cache by beginning the key name with and underscore. The underscore is part of the key so if you retrieve it elsewhere be sure to include it. this is intended to be used with on_click_event and on_tap_event, but it may have other uses.

The second part of the syntax is the **content model reference**. Typically you'll use the "this" value, which indicates the current ContentModelRef for this SmartLayout. You can also access any level of children that can be accessed through mContentModel.getChildByName(""), such as "slide.theme" or "sqlite.settings".

After a pointer arrow, you'll specify the **content model property** to use. In general these will be the column names from a sqlite db. Since all data in ContentModelRef is stored as string and type converted when applied, you can apply any content model property to any sprite property, so it's up to you to make sure it makes sense. On the flip side, you could apply properties to a text field for quick debugging. For instance, if a color is not appearing correctly, you could apply the color property to a text field to check the value.

Set multiple models separated by a **semi-colon and a space**.

```cpp
ds::model::ContentModelRef sampleSlideModel = ds::model::ContentModelRef("sample");
sampleSlideModel.setProperty("title", std::string("Hello there!"));

ds::model::ContentModelRef themeModel = ds::model::ContentModelRef("theme");
themeModel.setProperty("title_color", std::string("red"));

sampleSlideModel.addChild(themeModel);

auto mySlide = new ds::ui::SmartLayout(mEngine, "slide.xml");
mySlide->setContentModel(sampleSlideModel);
addChildPtr(mySlide);
```

```XML
<!-- slide.xml -->
<layout name="root_layout" >
	<text name="the_title"
		font="slide:title"
		model="color:theme->title_color; text:this->title"
		/>
</layout>
```
Connecting the model and events
---------------------------
An event's data parameter can be set from the model by using the userData cache. To do so, begin the property to set in the model with an **underscore**. This will set the value in the userData cache of the sprite. Later when an event fires you get the value from the userData cache. 

```XML
<layout_button name="button"
    model="_type:this->sensor_type"
    on_click_event="{SelectSensorEvent; user_data:_type}"
/>
```

in this example this->sensor_type is being stored in the layout_button's userData cache with the key '_type' and when the on_click_event is fired, it will look up that value in the cache and send a SelectSensorEvent with the value stored in the cache in it's Data property.

`text_model` & `text_model_format`
----------------------------------
When a content model is applied, any text sprite with both `text_model_format` & `text_model` set
will have the string in `text_model_format` post processed. First any values in '{...}' are replaced
with the corresponding property from the model. Then, any values in 'fn(...)' are processed.

Available Functions (See projects/essentials/src/ds/ui/util/text_model.cpp for details & adding
addional functions):
* fn(upper,{...}) - Convert string to uppercase
* fn(lower,{...}) - Convert string to lowercase
* fn(utc,{...},FMT[,optionalParseFmt]) - Attempt to parse string {...} as a date/time, and convert it into FMT.
* fn(utc_local,{...},FMT[,optionalParseFmt]) - Attempt to parse string {...} as a date/time, and convert it into FMT for local timezone.
	See the [Poco DateTimeFormatter](https://pocoproject.org/docs/Poco.DateTimeFormatter.html#9945)
	docs for valid format options.

```XML
	<text name="the_title"
		font="slide:title"
		text_model_format="{title} is the name of this slide. It was updated at fn(utc_format,{updated_at},%d-%m-%Y %H:%M)"
		model="text_model:this"
		/>
```


`each_model` Property
---------------------

`each_model` generates smart layouts for each child of a ContentModelRef, applies that child content
model and adds them to the sprite.
For all sprite types other than smart_scroll_list, this will clear/release all children every time a
content model is applied. Which means it's a bad idea to have children (especially named children!)

* **each_model**: String, colon-separated xml name & model to iterate

**Syntax**: {xml layout}:{model}

**Example**: 

```XML
<!-- slideshow.xml -->
<layout name="slides"
	each_model="single_slide.xml:slides"/>
		<!-- A warning will be thrown if you add any children in XML to a
		     sprite with each_model. And the property won't be set! -->
```

```XML
<!-- single_slide.xml -->
<layout name="root" shrink_to_children="both">
	<text name="the_title"
		font="slide:title"
		model="color:theme->title_color; text:this->title"
		/>
</layout>
```

`visible_if_exists` & `hidden_if_exists` Property
----------------------------

A model property that can hide/show children depending on if a ContentModel node or a property exists.
For ContentModels, use the syntax visible_if_exists:{node}, which will test node.empty()
For properties, this checks if the property string is empty. For instance, visible_if_exists:this->title will check node.getPropertyString("title").empty();

This is designed to be used with skip_hidden_children, so you can conditionally have parts of your layouts appear based on the content.

Additional properties could be added for bool and int checks (visible_if_bool perhaps?)

```XML
<layout name="root" 
	shrink_to_children="both"
	skip_hidden_children="true"
	model="visible_if_exists:this"
	>
	<layout name="title_layout"
		layout_type="horiz"
		model="visible_if_exists:this->title"
		>
		<image name="an_icon" src="%APP%/data/images/title_icon.png" />
		<text name="the_title"
			font="slide:title"
			model="color:theme->title_color; text:this->title"
			/>
	</layout>
</layout>
```


Caveats
-----------------------
Setting the model runs when you call setContentModel() on the SmartLayout. This means that it will be applied after all the other properties on the sprite. Expressions and parameters are evaluated both before and after the model is processed, so this could be valid: model="text:this->$_title_column_name", assuming "title_column_name" is in app_settings.xml. Additionally, you can put expressions and parameters into the content model itself, and they will be evaluated normally. The same limitations apply (e.g. no nested expressions).

- **name**: Must have a value for the sprite to have a model applied
- **resource and resource_cache**: These currently only apply to Images
- **media_player_src**: Tries to find an existing resource or looks it up on disk, but can apply to all media player types
- **text_update**: Very handy if you only want to overwrite the text if it exists. This is great if you have a text field with generic info that could show an error message if it's available. Or you could have one layout file for several different tables and only apply the title field text if it exists:

```XML
<text name="the_title"
	font="slide:title"
	model="text_update:this->title; text_update:this->name; text_update:this->label"
	/>
```
