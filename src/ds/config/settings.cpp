#include "ds/config/settings.h"

#include <cinder/Xml.h>
#include <Poco/File.h>
#include <Poco/String.h>
#include "ds/debug/debug_defines.h"

static bool check_bool(const std::string& text, const bool defaultValue);

namespace ds {

namespace cfg {

static const std::string		FALSE_SZ("false");
static const std::string		TRUE_SZ("true");

static const std::string		  COLOR_NAME("color");
static const std::string		  FLOAT_NAME("float");
static const std::string		  INT_NAME("int");
static const std::string		  RECT_NAME("rect");
static const std::string		  RESOURCE_ID_NAME("resource id");
static const std::string		  SIZE_NAME("size");
static const std::string		  TEXT_NAME("text");
static const std::string		  TEXTW_NAME("wtext");
static const std::string		  POINT_NAME("point");

static const cinder::Color    COLOR_TYPE;
static const cinder::ColorA   COLORA_TYPE;
static const float				    FLOAT_TYPE(0.0f);
static const int				      INT_TYPE(0);
static const ci::Rectf        RECT_TYPE;
static const Resource::Id     RESOURCE_ID_TYPE;
static const ci::Vec2f        SIZE_TYPE;
static const std::string      TEXT_TYPE;
static const std::wstring     TEXTW_TYPE;
static const ci::Vec3f        POINT_TYPE;

namespace {

// A is a map between a string and a vector
template <typename A>
static int get_size(const std::string& name, A& container)
{
	auto it = container.find(name);
	if (it != container.end()) return it->second.size();
	return 0;
}

// A is a map between a string and a templatized vector, whose type matches V
template <typename A, typename V>
static void add_item(const std::string& name, A& container, const V& value)
{
	auto it = container.find(name);
	if (it != container.end()) {
		it->second.push_back(value);
	} else {
		std::vector<V>		vec;
		vec.push_back(value);
		container[name] = vec;
	}
}

// V = std::map<std::string, V>
template <typename V>
static void merge(V& dst, const V& src)
{
	for (auto it=src.begin(), end=src.end(); it != end; ++it) {
		dst[it->first] = it->second;
	}
}

// V = std::map<std::string, std::vector<V>>
template <typename V>
static void merge_vec(V& dst, const V& src)
{
	for (auto it=src.begin(), end=src.end(); it != end; ++it) {
		auto f = dst.find(it->first);
		if (f == dst.end()) {
			dst[it->first] = it->second;
		} else {
			for (std::size_t k=0; k<it->second.size(); k++) {
				if (k <= f->second.size()) f->second[k] = it->second[k];
				else f->second.push_back(it->second[k]);
			}
		}
	}
}

template <typename A, typename V>
const V& get_or_throw(const std::string& name, const A& container, const int index, const V&, const std::string& typeName)
{
	auto it = container.find(name);
	if (it != container.end() && index >= 0 && index < (int)it->second.size()) return it->second[index];
	throw std::logic_error("Settings " + typeName + " (" + name + ") does not exist");
}

template <typename A, typename V>
const V& get(const std::string& name, const A& container, const int index, const V& defaultValue, const std::string& typeName)
{
	auto it = container.find(name);
	if (it != container.end() && index >= 0 && index < (int)it->second.size()) return it->second[index];
	return defaultValue;
}

} // namespace

/**
 * ds::cfg::Settings
 */
Settings::Settings()
{
}

void Settings::readFrom(const std::string& filename, const bool append)
{
	if (!append) {
		directReadFrom(filename, true);
		return;
	}

	// We're appending, so create a temporary object, then merge all the changes
	// on top of me.
	Settings		s;
	s.directReadFrom(filename, false);

	merge(mFloat, s.mFloat);
	merge(mRect, s.mRect);
	merge_vec(mInt, s.mInt);
	merge_vec(mRes, s.mRes);
	merge_vec(mColor, s.mColor);
	merge_vec(mColorA, s.mColorA);
	merge(mSize, s.mSize);
	merge_vec(mText, s.mText);
	merge_vec(mTextW, s.mTextW);
}

void Settings::directReadFrom(const std::string& filename, const bool clearAll)
{
  if (filename.empty()) {
    return;
  }

  // Load based on the file format.
  try {
    const cinder::fs::path  extPath(cinder::fs::path(filename).extension());
    const std::string       ext(Poco::toLower(extPath.generic_string()));
    if (ext == ".xml") {
      directReadXmlFrom(filename, clearAll);
    } else {
      throw std::exception("unsupported format");
    }
  } catch (std::exception const& ex) {
    // TODO:  Really need this writing to a log file, because it easily happens during construction of the app
    std::cout << "ds::cfg::Settings::directReadFrom() failed on " << filename << " exception=" << ex.what() << std::endl;
  }
}

void Settings::directReadXmlFrom(const std::string& filename, const bool clearAll)
{
  if (!Poco::File(filename).exists()) return;
  cinder::XmlTree     xml(cinder::loadFile(filename));

  if (clearAll) clear();

  // GENERIC DEFINES
  const std::string   NAME_SZ("name");
  const std::string   VALUE_SZ("value");
  const std::string   L_SZ("l");
  const std::string   T_SZ("t");
  const std::string   R_SZ("r");
  const std::string   B_SZ("b");
  const std::string   G_SZ("g");
  const std::string   A_SZ("a");
  const std::string   X_SZ("x");
  const std::string   Y_SZ("y");
  const std::string   Z_SZ("z");

  // FLOAT
  const std::string   FLOAT_PATH("settings/float");
  auto                end = xml.end();
  for (auto it = xml.begin(FLOAT_PATH); it != end; ++it) {
    const std::string name = it->getAttributeValue<std::string>(NAME_SZ);
    add_item(name, mFloat, it->getAttributeValue<float>(VALUE_SZ));
  }

  // RECT
  const std::string   RECT_PATH("settings/rect");
  for (auto it = xml.begin(RECT_PATH); it != end; ++it) {
    const std::string   name = it->getAttributeValue<std::string>(NAME_SZ);
    const cinder::Rectf value( it->getAttributeValue<float>(L_SZ),
                              it->getAttributeValue<float>(T_SZ),
                              it->getAttributeValue<float>(R_SZ),
                              it->getAttributeValue<float>(B_SZ));
    add_item(name, mRect, value);
  }

  // INT
  const std::string   INT_PATH("settings/int");
  for (auto it = xml.begin(INT_PATH); it != end; ++it) {
    const std::string name = it->getAttributeValue<std::string>(NAME_SZ);
    add_item(name, mInt, it->getAttributeValue<int>(VALUE_SZ));
  }

  // COLOR
  const std::string  COLOR_PATH("settings/color");
  for (auto it = xml.begin(COLOR_PATH); it != end; ++it) {
    const float             DEFV = 255.0f;
    const std::string       name = it->getAttributeValue<std::string>(NAME_SZ);
    const cinder::ColorA		c(it->getAttributeValue<float>(R_SZ, DEFV) / DEFV,
                              it->getAttributeValue<float>(G_SZ, DEFV) / DEFV,
                              it->getAttributeValue<float>(B_SZ, DEFV) / DEFV,
                              it->getAttributeValue<float>(A_SZ, DEFV) / DEFV);
    add_item(name, mColor, cinder::Color(c.r, c.g, c.b));
    add_item(name, mColorA, c);
  }

  // SIZE
  const std::string  SIZE_PATH("settings/size");
  for (auto it = xml.begin(SIZE_PATH); it != end; ++it) {
    const float             DEFV = 0.0f;
    const std::string       name = it->getAttributeValue<std::string>(NAME_SZ);
    const cinder::Vec2f     value( it->getAttributeValue<float>(X_SZ, DEFV),
                                  it->getAttributeValue<float>(Y_SZ, DEFV));
    add_item(name, mSize, value);
  }

  // TEXT
  const std::string  TEXT_PATH("settings/text");
  for (auto it = xml.begin(TEXT_PATH); it != end; ++it) {
    const std::string       name = it->getAttributeValue<std::string>(NAME_SZ);
    const std::string       value = it->getAttributeValue<std::string>(VALUE_SZ);
    add_item(name, mText, value);
  }

  // POINT
  const std::string  POINT_PATH("settings/point");
  for (auto it = xml.begin(POINT_PATH); it != end; ++it) {
    const float             DEFV = 0.0f;
    const std::string       name = it->getAttributeValue<std::string>(NAME_SZ);
    cinder::Vec3f           value( it->getAttributeValue<float>(X_SZ, DEFV),
                                   it->getAttributeValue<float>(Y_SZ, DEFV), DEFV);
    if (it->hasAttribute(Z_SZ)) {
      value.z = it->getAttributeValue<float>(Z_SZ, DEFV);
    }

    add_item(name, mPoints, value);
  }
}

bool Settings::empty() const
{
	if (!mFloat.empty()) return false;
	if (!mRect.empty()) return false;
	if (!mInt.empty()) return false;
	if (!mColor.empty()) return false;
	if (!mSize.empty()) return false;
	if (!mText.empty()) return false;
  if (!mTextW.empty()) return false;
  if (!mPoints.empty()) return false;
	return true;
}

void Settings::clear()
{
	mFloat.clear();
	mRect.clear();
	mRes.clear();
	mColor.clear();
	mColorA.clear();
	mSize.clear();
	mText.clear();
	mTextW.clear();
  mPoints.clear();
}

int Settings::getColorSize(const std::string& name) const
{
	return get_size(name, mColor);
}

int Settings::getFloatSize(const std::string& name) const
{
	return get_size(name, mFloat);
}

int Settings::getIntSize(const std::string& name) const
{
	return get_size(name, mInt);
}

int Settings::getRectSize(const std::string& name) const
{
	return get_size(name, mRect);
}

int Settings::getResourceIdSize(const std::string& name) const
{
	return get_size(name, mRes);
}

int Settings::getTextSize(const std::string& name) const
{
	return get_size(name, mText);
}

int Settings::getTextWSize(const std::string& name) const
{
	return get_size(name, mTextW);
}

int Settings::getPointSize( const std::string& name ) const
{
  return get_size(name, mPoints);
}

float Settings::getFloat(const std::string& name, const int index) const
{
	return get_or_throw(name, mFloat, index, FLOAT_TYPE, FLOAT_NAME);
}

const cinder::Rectf& Settings::getRect(const std::string& name, const int index) const
{
	return get_or_throw(name, mRect, index, RECT_TYPE, RECT_NAME);
}

int Settings::getInt(const std::string& name, const int index) const
{
	return get_or_throw(name, mInt, index, INT_TYPE, INT_NAME);
}

const Resource::Id& Settings::getResourceId(const std::string& name, const int index) const
{
	return get_or_throw(name, mRes, index, RESOURCE_ID_TYPE, RESOURCE_ID_NAME);
}

const cinder::Color& Settings::getColor(const std::string& name, const int index) const
{
	return get_or_throw(name, mColor, index, COLOR_TYPE, COLOR_NAME);
}

const cinder::ColorA& Settings::getColorA(const std::string& name, const int index) const
{
	return get_or_throw(name, mColorA, index, COLORA_TYPE, COLOR_NAME);
}

const cinder::Vec2f& Settings::getSize(const std::string& name, const int index) const
{
	return get_or_throw(name, mSize, index, SIZE_TYPE, SIZE_NAME);
}

const std::string& Settings::getText(const std::string& name, const int index) const
{
	return get_or_throw(name, mText, index, TEXT_TYPE, TEXT_NAME);
}

const std::wstring& Settings::getTextW(const std::string& name, const int index) const
{
	return get_or_throw(name, mTextW, index, TEXTW_TYPE, TEXTW_NAME);
}

const ci::Vec3f& Settings::getPoint( const std::string& name, const int index /*= 0*/ ) const
{
  return get_or_throw(name, mPoints, index, POINT_TYPE, POINT_NAME);
}

bool Settings::getBool(const std::string& name, const int index) const
{
	return check_bool(getText(name, index), false);
}

float Settings::getFloat(const std::string& name, const int index, const float defaultValue) const
{
	return get(name, mFloat, index, defaultValue, FLOAT_NAME);
}

cinder::Rectf Settings::getRect(const std::string& name, const int index, const ci::Rectf& defaultValue) const
{
	return get(name, mRect, index, defaultValue, RECT_NAME);
}

int Settings::getInt(const std::string& name, const int index, const int defaultValue) const
{
	return get(name, mInt, index, defaultValue, INT_NAME);
}

Resource::Id Settings::getResourceId(const std::string& name, const int index, const Resource::Id& defaultValue) const
{
	return get(name, mRes, index, defaultValue, RESOURCE_ID_NAME);
}

cinder::Color Settings::getColor(const std::string& name, const int index, const ci::Color& defaultValue) const
{
	return get(name, mColor, index, defaultValue, COLOR_NAME);
}

cinder::ColorA Settings::getColorA(const std::string& name, const int index, const ci::ColorA& defaultValue) const
{
	return get(name, mColorA, index, defaultValue, COLOR_NAME);
}

cinder::Vec2f Settings::getSize(const std::string& name, const int index, const ci::Vec2f& defaultValue) const
{
	return get(name, mSize, index, defaultValue, SIZE_NAME);
}

std::string Settings::getText(const std::string& name, const int index, const std::string& defaultValue) const
{
	return get(name, mText, index, defaultValue, TEXT_NAME);
}

std::wstring Settings::getTextW(const std::string& name, const int index, const std::wstring& defaultValue) const
{
	return get(name, mTextW, index, defaultValue, TEXT_NAME);
}

const ci::Vec3f& Settings::getPoint( const std::string& name, const int index /*= 0*/, const ci::Vec3f& defaultValue ) const
{
  return get_or_throw(name, mPoints, index, POINT_TYPE, POINT_NAME);
}

bool Settings::getBool(const std::string& name, const int index, const bool defaultValue) const
{
	return check_bool(getText(name, index, defaultValue ? TRUE_SZ : FALSE_SZ), false);
}

void Settings::forEachTextKey(const std::function<void(const std::string&)>& fn)
{
  if (!fn || mText.empty()) return;

  for (auto it=mText.begin(), end=mText.end(); it != end; ++it) fn(it->first);
}

/**
 * ds::xml::Settings::Editor
 */
Settings::Editor::Editor(Settings& s, const int mode)
	: mSettings(s)
	, mMode(mode)
{
}

template <typename A>
static void clear_vec(A& container)
{
	for (auto it=container.begin(), end=container.end(); it != end; ++it) it->second.clear();
	container.clear();
}

Settings::Editor& Settings::Editor::clear()
{
	mSettings.mFloat.clear();
	mSettings.mRect.clear();
	clear_vec(mSettings.mInt);
	clear_vec(mSettings.mRes);
	clear_vec(mSettings.mColor);
	clear_vec(mSettings.mColorA);
	mSettings.mSize.clear();
	clear_vec(mSettings.mText);
  clear_vec(mSettings.mTextW);
  clear_vec(mSettings.mPoints);
	return *this;
}

Settings::Editor& Settings::Editor::setMode(const int mode)
{
	mMode = mode;
	return *this;
}

template <typename A, typename V>
static void editor_set_vec(const int mode, const std::string& name, A& container, const V& value)
{
	// Always set the first value
	if (mode == ds::cfg::Settings::Editor::SET_MODE) {
		auto it = container.find(name);
		if (it != container.end()) {
			if (it->second.empty()) it->second.push_back(value);
			else it->second[0] = value;
		} else {
			std::vector<V>		vec;
			vec.push_back(value);
			container[name] = vec;
		}
	// Only set the first value if it doesn't exist
	} else if (mode == ds::cfg::Settings::Editor::IF_MISSING_MODE) {
		auto it = container.find(name);
		if (it == container.end()) {
			std::vector<V>		vec;
			vec.push_back(value);
			container[name] = vec;
		} else {
			if (it->second.empty()) it->second.push_back(value);
		}
	}
}

template <typename A, typename V>
static void editor_add_vec(const int mode, const std::string& name, A& container, const V& value)
{
	auto it = container.find(name);
	if (it != container.end()) {
		it->second.push_back(value);
	} else {
		std::vector<V>		vec;
		vec.push_back(value);
		container[name] = vec;
	}
}

Settings::Editor& Settings::Editor::setFloat(const std::string& name, const float v)
{
	editor_set_vec(mMode, name, mSettings.mFloat, v);
	return *this;
}

Settings::Editor& Settings::Editor::setResourceId(const std::string& name, const Resource::Id& v)
{
	editor_set_vec(mMode, name, mSettings.mRes, v);
	return *this;
}

Settings::Editor& Settings::Editor::setSize(const std::string& name, const cinder::Vec2f& v)
{
	editor_set_vec(mMode, name, mSettings.mSize, v);
	return *this;
}

Settings::Editor& Settings::Editor::setText(const std::string& name, const std::string& v)
{
  editor_set_vec(mMode, name, mSettings.mText, v);
  return *this;
}

Settings::Editor& Settings::Editor::addInt(const std::string& name, const int v)
{
	editor_add_vec(mMode, name, mSettings.mInt, v);
	return *this;
}

Settings::Editor& Settings::Editor::addResourceId(const std::string& name, const Resource::Id& v)
{
	editor_add_vec(mMode, name, mSettings.mRes, v);
	return *this;
}

Settings::Editor& Settings::Editor::addTextW(const std::string& name, const std::wstring& v)
{
	editor_add_vec(mMode, name, mSettings.mTextW, v);
	return *this;
}

Settings::Editor& Settings::Editor::setPoint( const std::string& name, const ci::Vec3f& v)
{
  editor_add_vec(mMode, name, mSettings.mPoints, v);
  return *this;
}

} // namespace cfg

} // namespace ds

/**
 * check_book
 */
static bool check_bool(const std::string& text, const bool defaultValue)
{
	if (text.empty()) return defaultValue;
	if (text[0] == 't' || text[0] == 'T') return true;
	if (text[0] == 'f' || text[0] == 'F') return false;
	return defaultValue;
}
