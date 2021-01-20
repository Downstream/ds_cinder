#include "stdafx.h"

#include "interface_xml_importer.h"

#include "stylesheet_parser.h"

#include <ds/app/engine/engine.h>
#include <ds/app/engine/engine_cfg.h>
#include <ds/app/environment.h>
#include <ds/app/event.h>
#include <ds/app/event_registry.h>
#include <ds/cfg/settings.h>
#include <ds/cfg/settings_variables.h>
#include <ds/debug/logger.h>
#include <ds/ui/button/image_button.h>
#include <ds/ui/button/layout_button.h>
#include <ds/ui/button/sprite_button.h>
#include <ds/ui/control/control_check_box.h>
#include <ds/ui/control/control_slider.h>
#include <ds/ui/layout/layout_sprite.h>
#include <ds/ui/layout/perspective_layout.h>
#include <ds/ui/scroll/centered_scroll_area.h>
#include <ds/ui/scroll/scroll_area.h>
#include <ds/ui/scroll/scroll_bar.h>
#include <ds/ui/scroll/scroll_list.h>
#include <ds/ui/scroll/smart_scroll_list.h>
#include <ds/ui/soft_keyboard/entry_field.h>
#include <ds/ui/soft_keyboard/soft_keyboard.h>
#include <ds/ui/soft_keyboard/soft_keyboard_builder.h>
#include <ds/ui/sprite/border.h>
#include <ds/ui/sprite/circle.h>
#include <ds/ui/sprite/circle_border.h>
#include <ds/ui/sprite/dashed_line.h>
#include <ds/ui/sprite/donut_arc.h>
#include <ds/ui/sprite/gradient_sprite.h>
#include <ds/ui/sprite/image.h>
#include <ds/ui/sprite/image_with_thumbnail.h>
#include <ds/ui/sprite/sprite.h>
#include <ds/ui/sprite/sprite_engine.h>
#include <ds/ui/sprite/text.h>
#include <ds/util/color_util.h>
#include <ds/util/file_meta_data.h>
#include <ds/util/markdown_to_pango.h>
#include <ds/util/string_util.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>

#include <Poco/Path.h>
#include <Poco/DateTimeParser.h>
#include <Poco/DateTimeFormatter.h>

#include <fstream>
#include <iostream>
#include <typeinfo>

namespace {

static std::unordered_map<std::string, ds::ui::XmlImporter::XmlPreloadData> PRELOADED_CACHE;
static bool																	AUTO_CACHE = false;

// Get the setting if we're caching or not and run it just before server setup
// That way we can clear the cache each time the server setup runs
class Init {
  public:
	Init() {
		ds::App::AddServerSetup([](ds::Engine& e) {
			AUTO_CACHE = e.getEngineSettings().getBool("xml_importer:cache");
			PRELOADED_CACHE.clear();
		});
	}
	void doNothing() {}
};
Init INIT;


}  // namespace

namespace ds {
namespace ui {

static const std::string INVALID_VALUE = "UNACCEPTABLE!!!!";


std::string XmlImporter::getGradientColorsAsString(ds::ui::Gradient* grad) {
	if (!grad) return "";
	std::stringstream ss;
	ss << ds::unparseColor(grad->getColorTL()) << ", " << ds::unparseColor(grad->getColorTR()) << ", "
	   << ds::unparseColor(grad->getColorBR()) << ", " << ds::unparseColor(grad->getColorBL());
	return ss.str();
}

namespace {
static const ci::vec3						  DEFAULT_SIZE					= ci::vec3(0.0f, 0.0f, 1.0f);
static const ci::vec3						  DEFAULT_CENTER				= ci::vec3(0.0f, 0.0f, 0.0f);
static const ci::vec3						  DEFAULT_POS					= ci::vec3(0.0f, 0.0f, 0.0f);
static const ci::vec3						  DEFAULT_ROT					= ci::vec3(0.0f, 0.0f, 0.0f);
static const ci::vec3						  DEFAULT_SCALE					= ci::vec3(1.0f, 1.0f, 1.0f);
static const ci::Color						  DEFAULT_COLOR					= ci::Color(1.0f, 1.0f, 1.0f);
static const float							  DEFAULT_OPACITY				= 1.0f;
static const bool							  DEFAULT_VISIBLE				= true;
static const bool							  DEFAULT_TRANSPARENT			= true;
static const bool							  DEFAULT_ENABLED				= false;
static const bool							  DEFAULT_CLIPPING				= false;
static const bool							  DEFAULT_CHECKBOUNDS			= false;
static const ds::ui::BlendMode				  DEFAULT_BLENDMODE				= ds::ui::NORMAL;
static const float							  DEFAULT_LAYOUT_PAD			= 0.0f;
static const float							  DEFAULT_LAYOUT_SPACING		= 0.0f;
static const ci::vec2						  DEFAULT_LAYOUT_SIZE			= ci::vec2();
static const ci::vec3						  DEFAULT_LAYOUT_FUDGE			= ci::vec3();
static const int							  DEFAULT_LAYOUT_ALIGN_USERTYPE = 0;
static const ds::ui::LayoutSprite::LayoutType DEFAULT_LAYOUT_TYPE			= ds::ui::LayoutSprite::kLayoutNone;
static const ds::ui::LayoutSprite::ShrinkType DEFAULT_SHRINK_TYPE			= ds::ui::LayoutSprite::kShrinkNone;
}  // namespace


// There's a lotta dynamic casts here and such, but this is pretty much a dev-only task.
// Don't expect this to ever really be called in production.
void XmlImporter::getSpriteProperties(ds::ui::Sprite& sp, ci::XmlTree& xml) {
	if (!sp.getSpriteName(false).empty()) xml.setAttribute("name", ds::utf8_from_wstr(sp.getSpriteName()));
	if (sp.getSize() != DEFAULT_SIZE) xml.setAttribute("size", unparseVector(sp.getSize()));
	if (sp.getColor() != DEFAULT_COLOR) xml.setAttribute("color", unparseColor(sp.getColor()));
	if (sp.getOpacity() != DEFAULT_OPACITY) xml.setAttribute("opacity", sp.getOpacity());
	if (sp.getPosition() != DEFAULT_POS) xml.setAttribute("position", unparseVector(sp.getPosition()));
	if (sp.getRotation() != DEFAULT_ROT) xml.setAttribute("rotation", unparseVector(sp.getRotation()));
	if (sp.getScale() != DEFAULT_SCALE) xml.setAttribute("scale", unparseVector(sp.getScale()));
	if (sp.getCenter() != DEFAULT_CENTER) xml.setAttribute("center", unparseVector(sp.getCenter()));
	if (sp.getClipping() != DEFAULT_CLIPPING) xml.setAttribute("clipping", unparseBoolean(sp.getClipping()));
	if (sp.getBlendMode() != DEFAULT_BLENDMODE) xml.setAttribute("blend_mode", ds::ui::getStringForBlendMode(sp.getBlendMode()));
	if (sp.isEnabled() != DEFAULT_ENABLED) xml.setAttribute("enable", unparseBoolean(sp.isEnabled()));
	if (!sp.getMultiTouchConstraints().isEmpty())
		xml.setAttribute("multitouch", getMultitouchStringForBitMask(sp.getMultiTouchConstraints()));
	if (sp.getTransparent() != DEFAULT_TRANSPARENT) xml.setAttribute("transparent", unparseBoolean(sp.getTransparent()));
	if (!sp.getAnimateOnScript().empty()) xml.setAttribute("animate_on", sp.getAnimateOnScript());
	if (sp.mLayoutTPad != DEFAULT_LAYOUT_PAD) xml.setAttribute("t_pad", sp.mLayoutTPad);
	if (sp.mLayoutBPad != DEFAULT_LAYOUT_PAD) xml.setAttribute("b_pad", sp.mLayoutBPad);
	if (sp.mLayoutLPad != DEFAULT_LAYOUT_PAD) xml.setAttribute("l_pad", sp.mLayoutLPad);
	if (sp.mLayoutRPad != DEFAULT_LAYOUT_PAD) xml.setAttribute("r_pad", sp.mLayoutRPad);
	if (sp.mLayoutFudge != DEFAULT_LAYOUT_FUDGE) xml.setAttribute("layout_fudge", unparseVector(sp.mLayoutFudge));
	if (sp.mLayoutSize != DEFAULT_LAYOUT_SIZE) xml.setAttribute("layout_size", unparseVector(sp.mLayoutSize));
	if (sp.mLayoutUserType != DEFAULT_LAYOUT_ALIGN_USERTYPE)
		xml.setAttribute("layout_size_mode", LayoutSprite::getLayoutSizeModeString(sp.mLayoutUserType));
	if (sp.mLayoutVAlign != DEFAULT_LAYOUT_ALIGN_USERTYPE)
		xml.setAttribute("layout_v_align", LayoutSprite::getLayoutVAlignString(sp.mLayoutVAlign));
	if (sp.mLayoutHAlign != DEFAULT_LAYOUT_ALIGN_USERTYPE)
		xml.setAttribute("layout_h_align", LayoutSprite::getLayoutHAlignString(sp.mLayoutHAlign));
	if (sp.getCornerRadius() > 0.0f) xml.setAttribute("corner_radius", sp.getCornerRadius());

	ds::ui::LayoutSprite* ls = dynamic_cast<ds::ui::LayoutSprite*>(&sp);
	if (ls) {
		if (ls->getLayoutType() != DEFAULT_LAYOUT_TYPE)
			xml.setAttribute("layout_type", LayoutSprite::getLayoutTypeString(ls->getLayoutType()));
		if (ls->getSpacing() != DEFAULT_LAYOUT_SPACING) xml.setAttribute("layout_spacing", ls->getSpacing());
		if (ls->getShrinkToChildren() != DEFAULT_SHRINK_TYPE)
			xml.setAttribute("shrink_to_children", LayoutSprite::getShrinkToChildrenString(ls->getShrinkToChildren()));
		if (ls->getOverallAlignment() != DEFAULT_SHRINK_TYPE)
			xml.setAttribute("overall_alignment", LayoutSprite::getLayoutVAlignString(ls->getOverallAlignment()));
	}

	ds::ui::Text* txt = dynamic_cast<ds::ui::Text*>(&sp);
	if (txt) {
		if (!txt->getText().empty()) xml.setAttribute("text", txt->getTextAsString());
		if (txt->getTextStyle().mName.empty()) {
			xml.setAttribute("text_style", txt->getTextStyle().mName);
		} else {
			xml.setAttribute("font", txt->getTextStyle().mFont);
			xml.setAttribute("font_leading", txt->getLeading());
		}

		xml.setAttribute("resize_limit", unparseVector(ci::vec2(txt->getResizeLimitWidth(), txt->getResizeLimitHeight())));
		if (txt->getAlignment() != ds::ui::Alignment::kLeft)
			xml.setAttribute("text_align", LayoutSprite::getLayoutHAlignString(txt->getAlignment()));
	}

	ds::ui::Image* img = dynamic_cast<ds::ui::Image*>(&sp);
	if (img) {
		if (!img->getImageFilename().empty()) xml.setAttribute("filename", ds::Environment::contract(img->getImageFilename()));
		if (img->getCircleCrop()) xml.setAttribute("circle_crop", "true");
	}

	ds::ui::ImageButton* imgB = dynamic_cast<ds::ui::ImageButton*>(&sp);
	if (imgB) {
		if (imgB->getNormalImagePath() == imgB->getHighImagePath()) {
			if (!imgB->getNormalImagePath().empty())
				xml.setAttribute("filename", ds::Environment::contract(imgB->getNormalImagePath()));
		} else {
			if (!imgB->getNormalImagePath().empty())
				xml.setAttribute("up_image", ds::Environment::contract(imgB->getNormalImagePath()));
			if (!imgB->getHighImagePath().empty())
				xml.setAttribute("down_image", ds::Environment::contract(imgB->getHighImagePath()));
		}
		xml.setAttribute("up_image_color", unparseColor(imgB->getNormalImageColor()));
		xml.setAttribute("down_image_color", unparseColor(imgB->getHighImageColor()));
		xml.setAttribute("btn_touch_padding", imgB->getPad());
	}

	ds::ui::Gradient* grad = dynamic_cast<ds::ui::Gradient*>(&sp);
	if (grad) {
		xml.setAttribute("gradientColors", getGradientColorsAsString(grad));
	}

	ds::ui::Circle* circ = dynamic_cast<ds::ui::Circle*>(&sp);
	if (circ) {
		xml.setAttribute("filled", unparseBoolean(circ->getFilled()));
		xml.setAttribute("radius", circ->getRadius());
		xml.setAttribute("line_width", circ->getLineWidth());
	}

	ds::ui::Border* border = dynamic_cast<ds::ui::Border*>(&sp);
	if (border) {
		if (border->getBorderWidth() != 0.0f) xml.setAttribute("border_width", border->getBorderWidth());
	}

	ds::ui::CircleBorder* circleBorder = dynamic_cast<ds::ui::CircleBorder*>(&sp);
	if (circleBorder) {
		if (circleBorder->getBorderWidth() != 0.0f) xml.setAttribute("border_width", circleBorder->getBorderWidth());
	}

	// scroll list
	// scroll area
}

ci::XmlTree XmlImporter::createXmlFromSprite(ds::ui::Sprite& sprite) {
	ci::XmlTree newXml = ci::XmlTree(getSpriteTypeForSprite(&sprite), "");
	getSpriteProperties(sprite, newXml);

	auto sprids = sprite.getChildren();
	for (auto it = sprids.begin(); it < sprids.end(); ++it) {
		if (!(*it)->mExportWithXml) continue;
		auto xmlly = createXmlFromSprite(*(*it));
		newXml.push_back(xmlly);
	}

	return newXml;
}

void XmlImporter::setSpriteProperty(ds::ui::Sprite& sprite, ci::XmlTree::Attr& attr, const std::string& referer,ds::cfg::VariableMap& local_map) {
	std::string property = attr.getName();
	setSpriteProperty(sprite, property, attr.getValue(), referer, local_map);
}


struct SprProps {
	SprProps(ds::ui::Sprite& spr, const std::string& prop, const std::string& val, const std::string& ref, ds::cfg::VariableMap& l_map)
		: sprite(spr), property(prop), value(val), referer(ref), local_map(l_map), engine(spr.getEngine()){}
	ds::ui::Sprite& sprite;
	const std::string& property;
	const std::string& value;
	const std::string& referer;
	ds::cfg::VariableMap& local_map;
	ds::ui::SpriteEngine& engine;
};

void logAttributionWarning(SprProps& p){
	DS_LOG_WARNING("XmlImporter: incompatible attribute \'" << p.property << "\' on sprite \'" << ds::utf8_from_wstr(p.sprite.getSpriteName(true)) << "\' of type \'" << typeid(p.sprite).name() << "\' from \'" << p.referer << "\'");
}

void logNotFoundWarning(SprProps& p) {
	DS_LOG_WARNING("XmlImporter: Property not found \'" << p.property << "\' on sprite \'" << ds::utf8_from_wstr(p.sprite.getSpriteName(true)) << "\' of type \'" << typeid(p.sprite).name() << "\' from \'" << p.referer << "\'");
}

void logInvalidValue(SprProps& p, std::string validValues) {
	DS_LOG_WARNING("XmlImporter: invalid value of \'" << p.value << "\' for property \'" << p.property << "\', allowed values are " << validValues << ". From sprite \'" << ds::utf8_from_wstr(p.sprite.getSpriteName(true)) << "\' of type \'" << typeid(p.sprite).name() << "\' from \'" << p.referer << "\'");
}

void XmlImporter::setSpriteProperty(ds::ui::Sprite& sprite, const std::string& property, const std::string& theValue,
	const std::string& referer, ds::cfg::VariableMap& local_map) {

	if (property.front() == '_') {
		DS_LOG_VERBOSE(2, "Sprite property commented out: " << property << " " << theValue << " " << referer);
		return;
	}

	DS_LOG_VERBOSE(4, "XmlImporter: setSpriteProperty, prop=" << property << " value=" << theValue << " referer=" << referer);

	if (local_map.size() > 0) {
		auto x = local_map;
	}
	std::string value = ds::cfg::SettingsVariables::replaceVariables(theValue, local_map);
	value = ds::cfg::SettingsVariables::parseAllExpressions(value);

	// TODO: build this in a different function?
	static std::unordered_map<std::string, std::function<void(SprProps& p)>> propertyMap;

	// build the static map on the first run
	if (propertyMap.empty()) {
		propertyMap["name"] = [](SprProps& p) {
			p.sprite.setSpriteName(ds::wstr_from_utf8(p.value));
		};
		propertyMap["class"] = [](SprProps& p) {
			// Do nothing, this is handled by css parsers, if any
		};
		propertyMap["attach_state"] = [](SprProps& p) {
			// This is a special function to apply children to a highlight or normal state of a sprite button, so ignore it.
		};
		propertyMap["sprite_link"] = [](SprProps& p) {
			// This is a special function to apply children to a highlight or normal state of a sprite button, so ignore it.
		};
		propertyMap["width"] = [](SprProps& p) {
			p.sprite.setSize(ds::string_to_float(p.value), p.sprite.getHeight());
		};
		propertyMap["height"] = [](SprProps& p) {
			p.sprite.setSize(p.sprite.getWidth(), ds::string_to_float(p.value));
		};
		propertyMap["depth"] = [](SprProps& p) {
			p.sprite.setSizeAll(p.sprite.getWidth(), p.sprite.getHeight(), ds::string_to_float(p.value));
		};
		propertyMap["size"] = [](SprProps& p) {
			ci::vec3 v = parseVector(p.value);
			p.sprite.setSize(v.x, v.y);
		};
		propertyMap["color"] = [](SprProps& p) {
			p.sprite.setTransparent(false);
			p.sprite.setColorA(parseColor(p.value, p.engine));
		};
		propertyMap["opacity"] = [](SprProps& p) {
			p.sprite.setOpacity(ds::string_to_float(p.value));
		};
		propertyMap["position"] = [](SprProps& p) {
			p.sprite.setPosition(parseVector(p.value));
		};
		propertyMap["rotation"] = [](SprProps& p) {
			p.sprite.setRotation(parseVector(p.value));
		};
		propertyMap["scale"] = [](SprProps& p) {
			p.sprite.setScale(parseVector(p.value));
		};
		propertyMap["center"] = [](SprProps& p) {
			p.sprite.setCenter(parseVector(p.value));
		};
		propertyMap["clipping"] = [](SprProps& p) {
			p.sprite.setClipping(parseBoolean(p.value));
		};
		propertyMap["blend_mode"] = [](SprProps& p) {
			p.sprite.setBlendMode(ds::ui::getBlendModeByString(p.value));
		};
		propertyMap["enable"] = [](SprProps& p) {
			p.sprite.enable(parseBoolean(p.value));
		};
		propertyMap["multitouch"] = [](SprProps& p) {
			p.sprite.enableMultiTouch(parseMultitouchMode(p.value));
		};
		propertyMap["transparent"] = [](SprProps& p) {
			p.sprite.setTransparent(parseBoolean(p.value));
		};
		propertyMap["visible"] = [](SprProps& p) {
			const auto isVisible = parseBoolean(p.value);
			(isVisible) ? p.sprite.show() : p.sprite.hide();
		};
		propertyMap["animate_on"] = [](SprProps& p) {
			p.sprite.setAnimateOnScript(p.value);
		};
		propertyMap["animate_off"] = [](SprProps& p) {
			p.sprite.setAnimateOffScript(p.value);
		};
		propertyMap["corner_radius"] = [](SprProps& p) {
			p.sprite.setCornerRadius(ds::string_to_float(p.value));
		};
		propertyMap["t_pad"] = [](SprProps& p) {
			p.sprite.mLayoutTPad = ds::string_to_float(p.value);
		};
		propertyMap["b_pad"] = [](SprProps& p) {
			p.sprite.mLayoutBPad = ds::string_to_float(p.value);
		};
		propertyMap["l_pad"] = [](SprProps& p) {
			p.sprite.mLayoutLPad = ds::string_to_float(p.value);
		};
		propertyMap["r_pad"] = [](SprProps& p) {
			p.sprite.mLayoutRPad = ds::string_to_float(p.value);
		};
		propertyMap["pad_all"] = [](SprProps& p) {
			const auto pad = ds::string_to_float(p.value);
			p.sprite.mLayoutLPad = pad;
			p.sprite.mLayoutTPad = pad;
			p.sprite.mLayoutRPad = pad;
			p.sprite.mLayoutBPad = pad;
		};
		propertyMap["padding"] = [](SprProps& p) {
			auto pads = ds::split(p.value, ", ", true);
			const auto county = pads.size();
			p.sprite.mLayoutLPad = (county > 0) ? ds::string_to_float(pads[0]) : 0.0f;
			p.sprite.mLayoutTPad = (county > 1) ? ds::string_to_float(pads[1]) : 0.0f;
			p.sprite.mLayoutRPad = (county > 2) ? ds::string_to_float(pads[2]) : 0.0f;
			p.sprite.mLayoutBPad = (county > 3) ? ds::string_to_float(pads[3]) : 0.0f;
		};
		propertyMap["layout_size_mode"] = [](SprProps& p) {
			const auto sizeMode = p.value;
			if (sizeMode == "fixed") {
				p.sprite.mLayoutUserType = LayoutSprite::kFixedSize;
			} else if (sizeMode == "flex") {
				p.sprite.mLayoutUserType = LayoutSprite::kFlexSize;
			} else if (sizeMode == "stretch") {
				p.sprite.mLayoutUserType = LayoutSprite::kStretchSize;
			} else if (sizeMode == "fill") {
				p.sprite.mLayoutUserType = LayoutSprite::kFillSize;
			} else {
				logInvalidValue(p, "fixed, flex, stretch, fill");
			}
		};
		propertyMap["layout_v_align"] = [](SprProps& p) {
			const auto alignMode = p.value;
			if (alignMode == "top") {
				p.sprite.mLayoutVAlign = LayoutSprite::kTop;
			} else if (alignMode == "middle" || alignMode == "center") {
				p.sprite.mLayoutVAlign = LayoutSprite::kMiddle;
			} else if (alignMode == "bottom") {
				p.sprite.mLayoutVAlign = LayoutSprite::kBottom;
			} else {
				logInvalidValue(p, "top, middle, center, bottom");
			}
		};
		propertyMap["layout_h_align"] = [](SprProps& p) {
			const auto alignMode = p.value;
			if (alignMode == "left") {
				p.sprite.mLayoutHAlign = LayoutSprite::kLeft;
			} else if (alignMode == "middle" || alignMode == "center") {
				p.sprite.mLayoutHAlign = LayoutSprite::kCenter;
			} else if (alignMode == "right") {
				p.sprite.mLayoutHAlign = LayoutSprite::kRight;
			} else {
				logInvalidValue(p, "left, middle, center, right");
			}
		};
		propertyMap["layout_fudge"] = [](SprProps& p) {
			p.sprite.mLayoutFudge = parseVector(p.value);
		};
		propertyMap["layout_size"] = [](SprProps& p) {
			p.sprite.mLayoutSize = ci::vec2(parseVector(p.value));
		};
		propertyMap["on_tap_event"] = [](SprProps& p) {
			auto theValue = p.value;
			p.sprite.setTapCallback([theValue](ds::ui::Sprite* bs, const ci::vec3& pos) { XmlImporter::dispatchStringEvents(theValue, bs, pos); });
		};
		propertyMap["layout_fixed_aspect"] = [](SprProps& p) {
			p.sprite.mLayoutFixedAspect = parseBoolean(p.value);
		};
		propertyMap["shader"] = [](SprProps& p) {
			using namespace boost::filesystem;
			boost::filesystem::path fullShaderPath(filePathRelativeTo(p.referer, p.value));
			p.sprite.setBaseShader(fullShaderPath.parent_path().string(), fullShaderPath.filename().string());

		};

		// LayoutpSprite specific (the other layout stuff could apply to any sprite)
		propertyMap["layout_type"] = [](SprProps& p) {
			auto layoutSprite = dynamic_cast<LayoutSprite*>(&p.sprite);
			if (layoutSprite) {
				const auto layoutType = p.value;
				if (layoutType == "vert") {
					layoutSprite->setLayoutType(LayoutSprite::kLayoutVFlow);
				} else if (layoutType == "horiz") {
					layoutSprite->setLayoutType(LayoutSprite::kLayoutHFlow);
				} else if (layoutType == "vert_wrap") {
					layoutSprite->setLayoutType(LayoutSprite::kLayoutVWrap);
				} else if (layoutType == "horiz_wrap") {
					layoutSprite->setLayoutType(LayoutSprite::kLayoutHWrap);
				} else if (layoutType == "size") {
					layoutSprite->setLayoutType(LayoutSprite::kLayoutSize);
				} else {
					layoutSprite->setLayoutType(LayoutSprite::kLayoutNone);
				} 
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["layout_spacing"] = [](SprProps& p) {
			auto layoutSprite = dynamic_cast<LayoutSprite*>(&p.sprite);
			if (layoutSprite) {
				layoutSprite->setSpacing(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["overall_alignment"] = [](SprProps& p) {
			auto layoutSprite = dynamic_cast<LayoutSprite*>(&p.sprite);
			if (layoutSprite) {
				const auto alignMode = p.value;
				if (alignMode == "left" || alignMode == "top") {
					layoutSprite->setOverallAlignment(LayoutSprite::kLeft);
				} else if (alignMode == "center" || alignMode == "middle") {
					layoutSprite->setOverallAlignment(LayoutSprite::kCenter);
				} else if (alignMode == "right" || alignMode == "bottom") {
					layoutSprite->setOverallAlignment(LayoutSprite::kRight);
				} else {
					logInvalidValue(p, "left, top, center, middle, right, bottom");
				}
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["shrink_to_children"] = [](SprProps& p) {
			auto layoutSprite = dynamic_cast<LayoutSprite*>(&p.sprite);
			if (layoutSprite) {
				const auto shrinkMode = p.value;
				if (shrinkMode == "" || shrinkMode == "false" || shrinkMode == "none") {
					layoutSprite->setShrinkToChildren(LayoutSprite::kShrinkNone);
				} else if (shrinkMode == "width") {
					layoutSprite->setShrinkToChildren(LayoutSprite::kShrinkWidth);
				} else if (shrinkMode == "height") {
					layoutSprite->setShrinkToChildren(LayoutSprite::kShrinkHeight);
				} else if (shrinkMode == "true" || shrinkMode == "both") {
					layoutSprite->setShrinkToChildren(LayoutSprite::kShrinkBoth);
				} else {
					logInvalidValue(p, "false, true, none, width, height, both");
				}
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["skip_hidden_children"] = [](SprProps& p) {
			auto layoutsprite = dynamic_cast<LayoutSprite*>(&p.sprite);
			if (layoutsprite) {
				layoutsprite->setSkipHiddenChildren(parseBoolean(p.value));
			} else {
				logAttributionWarning(p);
			}
		};

		// Text specific attributes
		propertyMap["font"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				text->setTextStyle(text->getEngine().getTextStyle(p.value));
			} else {
				auto controlBox = dynamic_cast<ControlCheckBox*>(&p.sprite);
				if (controlBox) {
					controlBox->setLabelTextStyle(p.value);
					logAttributionWarning(p);
				}
			}
		};
		propertyMap["font_name"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				text->setFont(p.value);
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["text_style"] = [](SprProps& p) {
			// Try to set the font
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				if (p.engine.getEngineCfg().hasTextStyle(p.value)) {
					text->setTextStyle(p.value);
				} else {
					text->setTextStyle(TextStyle::textStyleFromSetting(p.engine, p.value));
				}
			} else {
				auto controlBox = dynamic_cast<ControlCheckBox*>(&p.sprite);
				if (controlBox) {
					if (p.engine.getEngineCfg().hasTextStyle(p.value)) {
						controlBox->setLabelTextStyle(p.value);
					} else {
						controlBox->setLabelTextStyle(TextStyle::textStyleFromSetting(p.engine, p.value));
					}
				}
			}
		};
		propertyMap["resize_limit"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				auto v = parseVector(p.value);
				text->setResizeLimit(v.x, v.y);
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["fit_font_sizes"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				std::regex e3(",+");
				auto itr = std::sregex_token_iterator(p.value.begin(), p.value.end(), e3, -1);
				std::vector<double> size_values;
				double font_value;

				for (; itr != std::sregex_token_iterator(); ++itr) {
					if (ds::string_to_value<double>(itr->str(), font_value)) {
						size_values.push_back(font_value);
					}
				}

				text->setFitFontSizes(size_values);
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["fit_max_font_size"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				double v = ds::string_to_double(p.value);
				text->setFitMaxFontSize(v);
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["fit_min_font_size"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				double v = ds::string_to_double(p.value);
				text->setFitMinFontSize(v);
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["fit_font_size_range"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				ci::vec3 v = parseVector(p.value);
				text->setFitMinFontSize(v.x);
				text->setFitMaxFontSize(v.y);
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["fit_to_limit"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				bool v = parseBoolean(p.value);
				text->setFitToResizeLimit(v);
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["text_align"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				std::string alignString = p.value;
				if (alignString == "left") {
					text->setAlignment(ds::ui::Alignment::kLeft);
				} else if (alignString == "right") {
					text->setAlignment(ds::ui::Alignment::kRight);
				} else if (alignString == "center") {
					text->setAlignment(ds::ui::Alignment::kCenter);
				} else if (alignString == "justify") {
					text->setAlignment(ds::ui::Alignment::kJustify);
				} else {
					logInvalidValue(p, "left, right, center, justify");
					text->setAlignment(ds::ui::Alignment::kLeft);
				}
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["text"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				text->setText(p.value);
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["text_update"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				if (!p.value.empty()) {
					text->setText(p.value);
				}
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["text_model_format"] = [](SprProps& p) {
			if (auto text = dynamic_cast<Text*>(&p.sprite)) {
				if (!p.value.empty()) {
					text->getUserData().setString("model_format", p.value);
				}
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["text_utc_parse"] = [](SprProps& p) {
			if (auto text = dynamic_cast<Text*>(&p.sprite)) {
				if (!p.value.empty()) {
					text->getUserData().setString("utc_parse_fmt", p.value);
				}
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["text_utc_format"] = [](SprProps& p) {
			if (auto text = dynamic_cast<Text*>(&p.sprite)) {
				if (!p.value.empty()) {
					text->getUserData().setString("utc_out_fmt", p.value);
				}
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["text_utc"] = [](SprProps& p) {
			if (auto text = dynamic_cast<Text*>(&p.sprite)) {
				if (!p.value.empty()) {
					auto parseFmt = text->getUserData().getString("utc_parse_fmt");
					auto outFmt = text->getUserData().getString("utc_out_fmt");

					if (outFmt.empty()) {
						outFmt = std::string("%Y-%m-%d %H:%M:%S");
					}

					const bool isNow = (p.value == "now");
					bool didParse = false;
					Poco::DateTime date;
					int tzd = 0;
					if (!isNow && !parseFmt.empty()) {
						didParse = Poco::DateTimeParser::tryParse(parseFmt, p.value, date, tzd);
					}

					if (!isNow && !didParse) {
						didParse = Poco::DateTimeParser::tryParse(p.value, date, tzd);
					}

					if (isNow || didParse) {
						std::string reformedDate = Poco::DateTimeFormatter::format(date, outFmt);
						text->setText(reformedDate);
					} else {
						DS_LOG_WARNING("Unable to parse value '" << p.value << "' as date! parse_fmt='" << parseFmt << "' & out_fmt='" << outFmt << "' on sprite " << ds::utf8_from_wstr(p.sprite.getSpriteName()) << " from: " << p.referer);
						text->setText(p.value);
					}
				}
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["text_uppercase"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				auto upperText = p.value;
				std::transform(upperText.begin(), upperText.end(), upperText.begin(), ::toupper);
				text->setText(upperText);
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["text_lowercase"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				auto lowerText = p.value;
				std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
				text->setText(lowerText);
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["text_ellipses"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				EllipsizeMode theMode = EllipsizeMode::kEllipsizeNone;
				if (p.value == "start") {
					theMode = EllipsizeMode::kEllipsizeStart;
				} else if (p.value == "middle") {
					theMode = EllipsizeMode::kEllipsizeMiddle;
				} else if (p.value == "end") {
					theMode = EllipsizeMode::kEllipsizeEnd;
				} else if (p.value == "none") {
					theMode = EllipsizeMode::kEllipsizeNone;
				} else {
					logInvalidValue(p, "start, middle, end, none");
				}

				text->setEllipsizeMode(theMode);
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["text_wrap"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				WrapMode theMode = WrapMode::kWrapModeWordChar;
				if (p.value == "false" || p.value == "off") {
					theMode = WrapMode::kWrapModeOff;
				} else if (p.value == "word") {
					theMode = WrapMode::kWrapModeWord;
				} else if (p.value == "char") {
					theMode = WrapMode::kWrapModeChar;
				} else if (p.value == "word_char" || p.value == "wordchar") {
					theMode = WrapMode::kWrapModeWordChar;
				} else {
					logInvalidValue(p, "start, middle, end, none");
				}

				text->setWrapMode(theMode);
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["text_allow_markup"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				text->setAllowMarkup(parseBoolean(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["markdown"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				text->setText(ds::ui::markdown_to_pango(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["font_size"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				text->setFontSize(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["font_leading"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				text->setLeading(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["font_letter_spacing"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				text->setLetterSpacing(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["font_color"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				text->setColor(parseColor(p.value, text->getEngine()));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["preserve_span_colors"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				text->setPreserveSpanColors(parseBoolean(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["shrink_to_bounds"] = [](SprProps& p) {
			auto text = dynamic_cast<Text*>(&p.sprite);
			if (text) {
				text->setShrinkToBounds(parseBoolean(p.value));
			} else {
				logAttributionWarning(p);
			}
		};

		// Image properties
		propertyMap["on_click_event"] = [](SprProps& p) {
			auto imgBtn = dynamic_cast<ImageButton*>(&p.sprite);
			auto sprBtn = dynamic_cast<SpriteButton*>(&p.sprite);
			auto layBtn = dynamic_cast<LayoutButton*>(&p.sprite);
			auto theValue = p.value;
			if (imgBtn) {
				imgBtn->setClickFn([imgBtn, theValue]{ dispatchStringEvents(theValue, imgBtn, imgBtn->getGlobalPosition()); });
			} else if (sprBtn) {
				sprBtn->setClickFn([sprBtn, theValue]{ dispatchStringEvents(theValue, sprBtn, sprBtn->getGlobalPosition()); });
			} else if (layBtn) {
				layBtn->setClickFn([layBtn, theValue]{ dispatchStringEvents(theValue, layBtn, layBtn->getGlobalPosition()); });
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["circle_crop"] = [](SprProps& p) {
			auto image = dynamic_cast<Image*>(&p.sprite);
			if (image) {
				image->setCircleCrop(parseBoolean(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["auto_circle_crop"] = [](SprProps& p) {
			auto image = dynamic_cast<Image*>(&p.sprite);
			if (image) {
				if (parseBoolean(p.value)) {
					image->cicleCropAutoCenter();
				} else {
					image->setCircleCrop(false);
				}
			} else {
				logAttributionWarning(p);
			}
		};

		// Image Button properties
		propertyMap["down_image"] = [](SprProps& p) {
			auto image = dynamic_cast<ImageButton*>(&p.sprite);
			if (image) {
				image->setHighImage(filePathRelativeTo(p.referer, p.value), ds::ui::Image::IMG_CACHE_F);
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["up_image"] = [](SprProps& p) {
			auto image = dynamic_cast<ImageButton*>(&p.sprite);
			if (image) {
				image->setNormalImage(filePathRelativeTo(p.referer, p.value), ds::ui::Image::IMG_CACHE_F);
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["down_image_color"] = [](SprProps& p) {
			auto image = dynamic_cast<ImageButton*>(&p.sprite);
			if (image) {
				image->setHighImageColor(parseColor(p.value, p.engine));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["up_image_color"] = [](SprProps& p) {
			auto image = dynamic_cast<ImageButton*>(&p.sprite);
			if (image) {
				image->setNormalImageColor(parseColor(p.value, p.engine));
			} else {
				logAttributionWarning(p);
			}

		};
		propertyMap["btn_touch_padding"] = [](SprProps& p) {
			auto image = dynamic_cast<ImageButton*>(&p.sprite);
			if (image) {
				image->setTouchPad(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}			
		};

		// Gradient sprite properties
		propertyMap["colorTop"] = [](SprProps& p) {
			auto gradient = dynamic_cast<GradientSprite*>(&p.sprite);
			if (gradient) {
				DS_LOG_WARNING("DEPRECATION WARNING: colorTop from gradient sprites will soon be color_top. On sprite " << ds::utf8_from_wstr(p.sprite.getSpriteName(true)) << " from: " << p.referer);
				gradient->setColorsV(parseColor(p.value, p.engine), gradient->getColorBL());
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["color_top"] = [](SprProps& p) {
			auto gradient = dynamic_cast<GradientSprite*>(&p.sprite);
			if (gradient) {
				gradient->setColorsV(parseColor(p.value, p.engine), gradient->getColorBL());
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["colorBot"] = [](SprProps& p) {
			auto gradient = dynamic_cast<GradientSprite*>(&p.sprite);
			if (gradient) {
				DS_LOG_WARNING("DEPRECATION WARNING: change colorBot to color_bot. On sprite " << ds::utf8_from_wstr(p.sprite.getSpriteName(true)) << " from: " << p.referer);
				gradient->setColorsV(gradient->getColorTL(), parseColor(p.value, p.engine));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["color_bot"] = [](SprProps& p) {
			auto gradient = dynamic_cast<GradientSprite*>(&p.sprite);
			if (gradient) {
				gradient->setColorsV(gradient->getColorTL(), parseColor(p.value, p.engine));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["colorLeft"] = [](SprProps& p) {
			auto gradient = dynamic_cast<GradientSprite*>(&p.sprite);
			if (gradient) {
				DS_LOG_WARNING("DEPRECATION WARNING: change colorLeft to color_left. On sprite " << ds::utf8_from_wstr(p.sprite.getSpriteName(true)) << " from: " << p.referer);
				gradient->setColorsH(parseColor(p.value, p.engine), gradient->getColorTR());
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["color_left"] = [](SprProps& p) {
			auto gradient = dynamic_cast<GradientSprite*>(&p.sprite);
			if (gradient) {
				gradient->setColorsH(parseColor(p.value, p.engine), gradient->getColorTR());
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["colorRight"] = [](SprProps& p) {
			auto gradient = dynamic_cast<GradientSprite*>(&p.sprite);
			if (gradient) {
				DS_LOG_WARNING("DEPRECATION WARNING: change colorRight to color_right. On sprite " << ds::utf8_from_wstr(p.sprite.getSpriteName(true)) << " from: " << p.referer);
				gradient->setColorsH(gradient->getColorTL(), parseColor(p.value, p.engine));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["color_right"] = [](SprProps& p) {
			auto gradient = dynamic_cast<GradientSprite*>(&p.sprite);
			if (gradient) {
				gradient->setColorsH(gradient->getColorTL(), parseColor(p.value, p.engine));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["gradientColors"] = [](SprProps& p) {
			auto gradient = dynamic_cast<GradientSprite*>(&p.sprite);
			if (gradient) {
				DS_LOG_WARNING("DEPRECATION WARNING: change gradientColors to gradient_colors. On sprite " << ds::utf8_from_wstr(p.sprite.getSpriteName(true)) << " from: " << p.referer);
				auto colors = ds::split(p.value, ", ", true);
				if (colors.size() > 3) {
					auto colorOne = parseColor(colors[0], p.engine);
					auto colorTwo = parseColor(colors[1], p.engine);
					auto colorThr = parseColor(colors[2], p.engine);
					auto colorFor = parseColor(colors[3], p.engine);
					gradient->setColorsAll(colorOne, colorTwo, colorThr, colorFor);
				} else {
					logInvalidValue(p, "four colors separated by a comma and a space");
				}
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["gradient_colors"] = [](SprProps& p) {
			auto gradient = dynamic_cast<GradientSprite*>(&p.sprite);
			if (gradient) {
				auto colors = ds::split(p.value, ", ", true);
				if (colors.size() > 3) {
					auto colorOne = parseColor(colors[0], p.engine);
					auto colorTwo = parseColor(colors[1], p.engine);
					auto colorThr = parseColor(colors[2], p.engine);
					auto colorFor = parseColor(colors[3], p.engine);
					gradient->setColorsAll(colorOne, colorTwo, colorThr, colorFor);
				} else {
					logInvalidValue(p, "four colors separated by a comma and a space");
				}
			} else {
				logAttributionWarning(p);
			}
		};

		// Scroll sprite properties
		propertyMap["scroll_list_layout"] = [](SprProps& p) {
			auto scrollList = dynamic_cast<ds::ui::ScrollList*>(&p.sprite);
			if (scrollList) {
				auto vec = parseVector(p.value);
				scrollList->setLayoutParams(vec.x, vec.y, vec.z, true);
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["scroll_list_animate"] = [](SprProps& p) {
			auto scrollList = dynamic_cast<ds::ui::ScrollList*>(&p.sprite);
			if (scrollList) {
				auto vec = parseVector(p.value);
				scrollList->setAnimateOnParams(vec.x, vec.y);
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["scroll_fade_colors"] = [](SprProps& p) {
			auto				scrollList = dynamic_cast<ds::ui::ScrollList*>(&p.sprite);
			ds::ui::ScrollArea* scrollArea = nullptr;
			if (scrollList) {
				scrollArea = scrollList->getScrollArea();
			}

			if (!scrollArea) {
				scrollArea = dynamic_cast<ds::ui::ScrollArea*>(&p.sprite);
			}

			if (scrollArea) {
				auto colors = ds::split(p.value, ", ", true);
				if (colors.size() > 1) {
					auto colorOne = parseColor(colors[0], p.engine);
					auto colorTwo = parseColor(colors[1], p.engine);
					scrollArea->setFadeColors(colorOne, colorTwo);
				} else {
					logInvalidValue(p, "two colors separated by a comma and a space");
				}
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["scroll_area_vert"] = [](SprProps& p) {
			auto scrollArea = dynamic_cast<ds::ui::ScrollArea*>(&p.sprite);
			if (scrollArea) {
				auto vec = parseBoolean(p.value);
				scrollArea->setVertical(vec);
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["scroll_fade_size"] = [](SprProps& p) {
			auto				scrollList = dynamic_cast<ds::ui::ScrollList*>(&p.sprite);
			ds::ui::ScrollArea* scrollArea = nullptr;
			if (scrollList) {
				scrollArea = scrollList->getScrollArea();
			}

			if (!scrollArea) {
				scrollArea = dynamic_cast<ds::ui::ScrollArea*>(&p.sprite);
			}

			if (scrollArea) {
				scrollArea->setUseFades(true);
				scrollArea->setFadeHeight(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["scroll_shader_fade"] = [](SprProps& p) {
			auto				scrollList = dynamic_cast<ds::ui::ScrollList*>(&p.sprite);
			ds::ui::ScrollArea* scrollArea = nullptr;
			if (scrollList) {
				scrollArea = scrollList->getScrollArea();
			}

			if (!scrollArea) {
				scrollArea = dynamic_cast<ds::ui::ScrollArea*>(&p.sprite);
			}

			if (scrollArea) {
				auto fadesy = parseBoolean(p.value);
				scrollArea->setUseShaderFade(fadesy);
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["smart_scroll_item_layout"] = [](SprProps& p) {
			auto smartScrollList = dynamic_cast<ds::ui::SmartScrollList*>(&p.sprite);
			if (smartScrollList) {
				smartScrollList->setItemLayoutFile(p.value);
			} else {
				logAttributionWarning(p);
			}
		};

		propertyMap["scroll_bar_nub_color"] = [](SprProps& p) {
			auto scrollBar = dynamic_cast<ScrollBar*>(&p.sprite);
			if (scrollBar && scrollBar->getNubSprite()) {
				scrollBar->getNubSprite()->setColorA(parseColor(p.value, p.engine));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["scroll_bar_background_color"] = [](SprProps& p) {
			auto scrollBar = dynamic_cast<ScrollBar*>(&p.sprite);
			if (scrollBar && scrollBar->getBackgroundSprite()) {
				scrollBar->getBackgroundSprite()->setColorA(parseColor(p.value, p.engine));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["scroll_bar_corner_radius"] = [](SprProps& p) {
			auto scrollBar = dynamic_cast<ScrollBar*>(&p.sprite);
			if (scrollBar && scrollBar->getBackgroundSprite() && scrollBar->getNubSprite()) {
				scrollBar->getBackgroundSprite()->setCornerRadius(ds::string_to_float(p.value));
				scrollBar->getNubSprite()->setCornerRadius(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};

		// Border sprite properties
		propertyMap["border_width"] = [](SprProps& p) {
			auto border = dynamic_cast<Border*>(&p.sprite);
			if (border) {
				border->setBorderWidth(ds::string_to_float(p.value));
			} else {
				auto circle_border = dynamic_cast<CircleBorder*>(&p.sprite);
				if (circle_border) {
					circle_border->setBorderWidth(ds::string_to_float(p.value));
				} else {
					logAttributionWarning(p);
				}
			}
		};

		// Circle sprite properties
		propertyMap["filled"] = [](SprProps& p) {
			auto circle = dynamic_cast<Circle*>(&p.sprite);
			if (circle) {
				circle->setFilled(parseBoolean(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["radius"] = [](SprProps& p) {
			auto circle = dynamic_cast<Circle*>(&p.sprite);
			if (circle) {
				circle->setRadius(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["line_width"] = [](SprProps& p) {
			auto circle = dynamic_cast<Circle*>(&p.sprite);
			if (circle) {
				circle->setLineWidth(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["donut_width"] = [](SprProps& p) {
			auto donut = dynamic_cast<DonutArc*>(&p.sprite);
			if (donut) {
				donut->setDonutWidth(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["donut_percent"] = [](SprProps& p) {
			auto donut = dynamic_cast<DonutArc*>(&p.sprite);
			if (donut) {
				donut->setPercent(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["dash_length"] = [](SprProps& p) {
			auto dashed = dynamic_cast<DashedLine*>(&p.sprite);
			if (dashed) {
				dashed->setDashLength(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["dash_space_inc"] = [](SprProps& p) {
			auto dashed = dynamic_cast<DashedLine*>(&p.sprite);
			if (dashed) {
				dashed->setSpaceIncrement(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};

		/// Check box properties
		propertyMap["check_box_true_label"] = [](SprProps& p) {
			auto checkBox = dynamic_cast<ControlCheckBox*>(&p.sprite);
			if (checkBox) {
				checkBox->setTrueLabel(ds::wstr_from_utf8(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["check_box_false_label"] = [](SprProps& p) {
			auto checkBox = dynamic_cast<ControlCheckBox*>(&p.sprite);
			if (checkBox) {
				checkBox->setFalseLabel(ds::wstr_from_utf8(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["check_box_touch_pad"] = [](SprProps& p) {
			auto checkBox = dynamic_cast<ControlCheckBox*>(&p.sprite);
			if (checkBox) {
				checkBox->setTouchPadding(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["check_box_box_pad"] = [](SprProps& p) {
			auto checkBox = dynamic_cast<ControlCheckBox*>(&p.sprite);
			if (checkBox) {
				checkBox->setBoxPadding(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["check_box_label_pad"] = [](SprProps& p) {
			auto checkBox = dynamic_cast<ControlCheckBox*>(&p.sprite);
			if (checkBox) {
				checkBox->setLabelPadding(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};

		/// Persp layout
		propertyMap["persp_fov"] = [](SprProps& p) {
			auto perspLayout = dynamic_cast<ds::ui::PerspectiveLayout*>(&p.sprite);
			if (perspLayout) {
				perspLayout->setFov(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["persp_auto_clip"] = [](SprProps& p) {
			auto perspLayout = dynamic_cast<ds::ui::PerspectiveLayout*>(&p.sprite);
			if (perspLayout) {
				perspLayout->setAutoClip(ds::parseBoolean(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["persp_auto_clip_range"] = [](SprProps& p) {
			auto perspLayout = dynamic_cast<ds::ui::PerspectiveLayout*>(&p.sprite);
			if (perspLayout) {
				perspLayout->setAutoClipDepthRange(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["persp_near_clip"] = [](SprProps& p) {
			auto perspLayout = dynamic_cast<ds::ui::PerspectiveLayout*>(&p.sprite);
			if (perspLayout) {
				perspLayout->setNearClip(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["persp_far_clip"] = [](SprProps& p) {
			auto perspLayout = dynamic_cast<ds::ui::PerspectiveLayout*>(&p.sprite);
			if (perspLayout) {
				perspLayout->setFarClip(ds::string_to_float(p.value));
			} else {
				logAttributionWarning(p);
			}
		};
		propertyMap["persp_enabled"] = [](SprProps& p) {
			auto perspLayout = dynamic_cast<ds::ui::PerspectiveLayout*>(&p.sprite);
			if (perspLayout) {
				perspLayout->setPerspectiveEnabled(ds::parseBoolean(p.value));
			} else {
				logAttributionWarning(p);
			}
		};

		propertyMap["model"] = [](SprProps& p) {
			auto& ud = p.sprite.getUserData();
			if (p.sprite.getSpriteName(false).empty()) {
				logInvalidValue(p, "a sprite with a name set already");
			}
			ud.setString(p.property, p.value);

		};
		propertyMap["each_model"] = [](SprProps& p) {
			if (!p.sprite.getChildren().empty()) {
				DS_LOG_WARNING("Setting each_model on a sprite with children is risky, things might go wrong here! (Ignore for smart_scroll_list)");
			}
			p.sprite.getUserData().setString(p.property, p.value);
		};
		propertyMap["each_model_limit"] = [](SprProps& p) {
			p.sprite.getUserData().setInt(p.property, ds::string_to_int(p.value));
		};

	};

	// Find and apply the correct property
	auto findy = propertyMap.find(property);
	if (findy != propertyMap.end()) {
		findy->second(SprProps(sprite, property, value, referer, local_map));

	// a specific case where the filename and src properties can have flags with them, e.g. filename_cache_mipmap and src_preload_skipmeta_cache
	} else if (property.rfind("filename", 0) == 0 || property.rfind("src", 0) == 0) {
		int  flags = 0;
		if (property.find("_") != std::string::npos) {

			auto theFlags = ds::split(property, "_", true);
			for (auto val : theFlags) {

				if (val == "cache" || val == "c") {
					flags |= ds::ui::Image::IMG_CACHE_F;

				} else if (val == "mipmap" || val == "m") {
					flags |= ds::ui::Image::IMG_ENABLE_MIPMAP_F;

				} else if (val == "preload" || val == "p") {
					flags |= ds::ui::Image::IMG_PRELOAD_F;

				} else if (val == "skipmeta" || val == "s") {
					flags |= ds::ui::Image::IMG_SKIP_METADATA_F;

				} else if (val != "filename" && val != "src") {
					logInvalidValue(SprProps(sprite, property, value, referer, local_map), " cache, mipmap, preload, skipmeta in combination with filename or src, such as filename_cache or src_mipmap_preload");
				}
			}
		}

		auto imgBtn = dynamic_cast<ImageButton*>(&sprite);
		auto image = dynamic_cast<Image*>(&sprite);
		std::string filePath = value;
		if (!value.empty()) filePath = filePathRelativeTo(referer, value);
		if (image) {
			image->setImageFile(filePath, flags);
		} else if (imgBtn) {
			imgBtn->setNormalImage(filePath, flags);
			imgBtn->setHighImage(filePath, flags);
		} else {
			logAttributionWarning(SprProps(sprite, property, value, referer, local_map));
		}

	// fallback to the engine registered setting
	} else if (sprite.getEngine().setRegisteredSpriteProperty(property, sprite, value, referer)) {

	} else {
		logNotFoundWarning(SprProps(sprite, property, value, referer, local_map));
	}
}

void XmlImporter::dispatchStringEvents(const std::string& value, ds::ui::Sprite* bs, const ci::vec3& pos) {
	DS_LOG_VERBOSE(4, "XmlImporter: dispatchStringEvents value=" << value);

	auto leadingBracket = value.find("{");
	if (leadingBracket == 0) {
		auto events = ds::split(value, "},{", true);
		for (auto it : events) {
			ds::replace(it, "{", "");
			ds::replace(it, "}", "");
			dispatchSingleEvent(it, bs, pos);
		}
	} else {
		dispatchSingleEvent(value, bs, pos);
	}
}

void XmlImporter::dispatchSingleEvent(const std::string& value, ds::ui::Sprite* bs, const ci::vec3& globalPos) {
	DS_LOG_VERBOSE(4, "XmlImporter: dispatchSingleEvent value=" << value);

	auto tokens = ds::split(value, "; ", true);
	if (!tokens.empty()) {
		std::string eventName = tokens.front();
		ds::Event*  eventy	= ds::event::Registry::get().getEventCreator(eventName)();
		if (eventy->mWhat < 1) {
			DS_LOG_WARNING("XmlImporter::dispatchSingleEvent() Event not defined: " << eventName);
		}

		for (int i = 1; i < tokens.size(); i++) {
			auto colony = tokens[i].find(":");
			if (colony != std::string::npos) {
				std::string paramType  = tokens[i].substr(0, colony);
				std::string paramValue = tokens[i].substr(colony + 1);

				if (paramType.empty() || paramValue.empty()) continue;

				if (paramType == "data") {
					eventy->mUserStringData = paramValue;
				} else if (paramType == "id") {
					eventy->mUserId = ds::string_to_int(paramValue);
				} else if (paramType == "user_size") {
					eventy->mUserSize = parseVector(paramValue);
				} else if (paramType == "user_data") {
					auto click_data = bs->getUserData().getString(paramValue, 0, "");
					if (!click_data.empty()) {
						eventy->mUserStringData = click_data;
					}
				}
			}
		}
		eventy->mSpriteOriginator = bs;
		eventy->mEventOrigin	  = globalPos;
		bs->getEngine().getNotifier().notify(eventy);
	}
}

XmlImporter::~XmlImporter() {
	BOOST_FOREACH (auto s, mStylesheets) { delete s; }
}

bool XmlImporter::preloadXml(const std::string& filename, XmlPreloadData& outData) {
	DS_LOG_VERBOSE(3, "XmlImporter: preloadXml filename=" << filename);

	outData.mFilename = filename;
	try {
		if (!ds::safeFileExistsCheck(filename, false)) {
			DS_LOG_WARNING("XmlImporter file doesn't exist: " << filename);
			return false;
		}
		outData.mXmlTree = ci::XmlTree(cinder::loadFile(filename));
	} catch (ci::XmlTree::Exception& e) {
		DS_LOG_WARNING("XmlImporter doc " << filename << " not loaded!");
		if (e.what()) {
			DS_LOG_WARNING("XmlImporter load exception: " << e.what());
		}
		return false;
	}
	// Catch rapidxml::parse_errors too
	catch (std::exception& e) {
		DS_LOG_WARNING("XmlImporter doc " << filename << " not loaded!");
		if (e.what()) {
			DS_LOG_WARNING("XmlImporter load exception: " << e.what());
		}
		return false;
	}

	// Load the stylesheets
	auto iter = outData.mXmlTree.find("link");
	while (iter != outData.mXmlTree.end()) {
		auto node = *iter;
		iter++;
		if (node.getAttributeValue<std::string>("rel", "") != "stylesheet") {
			DS_LOG_WARNING("<link> tag specified without rel=\"stylesheet\", ignoring...");
			continue;
		}
		std::string stylesheet_file = node.getAttributeValue<std::string>("href", "");
		if (stylesheet_file == "") {
			DS_LOG_WARNING("<link> tag specified without an href attribute, ignoring...");
			continue;
		}

		// Load stylesheet file relative to the XML file
		stylesheet_file = filePathRelativeTo(filename, stylesheet_file);

		Stylesheet* s = new Stylesheet();
		if (!s->loadFile(stylesheet_file, filename)) {
			DS_LOG_WARNING("Error loading stylesheet: " << stylesheet_file);
			delete s;
		} else
			outData.mStylesheets.push_back(s);
	}

	// If automatically caching, add this to the cache
	if (AUTO_CACHE) {
		PRELOADED_CACHE[filename] = outData;
	}

	return true;
}

void XmlImporter::setAutoCache(const bool doCaching) {
	AUTO_CACHE = doCaching;
	if (!AUTO_CACHE) {
		PRELOADED_CACHE.clear();
	}
}

bool XmlImporter::loadXMLto(ds::ui::Sprite* parent, const std::string& filename, NamedSpriteMap& map,
							SpriteImporter customImporter, const std::string& prefixName, const bool mergeFirstChild, ds::cfg::Settings& override_map, ds::cfg::VariableMap local_map) {
	DS_LOG_VERBOSE(3, "XmlImporter: loadXMLto filename=" << filename << " prefix=" << prefixName);

	XmlImporter xmlImporter(parent, filename, map, customImporter, prefixName);

	XmlPreloadData preloadData;

	// if auto caching, look up the xml in the static cache
	bool cachedAlready = false;
	if (AUTO_CACHE) {
		auto xmlIt = PRELOADED_CACHE.find(filename);
		if (xmlIt != PRELOADED_CACHE.end()) {
			cachedAlready = true;
			preloadData   = xmlIt->second;
		}
	}

	// we don't have this in our cache, so look it up
	if (!cachedAlready) {
		preloadData.mFilename = filename;
		if (!preloadXml(filename, preloadData)) {
			return false;
		}
	}

	// copy each stylesheet, cause the xml importer will delete it's copies when it destructs
	for (auto it = preloadData.mStylesheets.begin(); it < preloadData.mStylesheets.end(); ++it) {
		Stylesheet* ss = new Stylesheet();
		ss->mRules	 = (*it)->mRules;
		ss->mReferer   = (*it)->mReferer;
		xmlImporter.mStylesheets.push_back(ss);
	}

	return xmlImporter.load(preloadData.mXmlTree, mergeFirstChild,override_map,local_map);
}

bool XmlImporter::loadXMLto(ds::ui::Sprite* parent, XmlPreloadData& preloadData, NamedSpriteMap& map,
							SpriteImporter customImporter, const std::string& prefixName, const bool mergeFirstChild, ds::cfg::Settings& override_map, ds::cfg::VariableMap local_map) {
	DS_LOG_VERBOSE(3, "XmlImporter: loadXMLto preloaded filename=" << preloadData.mFilename << " prefix=" << prefixName);
	XmlImporter xmlImporter(parent, preloadData.mFilename, map, customImporter, prefixName);

	// copy each stylesheet, cause the xml importer will delete it's copies when it destructs
	for (auto it = preloadData.mStylesheets.begin(); it < preloadData.mStylesheets.end(); ++it) {
		Stylesheet* ss = new Stylesheet();
		ss->mRules	 = (*it)->mRules;
		ss->mReferer   = (*it)->mReferer;
		xmlImporter.mStylesheets.push_back(ss);
	}

	return xmlImporter.load(preloadData.mXmlTree, mergeFirstChild,override_map,local_map);
}


bool XmlImporter::load(ci::XmlTree& xml, const bool mergeFirstChild, ds::cfg::Settings& override_map, ds::cfg::VariableMap local_map) {
	if (!xml.hasChild("interface")) {
		DS_LOG_WARNING("No interface found in xml file: " << mXmlFile);
		return false;
	}
	
	auto   interface = xml.getChild("interface");
	ds::cfg::VariableMap new_local_map;
	
	//if this file has a settings block grab it. 
	//then we merge with the incomming override_map, 
	//which should be an empty map in all cases except if
	//this file is being loaded by an <xml> tag
	auto settings = ds::cfg::Settings();
	if (interface.hasChild("settings")) {
		settings.readFrom(interface, mXmlFile, true);
	}

	settings.mergeSettings(override_map);
	settings.replaceSettingVariablesAndExpressions();

	//covert the setting into a variable map.
	settings.forEachSetting([&new_local_map, &local_map](const ds::cfg::Settings::Setting& theSetting) {
		if (!theSetting.mName.empty()) {
			new_local_map[theSetting.mName] = theSetting.mRawValue;
		}
	});
	
	//give the target sprite the settings so that they can be referenced later.
	mTargetSprite->setLayoutSettings(settings);

	//combine the current variable map with the new one.
	if (local_map.size() > 0) {
		new_local_map.insert(local_map.begin(), local_map.end());
		mCombinedSettings = new_local_map;
	}
	else {
		mCombinedSettings = ds::cfg::SettingsVariables::insertAppToLocal(new_local_map);
	}
	
	
	auto&  sprites   = interface.getChildren();
	size_t count = 0;
	/*
	size_t count	 = sprites.size();
	if (count < 1) {
		DS_LOG_WARNING("No sprites found in xml file: " << mXmlFile);
		return false;
	}
	*/

	bool mergeFirst = mergeFirstChild;

	count = 0;
	BOOST_FOREACH (auto& xmlNode, sprites) {
		if (xmlNode->getTag() != "settings") {
			readSprite(mTargetSprite, xmlNode, mergeFirst);
			mergeFirst = false;
			count++;
		}
	}

	if (count < 1) {
		DS_LOG_WARNING("No sprites found in xml file: " << mXmlFile);
		return false;
	}

	for (auto it : mSpriteLinks) {
		auto findy = mNamedSpriteMap.find(it.second);
		if (findy != mNamedSpriteMap.end()) {
			if (it.first && findy->second) {
				EntryField*   ef  = dynamic_cast<EntryField*>(it.first);
				SoftKeyboard* sfk = dynamic_cast<SoftKeyboard*>(findy->second);
				if (ef && sfk) {
					sfk->setKeyPressFunction([ef](const std::wstring& character, ds::ui::SoftKeyboardDefs::KeyType keyType) {
						if (ef) {
							ef->keyPressed(character, keyType);
						}
					});
				} else {
					ScrollBar* sb = dynamic_cast<ScrollBar*>(it.first);
					ScrollList* sl = dynamic_cast<ScrollList*>(findy->second);
					ScrollArea* sa = dynamic_cast<ScrollArea*>(findy->second);
					if(sb && sl){
						sb->linkScrollList(sl);
					}

					if(sb && sa){
						sb->linkScrollArea(sa);
					}
				}
			}
		}
	}

	return true;
}

struct SelectorMatchChecker : public boost::static_visitor<bool> {
	SelectorMatchChecker(const std::vector<std::string>& classesToCheck, const std::string& idToCheck)
	  : mClassesToCheck(classesToCheck)
	  , mIdToCheck(idToCheck) {}
	bool operator()(const ds::ui::stylesheets::IdSelector& s) const { return mIdToCheck == s.selector; }
	bool operator()(const ds::ui::stylesheets::ClassSelector& s) const {
		return (std::find(mClassesToCheck.begin(), mClassesToCheck.end(), s.selector) != mClassesToCheck.end());
	}

	const std::vector<std::string>& mClassesToCheck;
	const std::string&				mIdToCheck;
};

static void applyStylesheet(const Stylesheet& stylesheet, ds::ui::Sprite& sprite, const std::string& name,
							const std::string& classes) {

	DS_LOG_VERBOSE(
		3, "XmlImporter: applyStylesheet stylesheet=" << stylesheet.mReferer << " name=" << name << " classes=" << classes);

	BOOST_FOREACH (auto& rule, stylesheet.mRules) {
		auto classes_vec  = ds::split(classes, " ", true);
		bool matches_rule = false;
		BOOST_FOREACH (auto& matcher, rule.matchers) {

			// Iterate through .class_rules and #name(id)_rules
			// ALL the sub-matchers have to match for this matcher to match
			bool all_submatchers_match = true;
			BOOST_FOREACH (auto& selector, matcher) {
				if (!boost::apply_visitor(SelectorMatchChecker(classes_vec, name), selector)) {
					all_submatchers_match = false;
					break;
				}
			}
			matches_rule = all_submatchers_match;
			if (matches_rule) break;
		}

		if (matches_rule) {
			BOOST_FOREACH (auto& prop, rule.properties) {
				cinder::XmlTree::Attr attr(nullptr, prop.property_name, prop.property_value);
				XmlImporter::setSpriteProperty(sprite, attr, stylesheet.mReferer);
			}
		}
	}
}


std::string XmlImporter::getSpriteTypeForSprite(ds::ui::Sprite* sp) {
	if (dynamic_cast<ds::ui::LayoutSprite*>(sp)) return "layout";
	if (dynamic_cast<ds::ui::SpriteButton*>(sp)) return "sprite_button";
	if (dynamic_cast<ds::ui::LayoutButton*>(sp)) return "layout_button";
	if (dynamic_cast<ds::ui::ImageButton*>(sp)) return "image_button";
	if (dynamic_cast<ds::ui::ImageWithThumbnail*>(sp)) return "image_with_thumbnail";
	if (dynamic_cast<ds::ui::Image*>(sp)) return "image";
	if (dynamic_cast<ds::ui::Text*>(sp)) return "text";
	if (dynamic_cast<ds::ui::ScrollBar*>(sp)) return "scroll_bar";
	if (dynamic_cast<ds::ui::ScrollList*>(sp)) return "scroll_list";
	if (dynamic_cast<ds::ui::ScrollArea*>(sp)) return "scroll_area";
	if (dynamic_cast<ds::ui::CenteredScrollArea*>(sp)) return "centered_scroll_area";
	if (dynamic_cast<ds::ui::Circle*>(sp)) return "circle";
	if (dynamic_cast<ds::ui::Border*>(sp)) return "border";
	if (dynamic_cast<ds::ui::CircleBorder*>(sp)) return "circle_border";
	if (dynamic_cast<ds::ui::Gradient*>(sp)) return "gradient";
	if (dynamic_cast<ds::ui::SoftKeyboard*>(sp)) return "soft_keyboard";
	if (dynamic_cast<ds::ui::EntryField*>(sp)) return "entry_field";
	if (dynamic_cast<ds::ui::PerspectiveLayout*>(sp)) return "persp_layout";
	if (dynamic_cast<ds::ui::ControlSlider*>(sp)) {
		auto slider = dynamic_cast<ds::ui::ControlSlider*>(sp);
		if (slider->getIsVertical()) {
			return "control_slider_vertical";
		} else {
			return "control_slider_horizontal";
		}
		return "control_slider";
	}
	if (dynamic_cast<ds::ui::ControlCheckBox*>(sp)) return "control_check_box";
	return "sprite";
}

// NOTE! If you add a sprite below, please add it above! Thanks, byeeee!
ds::ui::Sprite* XmlImporter::createSpriteByType(ds::ui::SpriteEngine& engine, const std::string& type, const std::string& value) {
	DS_LOG_VERBOSE(4, "XmlImporter: createSpriteByType type=" << type << " value=" << value);

	ds::ui::Sprite* spriddy = nullptr;

	if (type == "sprite") {
		spriddy = new ds::ui::Sprite(engine);
	} else if (type == "image") {
		auto		image		  = new ds::ui::Image(engine);
		std::string relative_file = value;
		boost::trim(relative_file);
		if (relative_file != "") {
			setSpriteProperty(*image, ci::XmlTree::Attr(nullptr, "filename", relative_file), nullptr);
		}
		spriddy = image;
	} else if (type == "image_with_thumbnail") {
		auto image = new ds::ui::ImageWithThumbnail(engine);
		spriddy	= image;
	} else if (type == "text" || type == "multiline_text") {
		auto text	= new ds::ui::Text(engine);
		auto content = value;
		boost::trim(content);
		text->setText(content);
		spriddy = text;
	} else if (type == "image_button") {
		auto content = value;
		boost::trim(content);
		float touchPad = 0.0f;
		if (content.size() > 0) touchPad = (float)atof(content.c_str());
		auto imgButton = new ds::ui::ImageButton(engine, "", "", touchPad);
		spriddy		   = imgButton;
	} else if (type == "sprite_button") {
		spriddy = new ds::ui::SpriteButton(engine);
	} else if (type == "layout_button") {
		spriddy = new ds::ui::LayoutButton(engine);
	} else if (type == "gradient") {
		auto gradient = new ds::ui::GradientSprite(engine);
		spriddy		  = gradient;
	} else if (type == "layout") {
		auto layoutSprite = new ds::ui::LayoutSprite(engine);
		spriddy			  = layoutSprite;
	} else if (type == "border") {
		spriddy = new ds::ui::Border(engine);
	} else if (type == "circle") {
		spriddy = new ds::ui::Circle(engine);
	} else if (type == "circle_border") {
		spriddy = new ds::ui::CircleBorder(engine);
	} else if (type == "scroll_list" || type == "scroll_list_vertical") {
		spriddy = new ds::ui::ScrollList(engine);
	} else if (type == "scroll_list_horizontal") {
		spriddy = new ds::ui::ScrollList(engine, false);
	} else if (type == "scroll_area") {
		spriddy = new ds::ui::ScrollArea(engine, 0.0f, 0.0f);
	} else if (type == "centered_scroll_area") {
		spriddy = new ds::ui::CenteredScrollArea(engine, 0.0f, 0.0f);
	} else if (type == "smart_scroll_list" || type == "smart_scroll_list_vertical") {
		spriddy = new ds::ui::SmartScrollList(engine, value);
	} else if (type == "smart_scroll_list_horizontal") {
		spriddy = new ds::ui::SmartScrollList(engine, value, false);
	} else if (type == "control_check_box") {
		spriddy = new ds::ui::ControlCheckBox(engine);
	} else if (type == "control_slider" || type == "control_slider_horizontal") {
		spriddy = new ds::ui::ControlSlider(engine, false);
	} else if (type == "control_slider_vertical") {
		spriddy = new ds::ui::ControlSlider(engine, true);
	} else if (type == "donut_arc") {
		spriddy = new ds::ui::DonutArc(engine);
	} else if (type == "dashed_line") {
		spriddy = new DashedLine(engine);
	} else if (type == "scroll_bar") {
		spriddy = new ds::ui::ScrollBar(engine);
	} else if (type == "persp_layout") {
		spriddy = new ds::ui::PerspectiveLayout(engine);
	} else if (type == "soft_keyboard") {
		SoftKeyboardSettings sks;
		auto				 tokens		  = ds::split(value, "; ", true);
		std::string			 keyboardType = "standard";
		for (auto it : tokens) {
			auto colony = it.find(":");
			if (colony != std::string::npos) {
				std::string paramType  = it.substr(0, colony);
				std::string paramValue = it.substr(colony + 1);
				if (paramType.empty() || paramValue.empty()) continue;

				if (paramType == "type") {
					keyboardType = paramValue;
				} else if (paramType == "key_up_text_config") {
					sks.mKeyUpTextConfig = paramValue;
				} else if (paramType == "key_dn_text_config") {
					sks.mKeyDnTextConfig = paramValue;
				} else if (paramType == "key_up_color") {
					sks.mKeyUpColor = parseColor(paramValue, engine);
				} else if (paramType == "key_down_color") {
					sks.mKeyDownColor = parseColor(paramValue, engine);
				} else if (paramType == "key_text_offset") {
					sks.mKeyTextOffset = ci::vec2(parseVector(paramValue));
				} else if (paramType == "key_touch_padding") {
					sks.mKeyTouchPadding = ds::string_to_float(paramValue);
				} else if (paramType == "key_initial_position") {
					sks.mKeyInitialPosition = ci::vec2(parseVector(paramValue));
				} else if (paramType == "key_scale") {
					sks.mKeyScale = ds::string_to_float(paramValue);
				} else if (paramType == "img_letter_up") {
					sks.mKeyLetterUpImage = ds::Environment::expand(paramValue);
				} else if (paramType == "img_letter_dn") {
					sks.mKeyLetterDnImage = ds::Environment::expand(paramValue);
				} else if (paramType == "img_number_up") {
					sks.mKeyNumberUpImage = ds::Environment::expand(paramValue);
				} else if (paramType == "img_number_dn") {
					sks.mKeyNumberDnImage = ds::Environment::expand(paramValue);
				} else if (paramType == "img_space_up") {
					sks.mKeySpaceUpImage = ds::Environment::expand(paramValue);
				} else if (paramType == "img_space_dn") {
					sks.mKeySpaceDnImage = ds::Environment::expand(paramValue);
				} else if (paramType == "img_enter_up") {
					sks.mKeyEnterUpImage = ds::Environment::expand(paramValue);
				} else if (paramType == "img_enter_dn") {
					sks.mKeyEnterDnImage = ds::Environment::expand(paramValue);
				} else if (paramType == "img_delete_up") {
					sks.mKeyDeleteUpImage = ds::Environment::expand(paramValue);
				} else if (paramType == "img_delete_dn") {
					sks.mKeyDeleteDnImage = ds::Environment::expand(paramValue);
				} else if (paramType == "img_shift_up") {
					sks.mKeyShiftUpImage = ds::Environment::expand(paramValue);
				} else if (paramType == "img_shift_dn") {
					sks.mKeyShiftDnImage = ds::Environment::expand(paramValue);
				} else if (paramType == "img_tab_up") {
					sks.mKeyTabUpImage = ds::Environment::expand(paramValue);
				} else if (paramType == "img_tab_dn") {
					sks.mKeyTabDnImage = ds::Environment::expand(paramValue);
				} else if (paramType == "img_up_none") {
					if (parseBoolean(paramValue)) {
						sks.mKeyLetterUpImage = "";
						// sks.mKeyLetterDnImage = "";
						sks.mKeyNumberUpImage = "";
						// sks.mKeyNumberDnImage = "";
						sks.mKeySpaceUpImage = "";
						// sks.mKeySpaceDnImage = "";
						sks.mKeyEnterUpImage = "";
						// sks.mKeyEnterDnImage = "";
						sks.mKeyDeleteUpImage = "";
						// sks.mKeyDeleteDnImage = "";
						sks.mKeyShiftUpImage = "";
						// sks.mKeyShiftDnImage = "";
						sks.mKeyTabUpImage = "";
						// sks.mKeyTabDnImage = "";
					}
				} else if (paramType == "email_mode") {
					sks.mEmailMode = parseBoolean(paramValue);
				} else if(paramType == "graphic_keys") {
					sks.mGraphicKeys = parseBoolean(paramValue);
				} else if(paramType == "graphic_type") {
					if(paramValue == "solid") {
						sks.mGraphicType = SoftKeyboardSettings::kSolid;
					} else if(paramValue == "circular_border") {
						sks.mGraphicType = SoftKeyboardSettings::kCircularBorder;
					} else if(paramValue == "circular_solid") {
						sks.mGraphicType = SoftKeyboardSettings::kCircularSolid;
					} else {
						sks.mGraphicType = SoftKeyboardSettings::kBorder;
					}
				} else if(paramType == "graphic_key_size") {
					sks.mGraphicKeySize = ds::string_to_float(paramValue);
				} else if(paramType == "graphic_corner_radius") {
					sks.mGraphicRoundedCornerRadius = ds::string_to_float(paramValue);
				} else if(paramType == "graphic_border_width") {
					sks.mGraphicBorderWidth = ds::string_to_float(paramValue);
				} else {
					DS_LOG_WARNING("SoftKeyboard interface setting not recognized: " << paramType);
				}
			}
		}
		if (keyboardType == "lowercase") {
			spriddy = SoftKeyboardBuilder::buildLowercaseKeyboard(engine, sks);
		} else if (keyboardType == "uppercase") {
			spriddy = SoftKeyboardBuilder::buildUppercaseKeyboard(engine, sks);
		} else if (keyboardType == "pinpad") {
			spriddy = SoftKeyboardBuilder::buildPinPadKeyboard(engine, sks);
		} else if (keyboardType == "pincode") {
			spriddy = SoftKeyboardBuilder::buildPinCodeKeyboard(engine, sks);
		} else if (keyboardType == "extended") {
			spriddy = SoftKeyboardBuilder::buildExtendedKeyboard(engine, sks);
		} else if (keyboardType == "simplified") {
			spriddy = SoftKeyboardBuilder::buildSimplifiedKeyboard(engine, sks);
		} else {
			spriddy = SoftKeyboardBuilder::buildStandardKeyboard(engine, sks);
		}
	} else if (type == "entry_field") {
		EntryFieldSettings efs;
		auto			   tokens = ds::split(value, "; ", true);
		for (auto it : tokens) {
			auto colony = it.find(":");
			if (colony != std::string::npos) {
				std::string paramType  = it.substr(0, colony);
				std::string paramValue = it.substr(colony + 1);

				if (paramType.empty() || paramValue.empty()) continue;
				if (paramType == "text_config") {
					efs.mTextConfig = paramValue;
				} else if (paramType == "cursor_size") {
					efs.mCursorSize = ci::vec2(parseVector(paramValue));
				} else if (paramType == "field_size") {
					efs.mFieldSize = ci::vec2(parseVector(paramValue));
				} else if (paramType == "cursor_offset") {
					efs.mCursorOffset = ci::vec2(parseVector(paramValue));
				} else if (paramType == "cursor_color") {
					efs.mCursorColor = parseColor(paramValue, engine);
				} else if (paramType == "blink_rate") {
					efs.mBlinkRate = ds::string_to_float(paramValue);
				} else if (paramType == "animate_rate") {
					efs.mAnimationRate = ds::string_to_float(paramValue);
				} else if (paramType == "password_mode") {
					efs.mPasswordMode = parseBoolean(paramValue);
				} else if (paramType == "text_offset") {
					efs.mTextOffset = ci::vec2(parseVector(paramValue));
				} else if (paramType == "search_mode") {
					efs.mSearchMode = parseBoolean(paramValue);
				} else if (paramType == "auto_resize") {
					efs.mAutoResize = parseBoolean(paramValue);
				}
			}
		}

		auto ef = new EntryField(engine, efs);
		ef->focus();
		spriddy = ef;
	}

	return spriddy;
}

bool XmlImporter::readSprite(ds::ui::Sprite* parent, std::unique_ptr<ci::XmlTree>& node, const bool mergeFirstSprite) {
	if (!parent) {
		DS_LOG_WARNING("No parent sprite specified when reading a sprite from xml file=" << mXmlFile);
		return false;
	}

	std::string type   = node->getTag();
	std::string value  = node->getValue();
	auto&		engine = parent->getEngine();

	std::string layout_target = engine.getLayoutTarget();
	//if the node has a target attribute it should honor that.
	if (node->hasAttribute("target"))
	{
		if (!engine.hasLayoutTarget(node->getAttributeValue<std::string>("target")))
		{
			return true;
		}
	}

	DS_LOG_VERBOSE(6, "XmlImporter: readSprite type=" << type << " value=" << value);
	if (type == "xml") {
		std::string xmlPath = filePathRelativeTo(mXmlFile, node->getAttributeValue<std::string>("src", ""));
		if (xmlPath.empty()) {
			DS_LOG_WARNING("XmlImporter: Recursive XML: Specify a src parameter to load xml in " << mXmlFile);
			return false;
		}
		if (xmlPath == mXmlFile) {
			DS_LOG_WARNING("XmlImporter: Recursive XML: You cannot load the same xml from the same xml in " << mXmlFile);
			return false;
		}

		// Apply dot naming scheme
		std::string spriteName = node->getAttributeValue<std::string>("name", "");
		if (!mNamePrefix.empty()) {
			std::stringstream ss;
			ss << mNamePrefix << "." << spriteName;
			spriteName = ss.str();
		}

		//get xml settings override
		//these need to be merged with the settings file
		//in the incomming xml doc. with these taking precedent.
		ds::cfg::Settings override_map;
		if (node->hasChild("settings")) {
			std::stringstream ss;
			ss << mXmlFile << ":xml:" << spriteName;
			override_map.readFrom(*node, ss.str(), true);
		}


		if (!XmlImporter::loadXMLto(parent, xmlPath, mNamedSpriteMap, mCustomImporter, spriteName, mergeFirstSprite, override_map, mCombinedSettings)) {
			return false;
		}

		// Use child nodes of the "xml" node to set children properties
		BOOST_FOREACH (auto& newNode, node->getChildren()) {
			if (newNode->getTag() == "property") {

				//if the property node has a target attribute it should honor that.
				if (newNode->hasAttribute("target"))
				{
					auto target = newNode->getAttributeValue<std::string>("target");
					if (!engine.hasLayoutTarget(target))
					{
						continue;
					}
				}
				
				std::string		  childSpriteName = newNode->getAttributeValue<std::string>("name", "");
				std::stringstream ss;
				ss << spriteName << "." << childSpriteName;
				childSpriteName = ss.str();

				ds::ui::Sprite* spriddy = nullptr;

				for (auto it = mNamedSpriteMap.begin(); it != mNamedSpriteMap.end(); ++it) {
					if (it->first == childSpriteName) {
						spriddy = it->second;
					}
				}

				if (spriddy) {
					BOOST_FOREACH (auto& attr, newNode->getAttributes()) {
						if (attr.getName() == "name") continue;  // don't overwrite the name
						setSpriteProperty(*spriddy, attr, xmlPath,mCombinedSettings);
					}
				} else {
					DS_LOG_WARNING("XmlImporter: Recursive XML: Couldn't find a child with the name "
								   << childSpriteName << " to apply properties to");
				}
			}
			else if (newNode->getTag() == "settings") {
				//we skip the settings.
			} else {
				DS_LOG_WARNING("XmlImporter: Recursive XML: Regular children are not supported in recursive xml. Tagname="
							   << newNode->getTag());
			}
		}


	} else {
		ds::ui::Sprite* spriddy = nullptr;
		if (mergeFirstSprite) {
			auto parentType = getSpriteTypeForSprite(parent);
			if (parentType == type) {
				spriddy = parent;
			}
		}

		if (!spriddy) {
			spriddy = createSpriteByType(engine, type, value);
		}

		if (!spriddy) {
			spriddy = engine.createSpriteImporter(type);
		}

		if (!spriddy && mCustomImporter) {
			spriddy = mCustomImporter(type, *node);
		}

		if (!spriddy) {
			DS_LOG_WARNING("Error creating sprite! Type=" << type);
			return false;
		}

		BOOST_FOREACH (auto& sprite, node->getChildren())
		{
			if(sprite->getTag() != "override")
			{
				readSprite(spriddy, sprite, false);
			}
		}

		std::string linkValue = node->getAttributeValue<std::string>("sprite_link", "");

		if (!linkValue.empty()) {
			mSpriteLinks[spriddy] = linkValue;
		}

		ds::ui::ScrollArea*   parentScroll = dynamic_cast<ds::ui::ScrollArea*>(parent);
		ds::ui::SpriteButton* spriteButton = dynamic_cast<ds::ui::SpriteButton*>(parent);
		ds::ui::LayoutButton* layoutButton = dynamic_cast<ds::ui::LayoutButton*>(parent);
		if (parentScroll) {
			parentScroll->addSpriteToScroll(spriddy);
		} else if (spriteButton || layoutButton) {
			std::string attachState = node->getAttributeValue<std::string>("attach_state", "");
			if (attachState.empty()) {
				parent->addChildPtr(spriddy);
			} else if (attachState == "normal") {
				if (spriteButton) {
					spriteButton->getNormalSprite().addChildPtr(spriddy);
				} else if (layoutButton) {
					layoutButton->getNormalSprite().addChildPtr(spriddy);
				}
			} else if (attachState == "high") {
				if (spriteButton) {
					spriteButton->getHighSprite().addChildPtr(spriddy);
				} else if (layoutButton) {
					layoutButton->getHighSprite().addChildPtr(spriddy);
				}
			}
			if (spriteButton) {
				spriteButton->showUp();
			} else if (layoutButton) {
				layoutButton->showUp();
			}
		} else if (parent != spriddy) {
			parent->addChildPtr(spriddy);
		}

		// Get sprite name and classes
		std::string sprite_name	= node->getAttributeValue<std::string>("name", "");
		std::string sprite_classes = node->getAttributeValue<std::string>("class", "");

		// Apply stylesheet(s)
		BOOST_FOREACH (auto stylesheet, mStylesheets) { applyStylesheet(*stylesheet, *spriddy, sprite_name, sprite_classes); }
		
		// Set properties from xml attributes, overwriting those from the stylesheet(s)
		BOOST_FOREACH (auto& attr, node->getAttributes()) { if(attr.getName() != "target") setSpriteProperty(*spriddy, attr, mXmlFile, mCombinedSettings); }

		//apply the overrides
		BOOST_FOREACH(auto& override_, node->getChildren())
		{
			if (override_->getTag() == "override")
			{
				if (override_->hasAttribute("target"))
				{
					if (!engine.hasLayoutTarget(override_->getAttributeValue<std::string>("target")))
					{
						continue;
					}
				}
				for(auto& attr: override_->getAttributes())
				{
					if (attr.getName() != "target") {
						setSpriteProperty(*spriddy, attr, mXmlFile, mCombinedSettings);
					}
				}
			}
		}

		// Put sprite in named sprites map
		if (sprite_name != "") {

			if (!mNamePrefix.empty()) {
				std::stringstream ss;
				ss << mNamePrefix << "." << sprite_name;
				sprite_name = ss.str();
			}

			spriddy->setSpriteName(ds::wstr_from_utf8(sprite_name));

			if (mNamedSpriteMap.find(sprite_name) != mNamedSpriteMap.end()) {
 				DS_LOG_WARNING("Interface xml file " << mXmlFile <<" contains duplicate sprites named:"
							   << sprite_name << ", only the first one will be identified.");
			} else {
				mNamedSpriteMap.insert(std::make_pair(sprite_name, spriddy));
			}
		}
	}


	return true;
}

}  // namespace ui
}  // namespace ds
